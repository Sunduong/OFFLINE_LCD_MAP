# 📖 ILI9488 Datasheet — What You MUST Find

> **Strict Teacher's Note:** Reading a datasheet is NOT like reading a novel.
> You HUNT for specific information. Here's your hunting list.

---

## ✅ Checklist — Find These in the Datasheet

### 1. SPI Interface Section
- [ ] **Maximum SPI clock frequency** — What's the fastest you can drive the SPI bus?
- [ ] **SPI mode** — Is it Mode 0 (CPOL=0, CPHA=0) or Mode 3?
- [ ] **SPI data format** — How many bits per SPI transaction?
- [ ] **DC (Data/Command) pin timing** — When should DC be set relative to CS?

### 2. Initialization Sequence
- [ ] **Required init commands** — The datasheet usually has a recommended init sequence
- [ ] **Sleep Out (0x11) delay** — How long must you wait after SLPOUT?
- [ ] **Reset timing** — How long should RST be held LOW?

### 3. Display Memory (GRAM) Organization
- [ ] **Frame memory layout** — How are pixels arranged in the internal GRAM?
- [ ] **Column/Row address range** — What are the min/max values for CASET/PASET?
- [ ] **Address direction** — Does (0,0) start at top-left or bottom-left?

### 4. MADCTL Register (0x36) — CRITICAL!
- [ ] **MY bit** — Row address order (flip vertical)
- [ ] **MX bit** — Column address order (flip horizontal)
- [ ] **MV bit** — Row/Column exchange (rotation)
- [ ] **ML bit** — Vertical refresh order
- [ ] **RGB bit** — RGB vs BGR order

> **Why this matters:** You WILL need rotation. When you mount the LCD in your
> motorbike device, the display might be portrait or landscape. MADCTL controls this.

### 5. COLMOD Register (0x3A)
- [ ] **Value for 16-bit (RGB565)** — Should be 0x55
- [ ] **Value for 18-bit (RGB666)** — Should be 0x66
- [ ] **Which format does SPI mode support?** (Hint: SPI typically uses 16-bit)

### 6. Pixel Data Format
- [ ] **RGB565 bit layout in SPI mode** — How are 16 bits mapped to R/G/B?
- [ ] **Byte order** — MSB first or LSB first?
- [ ] **How RAMWR (0x2C) works** — Does it auto-increment address?

### 7. Power & Timing
- [ ] **Supply voltage (VDD)** — Typically 2.4V–3.3V
- [ ] **IO voltage (VDDI)** — Must match ESP32-S3 logic level (3.3V)
- [ ] **Backlight voltage/forward current** — For LED backlight pins

---

## 🔍 How to Read the Datasheet Efficiently

### Don't read cover-to-cover! Use this strategy:

```
Step 1: Find the TABLE OF CONTENTS
 ↓
Step 2: Jump to "SPI Interface" or "Serial Peripheral Interface"
 ↓
Step 3: Find "Command Table" or "Register Description"
 ↓
Step 4: Look for "Initialization Code" or "Recommended Settings"
 ↓
Step 5: Check "AC Characteristics" for timing diagrams
```

### Key Sections to Find (by typical datasheet structure):

| Section Name | What You Need From It |
|-------------|----------------------|
| Pin Description | Confirm DC, RST, CS pin functions |
| SPI Protocol | Timing, mode, max speed |
| Command List | All available commands and their parameters |
| Register Description | MADCTL, COLMOD, CASET, PASET details |
| Initialization | Recommended power-on sequence |
| AC Characteristics | Timing requirements (delays, setup times) |

---

## ⚠️ Common Gotchas When Reading ILI9488 Datasheet

### Gotcha 1: SPI vs Parallel Mode
The ILI9488 supports BOTH SPI and parallel (MCU) interfaces.
**Make sure you're reading the SPI section, NOT the parallel section!**
- SPI uses: MOSI, SCK, CS, DC
- Parallel uses: DB0-DB17, RD, WR, CS, DC

