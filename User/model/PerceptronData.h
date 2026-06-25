#ifndef PERCEPTRON_DATA_H
#define PERCEPTRON_DATA_H

#include <stdint.h>

#define PERCEPTRON_INPUT_SIZE 784U
#define PERCEPTRON_CLASS_COUNT 10U

#define PERCEPTRON_MODEL_GENERATED 1

extern const float g_perceptron_weights[PERCEPTRON_CLASS_COUNT][PERCEPTRON_INPUT_SIZE];
extern const float g_perceptron_bias[PERCEPTRON_CLASS_COUNT];

#endif /* PERCEPTRON_DATA_H */
