#include "digit_recognizer.h"

#include "PerceptronData.h"

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
    return PERCEPTRON_MODEL_GENERATED != 0;
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

    for (uint8_t class_idx = 0; class_idx < PERCEPTRON_CLASS_COUNT; ++class_idx)
    {
        float score = g_perceptron_bias[class_idx];
        for (uint16_t i = 0; i < PERCEPTRON_INPUT_SIZE; ++i)
        {
            score += ((float)input[i] / 255.0f) * g_perceptron_weights[class_idx][i];
        }

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
