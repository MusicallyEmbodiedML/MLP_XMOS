//============================================================================
// Name : MLP.cpp
// Author : David Nogueira
//============================================================================
#include "MLP.h"


#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>


#if 1
#include <stdio.h>
#endif


//desired call syntax :  MLP({64*64,20,4}, {"sigmoid", "linear"},
template<typename T>
MLP<T>::MLP(const std::vector<size_t> & layers_nodes,
         const std::vector<std::string> & layers_activfuncs,
         std::string loss_function,
         bool use_constant_weight_init,
         T constant_weight_init) {
  assert(layers_nodes.size() >= 2);
  assert(layers_activfuncs.size() + 1 == layers_nodes.size());

  CreateMLP(layers_nodes,
            layers_activfuncs,
            loss_function,
            use_constant_weight_init,
            constant_weight_init);
};


template<typename T>
MLP<T>::MLP(const std::string & filename) {
  LoadMLPNetwork(filename);
}


template<typename T>
MLP<T>::~MLP() {
  m_num_inputs = 0;
  m_num_outputs = 0;
  m_num_hidden_layers = 0;
  m_layers_nodes.clear();
  m_layers.clear();
};


template<typename T>
void MLP<T>::CreateMLP(const std::vector<size_t> & layers_nodes,
                    const std::vector<std::string> & layers_activfuncs,
                    std::string loss_function,
                    bool use_constant_weight_init,
                    T constant_weight_init) {
  m_layers_nodes = layers_nodes;
  m_num_inputs = m_layers_nodes[0];
  m_num_outputs = m_layers_nodes[m_layers_nodes.size() - 1];
  m_num_hidden_layers = m_layers_nodes.size() - 2;

  // Loss function selection
  loss::LossFunctionsManager<T> loss_mgr =
      loss::LossFunctionsManager<T>::Singleton();
  assert(loss_mgr.GetLossFunction(loss_function, &(this->loss_fn_)));

  for (size_t i = 0; i < m_layers_nodes.size() - 1; i++) {
    m_layers.emplace_back(Layer<T>(m_layers_nodes[i],
                                   m_layers_nodes[i + 1],
                                   layers_activfuncs[i],
                                   use_constant_weight_init,
                                   constant_weight_init));
  }
}


template<typename T>
void MLP<T>::ReportProgress(const bool output_log,
    const unsigned int every_n_iter,
    const unsigned int i,
    const T sampleLoss)
{
#if 1
    if (output_log && ((i % every_n_iter) == 0)) {
        printf("Iteration %i cost function f(error): %f\n",
                i, sampleLoss);
    }
#endif
}


template<typename T>
void MLP<T>::ReportFinish(const unsigned int i, const float current_iteration_cost_function)
{
#if 1
    printf("Iteration %i cost function f(error): %f\n",
        i, current_iteration_cost_function);

    printf("******************************\n");
    printf("******* TRAINING ENDED *******\n");
    printf("******* %d iters *******\n", i);
    printf("******************************\n");;
#endif
};


template<typename T>
void MLP<T>::SaveMLPNetwork(const std::string & filename)const {
  FILE * file;
  file = fopen(filename.c_str(), "wb");
  fwrite(&m_num_inputs, sizeof(m_num_inputs), 1, file);
  fwrite(&m_num_outputs, sizeof(m_num_outputs), 1, file);
  fwrite(&m_num_hidden_layers, sizeof(m_num_hidden_layers), 1, file);
  if (!m_layers_nodes.empty())
    fwrite(&m_layers_nodes[0], sizeof(m_layers_nodes[0]), m_layers_nodes.size(), file);
  for (size_t i = 0; i < m_layers.size(); i++) {
    m_layers[i].SaveLayer(file);
  }
  fclose(file);
};


