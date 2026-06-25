#include "handwriting_canvas.h"

#include "bsp_ili9341_lcd.h"
#include "bsp_xpt2046_lcd.h"

#include <stdio.h>
#include <string.h>

#define CANVAS_X 8U
#define CANVAS_Y 96U
#define CANVAS_SIZE 224U
#define CANVAS_BITS (CANVAS_SIZE * CANVAS_SIZE)
#define CANVAS_BYTES ((CANVAS_BITS + 7U) / 8U)

#define OK_X 64U
#define OK_Y 64U
#define OK_W 48U
#define OK_H 24U

#define CLEAR_X 166U
#define CLEAR_Y 64U
#define CLEAR_W 66U
#define CLEAR_H 24U

#define PREVIEW_X 4U
#define PREVIEW_Y 8U
#define PREVIEW_SCALE 2U

static uint8_t s_canvas[CANVAS_BYTES];
static DigitBoundingBox s_bbox;
static bool s_stroke_active;
static bool s_stroke_complete;
static bool s_touch_was_down;
static int16_t s_prev_x;
static int16_t s_prev_y;

static bool point_in_rect(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    return (x >= (int16_t)rx) && (x < (int16_t)(rx + rw)) &&
           (y >= (int16_t)ry) && (y < (int16_t)(ry + rh));
}

static bool point_in_canvas(int16_t x, int16_t y)
{
    return point_in_rect(x, y, CANVAS_X, CANVAS_Y, CANVAS_SIZE, CANVAS_SIZE);
}

static void bitmap_set(uint16_t x, uint16_t y)
{
    if ((x >= CANVAS_SIZE) || (y >= CANVAS_SIZE))
    {
        return;
    }

    uint32_t index = (uint32_t)y * CANVAS_SIZE + x;
    s_canvas[index >> 3] |= (uint8_t)(1U << (index & 7U));

    if (!s_bbox.valid)
    {
        s_bbox.min_x = x;
        s_bbox.max_x = x;
        s_bbox.min_y = y;
        s_bbox.max_y = y;
        s_bbox.valid = true;
        return;
    }

    if (x < s_bbox.min_x) s_bbox.min_x = x;
    if (x > s_bbox.max_x) s_bbox.max_x = x;
    if (y < s_bbox.min_y) s_bbox.min_y = y;
    if (y > s_bbox.max_y) s_bbox.max_y = y;
}

static void canvas_set_thick_point(uint16_t x, uint16_t y)
{
    for (int16_t dy = -2; dy <= 2; ++dy)
    {
        for (int16_t dx = -2; dx <= 2; ++dx)
        {
            int16_t px = (int16_t)x + dx;
            int16_t py = (int16_t)y + dy;
            if ((px >= 0) && (py >= 0))
            {
                bitmap_set((uint16_t)px, (uint16_t)py);
            }
        }
    }
}

static void canvas_record_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int16_t dx = (x0 < x1) ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y0 < y1) ? (int16_t)(y0 - y1) : (int16_t)(y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (int16_t)(dx + dy);
    int16_t x = (int16_t)x0;
    int16_t y = (int16_t)y0;

    while (1)
    {
        if ((x >= 0) && (y >= 0))
        {
            canvas_set_thick_point((uint16_t)x, (uint16_t)y);
        }

        if ((x == (int16_t)x1) && (y == (int16_t)y1))
        {
            break;
        }

        int16_t e2 = (int16_t)(2 * err);
        if (e2 >= dy)
        {
            err = (int16_t)(err + dy);
            x = (int16_t)(x + sx);
        }
        if (e2 <= dx)
        {
            err = (int16_t)(err + dx);
            y = (int16_t)(y + sy);
        }
    }
}

static void clear_canvas_data(void)
{
    memset(s_canvas, 0, sizeof(s_canvas));
    memset(&s_bbox, 0, sizeof(s_bbox));
    s_stroke_active = false;
    s_stroke_complete = false;
    s_touch_was_down = false;
    s_prev_x = -1;
    s_prev_y = -1;
}

static void draw_buttons(void)
{
    LCD_SetColors(WHITE, BLACK);
    ILI9341_DrawRectangle(OK_X, OK_Y, OK_W, OK_H, 0);
    ILI9341_DispString_EN(OK_X + 14U, OK_Y + 5U, "OK");

    ILI9341_DrawRectangle(CLEAR_X, CLEAR_Y, CLEAR_W, CLEAR_H, 0);
    ILI9341_DispString_EN(CLEAR_X + 9U, CLEAR_Y + 5U, "Clear");
}

