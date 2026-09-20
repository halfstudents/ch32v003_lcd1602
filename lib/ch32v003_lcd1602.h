/********************************* LCD1602 *************************************
 * File Name          : ch32v003_lcd1602.h
 * Description        : HD44780 / ST7066U character-LCD driver for CH32V003.
 *                      4-bit parallel GPIO or PCF8574 I2C backpack.
 *
 * Usage (C):
 *      #include "ch32v003_lcd1602.h"
 *
 *      SystemCoreClockUpdate();
 *      LCD1602_Init();
 *      LCD1602_Print("Hello, CH32V003");
 *      LCD1602_SetCursor(0, 1);
 *      LCD1602_Printf("count=%d", n);
 *
 * Usage (C++): see LCD1602.hpp for the class wrapper.
 *
 * Wiring and options live in lcd1602_config.h.
 *******************************************************************************/
#ifndef __CH32V003_LCD1602_H
#define __CH32V003_LCD1602_H

#include "ch32v00x.h"
#include "lcd1602_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * HD44780 instruction set (see the HD44780 practical guide, Table 5)
 *============================================================================*/
#define LCD1602_CMD_CLEAR           0x01    /* clear DDRAM, AC = 0, 1.64 ms   */
#define LCD1602_CMD_HOME            0x02    /* AC = 0, unshift, 1.64 ms       */
#define LCD1602_CMD_ENTRY           0x04    /* 0000 01 I/D S                  */
#define LCD1602_CMD_DISPLAY         0x08    /* 0000 1 D C B                   */
#define LCD1602_CMD_SHIFT           0x10    /* 0001 S/C R/L - -               */
#define LCD1602_CMD_FUNCTION        0x20    /* 001 DL N F - -                 */
#define LCD1602_CMD_SET_CGRAM       0x40    /* 01 AAAAAA                      */
#define LCD1602_CMD_SET_DDRAM       0x80    /* 1 AAAAAAA                      */

/* Entry-mode bits */
#define LCD1602_ENTRY_INCREMENT     0x02    /* I/D = 1                        */
#define LCD1602_ENTRY_SHIFT         0x01    /* S   = 1 (autoscroll)           */

/* Display-control bits */
#define LCD1602_DISPLAY_ON          0x04    /* D                              */
#define LCD1602_CURSOR_ON           0x02    /* C                              */
#define LCD1602_BLINK_ON            0x01    /* B                              */

/* Cursor/display-shift bits */
#define LCD1602_SHIFT_DISPLAY       0x08    /* S/C = 1: shift view, AC unchanged */
#define LCD1602_SHIFT_RIGHT         0x04    /* R/L = 1                        */

/* Function-set bits */
#define LCD1602_FUNC_8BIT           0x10    /* DL                             */
#define LCD1602_FUNC_2LINE          0x08    /* N                              */
#define LCD1602_FUNC_5x10           0x04    /* F                              */

/*==============================================================================
 * Setup
 *============================================================================*/

/* Configure the pins (or I2C1), run the HD44780 power-on sequence and leave
 * the display cleared, on, cursor off, entry mode = increment/no shift.
 * Calls Delay_Init() internally, so call SystemCoreClockUpdate() first. */
void     LCD1602_Init(void);

/*==============================================================================
 * Raw access - use these to reach anything the helpers below do not cover
 *============================================================================*/
void     LCD1602_Command(uint8_t cmd);          /* RS = 0 */
void     LCD1602_Data(uint8_t value);           /* RS = 1 */

/*==============================================================================
 * Screen control
 *============================================================================*/
void     LCD1602_Clear(void);                   /* erase + cursor home        */
void     LCD1602_Home(void);                    /* cursor home, keeps text    */
void     LCD1602_ClearLine(uint8_t row);        /* blank one row, cursor to its start */
void     LCD1602_SetCursor(uint8_t col, uint8_t row);

void     LCD1602_Display(uint8_t on);           /* whole display on/off       */
void     LCD1602_Cursor(uint8_t on);            /* underline cursor           */
void     LCD1602_Blink(uint8_t on);             /* blinking block cursor      */
void     LCD1602_Backlight(uint8_t on);         /* I2C backpack only; no-op on GPIO */

/*==============================================================================
 * Scrolling / entry mode
 *============================================================================*/
void     LCD1602_ScrollDisplayLeft(void);       /* move the window, not the AC */
void     LCD1602_ScrollDisplayRight(void);
void     LCD1602_MoveCursorLeft(void);          /* move the AC, not the window */
void     LCD1602_MoveCursorRight(void);
void     LCD1602_LeftToRight(void);             /* AC increments after a write */
void     LCD1602_RightToLeft(void);             /* AC decrements after a write */
void     LCD1602_Autoscroll(uint8_t on);        /* entry-mode S bit            */

/*==============================================================================
 * Text output
 *============================================================================*/
void     LCD1602_PutChar(char c);
void     LCD1602_Print(const char *s);
void     LCD1602_PrintAt(uint8_t col, uint8_t row, const char *s);

/* Integer, no newlib dependency.
 *   width = 0  : no padding
 *   width < 0  : right-justify in -width columns (pad with spaces first)
 *   width > 0  : left-justify in  width columns (pad with spaces after)
 * Padding is what stops a counter going 100 -> 99 from leaving a stale digit. */
void     LCD1602_PrintInt(int32_t value, int8_t width);

/* Fixed-point float, no newlib float printf needed (newlib-nano does not
 * support %f without extra linker flags). decimals is clamped to 0..4. */
void     LCD1602_PrintFloat(float value, uint8_t decimals);

#if LCD1602_ENABLE_PRINTF
/* Full printf. Output is truncated to LCD1602_PRINTF_BUFSIZE-1 characters.
 * %f is NOT available under newlib-nano - use LCD1602_PrintFloat instead. */
int      LCD1602_Printf(const char *fmt, ...);
#endif

/*==============================================================================
 * Custom characters (CGRAM)
 *   slot is 0..7, rows is 8 bytes, only bits 4..0 of each byte are visible.
 *   Print the glyph by writing character code 0..7.
 *============================================================================*/
void     LCD1602_CreateChar(uint8_t slot, const uint8_t rows[8]);

/*==============================================================================
 * Diagnostics
 *============================================================================*/
#if (LCD1602_IFACE == LCD1602_IFACE_GPIO4) && LCD1602_USE_RW
/* Raw instruction-register read: bit 7 = busy flag, bits 6..0 = address counter.
 * Only available when R/W is wired to an MCU pin. */
uint8_t  LCD1602_ReadStatus(void);
#endif

#if (LCD1602_IFACE == LCD1602_IFACE_I2C)
/* 1 if a device ACKs at LCD1602_I2C_ADDR, 0 otherwise. Call after LCD1602_Init()
 * (or after your own I2C1 setup) to tell "wrong address" from "wrong contrast". */
uint8_t  LCD1602_I2C_Present(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CH32V003_LCD1602_H */
