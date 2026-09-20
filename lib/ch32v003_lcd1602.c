/********************************* LCD1602 *************************************
 * File Name          : ch32v003_lcd1602.c
 * Description        : HD44780 / ST7066U character-LCD driver for CH32V003.
 *
 * Two transport back ends share one command layer:
 *   LCD1602_IFACE_GPIO4 - RS/E/D4..D7 bit-banged, optional R/W busy polling
 *   LCD1602_IFACE_I2C   - PCF8574 backpack on hardware I2C1 (PC1/PC2)
 *
 * Timing follows the HD44780 datasheet with margin: E high >= 450 ns (we use
 * ~1 us), 40 us per ordinary instruction (we use 50 us), 1.64 ms for clear and
 * home (we use 2 ms). With R/W wired up those fixed waits are replaced by real
 * busy-flag polling.
 *******************************************************************************/
#include "ch32v003_lcd1602.h"
#include "debug.h"          /* Delay_Init / Delay_Us / Delay_Ms */

#if LCD1602_ENABLE_PRINTF
#include <stdarg.h>
#include <stdio.h>
#endif

/* Busy-flag polling only makes sense on the parallel interface. */
#if (LCD1602_IFACE == LCD1602_IFACE_GPIO4) && LCD1602_USE_RW
#define LCD1602_RW_POLL     1
#else
#define LCD1602_RW_POLL     0
#endif

/*==============================================================================
 * Shadow state
 *   The HD44780 has no readable control registers, so the driver remembers what
 *   it last wrote. Without this, turning the cursor on would also clobber the
 *   display-on bit, because they live in the same instruction byte.
 *============================================================================*/
static uint8_t s_ctrl  = LCD1602_CMD_DISPLAY | LCD1602_DISPLAY_ON;
static uint8_t s_entry = LCD1602_CMD_ENTRY   | LCD1602_ENTRY_INCREMENT;
static uint8_t s_ready = 0;     /* 1 once the controller is in 4-bit mode */

/*==============================================================================
 * SECTION A - 4-bit parallel GPIO back end
 *============================================================================*/
#if (LCD1602_IFACE == LCD1602_IFACE_GPIO4)

static uint32_t lcd_port_rcc(GPIO_TypeDef *port)
{
    if (port == GPIOA) return RCC_APB2Periph_GPIOA;
    if (port == GPIOC) return RCC_APB2Periph_GPIOC;
    return RCC_APB2Periph_GPIOD;
}

/* BSHR/BCR are the atomic set/reset registers: one store, no read-modify-write,
 * so an interrupt can never corrupt a neighbouring pin on the same port. */
static void lcd_pin(GPIO_TypeDef *port, uint16_t pin, uint8_t level)
{
    if (level) port->BSHR = pin;
    else       port->BCR  = pin;
}

static void lcd_pin_mode(GPIO_TypeDef *port, uint16_t pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = pin;
    g.GPIO_Mode  = mode;
    g.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(port, &g);
}

static void lcd_strobe(void)
{
    lcd_pin(LCD1602_E_PORT, LCD1602_E_PIN, 1);
    Delay_Us(1);                                /* PW(EH) >= 450 ns */
    lcd_pin(LCD1602_E_PORT, LCD1602_E_PIN, 0);
    Delay_Us(1);                                /* enable cycle >= 1000 ns */
}

static void lcd_write_nibble(uint8_t nib)
{
    lcd_pin(LCD1602_D4_PORT, LCD1602_D4_PIN, (uint8_t)(nib & 0x01));
    lcd_pin(LCD1602_D5_PORT, LCD1602_D5_PIN, (uint8_t)(nib & 0x02));
    lcd_pin(LCD1602_D6_PORT, LCD1602_D6_PIN, (uint8_t)(nib & 0x04));
    lcd_pin(LCD1602_D7_PORT, LCD1602_D7_PIN, (uint8_t)(nib & 0x08));
    lcd_strobe();
}

#if LCD1602_RW_POLL
/* Flip D4..D7 between push-pull output and pulled-up input. The pull-up matters:
 * between the two read strobes the module briefly stops driving the bus. */
static void lcd_data_dir(uint8_t as_input)
{
    GPIOMode_TypeDef m = as_input ? GPIO_Mode_IPU : GPIO_Mode_Out_PP;
    lcd_pin_mode(LCD1602_D4_PORT, LCD1602_D4_PIN, m);
    lcd_pin_mode(LCD1602_D5_PORT, LCD1602_D5_PIN, m);
    lcd_pin_mode(LCD1602_D6_PORT, LCD1602_D6_PIN, m);
    lcd_pin_mode(LCD1602_D7_PORT, LCD1602_D7_PIN, m);
}