static void draw_preview(const uint8_t input[DIGIT_INPUT_SIZE])
{
    ILI9341_Clear(PREVIEW_X, PREVIEW_Y,
                  DIGIT_INPUT_WIDTH * PREVIEW_SCALE,
                  DIGIT_INPUT_HEIGHT * PREVIEW_SCALE);

    LCD_SetTextColor(GREEN);
    for (uint16_t y = 0; y < DIGIT_INPUT_HEIGHT; ++y)
    {
        for (uint16_t x = 0; x < DIGIT_INPUT_WIDTH; ++x)
        {
            if (input[(uint32_t)y * DIGIT_INPUT_WIDTH + x] == 0U)
            {
                continue;
            }
            ILI9341_DrawRectangle((uint16_t)(PREVIEW_X + x * PREVIEW_SCALE),
                                  (uint16_t)(PREVIEW_Y + y * PREVIEW_SCALE),
                                  PREVIEW_SCALE,
                                  PREVIEW_SCALE,
                                  1);
        }
    }
    LCD_SetTextColor(WHITE);
}

void HandwritingCanvas_ShowMessage(const char *line1, const char *line2)
{
    ILI9341_Clear(64, 4, 168, 54);
    if (line1 != NULL)
    {
        ILI9341_DispString_EN(66, 8, (char *)line1);
    }
    if (line2 != NULL)
    {
        ILI9341_DispString_EN(66, 28, (char *)line2);
    }
}

void HandwritingCanvas_ShowResult(uint8_t digit, bool model_ready)
{
    char line[24];
    snprintf(line, sizeof(line), "Result: %u", (unsigned int)digit);
    HandwritingCanvas_ShowMessage(line, model_ready ? "Model: trained" : "Model: placeholder");
}

void HandwritingCanvas_Init(void)
{
    clear_canvas_data();
    LCD_SetColors(WHITE, BLACK);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
    HandwritingCanvas_ShowMessage("Write digit", "Press OK when done");
    draw_buttons();
    ILI9341_DrawRectangle(CANVAS_X, CANVAS_Y, CANVAS_SIZE, CANVAS_SIZE, 0);
}

bool HandwritingCanvas_HasStroke(void)
{
    return s_bbox.valid;
}

HandwritingEvent HandwritingCanvas_ProcessTouch(uint8_t output[DIGIT_INPUT_SIZE])
{
    strType_XPT2046_Coordinate touch;
    uint8_t pressed = XPT2046_TouchDetect();

    if (pressed == TOUCH_PRESSED)
    {
        if (!XPT2046_Get_TouchedPoint(&touch, strXPT2046_TouchPara))
        {
            return HANDWRITING_EVENT_NONE;
        }

        if (!s_touch_was_down &&
            point_in_rect(touch.x, touch.y, CLEAR_X, CLEAR_Y, CLEAR_W, CLEAR_H))
        {
            HandwritingCanvas_Init();
            return HANDWRITING_EVENT_CLEARED;
        }

        if (!s_touch_was_down &&
            point_in_rect(touch.x, touch.y, OK_X, OK_Y, OK_W, OK_H))
        {
            s_touch_was_down = true;
            if (s_stroke_complete &&
                DigitPreprocess_FromBitmap(s_canvas, CANVAS_SIZE, CANVAS_SIZE, s_bbox, output))
            {
                draw_preview(output);
                return HANDWRITING_EVENT_DIGIT_READY;
            }

            HandwritingCanvas_ShowMessage("No stroke", "Write first");
            draw_buttons();
            return HANDWRITING_EVENT_NONE;
        }

        if (!point_in_canvas(touch.x, touch.y))
        {
            s_touch_was_down = true;
            return HANDWRITING_EVENT_NONE;
        }

        uint16_t local_x = (uint16_t)(touch.x - CANVAS_X);
        uint16_t local_y = (uint16_t)(touch.y - CANVAS_Y);

        LCD_SetTextColor(WHITE);
        if (s_stroke_active)
        {
            ILI9341_DrawLine((uint16_t)s_prev_x, (uint16_t)s_prev_y, (uint16_t)touch.x, (uint16_t)touch.y);
            canvas_record_line((uint16_t)(s_prev_x - CANVAS_X), (uint16_t)(s_prev_y - CANVAS_Y), local_x, local_y);
        }
        else
        {
            ILI9341_DrawCircle((uint16_t)touch.x, (uint16_t)touch.y, 2, 1);
            canvas_set_thick_point(local_x, local_y);
            s_stroke_active = true;
        }

        s_prev_x = touch.x;
        s_prev_y = touch.y;
        s_stroke_complete = true;
        s_touch_was_down = true;
        return HANDWRITING_EVENT_NONE;
    }

    s_touch_was_down = false;

    if (s_stroke_active)
    {
        s_stroke_active = false;
        s_prev_x = -1;
        s_prev_y = -1;
    }

    return HANDWRITING_EVENT_NONE;
}
