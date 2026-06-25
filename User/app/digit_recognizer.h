#ifndef DIGIT_RECOGNIZER_H
#define DIGIT_RECOGNIZER_H

#include "digit_preprocess.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t digit;
    float score;
    bool model_ready;
} DigitRecognitionResult;

DigitRecognitionResult DigitRecognizer_Predict(const uint8_t input[DIGIT_INPUT_SIZE]);
bool DigitRecognizer_HasTrainedModel(void);

#endif /* DIGIT_RECOGNIZER_H */