static uint8_t lcd_read_nibble(void)
{
    uint8_t n = 0;

    lcd_pin(LCD1602_E_PORT, LCD1602_E_PIN, 1);
    Delay_Us(1);                                /* t(DDR) <= 360 ns */
    if (GPIO_ReadInputDataBit(LCD1602_D4_PORT, LCD1602_D4_PIN)) n |= 0x01;
    if (GPIO_ReadInputDataBit(LCD1602_D5_PORT, LCD1602_D5_PIN)) n |= 0x02;
    if (GPIO_ReadInputDataBit(LCD1602_D6_PORT, LCD1602_D6_PIN)) n |= 0x04;
    if (GPIO_ReadInputDataBit(LCD1602_D7_PORT, LCD1602_D7_PIN)) n |= 0x08;
    lcd_pin(LCD1602_E_PORT, LCD1602_E_PIN, 0);
    Delay_Us(1);
    return n;
}

uint8_t LCD1602_ReadStatus(void)
{
    uint8_t hi, lo;

    lcd_pin(LCD1602_RS_PORT, LCD1602_RS_PIN, 0);
    lcd_pin(LCD1602_RW_PORT, LCD1602_RW_PIN, 1);
    lcd_data_dir(1);

    hi = lcd_read_nibble();
    lo = lcd_read_nibble();

    lcd_data_dir(0);
    lcd_pin(LCD1602_RW_PORT, LCD1602_RW_PIN, 0);

    return (uint8_t)((hi << 4) | lo);
}

/* Spin until DB7 clears. The guard means a mis-wired R/W degrades to the
 * write-only timing instead of hanging the whole application. */
static void lcd_wait_busy(void)
{
    uint16_t guard = LCD1602_BUSY_TIMEOUT;

    while (guard--)
    {
        if ((LCD1602_ReadStatus() & 0x80) == 0) return;
    }
    Delay_Ms(2);
}
#endif /* LCD1602_RW_POLL */

static void lcd_send(uint8_t rs, uint8_t value)
{
#if LCD1602_RW_POLL
    if (s_ready) lcd_wait_busy();
#endif

    lcd_pin(LCD1602_RS_PORT, LCD1602_RS_PIN, rs);
#if LCD1602_USE_RW
    lcd_pin(LCD1602_RW_PORT, LCD1602_RW_PIN, 0);
#endif

    lcd_write_nibble((uint8_t)(value >> 4));
    lcd_write_nibble((uint8_t)(value & 0x0F));

#if !LCD1602_RW_POLL
    Delay_Us(50);                               /* worst case is 40 us */
#endif
}

static void lcd_bus_init(void)
{
    uint32_t clocks = lcd_port_rcc(LCD1602_RS_PORT) |
                      lcd_port_rcc(LCD1602_E_PORT)  |
                      lcd_port_rcc(LCD1602_D4_PORT) |
                      lcd_port_rcc(LCD1602_D5_PORT) |
                      lcd_port_rcc(LCD1602_D6_PORT) |
                      lcd_port_rcc(LCD1602_D7_PORT);
#if LCD1602_USE_RW
    clocks |= lcd_port_rcc(LCD1602_RW_PORT);
#endif
    RCC_APB2PeriphClockCmd(clocks, ENABLE);

    lcd_pin_mode(LCD1602_RS_PORT, LCD1602_RS_PIN, GPIO_Mode_Out_PP);
    lcd_pin_mode(LCD1602_E_PORT,  LCD1602_E_PIN,  GPIO_Mode_Out_PP);
    lcd_pin_mode(LCD1602_D4_PORT, LCD1602_D4_PIN, GPIO_Mode_Out_PP);
    lcd_pin_mode(LCD1602_D5_PORT, LCD1602_D5_PIN, GPIO_Mode_Out_PP);
    lcd_pin_mode(LCD1602_D6_PORT, LCD1602_D6_PIN, GPIO_Mode_Out_PP);
    lcd_pin_mode(LCD1602_D7_PORT, LCD1602_D7_PIN, GPIO_Mode_Out_PP);
#if LCD1602_USE_RW
    lcd_pin_mode(LCD1602_RW_PORT, LCD1602_RW_PIN, GPIO_Mode_Out_PP);
    lcd_pin(LCD1602_RW_PORT, LCD1602_RW_PIN, 0);
#endif

    lcd_pin(LCD1602_RS_PORT, LCD1602_RS_PIN, 0);
    lcd_pin(LCD1602_E_PORT,  LCD1602_E_PIN,  0);
}

