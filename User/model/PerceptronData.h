#ifndef PERCEPTRON_DATA_H
#define PERCEPTRON_DATA_H

#include <stdint.h>

#define PERCEPTRON_INPUT_SIZE 784U
#define PERCEPTRON_CLASS_COUNT 10U

/* Set to 1 by tools/train/export_perceptron.py after real MNIST training. */
#define PERCEPTRON_MODEL_GENERATED 0

extern const float g_perceptron_weights[PERCEPTRON_CLASS_COUNT][PERCEPTRON_INPUT_SIZE];
extern const float g_perceptron_bias[PERCEPTRON_CLASS_COUNT];

#endif /* PERCEPTRON_DATA_H */