### Gotcha 2: 18-bit vs 16-bit Color
- In **parallel mode**, ILI9488 can do 18-bit (RGB666)
- In **SPI mode**, you typically use **16-bit (RGB565)** — fewer bits to transfer
- COLMOD = 0x55 for 16-bit, 0x66 for 18-bit

### Gotcha 3: RAMWR Auto-Increment
After you send RAMWR (0x2C), the address auto-increments:
```
Column increments → when it reaches end, wraps to next row
Row increments → when it reaches end, stops (or wraps to start)
```
This means you can send ALL pixels for a rectangle in ONE continuous stream!

### Gotcha 4: Caset/PASET Use 16-bit Values
CASET and PASET each take **4 bytes** (2 bytes for start, 2 bytes for end):
```
CASET: [Start_H, Start_L, End_H, End_L]
PASET: [Start_H, Start_L, End_H, End_L]
```
Even if your coordinates are small (e.g., x=10), you still send 2 bytes: [0x00, 0x0A]

---

## 📝 Your Assignment

After reading the datasheet, you should be able to answer:

1. What is the maximum SPI clock frequency for ILI9488?
2. What value do you write to MADCTL (0x36) to rotate the display 90° clockwise?
3. After sending SLPOUT (0x11), how long must you wait before the next command?
4. In RGB565 mode, what is the byte order for the color RED (0xF800)?
5. How many bytes do you need to send for CASET to set columns 10–300?

**Write your answers down. I'll check them.**

---

## 🛠️ Next Step After Datasheet: Write the LCD Driver

Once you understand the datasheet, you'll write `lcd_driver.c` with these functions:

```c
// Low-level SPI communication
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(uint8_t *data, uint16_t len);

// Initialization
void lcd_init(void);

// Drawing primitives
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_fill_screen(uint16_t color);

// Advanced (later phases)
void lcd_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data);
void lcd_set_rotation(uint8_t rotation);
```

**The datasheet gives you the KNOWLEDGE. The driver is the IMPLEMENTATION.**
**Don't write a single line of driver code until you understand the datasheet.**

*Class continues when you're ready.* 🏫




## PART 3: The Initialization Sequence — Step by Step

The ILI9488 MUST be initialized in a specific order. Skip a step or delay, 
and you get a blank screen. Here's the sequence with explanations:

```
┌─────────────────────────────────────────────────────────────────┐
│ INITIALIZATION FLOW │
├─────────────────────────────────────────────────────────────────┤
│ │
│ 1. HARDWARE RESET (RST pin) │
│ RST LOW → wait 10ms → RST HIGH → wait 100ms │
│ │ │
│ ▼ │
│ 2. SOFTWARE RESET (0x01) │
│ Send SWRESET → wait 120ms │
│ │ │
│ ▼ │
│ 3. SLEEP OUT (0x11) │
│ Send SLPOUT → wait 120ms ⚠️ DON'T SKIP THIS DELAY! │
│ │ │
│ ▼ │
│ 4. SET COLOR FORMAT (0x3A) │
│ Send COLMOD + data 0x55 (RGB565) │
│ │ │
│ ▼ │
│ 5. SET ROTATION (0x36) │
│ Send MADCTL + rotation value (0x00 for portrait) │
│ │ │
│ ▼ │
│ 6. NORMAL DISPLAY MODE (0x13) │
│ Send NORON │
│ │ │
│ ▼ │
│ 7. DISPLAY ON (0x29) │
│ Send DISPON → wait 100ms │
│ │ │
│ ▼ │
│ ✅ DISPLAY IS NOW ACTIVE! │
│ │
└─────────────────────────────────────────────────────────────────┘
```

### Why Each Delay Matters

```
After SWRESET: 120ms — ILI9488 is resetting ALL internal circuits
After SLPOUT: 120ms — Internal DC-DC converter needs time to stabilize
After DISPON: 100ms — Display timing generator needs to sync

⚠️ These aren't suggestions. They're MINIMUM requirements from the datasheet.
 Too short = display doesn't work. Too long = harmless (just slower boot).
```

---

## PART 4: RGB565 — The Color Language

### How RGB565 Works

