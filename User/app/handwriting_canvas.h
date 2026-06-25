#ifndef HANDWRITING_CANVAS_H
#define HANDWRITING_CANVAS_H

#include "digit_preprocess.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    HANDWRITING_EVENT_NONE = 0,
    HANDWRITING_EVENT_CLEARED,
    HANDWRITING_EVENT_DIGIT_READY
} HandwritingEvent;

void HandwritingCanvas_Init(void);
HandwritingEvent HandwritingCanvas_ProcessTouch(uint8_t output[DIGIT_INPUT_SIZE]);
void HandwritingCanvas_ShowResult(uint8_t digit, bool model_ready);
void HandwritingCanvas_ShowMessage(const char *line1, const char *line2);
bool HandwritingCanvas_HasStroke(void);

#endif /* HANDWRITING_CANVAS_H */
