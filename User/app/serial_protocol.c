#include "serial_protocol.h"

#include <stdio.h>
#include <string.h>

#define SERIAL_TIMEOUT_MS 100U

static uint32_t s_sequence;

static bool write_bytes(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len)
{
    return HAL_UART_Transmit(huart, (uint8_t *)data, len, SERIAL_TIMEOUT_MS) == HAL_OK;
}

static bool write_str(UART_HandleTypeDef *huart, const char *text)
{
    return write_bytes(huart, (const uint8_t *)text, (uint16_t)strlen(text));
}

static bool write_u32(UART_HandleTypeDef *huart, uint32_t value)
{
    char buffer[12];
    int len = snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)value);
    if ((len <= 0) || (len >= (int)sizeof(buffer)))
    {
        return false;
    }
    return write_bytes(huart, (const uint8_t *)buffer, (uint16_t)len);
}

static bool write_i32(UART_HandleTypeDef *huart, int32_t value)
{
    char buffer[14];
    int len = snprintf(buffer, sizeof(buffer), "%ld", (long)value);
    if ((len <= 0) || (len >= (int)sizeof(buffer)))
    {
        return false;
    }
    return write_bytes(huart, (const uint8_t *)buffer, (uint16_t)len);
}

static bool write_hex_pixels(UART_HandleTypeDef *huart, const uint8_t input[DIGIT_INPUT_SIZE])
{
    static const char hex[] = "0123456789ABCDEF";
    char pair[2];

    for (uint16_t i = 0; i < DIGIT_INPUT_SIZE; ++i)
    {
        pair[0] = hex[input[i] >> 4];
        pair[1] = hex[input[i] & 0x0FU];
        if (!write_bytes(huart, (const uint8_t *)pair, sizeof(pair)))
        {
            return false;
        }
    }

    return true;
}

void SerialProtocol_Init(void)
{
    s_sequence = 0;
}

bool SerialProtocol_SendPrediction(UART_HandleTypeDef *huart,
                                   const uint8_t input[DIGIT_INPUT_SIZE],
                                   const DigitTopKResult *result,
                                   uint32_t infer_ms)
{
    if ((huart == NULL) || (input == NULL) || (result == NULL))
    {
        return false;
    }

    s_sequence++;

    if (!write_str(huart, "{\"type\":\"prediction\",\"seq\":")) return false;
    if (!write_u32(huart, s_sequence)) return false;
    if (!write_str(huart, ",\"model\":\"")) return false;
    if (!write_str(huart, DigitRecognizer_ModelName())) return false;
    if (!write_str(huart, "\",\"modelType\":\"")) return false;
    if (!write_str(huart, DigitRecognizer_ModelTypeName())) return false;
    if (!write_str(huart, "\",\"quant\":\"")) return false;
    if (!write_str(huart, DigitRecognizer_ModelQuantName())) return false;
    if (!write_str(huart, "\"")) return false;
    if (!write_str(huart, ",\"inferMs\":")) return false;
    if (!write_u32(huart, infer_ms)) return false;
    if (!write_str(huart, ",\"w\":28,\"h\":28,\"pixels\":\"")) return false;
    if (!write_hex_pixels(huart, input)) return false;
    if (!write_str(huart, "\",\"result\":")) return false;
    if (!write_u32(huart, result->best.digit)) return false;
    if (!write_str(huart, ",\"top3\":[")) return false;

    for (uint8_t i = 0; i < 3U; ++i)
    {
        if (i != 0U)
        {
            if (!write_str(huart, ",")) return false;
        }
        if (!write_str(huart, "{\"digit\":")) return false;
        if (!write_u32(huart, result->top3[i].digit)) return false;
        if (!write_str(huart, ",\"scoreMilli\":")) return false;
        if (!write_i32(huart, result->top3[i].score_milli)) return false;
        if (!write_str(huart, "}")) return false;
    }

    return write_str(huart, "]}\n");
}