```
16-bit color value: RRRRR GGGGGG BBBBB
 ├─5──┤├──6──┤├─5──┤

 Red: 5 bits → 32 levels (0-31)
 Green: 6 bits → 64 levels (0-63) ← Green gets extra bit (human eye is more sensitive to green)
 Blue: 5 bits → 32 levels (0-31) 

Total colors: 32 × 64 × 32 = 65,536
```

### How to Build Colors

```c
// Method 1: Pre-defined colors
#define     COLOR_BLACK     0x0000 // 0b00000 000000 00000
#define     COLOR_WHITE     0xFFFF // 0b11111 111111 11111
#define     COLOR_RED       0xF800 // 0b11111 000000 00000
#define     COLOR_GREEN     0x07E0 // 0b00000 111111 00000
#define     COLOR_BLUE      0x001F // 0b00000 000000 11111
#define     COLOR_YELLOW    0xFFE0 // 0b11111 111111 00000
#define     COLOR_CYAN      0x07FF // 0b00000 111111 11111
#define     COLOR_MAGENTA   0xF81F // 0b11111 000000 11111
#define     COLOR_ORANGE    0xFD20 // 0b11111 101001 00000
#define     COLOR_GRAY      0x8410 // 0b10000 100001 10000

// Method 2: Convert from 8-bit RGB
#define     RGB_TO_RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// Example: Pure red (255, 0, 0)
// r=255=0b11111111, &0xF8=0b11111000, <<8 = 0xF800 ✅

// Method 3: Build manually
uint16_t color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
```

### Byte Order on SPI (BIG ENDIAN!)

```
RGB565 value 0xF800 (RED) is sent as TWO bytes:

 Byte 1 (first on wire): 0xF8 ← HIGH byte
 Byte 2 (second): 0x00 ← LOW byte

This is BIG ENDIAN (MSB first) — the ILI9488 expects high byte first!

In C on ESP32 (little-endian):
 uint16_t color = 0xF800;
 uint8_t *bytes = (uint8_t *)&color;
 // bytes[0] = 0x00 (LOW byte on ESP32!) ← WRONG ORDER!
 // bytes[1] = 0xF8 (HIGH byte on ESP32!)

 ⚠️ You MUST swap bytes or use __builtin_bswap16() or send high byte first!
```

---




## PART 5: Drawing a Rectangle — The Complete Flow

This is the MOST IMPORTANT operation. Everything else (lines, circles, text, 
bitmaps) builds on top of `fill_rect`.

### The 3-Step Process

```
Step 1: CASET (0x2A) — Set X range (columns)
Step 2: PASET (0x2B) — Set Y range (rows)
Step 3: RAMWR (0x2C) — Stream pixel data

After RAMWR, the ILI9488 expects pixel data for:
 (x2-x1+1) × (y2-y1+1) pixels
 Each pixel = 2 bytes (RGB565)
 Total bytes = (x2-x1+1) × (y2-y1+1) × 2
```

### Example: Fill the Entire Screen with RED

```
Screen: 320 × 480

Step 1: CASET — columns 0 to 319
 Send command: 0x2A
 Send data: 0x00, 0x00, 0x01, 0x3F
            │     │     │     │
            │     │     │     └─ 319 low byte
            │     │     └─ 319 high byte
            │     └─ 0 low byte
            └─ 0 high byte

Step 2: PASET — rows 0 to 479
 Send command: 0x2B
 Send data: 0x00, 0x00, 0x01, 0xDF
            │       │   │       │
            │       │   │       └─ 479 low byte
            │       │   └─ 479 high byte
            │       └─ 0 low byte
            └─ 0 high byte

Step 3: RAMWR — send 320 × 480 × 2 = 307,200 bytes of RED
 Send command: 0x2C
 Send data: 0xF8, 0x00, 0xF8, 0x00, ... (153,600 times)
```

### The Problem: 307 KB Is Too Much for RAM!

You can't allocate a 307 KB buffer in ESP32-S3's 512 KB SRAM. Solution:

```
STRATEGY: Send pixels in CHUNKS (line by line)

 ┌─────────────────────────────────────┐
 │ Line buffer: 320 pixels × 2 bytes │ = 640 bytes (fits in RAM!)
 │ │
 │ Fill buffer with RED │
 │ Send buffer → LCD (1 line) │
 │ Send buffer → LCD (1 line) │
 │ ... repeat 480 times │
 └─────────────────────────────────────┘

 Total SPI transactions: 1 (CASET) + 1 (PASET) + 1 (RAMWR) + 480 (data)
 But we can send ALL data lines under ONE RAMWR — address auto-increments!
```

---

## PART 6: Writing the LCD Driver — The Code

Now we put it ALL together. This is the actual driver you'll use in your project.

### File Structure

```
main/
├── main.c
└── drivers/
 └── lcd/
 ├── lcd_driver.h ← Public API
 └── lcd_driver.c ← Implementation
```

### lcd_driver.h

```c
hashtag#pragma once

hashtag#include <stdint.h>
hashtag#include "driver/spi_master.h"
hashtag#include "driver/gpio.h"

// ─── Pin Definitions ───────────────────────────────
hashtag#define LCD_PIN_MOSI 11
hashtag#define LCD_PIN_MISO 12
hashtag#define LCD_PIN_SCK 10
hashtag#define LCD_PIN_CS 9
hashtag#define LCD_PIN_DC 8
hashtag#define LCD_PIN_RST 7

// ─── Display Dimensions ────────────────────────────
hashtag#define LCD_WIDTH 320
hashtag#define LCD_HEIGHT 480

// ─── RGB565 Colors ──────────────────────────────────
hashtag#define COLOR_BLACK 0x0000
hashtag#define COLOR_WHITE 0xFFFF
hashtag#define COLOR_RED 0xF800
hashtag#define COLOR_GREEN 0x07E0
hashtag#define COLOR_BLUE 0x001F
hashtag#define COLOR_YELLOW 0xFFE0
hashtag#define COLOR_CYAN 0x07FF
hashtag#define COLOR_MAGENTA 0xF81F
hashtag#define COLOR_ORANGE 0xFD20

// ─── RGB to RGB565 Macro ───────────────────────────
hashtag#define RGB_TO_RGB565(r, g, b) \
 (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// ─── LCD Device Structure ──────────────────────────
typedef struct {
 spi_device_handle_t spi; // SPI device handle
 int dc_pin; // Data/Command pin
 int rst_pin; // Reset pin
 uint16_t width; // Current width (changes with rotation)
 uint16_t height; // Current height (changes with rotation)
 uint8_t rotation; // Current rotation (0, 1, 2, 3)
} lcd_t;

// ─── Public API ────────────────────────────────────
void lcd_init(lcd_t *lcd, spi_host_device_t host,
 int mosi, int sck, int cs, int dc, int rst);
void lcd_set_rotation(lcd_t *lcd, uint8_t rotation);
void lcd_fill_rect(lcd_t *lcd, uint16_t x1, uint16_t y1,
 uint16_t x2, uint16_t y2, uint16_t color);
void lcd_fill_screen(lcd_t *lcd, uint16_t color);
void lcd_draw_pixel(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_hline(lcd_t *lcd, uint16_t x, uint16_t y,
 uint16_t width, uint16_t color);
void lcd_draw_vline(lcd_t *lcd, uint16_t x, uint16_t y,
 uint16_t height, uint16_t color);
void lcd_draw_rect(lcd_t *lcd, uint16_t x1, uint16_t y1,
 uint16_t x2, uint16_t y2, uint16_t color);
```

### lcd_driver.c — Complete Implementation