template<typename T>
void MLP<T>::LoadMLPNetwork(const std::string & filename) {
  m_layers_nodes.clear();
  m_layers.clear();

  FILE * file;
  file = fopen(filename.c_str(), "rb");
  fread(&m_num_inputs, sizeof(m_num_inputs), 1, file);
  fread(&m_num_outputs, sizeof(m_num_outputs), 1, file);
  fread(&m_num_hidden_layers, sizeof(m_num_hidden_layers), 1, file);
  m_layers_nodes.resize(m_num_hidden_layers + 2);
  if (!m_layers_nodes.empty())
    fread(&m_layers_nodes[0], sizeof(m_layers_nodes[0]), m_layers_nodes.size(), file);
  m_layers.resize(m_layers_nodes.size() - 1);
  for (size_t i = 0; i < m_layers.size(); i++) {
    m_layers[i].LoadLayer(file);
  }
  fclose(file);
};


template<typename T>
void MLP<T>::GetOutput(const std::vector<T> &input,
                    std::vector<T> * output,
                    std::vector<std::vector<T>> * all_layers_activations) {
  assert(input.size() == m_num_inputs);
  int temp_size;
  if (m_num_hidden_layers == 0)
    temp_size = m_num_outputs;
  else
    temp_size = m_layers_nodes[1];

  std::vector<T> temp_in(m_num_inputs, 0);
  std::vector<T> temp_out(temp_size, 0);
  temp_in = input;

  for (size_t i = 0; i < m_layers.size(); ++i) {
    if (i > 0) {
      //Store this layer activation
      if (all_layers_activations != nullptr)
        all_layers_activations->emplace_back(std::move(temp_in));

      temp_in.clear();
      temp_in = temp_out;
      temp_out.clear();
      temp_out.resize(m_layers[i].GetOutputSize());
    }
    m_layers[i].GetOutputAfterActivationFunction(temp_in, &temp_out);
  }

  /*FIXME AM Why is it forcing a softmax even when doing regression???
             Also, softmax is not backprop'ed */
#if 0
  if (temp_out.size() > 1) {
      utils::Softmax(&temp_out);
  }
#endif
  *output = temp_out;

  //Add last layer activation
  if (all_layers_activations != nullptr)
    all_layers_activations->emplace_back(std::move(temp_in));
}


template<typename T>
void MLP<T>::GetOutputClass(const std::vector<T> &output, size_t * class_id) const {
  utils::GetIdMaxElement(output, class_id);
}


template<typename T>
void MLP<T>::UpdateWeights(const std::vector<std::vector<T>> & all_layers_activations,
                        const std::vector<T> &deriv_error,
                        float learning_rate) {

  std::vector<T> temp_deriv_error = deriv_error;
  std::vector<T> deltas{};
  //m_layers.size() equals (m_num_hidden_layers + 1)
  for (int i = m_num_hidden_layers; i >= 0; --i) {
    m_layers[i].UpdateWeights(all_layers_activations[i], temp_deriv_error, learning_rate, &deltas);
    if (i > 0) {
      temp_deriv_error.clear();
      temp_deriv_error = std::move(deltas);
      deltas.clear();
    }
  }
};


