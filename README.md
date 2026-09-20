# ch32v003_lcd1602

An HD44780 / ST7066U character-LCD driver for the **CH32V003**, written for
MounRiver Studio. Plain C with an optional C++ class on top, two transports
(4-bit parallel or a PCF8574 I2C backpack), and four example projects that open
and build without any setup.

The folder is self-contained: it carries its own copy of the WCH SRC tree, so
you can move it, zip it, or hand it to someone else and it still builds.

---

## Layout

```
ch32v003_lcd1602/
├── lib/                        the driver - this is the part you reuse
│   ├── lcd1602_config.h        wiring and options (the only file you edit)
│   ├── ch32v003_lcd1602.h      C API
│   ├── ch32v003_lcd1602.c      implementation
│   └── LCD1602.hpp             optional header-only C++ class
├── SRC/                        WCH standard peripheral library (Core, Debug,
│                               Ld, Peripheral, Startup) - do not rename
└── examples/
    ├── 01_HelloWorld/          banner + live counter
    ├── 02_CursorControl/       cursor, blink, display on/off, addressing
    ├── 03_CustomChars/         all 8 CGRAM slots + runtime animation
    ├── 04_ScrollingText/       hardware shift vs software window
    └── CPP_usage/              C++ snippet + the four MRS steps
```

`SRC/` and `lib/` are shared by every example through Eclipse *linked folders*,
so there is one copy of the driver and one copy of the vendor library. Fix a bug
once and all four projects pick it up.

**Do not move an example project out of `examples/`.** The links resolve as
`PARENT-2-PROJECT_LOC`, which means "two directories up from the project". A
project sitting anywhere else will open with five empty folders.

---

## Quick start

1. **File > Open Projects from File System…** in MounRiver Studio.
2. Point it at `ch32v003_lcd1602\examples` — it will list all four projects.
   Import the ones you want.
3. Wire the display (below). Set the contrast pot.
4. Build (hammer icon), then Download with a WCH-Link.

The chip is preset to CH32V003F4P6 in each `.template`. If you have a different
package, change it in **Project > Configuration Register / Download settings**.

---

## Wiring

### 4-bit parallel (default)

Six MCU pins. The default map deliberately avoids every special-function pin on
the TSSOP20, so SWIO debugging, USART1 `printf` and I2C1 all stay free.

| CH32V003 | Pin # | LCD1602 | LCD pin |
|----------|-------|---------|---------|
| PC0      | 10    | RS      | 4       |
| PC3      | 13    | E       | 6       |
| PC4      | 14    | D4      | 11      |
| PC5      | 15    | D5      | 12      |
| PC6      | 16    | D6      | 13      |
| PC7      | 17    | D7      | 14      |
| GND      | 7     | VSS, RW, K | 1, 5, 16 |
| VDD      | 9     | VDD, A  | 2, 15 (A through 100 Ω) |
| —        | —     | V0      | 3 — **10 kΩ pot wiper** |

D0–D3 (LCD pins 7–10) are left unconnected. That is correct for 4-bit mode.

The contrast pot is not optional. Wire the pot's outer legs to VDD and GND and
its wiper to V0. Without it you get either a blank screen or a solid row of
blocks, and both look exactly like a dead driver.

**5 V modules on a 3.3 V MCU.** An HD44780 running at 5 V wants VIH ≈ 0.7 × VDD
= 3.5 V on its inputs, so 3.3 V logic is marginal by about 200 mV. It usually
works, and when it does not it fails intermittently, which is worse. Three ways
out, best first:

- Power the module at 3.3 V. Most modules sold today accept it; contrast just
  needs the pot turned further. Nothing else to do.
- Power it at 5 V and put a level shifter on RS, E and D4–D7. All six lines are
  MCU → LCD in the default build, so a cheap unidirectional shifter is enough.
- Power it at 5 V and accept the margin, after confirming it is stable warm and
  cold.

Whichever you pick, do not let the module drive 5 V back into a CH32V003 pin.
That is exactly why R/W is tied to GND unless you deliberately enable it — with
`LCD1602_USE_RW` on, D4–D7 become inputs during a read and a 5 V module will
push 5 V into them.

### PCF8574 I2C backpack

Two MCU pins.

| CH32V003 | Pin # | Backpack |
|----------|-------|----------|
| PC1      | 11    | SDA      |
| PC2      | 12    | SCL      |
| GND      | 7     | GND      |
| 5 V      | —     | VCC      |

Pull-ups are almost always already on the backpack board. The contrast pot is
on the backpack too — the little blue trimmer.

To switch, change one line in `lib/lcd1602_config.h`:

```c
#define LCD1602_IFACE           LCD1602_IFACE_I2C
```

No example code changes. Every function behaves identically, and
`LCD1602_Backlight()` starts doing something real.

If nothing appears, check the address before anything else:

```c
if (!LCD1602_I2C_Present()) { /* wrong address, or SDA/SCL swapped */ }
```

`0x27` is a PCF8574, `0x3F` is a PCF8574A. Trying the other one fixes most
"dead" backpacks.

---

## API

