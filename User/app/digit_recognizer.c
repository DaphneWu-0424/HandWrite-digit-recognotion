#include "digit_recognizer.h"

#include "ModelData.h"

#include <float.h>

static int32_t score_to_milli(float score)
{
    if (score >= 0.0f)
    {
        return (int32_t)(score * 1000.0f + 0.5f);
    }
    return (int32_t)(score * 1000.0f - 0.5f);
}

bool DigitRecognizer_HasTrainedModel(void)
{
    return MODEL_GENERATED != 0;
}

const char *DigitRecognizer_ModelName(void)
{
    return MODEL_NAME;
}

const char *DigitRecognizer_ModelTypeName(void)
{
    return MODEL_TYPE_NAME;
}

const char *DigitRecognizer_ModelQuantName(void)
{
    return MODEL_QUANT_NAME;
}

static float normalized_input(const uint8_t input[DIGIT_INPUT_SIZE], uint16_t index)
{
    return (float)input[index] / 255.0f;
}

static float relu(float value)
{
    return (value > 0.0f) ? value : 0.0f;
}

#if MODEL_TYPE == MODEL_TYPE_CNN
static float s_pool1[MODEL_CONV1_OUT][14][14];
static float s_pool2[MODEL_CONV2_OUT][7][7];
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
static uint8_t s_pool1_q[MODEL_CONV1_OUT][14][14];
static uint8_t s_pool2_q[MODEL_CONV2_OUT][7][7];
static float s_pool1_scale;
static float s_pool2_scale;
static int32_t cnn_conv1_acc_at(const uint8_t input[DIGIT_INPUT_SIZE], uint8_t oc, uint8_t y, uint8_t x);
static float quantize_pool1(void);
static int32_t cnn_conv2_acc_at(uint8_t oc, uint8_t y, uint8_t x);
static float quantize_pool2(void);
#endif

#if MODEL_QUANT_TYPE != MODEL_QUANT_INT8
static float cnn_input_at(const uint8_t input[DIGIT_INPUT_SIZE], int16_t y, int16_t x)
{
    if ((x < 0) || (y < 0) || (x >= 28) || (y >= 28))
    {
        return 0.0f;
    }
    return normalized_input(input, (uint16_t)y * 28U + (uint16_t)x);
}

static float cnn_conv1_at(const uint8_t input[DIGIT_INPUT_SIZE], uint8_t oc, uint8_t y, uint8_t x)
{
    float sum = g_model_conv1_bias[oc];
    for (int8_t ky = -1; ky <= 1; ++ky)
    {
        for (int8_t kx = -1; kx <= 1; ++kx)
        {
            sum += cnn_input_at(input, (int16_t)y + ky, (int16_t)x + kx) *
                   g_model_conv1_weights[oc][0][ky + 1][kx + 1];
        }
    }
    return relu(sum);
}
#endif

#if MODEL_QUANT_TYPE != MODEL_QUANT_INT8
static float cnn_pool1_at(uint8_t channel, int16_t y, int16_t x)
{
    if ((x < 0) || (y < 0) || (x >= 14) || (y >= 14))
    {
        return 0.0f;
    }
    return s_pool1[channel][y][x];
}

static float cnn_conv2_at(uint8_t oc, uint8_t y, uint8_t x)
{
    float sum = g_model_conv2_bias[oc];
    for (uint8_t ic = 0; ic < MODEL_CONV1_OUT; ++ic)
    {
        for (int8_t ky = -1; ky <= 1; ++ky)
        {
            for (int8_t kx = -1; kx <= 1; ++kx)
            {
                sum += cnn_pool1_at(ic, (int16_t)y + ky, (int16_t)x + kx) *
                       g_model_conv2_weights[oc][ic][ky + 1][kx + 1];
            }
        }
    }
    return relu(sum);
}
#endif

static void cnn_build_pool1(const uint8_t input[DIGIT_INPUT_SIZE])
{
    for (uint8_t oc = 0; oc < MODEL_CONV1_OUT; ++oc)
    {
        for (uint8_t py = 0; py < 14U; ++py)
        {
            for (uint8_t px = 0; px < 14U; ++px)
            {
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
                int32_t max_acc = cnn_conv1_acc_at(input, oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U));
                int32_t acc = cnn_conv1_acc_at(input, oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U + 1U));
                if (acc > max_acc) max_acc = acc;
                acc = cnn_conv1_acc_at(input, oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U));
                if (acc > max_acc) max_acc = acc;
                acc = cnn_conv1_acc_at(input, oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U + 1U));
                if (acc > max_acc) max_acc = acc;
                s_pool1[oc][py][px] = relu(((float)max_acc * (g_model_conv1_weight_scale / 255.0f)) + g_model_conv1_bias[oc]);