```c
hashtag#include "lcd_driver.h"
hashtag#include "freertos/FreeRTOS.h"
hashtag#include "freertos/task.h"
hashtag#include "esp_log.h"
hashtag#include <string.h>

static const char *TAG = "LCD";

// ═══════════════════════════════════════════════════════
// PRIVATE: Low-Level SPI Communication
// ═══════════════════════════════════════════════════════

// Pre-transfer callback: sets DC pin before each SPI transaction
static void lcd_spi_pre_transfer(spi_transaction_t *t)
{
 int dc = (int)t->user; // 0 = command, 1 = data
 gpio_set_level(LCD_PIN_DC, dc);
}

// Send a command byte (DC = LOW)
static void lcd_send_cmd(lcd_t *lcd, uint8_t cmd)
{
 spi_transaction_t t = {
 .length = 8, // 8 bits
 .tx_buffer = &cmd, // Command byte
 .user = (void*)0, // DC = 0 = COMMAND
 };
 ESP_ERROR_CHECK(spi_device_polling_transmit(lcd->spi, &t));
}

// Send data bytes (DC = HIGH)
static void lcd_send_data(lcd_t *lcd, const uint8_t *data, int len)
{
 if (len == 0) return;
 
 spi_transaction_t t = {
 .length = len * 8, // Total bits
 .tx_buffer = data, // Data bytes
 .user = (void*)1, // DC = 1 = DATA
 };
 ESP_ERROR_CHECK(spi_device_polling_transmit(lcd->spi, &t));
}

// ═══════════════════════════════════════════════════════
// PRIVATE: ILI9488 Command Helpers
// ═══════════════════════════════════════════════════════

// Set the drawing window (rectangle area)
static void lcd_set_window(lcd_t *lcd, uint16_t x1, uint16_t y1,
 uint16_t x2, uint16_t y2)
{
 // CASET: Set column range (X)
 lcd_send_cmd(lcd, 0x2A);
 uint8_t col[4] = {
 (x1 >> 8) & 0xFF, x1 & 0xFF, // Start column (high, low)
 (x2 >> 8) & 0xFF, x2 & 0xFF, // End column (high, low)
 };
 lcd_send_data(lcd, col, 4);
 
 // PASET: Set row range (Y)
 lcd_send_cmd(lcd, 0x2B);
 uint8_t row[4] = {
 (y1 >> 8) & 0xFF, y1 & 0xFF, // Start row (high, low)
 (y2 >> 8) & 0xFF, y2 & 0xFF, // End row (high, low)
 };
 lcd_send_data(lcd, row, 4);
 
 // RAMWR: Prepare to receive pixel data
 lcd_send_cmd(lcd, 0x2C);
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Initialization
// ═══════════════════════════════════════════════════════

void lcd_init(lcd_t *lcd, spi_host_device_t host,
 int mosi, int sck, int cs, int dc, int rst)
{
 lcd->dc_pin = dc;
 lcd->rst_pin = rst;
 lcd->rotation = 0;
 lcd->width = LCD_WIDTH;
 lcd->height = LCD_HEIGHT;
 
 ESP_LOGI(TAG, "Initializing LCD...");
 
 // ── Step 1: Configure GPIOs ──────────────────────
 gpio_set_direction(dc, GPIO_MODE_OUTPUT);
 gpio_set_direction(rst, GPIO_MODE_OUTPUT);
 
 // ── Step 2: Initialize SPI bus ──────────────────
 spi_bus_config_t buscfg = {
 .mosi_io_num = mosi,
 .miso_io_num = -1, // LCD doesn't send data back
 .sclk_io_num = sck,
 .quadwp_io_num = -1,
 .quadhd_io_num = -1,
 .max_transfer_sz = 65536, // Large enough for full-screen transfers
 // (if PSRAM available)
 };
 ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO));
 ESP_LOGI(TAG, "SPI bus initialized");
 
 // ── Step 3: Add LCD device on the bus ────────────
 spi_device_interface_config_t devcfg = {
 .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz
 .mode = 0, // SPI Mode 0
 .spics_io_num = cs,
 .queue_size = 7,
 .pre_cb = lcd_spi_pre_transfer, // DC pin callback!
 };
 ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &lcd->spi));
 ESP_LOGI(TAG, "SPI device added");
 
 // ── Step 4: Hardware Reset ──────────────────────
 gpio_set_level(rst, 0);
 vTaskDelay(pdMS_TO_TICKS(10));
 gpio_set_level(rst, 1);
 vTaskDelay(pdMS_TO_TICKS(100));
 ESP_LOGI(TAG, "Hardware reset done");
 
 // ── Step 5: Software Reset ──────────────────────
 lcd_send_cmd(lcd, 0x01); // SWRESET
 vTaskDelay(pdMS_TO_TICKS(120));
 
 // ── Step 6: Sleep Out ───────────────────────────
 lcd_send_cmd(lcd, 0x11); // SLPOUT
 vTaskDelay(pdMS_TO_TICKS(120)); // ⚠️ DON'T SKIP!
 
 // ── Step 7: Set Pixel Format to RGB565 ──────────
 lcd_send_cmd(lcd, 0x3A); // COLMOD
 uint8_t pixel_format = 0x55; // 16-bit RGB565
 lcd_send_data(lcd, &pixel_format, 1);
 
 // ── Step 8: Set Rotation ────────────────────────
 lcd_send_cmd(lcd, 0x36); // MADCTL
 uint8_t madctl = 0x00; // Portrait, no flip
 lcd_send_data(lcd, &madctl, 1);
 
 // ── Step 9: Normal Display Mode ──────────────────
 lcd_send_cmd(lcd, 0x13); // NORON
 
 // ── Step 10: Display ON ─────────────────────────
 lcd_send_cmd(lcd, 0x29); // DISPON
 vTaskDelay(pdMS_TO_TICKS(100));
 
 ESP_LOGI(TAG, "LCD initialized successfully!");
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Rotation
// ═══════════════════════════════════════════════════════

void lcd_set_rotation(lcd_t *lcd, uint8_t rotation)
{
 lcd->rotation = rotation % 4;
 
 uint8_t madctl;
 switch (lcd->rotation) {
 case 0: // Portrait
 madctl = 0x00;
 lcd->width = LCD_WIDTH;
 lcd->height = LCD_HEIGHT;
 break;
 case 1: // Landscape (90° CW)
 madctl = 0x60; // MX + MV
 lcd->width = LCD_HEIGHT;
 lcd->height = LCD_WIDTH;
 break;
 case 2: // Portrait inverted (180°)
 madctl = 0xC0; // MY + MX
 lcd->width = LCD_WIDTH;
 lcd->height = LCD_HEIGHT;
 break;
 case 3: // Landscape inverted (270° CW)
 madctl = 0xA0; // MY + MV
 lcd->width = LCD_HEIGHT;
 lcd->height = LCD_WIDTH;
 break;
 }
 
 lcd_send_cmd(lcd, 0x36); // MADCTL
 lcd_send_data(lcd, &madctl, 1);
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Drawing Functions
// ═══════════════════════════════════════════════════════

void lcd_fill_rect(lcd_t *lcd, uint16_t x1, uint16_t y1,
 uint16_t x2, uint16_t y2, uint16_t color)
{
 // Set the drawing window
 lcd_set_window(lcd, x1, y1, x2, y2);
 
 uint16_t width = x2 - x1 + 1;
 uint16_t height = y2 - y1 + 1;
 uint32_t total_pixels = (uint32_t)width * height;
 
 // Allocate a line buffer (DMA-capable memory)
 // Size: one row of pixels × 2 bytes per pixel
 uint16_t *line_buf = heap_caps_malloc(width * 2, MALLOC_CAP_DMA);
 if (line_buf == NULL) {
 ESP_LOGE(TAG, "Failed to allocate line buffer!");
 return;
 }
 
 // Fill line buffer with the color
 // ⚠️ RGB565 is big-endian on SPI, but ESP32 is little-endian!
 // We need to byte-swap the color value
 uint16_t color_swapped = (color >> 8) | (color << 8);
 for (int i = 0; i < width; i++) {
 line_buf[i] = color_swapped;
 }
 
 // Send line by line
 for (int y = 0; y < height; y++) {
 lcd_send_data(lcd, (uint8_t*)line_buf, width * 2);
 }
 
 free(line_buf);
}

void lcd_fill_screen(lcd_t *lcd, uint16_t color)
{
 lcd_fill_rect(lcd, 0, 0, lcd->width - 1, lcd->height - 1, color);
}

void lcd_draw_pixel(lcd_t *lcd, uint16_t x, uint16_t y, uint16_t color)
{
 lcd_set_window(lcd, x, y, x, y);
 
 // Send 2 bytes for 1 pixel (big-endian)
 uint8_t data[2] = { (color >> 8) & 0xFF, color & 0xFF };
 lcd_send_data(lcd, data, 2);
}

void lcd_draw_hline(lcd_t *lcd, uint16_t x, uint16_t y,
 uint16_t width, uint16_t color)
{
 lcd_fill_rect(lcd, x, y, x + width - 1, y, color);
}

void lcd_draw_vline(lcd_t *lcd, uint16_t x, uint16_t y,
 uint16_t height, uint16_t color)
{
 lcd_fill_rect(lcd, x, y, x, y + height - 1, color);
}

void lcd_draw_rect(lcd_t *lcd, uint16_t x1, uint16_t y1,
 uint16_t x2, uint16_t y2, uint16_t color)
{
 // Draw 4 lines to form a rectangle outline
 lcd_draw_hline(lcd, x1, y1, x2 - x1 + 1, color); // Top
 lcd_draw_hline(lcd, x1, y2, x2 - x1 + 1, color); // Bottom
 lcd_draw_vline(lcd, x1, y1, y2 - y1 + 1, color); // Left
 lcd_draw_vline(lcd, x2, y1, y2 - y1 + 1, color); // Right
}
```
---