/* The wake-up sequence is sent as bare high nibbles, because the controller
 * does not know it is in 4-bit mode yet and would otherwise swallow the second
 * half of every byte. */
static void lcd_reset_sequence(void)
{
    lcd_pin(LCD1602_RS_PORT, LCD1602_RS_PIN, 0);

    Delay_Ms(50);                   /* >= 15 ms after VDD and contrast settle */
    lcd_write_nibble(0x03); Delay_Ms(5);        /* >= 4.1 ms */
    lcd_write_nibble(0x03); Delay_Us(200);      /* >= 100 us */
    lcd_write_nibble(0x03); Delay_Us(200);
    lcd_write_nibble(0x02); Delay_Us(200);      /* now in 4-bit mode */
}

void LCD1602_Backlight(uint8_t on)
{
    (void)on;                       /* no backlight control pin in this mode */
}

/*==============================================================================
 * SECTION B - PCF8574 I2C back end
 *============================================================================*/
#else /* LCD1602_IFACE == LCD1602_IFACE_I2C */

#define LCD1602_I2C_TIMEOUT     20000u

static uint8_t s_shadow = LCD1602_I2C_BIT_BL;   /* backlight on by default */

/* One PCF8574 byte. Returns 0 on timeout or NACK instead of spinning forever,
 * which is what makes a wrong I2C address show up as a blank screen rather
 * than a dead firmware. */
static uint8_t lcd_i2c_put(uint8_t value)
{
    uint32_t t;

    t = LCD1602_I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
    {
        if (--t == 0) return 0;
    }

    I2C_GenerateSTART(I2C1, ENABLE);
    t = LCD1602_I2C_TIMEOUT;
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != READY)
    {
        if (--t == 0) goto fail;
    }

    I2C_Send7bitAddress(I2C1, (uint8_t)(LCD1602_I2C_ADDR << 1), I2C_Direction_Transmitter);
    t = LCD1602_I2C_TIMEOUT;
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != READY)
    {
        if (--t == 0) goto fail;
    }

    I2C_SendData(I2C1, value);
    t = LCD1602_I2C_TIMEOUT;
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != READY)
    {
        if (--t == 0) goto fail;
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
    return 1;

fail:
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);   /* a NACK latches AF; clear it or the
                                           next transfer fails immediately */
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}

static void lcd_i2c_nibble(uint8_t nib, uint8_t rs)
{
    uint8_t v = (uint8_t)((nib << 4) | (rs ? LCD1602_I2C_BIT_RS : 0) | (s_shadow & LCD1602_I2C_BIT_BL));

    lcd_i2c_put((uint8_t)(v | LCD1602_I2C_BIT_E));
    Delay_Us(1);
    lcd_i2c_put(v);                     /* falling edge on E latches the nibble */
    Delay_Us(1);
}

static void lcd_write_nibble(uint8_t nib)
{
    lcd_i2c_nibble(nib, 0);
}

static void lcd_send(uint8_t rs, uint8_t value)
{
    lcd_i2c_nibble((uint8_t)(value >> 4),   rs);
    lcd_i2c_nibble((uint8_t)(value & 0x0F), rs);
    Delay_Us(50);
}

static void lcd_bus_init(void)
{
    GPIO_InitTypeDef g = {0};
    I2C_InitTypeDef  i = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    g.GPIO_Mode  = GPIO_Mode_AF_OD;
    g.GPIO_Speed = GPIO_Speed_30MHz;
    g.GPIO_Pin   = GPIO_Pin_2;                  /* SCL */
    GPIO_Init(GPIOC, &g);
    g.GPIO_Pin   = GPIO_Pin_1;                  /* SDA */
    GPIO_Init(GPIOC, &g);

    i.I2C_ClockSpeed          = LCD1602_I2C_SPEED;
    i.I2C_Mode                = I2C_Mode_I2C;
    i.I2C_DutyCycle           = I2C_DutyCycle_2;
    i.I2C_OwnAddress1         = 0x02;           /* unused, we are always master */
    i.I2C_Ack                 = I2C_Ack_Enable;
    i.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i);
    I2C_Cmd(I2C1, ENABLE);

    s_shadow = LCD1602_I2C_BIT_BL;
    lcd_i2c_put(s_shadow);                      /* idle: E low, backlight on */
}

