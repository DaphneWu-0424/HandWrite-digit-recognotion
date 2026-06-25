#include "digit_recognizer.h"

#include "PerceptronData.h"

#include <float.h>

bool DigitRecognizer_HasTrainedModel(void)
{
    return PERCEPTRON_MODEL_GENERATED != 0;
}

DigitRecognitionResult DigitRecognizer_Predict(const uint8_t input[DIGIT_INPUT_SIZE])
{
    DigitRecognitionResult result = {0U, -FLT_MAX, DigitRecognizer_HasTrainedModel()};

    for (uint8_t class_idx = 0; class_idx < PERCEPTRON_CLASS_COUNT; ++class_idx)
    {
        float score = g_perceptron_bias[class_idx];
        for (uint16_t i = 0; i < PERCEPTRON_INPUT_SIZE; ++i)
        {
            score += ((float)input[i] / 255.0f) * g_perceptron_weights[class_idx][i];
        }

        if ((class_idx == 0U) || (score > result.score))
        {
            result.score = score;
            result.digit = class_idx;
        }
    }

    return result;
}