```c
#include "ch32v003_lcd1602.h"

SystemCoreClockUpdate();        /* must come first - Delay_Init needs it */
LCD1602_Init();
LCD1602_Print("Hello");
```

| Function | Notes |
|---|---|
| `LCD1602_Init()` | pins/I2C + HD44780 wake-up. Calls `Delay_Init()` for you. |
| `LCD1602_Clear()` / `LCD1602_Home()` | 1.64 ms instructions; the driver waits |
| `LCD1602_ClearLine(row)` | blanks one row, leaves the cursor at its start |
| `LCD1602_SetCursor(col, row)` | clamps out-of-range values instead of wrapping |
| `LCD1602_Display/Cursor/Blink(on)` | independent; the driver shadows the shared byte |
| `LCD1602_Backlight(on)` | I2C only; a no-op on parallel |
| `LCD1602_ScrollDisplayLeft/Right()` | moves the window, not the cursor |
| `LCD1602_MoveCursorLeft/Right()` | moves the cursor, not the window |
| `LCD1602_LeftToRight/RightToLeft()` | entry-mode direction |
| `LCD1602_Autoscroll(on)` | entry-mode S bit |
| `LCD1602_PutChar/Print/PrintAt` | plain text |
| `LCD1602_PrintInt(v, width)` | `0` none, `<0` right-justify, `>0` left-justify |
| `LCD1602_PrintFloat(v, decimals)` | fixed-point, 0–4 decimals, no newlib float |
| `LCD1602_Printf(fmt, …)` | full printf, truncated to `LCD1602_PRINTF_BUFSIZE` |
| `LCD1602_CreateChar(slot, rows[8])` | CGRAM slot 0–7; restores the DDRAM address after |
| `LCD1602_Command/Data(v)` | raw escape hatch |
| `LCD1602_ReadStatus()` | R/W builds only: bit 7 busy, bits 6–0 address counter |
| `LCD1602_I2C_Present()` | I2C builds only |

### The `width` argument

`LCD1602_PrintInt(seconds, 6)` pads to six columns. That padding is the whole
point: a counter going `100` → `99` leaves a stale `0` on screen otherwise, and
it is the single most common bug in first LCD projects.

### Floats

`LCD1602_PrintFloat` does the conversion in integer arithmetic on purpose.
MounRiver links **newlib-nano**, whose `printf` has no `%f` unless you add
`-u _printf_float` to the linker. Using the helper keeps a couple of kilobytes
out of a 16 KB part.

---

## Configuration

Everything lives in `lib/lcd1602_config.h`, and every macro is `#ifndef`-wrapped,
so you can also override any of it from
**Project > Properties > C/C++ Build > Settings > GNU RISC-V Cross C Compiler >
Preprocessor > Defined symbols** without touching the shared file. That matters
here, because `lib/` is linked into all four projects at once — an override
keeps one project on I2C while the others stay parallel.

| Macro | Default | What it does |
|---|---|---|
| `LCD1602_IFACE` | `LCD1602_IFACE_GPIO4` | or `LCD1602_IFACE_I2C` |
| `LCD1602_COLS` / `LCD1602_ROWS` | 16 / 2 | 20 / 4 also works |
| `LCD1602_ROW_OFFSETS` | `{0x00,0x40,0x14,0x54}` | DDRAM row bases |
| `LCD1602_RS_PORT` / `_PIN`, `_E_`, `_D4_`…`_D7_` | PC0, PC3, PC4–PC7 | pin map |
| `LCD1602_USE_RW` | `0` | 1 = poll the busy flag |
| `LCD1602_RW_PORT` / `_PIN` | PD2 | only used when `USE_RW` is 1 |
| `LCD1602_BUSY_TIMEOUT` | 2000 | polls before falling back to a delay |
| `LCD1602_I2C_ADDR` | `0x27` | `0x3F` for a PCF8574A |
| `LCD1602_I2C_SPEED` | 100000 | 400000 also works on short wiring |
| `LCD1602_ENABLE_PRINTF` | `1` | 0 drops `LCD1602_Printf` and its newlib baggage |

### 20x4 displays

The driver already handles them. Set `LCD1602_COLS 20` and `LCD1602_ROWS 4` and
leave the offsets alone — `0x14` and `0x54` are why rows 3 and 4 of a 20x4 are
physically the continuation of rows 1 and 2.

### R/W busy-flag polling

Wire LCD pin 5 (R/W) to PD2 instead of GND, then set `LCD1602_USE_RW 1`. The
driver stops using fixed 50 µs waits and returns the moment the controller
actually clears DB7. It is faster and it adapts to modules whose internal
oscillator runs slow.

Two things to know before you enable it. First, D4–D7 become inputs during a
read, so if your module runs at 5 V it will drive 5 V into the MCU — only do
this on a 3.3 V module or through level shifting. Second, if R/W is not actually
connected the driver gives up after `LCD1602_BUSY_TIMEOUT` polls and falls back
to a 2 ms delay, so a mis-wire shows up as a slow display rather than a hang.

---

## C++

`lib/LCD1602.hpp` is header-only and stateless — an instance costs zero bytes
of RAM and the calls inline away.

