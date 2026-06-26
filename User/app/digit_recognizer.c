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

static float normalized_input(const uint8_t input[DIGIT_INPUT_SIZE], uint16_t index)
{
    return (float)input[index] / 255.0f;
}

static float relu(float value)
{
    return (value > 0.0f) ? value : 0.0f;
}

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
#endif

    for (uint8_t class_idx = 0; class_idx < MODEL_CLASS_COUNT; ++class_idx)
    {
        float score = g_model_output_bias[class_idx];
#if MODEL_TYPE == MODEL_TYPE_MLP
        for (uint16_t i = 0; i < MODEL_HIDDEN_SIZE; ++i)
        {
            score += hidden[i] * g_model_output_weights[class_idx][i];
        }
#else
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
