//============================================================================
// Name : Utils.h
// Author : David Nogueira
//============================================================================
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <numeric>
#include <chrono>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iterator>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <cassert>

#include "Chrono.h"
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

namespace utils {
//Typical sigmoid function created from input x
//Returns the sigmoided value
template<typename T>
inline T sigmoid(T x) {
  return 1 / (1 + std::exp(-x));
}

// Derivative of sigmoid function
template<typename T>
inline T deriv_sigmoid(T x) {
  return sigmoid(x)*(1 - sigmoid(x));
}

//Compute hyperbolic tangent (tanh)
//Returns the hyperbolic tangent of x.
template<typename T>
inline T hyperbolic_tan(T x) {
    return (std::tanh)(x);
}

// Derivative of hyperbolic tangent function
template<typename T>
inline T deriv_hyperbolic_tan(T x) {
  return 1 - (std::pow)(hyperbolic_tan(x), 2);
}


template<typename T>
inline T linear(T x) {
  return x;
}

// Derivative of linear function
template<typename T>
inline T deriv_linear(T x) {
    return static_cast<T>(1);
};


template<typename T>
struct ActivationFunctionsManager {
  bool GetActivationFunctionPair(const std::string & activation_name,
                                    std::pair<std::function<T(T)>,
                                    std::function<T(T)> > **pair) {
    auto iter = activation_functions_map.find(activation_name);
    if (iter != activation_functions_map.end())
      *pair = &(iter->second);
    else
      return false;
    return true;
  }

  static ActivationFunctionsManager & Singleton() {
    static ActivationFunctionsManager instance;
    return instance;
  }
private:
  void AddNewPair(std::string function_name,
                  std::function<T(T)> function,
                  std::function<T(T)> deriv_function) {
    activation_functions_map.insert(std::make_pair(function_name,
                                                   std::make_pair(function,
                                                                  deriv_function)));
  };

  ActivationFunctionsManager() {
    AddNewPair("sigmoid", sigmoid<T>, deriv_sigmoid<T>);
    AddNewPair("tanh", hyperbolic_tan<T>, deriv_hyperbolic_tan<T>);
    AddNewPair("linear", linear<T>, deriv_linear<T>);
    // TODO AM insert RELU
  };

  std::unordered_map<std::string,
    std::pair<std::function<T(T)>, std::function<T(T)> > >
    activation_functions_map;
};

// TODO AM handle loss functions != MSE here

template<typename T>
struct gen_rand {
  T factor;
  T offset;
public:
  gen_rand(T r = 2.0) : factor(r / RAND_MAX), offset(r / 2) {}
  T operator()() {
    return rand() * factor - offset;
  }
};


template<typename T>
inline void Softmax(std::vector<T> *output) {
  size_t num_elements = output->size();
  std::vector<T> exp_output(num_elements);
  T exp_total = 0;
  for (size_t i = 0; i < num_elements; i++) {
    exp_output[i] = std::exp((*output)[i]);
    exp_total += exp_output[i];
  }
  for (size_t i = 0; i < num_elements; i++) {
    (*output)[i] = exp_output[i] / exp_total;
  }
}


template<typename T>
inline void  GetIdMaxElement(const std::vector<T> &output, size_t * class_id) {
  *class_id = std::distance(output.begin(),
                            std::max_element(output.begin(),
                                             output.end()));
}

}  // namespace utils

#endif // UTILS_H
