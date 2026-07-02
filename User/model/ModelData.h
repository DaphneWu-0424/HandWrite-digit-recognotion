#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

#define MODEL_TYPE_PERCEPTRON 1
#define MODEL_TYPE_MLP 2
#define MODEL_TYPE_CNN 3
#define MODEL_QUANT_FP32 0
#define MODEL_QUANT_INT8 1

#define MODEL_INPUT_SIZE 784U
#define MODEL_CLASS_COUNT 10U
#define MODEL_HIDDEN_SIZE 0U
#define MODEL_CONV1_OUT 8U
#define MODEL_CONV2_OUT 16U
#define MODEL_CNN_FC_INPUT_SIZE 784U
#define MODEL_TYPE MODEL_TYPE_CNN
#define MODEL_QUANT_TYPE MODEL_QUANT_INT8
#define MODEL_NAME "cnn8x16-int8-20260702-140745-wuyueying-int8"
#define MODEL_TYPE_NAME "cnn"
#define MODEL_QUANT_NAME "int8"
#define MODEL_GENERATED 1

#if MODEL_TYPE == MODEL_TYPE_CNN && MODEL_QUANT_TYPE == MODEL_QUANT_INT8
extern const int8_t g_model_conv1_weights_q[MODEL_CONV1_OUT][1][3][3];
extern const float g_model_conv1_weight_scale;
extern const float g_model_conv1_bias[MODEL_CONV1_OUT];
extern const int8_t g_model_conv2_weights_q[MODEL_CONV2_OUT][MODEL_CONV1_OUT][3][3];
extern const float g_model_conv2_weight_scale;
extern const float g_model_conv2_bias[MODEL_CONV2_OUT];
extern const int8_t g_model_fc_weights_q[MODEL_CLASS_COUNT][MODEL_CNN_FC_INPUT_SIZE];
extern const float g_model_fc_weight_scale;
extern const float g_model_fc_bias[MODEL_CLASS_COUNT];
#elif MODEL_TYPE == MODEL_TYPE_CNN
extern const float g_model_conv1_weights[MODEL_CONV1_OUT][1][3][3];
extern const float g_model_conv1_bias[MODEL_CONV1_OUT];
extern const float g_model_conv2_weights[MODEL_CONV2_OUT][MODEL_CONV1_OUT][3][3];
extern const float g_model_conv2_bias[MODEL_CONV2_OUT];
extern const float g_model_fc_weights[MODEL_CLASS_COUNT][MODEL_CNN_FC_INPUT_SIZE];
extern const float g_model_fc_bias[MODEL_CLASS_COUNT];
#elif MODEL_TYPE == MODEL_TYPE_MLP
extern const float g_model_hidden_weights[MODEL_HIDDEN_SIZE][MODEL_INPUT_SIZE];
extern const float g_model_hidden_bias[MODEL_HIDDEN_SIZE];
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_HIDDEN_SIZE];
extern const float g_model_output_bias[MODEL_CLASS_COUNT];
#else
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_INPUT_SIZE];
extern const float g_model_output_bias[MODEL_CLASS_COUNT];
#endif

#endif /* MODEL_DATA_H */