static void lcd_reset_sequence(void)
{
    Delay_Ms(50);
    lcd_write_nibble(0x03); Delay_Ms(5);
    lcd_write_nibble(0x03); Delay_Us(200);
    lcd_write_nibble(0x03); Delay_Us(200);
    lcd_write_nibble(0x02); Delay_Us(200);
}

void LCD1602_Backlight(uint8_t on)
{
    if (on) s_shadow |= LCD1602_I2C_BIT_BL;
    else    s_shadow = (uint8_t)(s_shadow & ~LCD1602_I2C_BIT_BL);
    lcd_i2c_put(s_shadow);
}

uint8_t LCD1602_I2C_Present(void)
{
    return lcd_i2c_put(s_shadow);
}

#endif /* interface selection */

/*==============================================================================
 * SECTION C - interface-independent command layer
 *============================================================================*/

void LCD1602_Command(uint8_t cmd)
{
    lcd_send(0, cmd);

#if !LCD1602_RW_POLL
    /* Clear (0x01) and return-home (0x02, 0x03) need 1.64 ms, not 40 us. */
    if (cmd == LCD1602_CMD_CLEAR || cmd == LCD1602_CMD_HOME || cmd == 0x03)
    {
        Delay_Ms(2);
    }
#endif
}

void LCD1602_Data(uint8_t value)
{
    lcd_send(1, value);
}

void LCD1602_Init(void)
{
    uint8_t fn = LCD1602_CMD_FUNCTION;          /* DL = 0 -> 4-bit */

#if (LCD1602_ROWS > 1)
    fn |= LCD1602_FUNC_2LINE;
#endif

    Delay_Init();                               /* safe to repeat; needs a valid
                                                   SystemCoreClock            */
    s_ready = 0;
    lcd_bus_init();
    lcd_reset_sequence();

    LCD1602_Command(fn);                        /* 0x28 on a normal 16x2 */
    s_ready = 1;                                /* busy flag is meaningful now */

    LCD1602_Command(LCD1602_CMD_DISPLAY);       /* 0x08 display off */
    LCD1602_Command(LCD1602_CMD_CLEAR);         /* 0x01 */

    s_entry = LCD1602_CMD_ENTRY | LCD1602_ENTRY_INCREMENT;
    LCD1602_Command(s_entry);                   /* 0x06 */

    s_ctrl = LCD1602_CMD_DISPLAY | LCD1602_DISPLAY_ON;
    LCD1602_Command(s_ctrl);                    /* 0x0C */
}

/*---------------------------------------------------------------- screen ----*/

void LCD1602_Clear(void)
{
    LCD1602_Command(LCD1602_CMD_CLEAR);
}

void LCD1602_Home(void)
{
    LCD1602_Command(LCD1602_CMD_HOME);
}

void LCD1602_SetCursor(uint8_t col, uint8_t row)
{
    static const uint8_t offset[4] = LCD1602_ROW_OFFSETS;

    if (row >= LCD1602_ROWS) row = LCD1602_ROWS - 1;
    if (col >= LCD1602_COLS) col = LCD1602_COLS - 1;

    LCD1602_Command((uint8_t)(LCD1602_CMD_SET_DDRAM | (offset[row] + col)));
}

void LCD1602_ClearLine(uint8_t row)
{
    uint8_t i;

    LCD1602_SetCursor(0, row);
    for (i = 0; i < LCD1602_COLS; i++) LCD1602_Data(' ');
    LCD1602_SetCursor(0, row);
}

void LCD1602_Display(uint8_t on)
{
    if (on) s_ctrl |= LCD1602_DISPLAY_ON;
    else    s_ctrl = (uint8_t)(s_ctrl & ~LCD1602_DISPLAY_ON);
    LCD1602_Command(s_ctrl);
}

void LCD1602_Cursor(uint8_t on)
{
    if (on) s_ctrl |= LCD1602_CURSOR_ON;
    else    s_ctrl = (uint8_t)(s_ctrl & ~LCD1602_CURSOR_ON);
    LCD1602_Command(s_ctrl);
}

void LCD1602_Blink(uint8_t on)
{
    if (on) s_ctrl |= LCD1602_BLINK_ON;
    else    s_ctrl = (uint8_t)(s_ctrl & ~LCD1602_BLINK_ON);
    LCD1602_Command(s_ctrl);
}

/*--------------------------------------------------------------- scroll -----*/

