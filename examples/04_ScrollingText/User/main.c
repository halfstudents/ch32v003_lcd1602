/********************************* LCD1602 *************************************
 * Example 04 - Scrolling text
 *
 * Two different ways to move text, because they solve different problems.
 *
 *   Hardware shift (LCD1602_ScrollDisplayLeft)
 *     Moves the 16-character viewing window across the controller's own DDRAM.
 *     Each line of a 2-line module holds 40 characters, so up to 40 fit and
 *     scroll for free - no MCU work per frame, no reprinting. The catches:
 *     both rows shift together, 40 characters is a hard ceiling, and after 40
 *     shifts the window wraps back around to the start of the same line.
 *
 *   Software window (scroll_window below)
 *     Reprints a 16-character slice of an arbitrarily long string every frame.
 *     Costs one full line write per step, but the message can be any length
 *     and each row scrolls independently.
 *
 * Wiring: see example 01 or Lcd/lcd1602_config.h.
 *******************************************************************************/
#include "debug.h"
#include "ch32v003_lcd1602.h"

/* Rows are 40 characters wide inside the controller even though only 16 show. */
#define DDRAM_LINE_LEN  40

static const char *hw_top = "CH32V003 driving an HD44780 in 4-bit ";
static const char *hw_bot = "40 chars per line fit in DDRAM, 16 show ";

static const char *sw_msg =
    "This message is far longer than forty characters, so the hardware "
    "shift cannot hold it. The MCU reprints a sixteen-character window "
    "instead.   ";

static uint16_t str_len(const char *s)
{
    uint16_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Print the 16-character slice of s that starts at offset, wrapping around the
 * end so the message loops seamlessly. */
static void scroll_window(const char *s, uint16_t len, uint16_t offset, uint8_t row)
{
    uint8_t i;

    LCD1602_SetCursor(0, row);
    for (i = 0; i < LCD1602_COLS; i++)
    {
        LCD1602_PutChar(s[(offset + i) % len]);
    }
}

int main(void)
{
    uint16_t len = str_len(sw_msg);
    uint16_t offset;
    uint8_t  i;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    LCD1602_Init();

    while (1)
    {
        /*=== PART 1 - hardware shift =====================================*/
        LCD1602_Clear();

        /* Write past column 15 on purpose. Those characters land in the
         * invisible part of DDRAM and only appear once the window moves. */
        LCD1602_SetCursor(0, 0);
        LCD1602_Print(hw_top);
        LCD1602_SetCursor(0, 1);
        LCD1602_Print(hw_bot);
        Delay_Ms(1500);

        for (i = 0; i < (DDRAM_LINE_LEN - LCD1602_COLS); i++)
        {
            LCD1602_ScrollDisplayLeft();
            Delay_Ms(200);
        }
        Delay_Ms(600);

        /* Shift back rather than using Home, so you can see the window is a
         * view and the text never moved. Home would jump straight back. */
        for (i = 0; i < (DDRAM_LINE_LEN - LCD1602_COLS); i++)
        {
            LCD1602_ScrollDisplayRight();
            Delay_Ms(60);
        }
        LCD1602_Home();                 /* clears the accumulated shift offset */
        Delay_Ms(1200);

        /*=== PART 2 - software window ====================================*/
        LCD1602_Clear();
        LCD1602_PrintAt(0, 0, "Software window");

        for (offset = 0; offset < len; offset++)
        {
            scroll_window(sw_msg, len, offset, 1);
            Delay_Ms(180);
        }

        /*=== PART 3 - both rows, opposite directions =====================*/
        LCD1602_Clear();
        for (offset = 0; offset < len; offset++)
        {
            scroll_window(sw_msg, len, offset, 0);
            scroll_window(sw_msg, len, (uint16_t)(len - 1 - offset), 1);
            Delay_Ms(180);
        }
    }
}
