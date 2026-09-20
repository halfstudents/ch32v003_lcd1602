/********************************* LCD1602 *************************************
 * File Name          : LCD1602.hpp
 * Description        : Optional header-only C++ wrapper around the C driver.
 *                      Nothing to add to the build - just #include it from a
 *                      MounRiver C++ project ("Use MRS Create C++ project").
 *
 * The class holds no state of its own; it forwards to the C driver, so an
 * instance costs zero bytes of RAM and every method inlines away. The point is
 * the nicer call syntax and the overloaded print(), not extra machinery.
 *
 *      #include "LCD1602.hpp"
 *
 *      LCD1602 lcd;
 *
 *      int main()
 *      {
 *          SystemCoreClockUpdate();
 *          lcd.begin();
 *          lcd.print("Hello, CH32V003");
 *          lcd.setCursor(0, 1);
 *          lcd.print(3.284f, 3);   lcd.print(" V");
 *          while (1) { }
 *      }
 *******************************************************************************/
#ifndef __LCD1602_HPP
#define __LCD1602_HPP

#include "ch32v003_lcd1602.h"

class LCD1602
{
public:
    /* ---- setup ---------------------------------------------------------- */
    void begin() const                                  { LCD1602_Init(); }

    /* ---- screen --------------------------------------------------------- */
    void clear() const                                  { LCD1602_Clear(); }
    void home() const                                   { LCD1602_Home(); }
    void clearLine(uint8_t row) const                   { LCD1602_ClearLine(row); }
    void setCursor(uint8_t col, uint8_t row) const      { LCD1602_SetCursor(col, row); }

    void display(bool on) const                         { LCD1602_Display(on ? 1 : 0); }
    void cursor(bool on) const                          { LCD1602_Cursor(on ? 1 : 0); }
    void blink(bool on) const                           { LCD1602_Blink(on ? 1 : 0); }
    void backlight(bool on) const                       { LCD1602_Backlight(on ? 1 : 0); }

    /* ---- scrolling ------------------------------------------------------ */
    void scrollDisplayLeft() const                      { LCD1602_ScrollDisplayLeft(); }
    void scrollDisplayRight() const                     { LCD1602_ScrollDisplayRight(); }
    void moveCursorLeft() const                         { LCD1602_MoveCursorLeft(); }
    void moveCursorRight() const                        { LCD1602_MoveCursorRight(); }
    void leftToRight() const                            { LCD1602_LeftToRight(); }
    void rightToLeft() const                            { LCD1602_RightToLeft(); }
    void autoscroll(bool on) const                      { LCD1602_Autoscroll(on ? 1 : 0); }

    /* ---- custom glyphs -------------------------------------------------- */
    void createChar(uint8_t slot, const uint8_t rows[8]) const
                                                        { LCD1602_CreateChar(slot, rows); }

    /* ---- raw ------------------------------------------------------------ */
    void command(uint8_t c) const                       { LCD1602_Command(c); }
    void write(uint8_t v) const                         { LCD1602_Data(v); }

    /* ---- print ---------------------------------------------------------- */
    /* Overloads are written on the built-in types rather than on int32_t,
     * because on RV32 int32_t is a typedef of one of them and declaring both
     * would be a redefinition. int/long/unsigned/unsigned long are distinct. */
    void print(const char *s) const                     { LCD1602_Print(s); }
    void print(char c) const                            { LCD1602_PutChar(c); }
    void print(int v, int8_t width = 0) const           { LCD1602_PrintInt((int32_t)v, width); }
    void print(long v, int8_t width = 0) const          { LCD1602_PrintInt((int32_t)v, width); }
    void print(unsigned int v, int8_t width = 0) const  { LCD1602_PrintInt((int32_t)v, width); }
    void print(unsigned long v, int8_t width = 0) const { LCD1602_PrintInt((int32_t)v, width); }
    void print(float v, uint8_t decimals = 2) const     { LCD1602_PrintFloat(v, decimals); }
    void print(double v, uint8_t decimals = 2) const    { LCD1602_PrintFloat((float)v, decimals); }

    /* Position and print in one call - the pattern almost every UI loop uses. */
    void printAt(uint8_t col, uint8_t row, const char *s) const
                                                        { LCD1602_PrintAt(col, row, s); }

    template <typename T>
    void printAt(uint8_t col, uint8_t row, T v) const
    {
        LCD1602_SetCursor(col, row);
        print(v);
    }

#if LCD1602_ENABLE_PRINTF && (__cplusplus >= 201103L)
    /* Variadic templates are C++11. MounRiver's C++ projects can still default
     * to gnu++98, so this is guarded rather than assumed - the rest of the
     * class compiles either way. Set -std=gnu++11 or newer to get it back.
     * %f is not available under newlib-nano; use print(float, decimals). */
    template <typename... Args>
    int printf(const char *fmt, Args... args) const
    {
        return LCD1602_Printf(fmt, args...);
    }
#endif

    /* ---- diagnostics ---------------------------------------------------- */
#if (LCD1602_IFACE == LCD1602_IFACE_GPIO4) && LCD1602_USE_RW
    uint8_t readStatus() const                          { return LCD1602_ReadStatus(); }
    bool    busy() const                                { return (LCD1602_ReadStatus() & 0x80) != 0; }
#endif
#if (LCD1602_IFACE == LCD1602_IFACE_I2C)
    bool    present() const                             { return LCD1602_I2C_Present() != 0; }
#endif
};

#endif /* __LCD1602_HPP */