## PART 7: The Test Program — main.c

```c
hashtag#include <stdio.h>
hashtag#include "freertos/FreeRTOS.h"
hashtag#include "freertos/task.h"
hashtag#include "drivers/lcd/lcd_driver.h"

void app_main(void)
{
 // Create LCD device
 lcd_t lcd;
 
 // Initialize with our pin configuration
 lcd_init(&lcd, SPI2_HOST,
 LCD_PIN_MOSI, LCD_PIN_SCK, LCD_PIN_CS,
 LCD_PIN_DC, LCD_PIN_RST);
 
 printf("LCD initialized! Starting color test...\n");
 
 // Test: Cycle through colors
 uint16_t colors[] = {
 COLOR_RED, COLOR_GREEN, COLOR_BLUE,
 COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
 COLOR_WHITE, COLOR_BLACK
 };
 const char *names[] = {
 "RED", "GREEN", "BLUE",
 "YELLOW", "CYAN", "MAGENTA",
 "WHITE", "BLACK"
 };
 
 while (1) {
 for (int i = 0; i < 8; i++) {
 printf("Filling screen: %s\n", names[i]);
 lcd_fill_screen(&lcd, colors[i]);
 vTaskDelay(pdMS_TO_TICKS(1000));
 }
 
 // Test: Draw some shapes
 lcd_fill_screen(&lcd, COLOR_BLACK);
 
 // Red rectangle
 lcd_draw_rect(&lcd, 10, 10, 100, 100, COLOR_RED);
 
 // Green filled rectangle
 lcd_fill_rect(&lcd, 120, 10, 210, 100, COLOR_GREEN);
 
 // Blue horizontal line
 lcd_draw_hline(&lcd, 10, 120, 200, COLOR_BLUE);
 
 // Yellow vertical line
 lcd_draw_vline(&lcd, 220, 10, 200, COLOR_YELLOW);
 
 // Individual pixels
 for (int i = 0; i < 50; i++) {
 lcd_draw_pixel(&lcd, 250 + i, 150 + i, COLOR_ORANGE);
 }
 
 vTaskDelay(pdMS_TO_TICKS(5000));
 }
}
```

