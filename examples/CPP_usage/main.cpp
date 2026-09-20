/********************************* LCD1602 *************************************
 * C++ usage - example 01 rewritten with the LCD1602 class.
 *
 * This is a snippet, not a ready-made MounRiver project, because MRS has to
 * create the C++ project itself (it adds the C++ nature and the C++ tool
 * settings that a hand-written .cproject cannot reliably fake). See
 * examples/CPP_usage/README.txt for the four steps.
 *
 * The class holds no members, so an instance costs zero bytes of RAM and every
 * call inlines straight through to the C driver. Nothing extra is added to the
 * build - LCD1602.hpp is header-only and ch32v003_lcd1602.c stays plain C.
 *******************************************************************************/
extern "C" {
#include "debug.h"
}

#include "LCD1602.hpp"

static LCD1602 lcd;

int main(void)
{
    uint32_t seconds = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    lcd.begin();

    lcd.print("Hello, CH32V003");
    lcd.printAt(0, 1, "Up:");

    while (1)
    {
        lcd.setCursor(4, 1);
        lcd.print((int)seconds, 6);     /* width 6, left-justified and padded */
        lcd.print("s");

        /* Overload set: const char*, char, int, long, unsigned, float, double.
         * float/double go through the fixed-point helper, so no newlib float
         * printf and no extra kilobytes:
         *
         *      lcd.setCursor(0, 1);
         *      lcd.print(3.284f, 3);   lcd.print(" V");
         *
         * lcd.printf("n=%d", n) is available too, but only when the project is
         * built with -std=gnu++11 or newer (it is a variadic template). */

        Delay_Ms(1000);
        seconds++;
    }
}