#else
                float max_value = cnn_conv1_at(input, oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U));
                float v = cnn_conv1_at(input, oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U + 1U));
                if (v > max_value) max_value = v;
                v = cnn_conv1_at(input, oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U));
                if (v > max_value) max_value = v;
                v = cnn_conv1_at(input, oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U + 1U));
                if (v > max_value) max_value = v;
                s_pool1[oc][py][px] = max_value;
#endif
            }
        }
    }
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
    s_pool1_scale = quantize_pool1();
#endif
}

static void cnn_build_pool2(void)
{
    for (uint8_t oc = 0; oc < MODEL_CONV2_OUT; ++oc)
    {
        for (uint8_t py = 0; py < 7U; ++py)
        {
            for (uint8_t px = 0; px < 7U; ++px)
            {
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
                int32_t max_acc = cnn_conv2_acc_at(oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U));
                int32_t acc = cnn_conv2_acc_at(oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U + 1U));
                if (acc > max_acc) max_acc = acc;
                acc = cnn_conv2_acc_at(oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U));
                if (acc > max_acc) max_acc = acc;
                acc = cnn_conv2_acc_at(oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U + 1U));
                if (acc > max_acc) max_acc = acc;
                s_pool2[oc][py][px] = relu(((float)max_acc * s_pool1_scale * g_model_conv2_weight_scale) +
                                           g_model_conv2_bias[oc]);
#else
                float max_value = cnn_conv2_at(oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U));
                float v = cnn_conv2_at(oc, (uint8_t)(py * 2U), (uint8_t)(px * 2U + 1U));
                if (v > max_value) max_value = v;
                v = cnn_conv2_at(oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U));
                if (v > max_value) max_value = v;
                v = cnn_conv2_at(oc, (uint8_t)(py * 2U + 1U), (uint8_t)(px * 2U + 1U));
                if (v > max_value) max_value = v;
                s_pool2[oc][py][px] = max_value;
#endif
            }
        }
    }
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
    s_pool2_scale = quantize_pool2();
#endif
}

#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
static int32_t cnn_conv1_acc_at(const uint8_t input[DIGIT_INPUT_SIZE], uint8_t oc, uint8_t y, uint8_t x)
{
    int32_t acc = 0;
    for (int8_t ky = -1; ky <= 1; ++ky)
    {
        for (int8_t kx = -1; kx <= 1; ++kx)
        {
            int16_t iy = (int16_t)y + ky;
            int16_t ix = (int16_t)x + kx;
            uint8_t input_value = 0U;
            if ((ix >= 0) && (iy >= 0) && (ix < 28) && (iy < 28))
            {
                input_value = input[(uint16_t)iy * 28U + (uint16_t)ix];
            }
            acc += (int32_t)input_value * (int32_t)g_model_conv1_weights_q[oc][0][ky + 1][kx + 1];
        }
    }
    return acc;
}

static uint8_t quantize_activation(float value, float scale)
{
    if (value <= 0.0f)
    {
        return 0U;
    }
    uint32_t q = (uint32_t)(value / scale + 0.5f);
    return (q > 255U) ? 255U : (uint8_t)q;
}

static float quantize_pool1(void)
{
    float max_value = 0.0f;
    for (uint8_t oc = 0; oc < MODEL_CONV1_OUT; ++oc)
    {
        for (uint8_t y = 0; y < 14U; ++y)
        {
            for (uint8_t x = 0; x < 14U; ++x)
            {
                if (s_pool1[oc][y][x] > max_value)
                {
                    max_value = s_pool1[oc][y][x];
                }
            }
        }
    }

    float scale = (max_value > 0.0f) ? (max_value / 255.0f) : 1.0f;
    for (uint8_t oc = 0; oc < MODEL_CONV1_OUT; ++oc)
    {
        for (uint8_t y = 0; y < 14U; ++y)
        {
            for (uint8_t x = 0; x < 14U; ++x)
            {
                s_pool1_q[oc][y][x] = quantize_activation(s_pool1[oc][y][x], scale);
            }
        }
    }
    return scale;
}

static uint8_t cnn_pool1_q_at(uint8_t channel, int16_t y, int16_t x)
{
    if ((x < 0) || (y < 0) || (x >= 14) || (y >= 14))
    {
        return 0U;
    }
    return s_pool1_q[channel][y][x];
}

static int32_t cnn_conv2_acc_at(uint8_t oc, uint8_t y, uint8_t x)
{
    int32_t acc = 0;
    for (uint8_t ic = 0; ic < MODEL_CONV1_OUT; ++ic)
    {
        for (int8_t ky = -1; ky <= 1; ++ky)
        {
            for (int8_t kx = -1; kx <= 1; ++kx)
            {
                acc += (int32_t)cnn_pool1_q_at(ic, (int16_t)y + ky, (int16_t)x + kx) *
                       (int32_t)g_model_conv2_weights_q[oc][ic][ky + 1][kx + 1];
            }
        }
    }
    return acc;
}

