//============================================================================
// Name : MLP.cpp
// Author : David Nogueira
//============================================================================
#ifndef MLP_H
#define MLP_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <exception>
#include "Layer.h"
#include "Sample.h"
#include "Utils.h"


template<typename T>
class MLP {
public:
  //desired call syntax :  MLP({64*64,20,4}, {"sigmoid", "linear"},
  MLP(const std::vector<uint64_t> & layers_nodes,
      const std::vector<std::string> & layers_activfuncs,
      bool use_constant_weight_init = false,
      T constant_weight_init = 0.5);
  MLP(const std::string & filename);
  ~MLP();

  void SaveMLPNetwork(const std::string & filename) const;
  void LoadMLPNetwork(const std::string & filename);

  void GetOutput(const std::vector<T> &input,
                 std::vector<T> * output,
                 std::vector<std::vector<T>> * all_layers_activations = nullptr) const;
  void GetOutputClass(const std::vector<T> &output, size_t * class_id) const;

  void Train(const std::vector<TrainingSample<T>> &training_sample_set_with_bias,
                       float learning_rate,
                       int max_iterations = 5000,
                       float min_error_cost = 0.001,
                       bool output_log = true);
  size_t GetNumLayers();
  std::vector<std::vector<T>> GetLayerWeights( size_t layer_i );
  void SetLayerWeights( size_t layer_i, std::vector<std::vector<T>> & weights );

protected:
  void UpdateWeights(const std::vector<std::vector<T>> & all_layers_activations,
                     const std::vector<T> &error,
                     float learning_rate);
private:
  void CreateMLP(const std::vector<uint64_t> & layers_nodes,
                 const std::vector<std::string> & layers_activfuncs,
                 bool use_constant_weight_init,
                 T constant_weight_init = 0.5);
  size_t m_num_inputs{ 0 };
  int m_num_outputs{ 0 };
  int m_num_hidden_layers{ 0 };
  std::vector<uint64_t> m_layers_nodes;
  std::vector<Layer<T>> m_layers;
};

#endif //MLP_H
