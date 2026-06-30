#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include "digit_preprocess.h"
#include "digit_recognizer.h"

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

void SerialProtocol_Init(void);
bool SerialProtocol_SendPrediction(UART_HandleTypeDef *huart,
                                   const uint8_t input[DIGIT_INPUT_SIZE],
                                   const DigitTopKResult *result,
                                   uint32_t infer_ms);

#endif /* SERIAL_PROTOCOL_H */