```cpp
#include "LCD1602.hpp"

static LCD1602 lcd;

lcd.begin();
lcd.print("Hello, CH32V003");
lcd.setCursor(0, 1);
lcd.print(3.284f, 3);   lcd.print(" V");
lcd.printAt(0, 1, 42);          // templated: position + print in one call
```

MounRiver has to create the C++ project itself, so see
`examples/CPP_usage/README.txt` for the four steps and a ready `main.cpp`.
`lcd.printf(...)` needs `-std=gnu++11` or newer; everything else compiles under
gnu++98 as well.

---

## Code size

Measured on a CH32V003F4P6 (16 KB flash, 2 KB SRAM) with `-Os` and
`--gc-sections`, building example 01 plus the whole WCH SPL:

| Build | Flash | RAM |
|---|---|---|
| 4-bit parallel | 2,116 B | 288 B |
| PCF8574 I2C | 2,964 B | 328 B |
| parallel, `LCD1602_Printf` actually called | 4,784 B | 412 B |

So the driver costs a little over 2 KB of a 16 KB part, and the I2C back end
adds roughly 850 bytes for the hardware-I2C setup and transfer code.

`LCD1602_Printf` is the one expensive item: **+2,736 bytes of flash and +128
bytes of RAM**, almost all of it newlib's `vsnprintf`. But you only pay it if
you call it — `--gc-sections` drops the whole thing otherwise, so leaving
`LCD1602_ENABLE_PRINTF` at 1 costs nothing until the first `LCD1602_Printf`
in your code. Setting it to 0 mainly serves to make that impossible by
accident.

`LCD1602_PrintInt` and `LCD1602_PrintFloat` are the cheap alternatives and
cover most of what 16 columns can usefully show. MounRiver prints the section
sizes in the Console after every build, and `obj/*.map` has the per-symbol
breakdown.

---

## Troubleshooting

| Symptom | Cause |
|---|---|
| Blank screen, backlight on | Contrast. Turn the pot through its whole range. |
| One row of solid blocks | The controller never got initialised — normal before `LCD1602_Init()`, so check E and the four data lines if it persists. |
| Two rows of blocks | Same, but the module is in 2-line mode. Almost always a wiring fault on E. |
| Garbage characters | E strobe or a data line. Check D4–D7 are not swapped, and that D0–D3 are left floating, not grounded. |
| First character correct, rest garbage | Nibble order. One data line is on the wrong pin. |
| Text runs off after column 15 instead of wrapping | Not a bug. Row 1 starts at DDRAM `0x40`; column 16 of row 0 is invisible DDRAM, not the start of row 1. Use `LCD1602_SetCursor(0, 1)`. |
| Row 2 shows nothing | The module is in 1-line mode. `LCD1602_ROWS` must be ≥ 2 so `Init` sets the N bit. |
| Nothing at all on I2C | Wrong address. Try `0x3F`. Then check SDA/SCL are not swapped, then check the backpack's own contrast trimmer. |
| I2C worked once, then froze | Was a hang before the timeouts were added; if you still see it, a NACK left AF latched — the driver clears it, so suspect wiring or missing pull-ups. |
| Display works, then dies after a while | Almost always a brownout from the backlight. The A pin wants a series resistor (100 Ω is a reasonable start), not a direct connection to VDD. |
| `Delay_Us` far too short or long | `SystemCoreClockUpdate()` was not called before `LCD1602_Init()`. |
| Linker error on `vsnprintf` | Set `LCD1602_ENABLE_PRINTF 0`, or turn off "Use wchprintf (-lprintf)" in the linker settings. |
| Project opens with five empty folders | The project was moved. It must stay two levels below the folder holding `SRC/` and `lib/`. |

---

## Licence

The driver, the examples and the documentation are MIT — see `LICENSE`.

`SRC/` is WCH's Standard Peripheral Library, redistributed unmodified from the
official CH32V003 EVT package under its own vendor terms, and is **not** covered
by the MIT licence. See `NOTICE` for the details and for how to remove it if you
would rather pull the EVT package yourself.

---

## Reference

Timing and the initialisation sequence follow the Hitachi HD44780U datasheet
(ADE-207-272(Z), '99.09, Rev 0.0) and the Sitronix ST7066U datasheet, which is
the controller actually fitted to most modules sold today:

- the 0x3 / 0x3 / 0x3 / 0x2 power-on wake-up, sent as bare high nibbles
- E pulse width PW(EH) ≥ 450 ns
- 37 µs per ordinary instruction at f(OSC) = 270 kHz, quoted as 40 µs
- 1.52 ms for Clear Display and Return Home, quoted as 1.64 ms
- DDRAM row bases 0x00 / 0x40, plus 0x14 / 0x54 on 4-line modules

Every fixed delay in the driver carries margin over those figures (1 µs E
strobe, 50 µs per instruction, 2 ms for clear and home). That margin is
deliberate: it is what makes the driver work on the unbranded modules whose
controllers are only approximately HD44780-compatible and whose internal
oscillators run well off nominal. Wire up R/W and set `LCD1602_USE_RW 1` if you
would rather have real busy-flag polling than margin.