---

👏
👍
😊



## PART 8: Key Concepts Recap

### The Big Picture — How Pixels Get on Screen

```
Your Code ESP32-S3 ILI9488 LCD Panel
──────── ──────── ─────── ─────────
lcd_fill_rect()
 │
 ├─ lcd_set_window()
 │ ├─ CASET (0x2A) ────► SPI MOSI ────────► Set X range
 │ ├─ PASET (0x2B) ────► SPI MOSI ────────► Set Y range
 │ └─ RAMWR (0x2C) ────► SPI MOSI ────────► Ready for pixels
 │
 └─ lcd_send_data() ────► SPI MOSI ────────► Write to GRAM ────► Display!
 (line by line) (DMA helps) (auto-increment)
```

### The 3 Most Important Things to Remember

```
1. ALWAYS set window (CASET + PASET + RAMWR) before sending pixel data
2. RGB565 byte order is BIG-ENDIAN on SPI (high byte first)
3. Send in CHUNKS (line by line) — you can't fit a full framebuffer in RAM
```

### Common Mistakes & Fixes

```
┌────────────────────────────────┬──────────────────────────────────────────┐
│ Mistake │ Fix │
├────────────────────────────────┼──────────────────────────────────────────┤
│ Colors look wrong │ Byte-swap RGB565: (color>>8)│(color<<8) │
│ Screen is blank after init │ Check 120ms delay after SLPOUT │
│ Only part of screen fills │ Check CASET/PASET values (4 bytes each) │
│ SPI errors on console │ Reduce clock speed (try 10 MHz) │
│ Garbage on screen │ Check DC pin wiring & pre_cb callback │
│ Colors are inverted │ Try setting RGB bit in MADCTL │
│ Screen is mirrored │ Adjust MY/MX bits in MADCTL │
│ MISO set but LCD has no MISO │ Set miso_io_num = -1 in bus config │
└────────────────────────────────┴──────────────────────────────────────────┘
```

