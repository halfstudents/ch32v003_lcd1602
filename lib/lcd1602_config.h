/********************************* LCD1602 *************************************
 * File Name          : lcd1602_config.h
 * Description        : Build-time configuration for the CH32V003 HD44780 driver.
 *                      THIS is the only file you normally need to edit.
 *
 * Everything below can also be overridden from the compiler command line
 * (MounRiver: Project > Properties > C/C++ Build > Settings > Defined symbols)
 * because every macro is wrapped in #ifndef.
 *******************************************************************************/
#ifndef __LCD1602_CONFIG_H
#define __LCD1602_CONFIG_H

/*==============================================================================
 * 1. INTERFACE SELECTION
 *============================================================================*/
#define LCD1602_IFACE_GPIO4     0   /* 4-bit parallel, 6 MCU pins            */
#define LCD1602_IFACE_I2C       1   /* PCF8574 "backpack", 2 MCU pins        */

#ifndef LCD1602_IFACE
#define LCD1602_IFACE           LCD1602_IFACE_I2C   // Change this thing to change I2C and normal 4-bit mode
#endif

/*==============================================================================
 * 2. DISPLAY GEOMETRY
 *   16x2 -> COLS 16, ROWS 2.  A 20x4 module works too: COLS 20, ROWS 4.
 *   Row offsets are HD44780 DDRAM base addresses, NOT sequential. See the
 *   HD44780 practical guide: row 1 lives at 0x40, not at 0x10.
 *============================================================================*/
#ifndef LCD1602_COLS
#define LCD1602_COLS            16
#endif

#ifndef LCD1602_ROWS
#define LCD1602_ROWS            2
#endif

#ifndef LCD1602_ROW_OFFSETS
#define LCD1602_ROW_OFFSETS     { 0x00, 0x40, 0x14, 0x54 }
#endif

/*==============================================================================
 * 3. PIN MAP  (LCD1602_IFACE_GPIO4 only)
 *
 *   Default map keeps every "special" CH32V003F4P6 pin free:
 *     PD1  = SWIO debug        PC1/PC2 = I2C1        PD5/PD6 = USART1
 *
 *      CH32V003            16x2 module (HD44780)
 *        PC0  ------------>  RS   (pin 4)
 *        PC3  ------------>  E    (pin 6)
 *        PC4  ------------>  D4   (pin 11)
 *        PC5  ------------>  D5   (pin 12)
 *        PC6  ------------>  D6   (pin 13)
 *        PC7  ------------>  D7   (pin 14)
 *        GND  ------------>  RW (pin 5), VSS (pin 1), K (pin 16)
 *        VDD  ------------>  VDD (pin 2), A (pin 15, through 100R)
 *        10k pot wiper ---->  V0  (pin 3)   <-- contrast. Without it: blank
 *                                               screen or two rows of blocks.
 *============================================================================*/
#ifndef LCD1602_RS_PORT
#define LCD1602_RS_PORT         GPIOC
#define LCD1602_RS_PIN          GPIO_Pin_0
#endif

#ifndef LCD1602_E_PORT
#define LCD1602_E_PORT          GPIOC
#define LCD1602_E_PIN           GPIO_Pin_3
#endif

#ifndef LCD1602_D4_PORT
#define LCD1602_D4_PORT         GPIOC
#define LCD1602_D4_PIN          GPIO_Pin_4
#endif

#ifndef LCD1602_D5_PORT
#define LCD1602_D5_PORT         GPIOC
#define LCD1602_D5_PIN          GPIO_Pin_5
#endif

#ifndef LCD1602_D6_PORT
#define LCD1602_D6_PORT         GPIOC
#define LCD1602_D6_PIN          GPIO_Pin_6
#endif

#ifndef LCD1602_D7_PORT
#define LCD1602_D7_PORT         GPIOC
#define LCD1602_D7_PIN          GPIO_Pin_7
#endif

/*==============================================================================
 * 4. R/W BUSY-FLAG POLLING  (LCD1602_IFACE_GPIO4 only)
 *
 *   0 = write-only. RW is tied to GND, the driver uses conservative fixed
 *       delays (50 us normal, 2 ms clear/home). Always works.
 *   1 = RW is wired to an MCU pin. The driver reads the busy flag on DB7 and
 *       returns as soon as the controller is actually ready. Faster, and it
 *       adapts to modules whose internal oscillator runs slow.
 *
 *   Ignored in I2C mode: reading back through a PCF8574 is unreliable on most
 *   backpacks (RW is often hard-wired low on the board), so fixed delays are
 *   always used there.
 *============================================================================*/
#ifndef LCD1602_USE_RW
#define LCD1602_USE_RW          0
#endif

#ifndef LCD1602_RW_PORT
#define LCD1602_RW_PORT         GPIOD
#define LCD1602_RW_PIN          GPIO_Pin_2
#endif

/* Give up on the busy flag after this many polls and fall back to a delay,
 * so a mis-wired RW pin cannot lock the firmware up forever. */
#ifndef LCD1602_BUSY_TIMEOUT
#define LCD1602_BUSY_TIMEOUT    2000
#endif

/*==============================================================================
 * 5. I2C BACKPACK  (LCD1602_IFACE_I2C only)
 *
 *      CH32V003            PCF8574 backpack
 *        PC1  <----------->  SDA        (4k7 pull-up to VCC, usually on-board)
 *        PC2  ------------>  SCL        (4k7 pull-up to VCC)
 *        GND  ------------>  GND
 *        5V   ------------>  VCC        (most backpacks want 5 V for contrast)
 *
 *   Address: 0x27 for a PCF8574, 0x3F for a PCF8574A. Both are 7-bit.
 *   If nothing appears, try the other one - it is the single most common cause.
 *
 *   Standard backpack bit order (matches every cheap module on the market):
 *      P0=RS  P1=RW  P2=E  P3=Backlight  P4=D4  P5=D5  P6=D6  P7=D7
 *============================================================================*/
#ifndef LCD1602_I2C_ADDR
#define LCD1602_I2C_ADDR        0x27
#endif

#ifndef LCD1602_I2C_SPEED
#define LCD1602_I2C_SPEED       100000
#endif

#ifndef LCD1602_I2C_BIT_RS
#define LCD1602_I2C_BIT_RS      0x01
#define LCD1602_I2C_BIT_RW      0x02
#define LCD1602_I2C_BIT_E       0x04
#define LCD1602_I2C_BIT_BL      0x08
#endif

/*==============================================================================
 * 6. OPTIONAL CODE SIZE SWITCHES
 *   The CH32V003 has 16 KB of flash. LCD1602_Printf drags in newlib's
 *   vsnprintf (roughly 2-3 KB). Set to 0 if you are running out of room and
 *   use LCD1602_PrintInt / LCD1602_PrintFloat instead.
 *============================================================================*/
#ifndef LCD1602_ENABLE_PRINTF
#define LCD1602_ENABLE_PRINTF   1
#endif

#ifndef LCD1602_PRINTF_BUFSIZE
#define LCD1602_PRINTF_BUFSIZE  (LCD1602_COLS + 8)
#endif

#endif /* __LCD1602_CONFIG_H */
