#include "digit_preprocess.h"

#include <string.h>

static bool bitmap_get(const uint8_t *bitmap, uint16_t width, uint16_t x, uint16_t y)
{
    uint32_t index = (uint32_t)y * width + x;
    return (bitmap[index >> 3] & (uint8_t)(1U << (index & 7U))) != 0U;
}

void DigitPreprocess_Clear(uint8_t output[DIGIT_INPUT_SIZE])
{
    memset(output, 0, DIGIT_INPUT_SIZE);
}

bool DigitPreprocess_FromBitmap(const uint8_t *bitmap,
                                uint16_t bitmap_width,
                                uint16_t bitmap_height,
                                DigitBoundingBox bbox,
                                uint8_t output[DIGIT_INPUT_SIZE])
{
    DigitPreprocess_Clear(output);

    if ((bitmap == NULL) || !bbox.valid ||
        (bbox.min_x > bbox.max_x) || (bbox.min_y > bbox.max_y) ||
        (bbox.max_x >= bitmap_width) || (bbox.max_y >= bitmap_height))
    {
        return false;
    }

    uint16_t src_w = (uint16_t)(bbox.max_x - bbox.min_x + 1U);
    uint16_t src_h = (uint16_t)(bbox.max_y - bbox.min_y + 1U);
    uint16_t max_dim = (src_w > src_h) ? src_w : src_h;
    if (max_dim == 0U)
    {
        return false;
    }

    uint16_t scaled_w = (uint16_t)(((uint32_t)src_w * 20U + (max_dim / 2U)) / max_dim);
    uint16_t scaled_h = (uint16_t)(((uint32_t)src_h * 20U + (max_dim / 2U)) / max_dim);
    if (scaled_w == 0U)
    {
        scaled_w = 1U;
    }
    if (scaled_h == 0U)
    {
        scaled_h = 1U;
    }
    if (scaled_w > DIGIT_INPUT_WIDTH)
    {
        scaled_w = DIGIT_INPUT_WIDTH;
    }
    if (scaled_h > DIGIT_INPUT_HEIGHT)
    {
        scaled_h = DIGIT_INPUT_HEIGHT;
    }

    uint16_t x_offset = (uint16_t)((DIGIT_INPUT_WIDTH - scaled_w) / 2U);
    uint16_t y_offset = (uint16_t)((DIGIT_INPUT_HEIGHT - scaled_h) / 2U);

    for (uint16_t y = bbox.min_y; y <= bbox.max_y; ++y)
    {
        for (uint16_t x = bbox.min_x; x <= bbox.max_x; ++x)
        {
            if (!bitmap_get(bitmap, bitmap_width, x, y))
            {
                continue;
            }

            uint16_t rel_x = (uint16_t)(x - bbox.min_x);
            uint16_t rel_y = (uint16_t)(y - bbox.min_y);
            uint16_t dst_x = (uint16_t)(x_offset + ((uint32_t)rel_x * scaled_w) / src_w);
            uint16_t dst_y = (uint16_t)(y_offset + ((uint32_t)rel_y * scaled_h) / src_h);

            if ((dst_x < DIGIT_INPUT_WIDTH) && (dst_y < DIGIT_INPUT_HEIGHT))
            {
                output[(uint32_t)dst_y * DIGIT_INPUT_WIDTH + dst_x] = 255U;
            }
        }
    }

    return true;
}
