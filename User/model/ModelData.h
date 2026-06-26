#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

#define MODEL_TYPE_PERCEPTRON 1
#define MODEL_TYPE_MLP 2

#define MODEL_INPUT_SIZE 784U
#define MODEL_CLASS_COUNT 10U
#define MODEL_HIDDEN_SIZE 64U
#define MODEL_TYPE MODEL_TYPE_MLP
#define MODEL_NAME "mlp64-20260626-162302-wuyueying"
#define MODEL_TYPE_NAME "mlp"
#define MODEL_GENERATED 1

#if MODEL_TYPE == MODEL_TYPE_MLP
extern const float g_model_hidden_weights[MODEL_HIDDEN_SIZE][MODEL_INPUT_SIZE];
extern const float g_model_hidden_bias[MODEL_HIDDEN_SIZE];
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_HIDDEN_SIZE];
#else
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_INPUT_SIZE];
#endif
extern const float g_model_output_bias[MODEL_CLASS_COUNT];

#endif /* MODEL_DATA_H */
