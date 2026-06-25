#ifndef DIGIT_PREPROCESS_H
#define DIGIT_PREPROCESS_H

#include <stdbool.h>
#include <stdint.h>

#define DIGIT_INPUT_WIDTH 28U
#define DIGIT_INPUT_HEIGHT 28U
#define DIGIT_INPUT_SIZE (DIGIT_INPUT_WIDTH * DIGIT_INPUT_HEIGHT)

typedef struct
{
    uint16_t min_x;
    uint16_t min_y;
    uint16_t max_x;
    uint16_t max_y;
    bool valid;
} DigitBoundingBox;

void DigitPreprocess_Clear(uint8_t output[DIGIT_INPUT_SIZE]);
bool DigitPreprocess_FromBitmap(const uint8_t *bitmap,
                                uint16_t bitmap_width,
                                uint16_t bitmap_height,
                                DigitBoundingBox bbox,
                                uint8_t output[DIGIT_INPUT_SIZE]);

#endif /* DIGIT_PREPROCESS_H */
