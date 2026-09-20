/********************************* LCD1602 *************************************
 * Example 01 - Hello World
 *
 * The smallest useful program: bring the display up, put a fixed banner on
 * row 0, and update a live counter on row 1. If this runs, your wiring and
 * your contrast pot are both correct.
 *
 * Default wiring (4-bit parallel, from lcd1602_config.h):
 *
 *      CH32V003            16x2 module
 *        PC0  ----------->  RS  (pin 4)
 *        PC3  ----------->  E   (pin 6)
 *        PC4  ----------->  D4  (pin 11)
 *        PC5  ----------->  D5  (pin 12)
 *        PC6  ----------->  D6  (pin 13)
 *        PC7  ----------->  D7  (pin 14)
 *        GND  ----------->  RW (5), VSS (1), K (16)
 *        VDD  ----------->  VDD (2), A (15) through 100R
 *        10k pot wiper -->  V0  (3)      <-- contrast, not optional
 *
 * To use an I2C backpack instead, set LCD1602_IFACE to LCD1602_IFACE_I2C in
 * Lcd/lcd1602_config.h and move to PC1 = SDA, PC2 = SCL. Nothing below changes.
 *******************************************************************************/
#include "debug.h"
#include "ch32v003_lcd1602.h"

int main(void)
{
    uint32_t seconds = 0;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();        /* LCD1602_Init calls Delay_Init, which
                                       needs SystemCoreClock to be correct */
    LCD1602_Init();

    LCD1602_Print("Hello, CH32V003");
    LCD1602_PrintAt(0, 1, "Up:");

    while (1)
    {
        LCD1602_SetCursor(4, 1);

        /* width = 6 left-justifies and pads with spaces. That padding is what
         * stops 100 -> 99 from leaving a stale "0" hanging on the right. */
        LCD1602_PrintInt((int32_t)seconds, 6);
        LCD1602_Print("s");

        Delay_Ms(1000);
        seconds++;
    }
}