template<typename T>
void MLP<T>::Train(const std::vector<TrainingSample<T>> &training_sample_set_with_bias,
                          float learning_rate,
                          int max_iterations,
                          float min_error_cost,
                          bool output_log) {
  
  //TODO AM this signature shouldn't exist - TrainingSample should be removed

  //rlunaro.03/01/2019. the compiler says that these variables are unused
  //int num_examples = training_sample_set_with_bias.size();
  //int num_features = training_sample_set_with_bias[0].GetInputVectorSize();

  //{
  //  int layer_i = -1;
  //  int node_i = -1;
  //  std::cout << "Starting weights:" << std::endl;
  //  for (const auto & layer : m_layers) {
  //    layer_i++;
  //    node_i = -1;
  //    std::cout << "Layer " << layer_i << " :" << std::endl;
  //    for (const auto & node : layer.GetNodes()) {
  //      node_i++;
  //      std::cout << "\tNode " << node_i << " :\t";
  //      for (auto m_weightselement : node.GetWeights()) {
  //        std::cout << m_weightselement << "\t";
  //      }
  //      std::cout << std::endl;
  //    }
  //  }
  //}

  int i = 0;
  float current_iteration_cost_function = 0.f;
  T sampleSizeReciprocal = 1.f / training_sample_set_with_bias.size();


  for (i = 0; i < max_iterations; i++) {
    current_iteration_cost_function = 0.f;
    for (auto & training_sample_with_bias : training_sample_set_with_bias) {

      std::vector<T> predicted_output;
      std::vector< std::vector<T> > all_layers_activations;

      GetOutput(training_sample_with_bias.input_vector(),
                &predicted_output,
                &all_layers_activations);

      const std::vector<T> &  correct_output =
        training_sample_with_bias.output_vector();

      assert(correct_output.size() == predicted_output.size());
      std::vector<T> deriv_error_output(predicted_output.size());

      // Loss funtion
      current_iteration_cost_function =
          this->loss_fn_(correct_output, predicted_output, deriv_error_output,sampleSizeReciprocal);

      UpdateWeights(all_layers_activations,
                    deriv_error_output,
                    learning_rate);
    }

#if defined(MLP_VERBOSE)
    ReportProgress(output_log, 1, i, current_iteration_cost_function);

#endif  // EASYLOGGING_ON

    // Early stopping
    // TODO AM early stopping should be optional and metric-dependent
    if (current_iteration_cost_function < min_error_cost) {
      break;
    }

  }

#if defined(MLP_VERBOSE)
  ReportFinish(i, current_iteration_cost_function);
#endif  // EASYLOGGING_ON

  //{
  //  int layer_i = -1;
  //  int node_i = -1;
  //  std::cout << "Final weights:" << std::endl;
  //  for (const auto & layer : m_layers) {
  //    layer_i++;
  //    node_i = -1;
  //    std::cout << "Layer " << layer_i << " :" << std::endl;
  //    for (const auto & node : layer.GetNodes()) {
  //      node_i++;
  //      std::cout << "\tNode " << node_i << " :\t";
  //      for (auto m_weightselement : node.GetWeights()) {
  //        std::cout << m_weightselement << "\t";
  //      }
  //      std::cout << std::endl;
  //    }
  //  }
  //}
};



template<typename T>
void MLP<T>::Train(const training_pair_t& training_sample_set_with_bias,
    float learning_rate,
    int max_iterations,
    float min_error_cost,
    bool output_log) {

    int i = 0;
    float current_iteration_cost_function = 0.f;

    T sampleSizeReciprocal = 1.f / training_sample_set_with_bias.first.size();

    for (i = 0; i < max_iterations; i++) {
        current_iteration_cost_function = 0.f;

        auto training_features = training_sample_set_with_bias.first;
        auto training_labels = training_sample_set_with_bias.second;
        auto t_feat = training_features.begin();
        auto t_label = training_labels.begin();

        while (t_feat != training_features.end() || t_label != training_labels.end()) {

            // Payload
            std::vector<T> predicted_output;
            std::vector< std::vector<T> > all_layers_activations;

            GetOutput(*t_feat,
                &predicted_output,
                &all_layers_activations);

            const std::vector<T>& correct_output{ *t_label };

            assert(correct_output.size() == predicted_output.size());
            std::vector<T> deriv_error_output(predicted_output.size());

            // Loss funtion
            current_iteration_cost_function =
                this->loss_fn_(correct_output, predicted_output, deriv_error_output, sampleSizeReciprocal);

            UpdateWeights(all_layers_activations,
                deriv_error_output,
                learning_rate);

            // \Payload
            if (t_feat != training_features.end())
            {
                ++t_feat;
            }
            if (t_label != training_labels.end())
            {
                ++t_label;
            }
        }

#if 1
        ReportProgress(true, 100, i, current_iteration_cost_function);

#endif  // EASYLOGGING_ON

        // Early stopping
        // TODO AM early stopping should be optional and metric-dependent
        if (current_iteration_cost_function < min_error_cost) {
            break;
        }

    }

#if 1
    ReportFinish(i, current_iteration_cost_function);
#endif  // EASYLOGGING_ON
};


