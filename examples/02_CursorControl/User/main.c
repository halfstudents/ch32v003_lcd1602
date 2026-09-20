/********************************* LCD1602 *************************************
 * Example 02 - Cursor and display control
 *
 * Walks through everything the HD44780 display-control instruction (0x08 | D C B)
 * can do, plus addressing. Each step names itself on row 0 so you can see which
 * call produced which effect.
 *
 * The one thing worth internalising: D, C and B live in the SAME instruction
 * byte. There is no way to change one without rewriting all three, and the
 * controller has no readable registers. That is why the driver keeps a shadow
 * copy - LCD1602_Cursor(1) will not accidentally blank your display.
 *
 * Wiring: see example 01 or Lcd/lcd1602_config.h.
 *******************************************************************************/
#include "debug.h"
#include "ch32v003_lcd1602.h"

#define STEP_MS     2500

static void banner(const char *title)
{
    LCD1602_Clear();
    LCD1602_PrintAt(0, 0, title);
    LCD1602_SetCursor(0, 1);
}

int main(void)
{
    uint8_t i;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    LCD1602_Init();

    while (1)
    {
        /* --- 1. cursor off (the power-on default) ------------------------- */
        banner("1 Cursor OFF");
        LCD1602_Cursor(0);
        LCD1602_Blink(0);
        LCD1602_Print("no marker");
        Delay_Ms(STEP_MS);

        /* --- 2. underline cursor ------------------------------------------ */
        banner("2 Cursor ON");
        LCD1602_Print("underline: ");
        LCD1602_Cursor(1);
        Delay_Ms(STEP_MS);

        /* --- 3. blinking block -------------------------------------------- */
        banner("3 Blink ON");
        LCD1602_Print("block: ");
        LCD1602_Cursor(0);
        LCD1602_Blink(1);
        Delay_Ms(STEP_MS);

        /* --- 4. both at once ---------------------------------------------- */
        banner("4 Cursor+Blink");
        LCD1602_Print("both: ");
        LCD1602_Cursor(1);
        LCD1602_Blink(1);
        Delay_Ms(STEP_MS);

        LCD1602_Cursor(0);
        LCD1602_Blink(0);

        /* --- 5. addressing: any column, any row --------------------------- */
        banner("5 SetCursor");
        for (i = 0; i < LCD1602_COLS; i++)
        {
            LCD1602_SetCursor(i, 1);
            LCD1602_PutChar((char)('0' + (i % 10)));
            Delay_Ms(120);
        }
        Delay_Ms(1200);

        /* Row 1 starts at DDRAM 0x40, not 0x10. SetCursor hides that, but it
         * is why writing 17 characters in a row does NOT wrap onto line 2 -
         * it runs off into the invisible part of line 1 instead. */
        banner("5b Overrun");
        LCD1602_SetCursor(10, 1);
        LCD1602_Print("ABCDEFGHIJ");     /* only "ABCDEF" is visible */
        Delay_Ms(STEP_MS);

        /* --- 6. moving the cursor without writing -------------------------- */
        banner("6 MoveCursor");
        LCD1602_Print("<-  ->");
        LCD1602_Cursor(1);
        LCD1602_SetCursor(6, 1);
        for (i = 0; i < 6; i++) { LCD1602_MoveCursorLeft();  Delay_Ms(250); }
        for (i = 0; i < 6; i++) { LCD1602_MoveCursorRight(); Delay_Ms(250); }
        LCD1602_Cursor(0);

        /* --- 7. display off keeps DDRAM intact ----------------------------- */
        banner("7 Display OFF");
        LCD1602_Print("text survives");
        Delay_Ms(1500);
        LCD1602_Display(0);             /* blank, but nothing is erased */
        Delay_Ms(1500);
        LCD1602_Display(1);             /* same text comes straight back */
        Delay_Ms(STEP_MS);

        /* --- 8. right-to-left entry mode ----------------------------------- */
        banner("8 RightToLeft");
        LCD1602_SetCursor(15, 1);
        LCD1602_RightToLeft();          /* address counter now decrements */
        LCD1602_Print("desrever");
        LCD1602_LeftToRight();          /* always put it back */
        Delay_Ms(STEP_MS);

        /* --- 9. backlight (I2C backpack only; a no-op on parallel) --------- */
        banner("9 Backlight");
        LCD1602_Print("I2C only");
        LCD1602_Backlight(0);
        Delay_Ms(1000);
        LCD1602_Backlight(1);
        Delay_Ms(1500);
    }
}