static float quantize_pool2(void)
{
    float max_value = 0.0f;
    for (uint8_t oc = 0; oc < MODEL_CONV2_OUT; ++oc)
    {
        for (uint8_t y = 0; y < 7U; ++y)
        {
            for (uint8_t x = 0; x < 7U; ++x)
            {
                if (s_pool2[oc][y][x] > max_value)
                {
                    max_value = s_pool2[oc][y][x];
                }
            }
        }
    }

    float scale = (max_value > 0.0f) ? (max_value / 255.0f) : 1.0f;
    for (uint8_t oc = 0; oc < MODEL_CONV2_OUT; ++oc)
    {
        for (uint8_t y = 0; y < 7U; ++y)
        {
            for (uint8_t x = 0; x < 7U; ++x)
            {
                s_pool2_q[oc][y][x] = quantize_activation(s_pool2[oc][y][x], scale);
            }
        }
    }
    return scale;
}

static uint8_t cnn_pool2_q_flat_at(uint16_t index)
{
    uint8_t channel = (uint8_t)(index / 49U);
    uint8_t offset = (uint8_t)(index % 49U);
    return s_pool2_q[channel][offset / 7U][offset % 7U];
}
#else
static float cnn_pool2_flat_at(uint16_t index)
{
    uint8_t channel = (uint8_t)(index / 49U);
    uint8_t offset = (uint8_t)(index % 49U);
    return s_pool2[channel][offset / 7U][offset % 7U];
}
#endif
#endif

DigitTopKResult DigitRecognizer_PredictTop3(const uint8_t input[DIGIT_INPUT_SIZE])
{
    DigitTopKResult result = {
        {0U, -FLT_MAX, DigitRecognizer_HasTrainedModel()},
        {
            {0U, -FLT_MAX, 0},
            {0U, -FLT_MAX, 0},
            {0U, -FLT_MAX, 0},
        },
    };

#if MODEL_TYPE == MODEL_TYPE_MLP
    float hidden[MODEL_HIDDEN_SIZE];
    for (uint16_t hidden_idx = 0; hidden_idx < MODEL_HIDDEN_SIZE; ++hidden_idx)
    {
        float sum = g_model_hidden_bias[hidden_idx];
        for (uint16_t i = 0; i < MODEL_INPUT_SIZE; ++i)
        {
            sum += normalized_input(input, i) * g_model_hidden_weights[hidden_idx][i];
        }
        hidden[hidden_idx] = relu(sum);
    }
#elif MODEL_TYPE == MODEL_TYPE_CNN
    cnn_build_pool1(input);
    cnn_build_pool2();
#endif

    for (uint8_t class_idx = 0; class_idx < MODEL_CLASS_COUNT; ++class_idx)
    {
#if MODEL_TYPE == MODEL_TYPE_CNN
#if MODEL_QUANT_TYPE == MODEL_QUANT_INT8
        int32_t acc = 0;
        for (uint16_t i = 0; i < MODEL_CNN_FC_INPUT_SIZE; ++i)
        {
            acc += (int32_t)cnn_pool2_q_flat_at(i) * (int32_t)g_model_fc_weights_q[class_idx][i];
        }
        float score = ((float)acc * s_pool2_scale * g_model_fc_weight_scale) + g_model_fc_bias[class_idx];
#else
        float score = g_model_fc_bias[class_idx];
        for (uint16_t i = 0; i < MODEL_CNN_FC_INPUT_SIZE; ++i)
        {
            score += cnn_pool2_flat_at(i) * g_model_fc_weights[class_idx][i];
        }
#endif
#elif MODEL_TYPE == MODEL_TYPE_MLP
        float score = g_model_output_bias[class_idx];
        for (uint16_t i = 0; i < MODEL_HIDDEN_SIZE; ++i)
        {
            score += hidden[i] * g_model_output_weights[class_idx][i];
        }
#else
        float score = g_model_output_bias[class_idx];
        for (uint16_t i = 0; i < MODEL_INPUT_SIZE; ++i)
        {
            score += normalized_input(input, i) * g_model_output_weights[class_idx][i];
        }
#endif

        if ((class_idx == 0U) || (score > result.best.score))
        {
            result.best.score = score;
            result.best.digit = class_idx;
        }

        DigitTopKItem item = {class_idx, score, score_to_milli(score)};
        for (uint8_t rank = 0; rank < 3U; ++rank)
        {
            if (score > result.top3[rank].score)
            {
                for (uint8_t move = 2U; move > rank; --move)
                {
                    result.top3[move] = result.top3[move - 1U];
                }
                result.top3[rank] = item;
                break;
            }
        }
    }

    return result;
}

DigitRecognitionResult DigitRecognizer_Predict(const uint8_t input[DIGIT_INPUT_SIZE])
{
    return DigitRecognizer_PredictTop3(input).best;
}