void LCD1602_ScrollDisplayLeft(void)
{
    LCD1602_Command(LCD1602_CMD_SHIFT | LCD1602_SHIFT_DISPLAY);
}

void LCD1602_ScrollDisplayRight(void)
{
    LCD1602_Command(LCD1602_CMD_SHIFT | LCD1602_SHIFT_DISPLAY | LCD1602_SHIFT_RIGHT);
}

void LCD1602_MoveCursorLeft(void)
{
    LCD1602_Command(LCD1602_CMD_SHIFT);
}

void LCD1602_MoveCursorRight(void)
{
    LCD1602_Command(LCD1602_CMD_SHIFT | LCD1602_SHIFT_RIGHT);
}

void LCD1602_LeftToRight(void)
{
    s_entry |= LCD1602_ENTRY_INCREMENT;
    LCD1602_Command(s_entry);
}

void LCD1602_RightToLeft(void)
{
    s_entry = (uint8_t)(s_entry & ~LCD1602_ENTRY_INCREMENT);
    LCD1602_Command(s_entry);
}

void LCD1602_Autoscroll(uint8_t on)
{
    if (on) s_entry |= LCD1602_ENTRY_SHIFT;
    else    s_entry = (uint8_t)(s_entry & ~LCD1602_ENTRY_SHIFT);
    LCD1602_Command(s_entry);
}

/*----------------------------------------------------------------- text -----*/

void LCD1602_PutChar(char c)
{
    LCD1602_Data((uint8_t)c);
}

void LCD1602_Print(const char *s)
{
    if (s == 0) return;
    while (*s) LCD1602_Data((uint8_t)*s++);
}

void LCD1602_PrintAt(uint8_t col, uint8_t row, const char *s)
{
    LCD1602_SetCursor(col, row);
    LCD1602_Print(s);
}

void LCD1602_PrintInt(int32_t value, int8_t width)
{
    char     digits[11];
    uint8_t  n = 0;
    uint8_t  len;
    uint32_t u;

    if (value < 0)
    {
        /* two's complement negate in unsigned space: correct even for INT32_MIN */
        u = (uint32_t)(~((uint32_t)value) + 1u);
    }
    else
    {
        u = (uint32_t)value;
    }

    do
    {
        digits[n++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u);

    if (value < 0) digits[n++] = '-';
    len = n;                                        /* digits are emitted below */

    if (width < 0)                                  /* right-justify: pad first */
    {
        uint8_t target = (uint8_t)(-width);
        while (target > len) { LCD1602_Data(' '); target--; }
    }

    while (n) LCD1602_Data((uint8_t)digits[--n]);

    if (width > 0)                                  /* left-justify: pad after */
    {
        uint8_t target = (uint8_t)width;
        while (target > len) { LCD1602_Data(' '); target--; }
    }
}

void LCD1602_PrintFloat(float value, uint8_t decimals)
{
    static const uint32_t p10[5] = { 1u, 10u, 100u, 1000u, 10000u };
    uint32_t scale, total, whole, frac;
    uint8_t  i;

    if (decimals > 4) decimals = 4;
    scale = p10[decimals];

    if (value < 0.0f)
    {
        LCD1602_Data('-');
        value = -value;
    }

    total = (uint32_t)((value * (float)scale) + 0.5f);   /* round, do not truncate */
    whole = total / scale;
    frac  = total % scale;

    LCD1602_PrintInt((int32_t)whole, 0);

    if (decimals)
    {
        LCD1602_Data('.');
        for (i = decimals; i > 0; i--)
        {
            LCD1602_Data((uint8_t)('0' + ((frac / p10[i - 1]) % 10u)));
        }
    }
}

#if LCD1602_ENABLE_PRINTF
int LCD1602_Printf(const char *fmt, ...)
{
    char    buf[LCD1602_PRINTF_BUFSIZE];
    va_list ap;
    int     n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    LCD1602_Print(buf);
    return n;
}
#endif

/*------------------------------------------------------------- CGRAM --------*/

void LCD1602_CreateChar(uint8_t slot, const uint8_t rows[8])
{
    uint8_t i;

    LCD1602_Command((uint8_t)(LCD1602_CMD_SET_CGRAM | ((slot & 0x07) << 3)));
    for (i = 0; i < 8; i++) LCD1602_Data(rows[i]);

    /* Leave the address counter pointing back at DDRAM, otherwise the next
     * LCD1602_Print would keep writing into the glyph table. */
    LCD1602_Command(LCD1602_CMD_SET_DDRAM);
}