template<typename T>
void MLP<T>::MiniBatchTrain(const training_pair_t& training_sample_set_with_bias,
    float learning_rate,
    int max_iterations,
    size_t miniBatchSize,
    float min_error_cost,
    bool output_log) {

    assert(miniBatchSize > 0);

    int i = 0;
    float current_iteration_cost_function = 0.f;



    auto training_features = training_sample_set_with_bias.first;
    auto training_labels = training_sample_set_with_bias.second;
    auto t_feat = training_features.begin();
    auto t_label = training_labels.begin();

    //how many batches
    size_t nBatches = training_features.size() / miniBatchSize;
    size_t lastBatchSize = training_features.size() % miniBatchSize;
    //extra batch if not precisely divisible by batch size
    nBatches += (lastBatchSize > 0);


    for (i = 0; i < max_iterations; i++) {
        current_iteration_cost_function = 0.f;

        //randomly shuffle training data into mini batches
        std::vector<size_t> shuffledIndexes(training_features.size());
        //fill with indexes
        for(size_t i_shuffle=0; i_shuffle < shuffledIndexes.size(); i_shuffle++) {
          shuffledIndexes[i_shuffle] = i_shuffle;
        }
        //shuffle
        std::random_shuffle(shuffledIndexes.begin(), shuffledIndexes.end());

        //process the mini batches
        T epochLoss = 0;
        size_t trainingIndex = 0;
        T sampleSizeReciprocal = 1.f / miniBatchSize;

        for(size_t i_batch=0; i_batch < nBatches; i_batch++) {
          size_t currBatchSize = miniBatchSize;
          if (lastBatchSize > 0 && i_batch + 1 == nBatches) {
            currBatchSize = lastBatchSize;
            sampleSizeReciprocal = 1.f / currBatchSize;
          }

          //process minibatch
          T sampleLoss = 0;
          for(size_t i_trainingItem=0; i_trainingItem < currBatchSize; i_trainingItem++) {
            std::vector<T> predicted_output;
            std::vector< std::vector<T> > all_layers_activations;
            size_t shuffledTrainingIndex = shuffledIndexes[trainingIndex];
            GetOutput(training_features[shuffledTrainingIndex],
                &predicted_output,
                &all_layers_activations);

            const std::vector<T>& correct_output{ training_labels[shuffledTrainingIndex] };

            assert(correct_output.size() == predicted_output.size());
            std::vector<T> deriv_error_output(predicted_output.size());

            // Loss funtion
            sampleLoss +=
                this->loss_fn_(correct_output, predicted_output, deriv_error_output, sampleSizeReciprocal);

            UpdateWeights(all_layers_activations,
                deriv_error_output,
                learning_rate);
            epochLoss += sampleLoss;
            trainingIndex++;
          }
        }

#if 1
        ReportProgress(true, 2, i, epochLoss);

#endif  // EASYLOGGING_ON

        // Early stopping
        // TODO AM early stopping should be optional and metric-dependent
        if (current_iteration_cost_function < min_error_cost) {
            break;
        }

    }

#if 1
    ReportFinish(i, current_iteration_cost_function);
#endif  // EASYLOGGING_ON
};



template<typename T>
size_t MLP<T>::GetNumLayers()
{
    return m_layers.size();
}


template<typename T>
std::vector<std::vector<T>> MLP<T>::GetLayerWeights( size_t layer_i )
{
    std::vector<std::vector<T>> ret_val;
    // check parameters
    assert(0 <= layer_i && layer_i < m_layers.size() /* Incorrect layer number in GetLayerWeights call */);
    {
        Layer<T> current_layer = m_layers[layer_i];
        for( Node<T> & node : current_layer.GetNodesChangeable() )
        {
            ret_val.push_back( node.GetWeights() );
        }
        return ret_val;
    }

}


template<typename T>
void MLP<T>::SetLayerWeights( size_t layer_i, std::vector<std::vector<T>> & weights )
{
    // check parameters
    assert(0 <= layer_i && layer_i < m_layers.size() /* Incorrect layer number in SetLayerWeights call */);
    {
        m_layers[layer_i].SetWeights( weights );
    }
}


// Explicit instantiations
template class MLP<double>;
template class MLP<float>;