---

## PART 9: Answers to the Datasheet Study Guide Questions

Here are the answers to the 5 questions from `ILI9488_DATASHEET_STUDY_GUIDE.md`:

### Q1: What is the maximum SPI clock frequency for ILI9488?
**Answer:** The ILI9488 supports SPI clock up to approximately **20 MHz** in typical 
operation. In practice, 10-20 MHz is safe. Going above 20 MHz may cause data 
corruption. Start at 1 MHz for debugging, then increase.

### Q2: What value do you write to MADCTL (0x36) to rotate 90° clockwise?
**Answer:** **0x60** — This sets MX (bit 6 = column order flip) + MV (bit 5 = 
row/column exchange). The combination of MX+MV gives 90° clockwise rotation.

### Q3: After sending SLPOUT (0x11), how long must you wait?
**Answer:** **120 ms minimum.** The ILI9488's internal DC-DC converter needs this 
time to stabilize. Sending commands before this delay can result in a blank or 
garbled display.

### Q4: In RGB565 mode, what is the byte order for RED (0xF800)?
**Answer:** **0xF8, 0x00** — High byte first (big-endian). The ILI9488 expects 
MSB first on SPI. On ESP32 (little-endian), a `uint16_t` value `0xF800` is stored 
in memory as `[0x00, 0xF8]` (low byte first). So you can't just cast a `uint16_t*` 
to `uint8_t*` and send — the bytes will be reversed!

**The fix:** Either:
- Send as individual bytes: `{ (color >> 8) & 0xFF, color & 0xFF }`
- Byte-swap the uint16_t: `(color >> 8) | (color << 8)` then send the buffer
- Use `__builtin_bswap16(color)` (GCC built-in)

This is the #1 source of "wrong colors" bugs!

### Q5: How many bytes do you need to send for CASET to set columns 10–300?
**Answer:** **4 bytes.** CASET always takes exactly 4 bytes: 2 bytes for start 
column + 2 bytes for end column. Even though 10 and 300 are small numbers, 
each value is sent as a 16-bit (2-byte) value, big-endian:

```
Start = 10 = 0x000A → bytes: 0x00, 0x0A
End = 300 = 0x012C → bytes: 0x01, 0x2C

Full CASET data: 0x00, 0x0A, 0x01, 0x2C (4 bytes total)
```

This is a common gotcha — people forget that CASET/PASET always use 16-bit 
values and try to send only 1 byte per coordinate. That will silently fail.

---

## 🎯 Your Next Steps

```
✅ You understand SPI communication
✅ You understand the DC pin concept
✅ You know the 10 key ILI9488 commands
✅ You understand the init sequence
✅ You understand RGB565 and byte ordering
✅ You have the complete lcd_driver.c code

⬜ NEXT: Set up ESP-IDF project and test this code on real hardware!
```

**When you're ready to set up the project and flash to your ESP32-S3, let me know. 
That's when the pixels actually light up and the real fun begins!** 🚀

*Class dismissed.* 🏫