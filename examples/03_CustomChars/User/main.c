/********************************* LCD1602 *************************************
 * Example 03 - Custom characters (CGRAM)
 *
 * The HD44780 has 64 bytes of character-generator RAM: eight glyphs of eight
 * rows, five pixels wide (only bits 4..0 of each row byte are visible, bit 4
 * is the leftmost pixel). Once written, you display a glyph by printing
 * character code 0..7 - they behave exactly like ordinary characters.
 *
 * Row 7 of each glyph is where the underline cursor draws, so it is normally
 * left blank unless the glyph is meant to reach the bottom of the cell.
 *
 * The example does three things:
 *   1. defines all eight slots and shows them side by side
 *   2. uses two of them in a realistic readout line
 *   3. rewrites slot 7 while it is on screen, which animates it - CGRAM is
 *      re-read continuously, so you do not have to reprint the character
 *
 * Wiring: see example 01 or Lcd/lcd1602_config.h.
 *******************************************************************************/
#include "debug.h"
#include "ch32v003_lcd1602.h"

/* Each byte is one row, top to bottom. Written in hex here, but the bit
 * pattern is the picture: 0x1F = 11111 = a full row. */
static const uint8_t glyph_degree[8] = { 0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t glyph_ohm[8]    = { 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0A, 0x1B, 0x00 };
static const uint8_t glyph_arrow[8]  = { 0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08, 0x00 };
static const uint8_t glyph_heart[8]  = { 0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00 };
static const uint8_t glyph_bell[8]   = { 0x04, 0x0E, 0x0E, 0x0E, 0x1F, 0x00, 0x04, 0x00 };
static const uint8_t glyph_speaker[8]= { 0x01, 0x03, 0x0F, 0x0F, 0x0F, 0x03, 0x01, 0x00 };
static const uint8_t glyph_lock[8]   = { 0x0E, 0x11, 0x11, 0x1F, 0x1B, 0x1B, 0x1F, 0x00 };
static const uint8_t glyph_batt[8]   = { 0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00 };

#define CH_DEGREE   0
#define CH_OHM      1
#define CH_ARROW    2
#define CH_HEART    3
#define CH_BELL     4
#define CH_SPEAKER  5
#define CH_LOCK     6
#define CH_BATT     7

static void load_glyphs(void)
{
    LCD1602_CreateChar(CH_DEGREE,  glyph_degree);
    LCD1602_CreateChar(CH_OHM,     glyph_ohm);
    LCD1602_CreateChar(CH_ARROW,   glyph_arrow);
    LCD1602_CreateChar(CH_HEART,   glyph_heart);
    LCD1602_CreateChar(CH_BELL,    glyph_bell);
    LCD1602_CreateChar(CH_SPEAKER, glyph_speaker);
    LCD1602_CreateChar(CH_LOCK,    glyph_lock);
    LCD1602_CreateChar(CH_BATT,    glyph_batt);
}

int main(void)
{
    uint8_t frame[8];
    uint8_t i, level;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    LCD1602_Init();
    load_glyphs();

    while (1)
    {
        /* --- 1. all eight slots, spaced out ------------------------------- */
        LCD1602_Clear();
        LCD1602_PrintAt(0, 0, "CH32V003");
        LCD1602_SetCursor(0, 1);
        for (i = 0; i < 8; i++)
        {
            LCD1602_PutChar((char)i);
            LCD1602_PutChar(' ');
        }
        Delay_Ms(10000);

        /* --- 2. used in a readout ----------------------------------------- */
        LCD1602_Clear();
        LCD1602_SetCursor(0, 0);
        LCD1602_Print("Temp ");
        LCD1602_PrintFloat(23.7f, 1);       /* newlib-nano has no %f, so this
                                               helper does the fixed-point work */
        LCD1602_PutChar(CH_DEGREE);
        LCD1602_Print("C");

        LCD1602_SetCursor(0, 1);
        LCD1602_Print("Load ");
        LCD1602_PrintInt(470, 0);
        LCD1602_PutChar(CH_OHM);
        LCD1602_Print("  ");
        LCD1602_PutChar(CH_BATT);
        LCD1602_PutChar(CH_HEART);
        Delay_Ms(3000);

        /* --- 3. animate slot 7 by rewriting it in place -------------------- */
        LCD1602_Clear();
        LCD1602_PrintAt(0, 0, "Animate slot 7");
        LCD1602_PrintAt(0, 1, "Filling: ");
        LCD1602_PutChar(CH_BATT);           /* printed once, never again */

        for (level = 0; level <= 8; level++)
        {
            /* Fill from the bottom row upwards. */
            for (i = 0; i < 8; i++)
            {
                frame[i] = (uint8_t)((i >= (8 - level)) ? 0x1F : 0x00);
            }
            LCD1602_CreateChar(CH_BATT, frame);
            Delay_Ms(220);
        }
        Delay_Ms(800);

        load_glyphs();                      /* put the battery glyph back */
    }
}
