

## 
PART 3: Your Practice Exercise

### Exercise 1: SPI Loopback Test (No extra hardware needed!)

Connect **GPIO11 (MOSI) to GPIO12 (MISO)** with a jumper wire.
This makes the ESP32 talk to itself — whatever it sends, it receives back.

```c
hashtag#include <stdio.h>
hashtag#include "freertos/FreeRTOS.h"
hashtag#include "freertos/task.h"
hashtag#include "driver/spi_master.h"

hashtag#define PIN_MOSI 11
hashtag#define PIN_MISO 12
hashtag#define PIN_SCK 10
hashtag#define PIN_CS 9

spi_device_handle_t spi_dev;

void app_main(void) {
 // ===== SETUP =====
 
 // 1. Initialize SPI bus
 spi_bus_config_t buscfg = {
 .mosi_io_num = PIN_MOSI,
 .miso_io_num = PIN_MISO,
 .sclk_io_num = PIN_SCK,
 .quadwp_io_num = -1,
 .quadhd_io_num = -1,
 .max_transfer_sz = 64,
 };
 ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
 printf("SPI bus initialized!\n");
 
 // 2. Add device
 spi_device_interface_config_t devcfg = {
 .clock_speed_hz = 1000000, // 1 MHz
 .mode = 0,
 .spics_io_num = PIN_CS,
 .queue_size = 1,
 };
 ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev));
 printf("SPI device added!\n");
 
 // ===== TEST =====
 
 // 3. Send a byte and receive it back
 uint8_t send_data = 0xA5; // 10100101 in binary
 uint8_t recv_data = 0x00;
 
 spi_transaction_t t = {
 .length = 8,
 .tx_buffer = &send_data,
 .rx_buffer = &recv_data,
 };
 
 ESP_ERROR_CHECK(spi_device_polling_transmit(spi_dev, &t));
 
 printf("Sent: 0x%02X\n", send_data);
 printf("Received: 0x%02X\n", recv_data);
 
 if (send_data == recv_data) {
 printf("✅ SUCCESS! SPI loopback works!\n");
 } else {
 printf("❌ FAIL! Data mismatch. Check MOSI-MISO jumper wire.\n");
 }
 
 // 4. Send multiple bytes
 uint8_t send_buf[] = {0x01, 0x02, 0x03, 0x04, 0x05};
 uint8_t recv_buf[5] = {0};
 
 spi_transaction_t t2 = {
 .length = sizeof(send_buf) * 8,
 .tx_buffer = send_buf,
 .rx_buffer = recv_buf,
 };
 
 ESP_ERROR_CHECK(spi_device_polling_transmit(spi_dev, &t2));
 
 printf("\nMulti-byte test:\n");
 for (int i = 0; i < 5; i++) {
 printf(" [%d] Sent: 0x%02X, Recv: 0x%02X %s\n", 
 i, send_buf[i], recv_buf[i],
 send_buf[i] == recv_buf[i] ? "✅" : "❌");
 }
}
```


**What this teaches you:**
- How to initialize the SPI bus
- How to add a device
- How to send and receive data
- That SPI is just: configure → create transaction → transmit

**Expected output (with MOSI-MISO jumper):**
```
SPI bus initialized!
SPI device added!
Sent: 0xA5
Received: 0xA5
✅ SUCCESS! SPI loopback works!

Multi-byte test:
 [0] Sent: 0x01, Recv: 0x01 ✅
 [1] Sent: 0x02, Recv: 0x02 ✅
 [2] Sent: 0x03, Recv: 0x03 ✅
 [3] Sent: 0x04, Recv: 0x04 ✅
 [4] Sent: 0x05, Recv: 0x05 ✅
```

👏
👍
😊



## PART 4: The DC Pin — The Missing Piece for LCD

Once you understand basic SPI, there's ONE more concept for LCDs:

### The DC (Data/Command) Pin

The ILI9488 has an extra pin called **DC** (Data/Command), also called **RS** (Register Select) on some displays.

**Why?** Because SPI only sends bytes — the LCD needs to know: "is this byte a COMMAND or DATA?"

```
DC = LOW (0) → "The next byte is a COMMAND" (e.g., 0x29 = Display On)
DC = HIGH (1) → "The next byte is DATA" (e.g., pixel color values)
```

**How to implement DC with ESP-IDF SPI:**

```c
hashtag#include "driver/gpio.h"

hashtag#define PIN_DC 8

// This callback runs BEFORE each SPI transaction
void spi_pre_transfer_callback(spi_transaction_t *t) {
 int dc = (int)t->user; // Get DC value from user field
 gpio_set_level(PIN_DC, dc);
}

// Setup DC pin as output
void dc_pin_setup(void) {
 gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
}

// Send a COMMAND (DC = LOW)
void lcd_send_cmd(uint8_t cmd) {
 spi_transaction_t t = {
 .length = 8,
 .tx_buffer = &cmd,
 .user = (void*)0, // DC = 0 = COMMAND
 };
 spi_device_polling_transmit(spi_dev, &t);
}

// Send DATA (DC = HIGH)
void lcd_send_data(uint8_t *data, int len) {
 spi_transaction_t t = {
 .length = len * 8,
 .tx_buffer = data,
 .user = (void*)1, // DC = 1 = DATA
 };
 spi_device_polling_transmit(spi_dev, &t);
}
```

**The `user` field trick:** ESP-IDF's `spi_transaction_t` has a `user` field that's a `void*`. You can store anything there. We store 0 or 1 to indicate DC level. The `pre_cb` callback reads this and sets the GPIO before the transaction starts.

**This is the KEY pattern for ILI9488 SPI communication.**

---

## PART 5: Your Learning Path — Updated

```
✅ Step 1: Read this file (SPI_PRACTICE.md) ← YOU ARE HERE
⬜ Step 2: Do Exercise 1 (SPI loopback test)
⬜ Step 3: Understand the DC pin concept
⬜ Step 4: NOW read the ILI9488 datasheet
⬜ Step 5: Answer the 5 questions in ILI9488_DATASHEET_STUDY_GUIDE.md
⬜ Step 6: Write lcd_driver.c
⬜ Step 7: Test LCD with color fill
⬜ Step 8: Celebrate your first milestone! 🎉
```

---

## 🧠 Quick Recap — What You Now Know

| Concept | One-Line Summary |
|---------|-----------------|
| SPI | Master sends clock + data, slave follows along |
| MOSI | Data from master to slave |
| MISO | Data from slave to master |
| SCK | Clock signal (master generates it) |
| CS | Chip select (active LOW, picks which device) |
| SPI Mode 0 | Clock idles LOW, data sampled on rising edge |
| DC pin | Tells LCD: "command" (LOW) or "data" (HIGH) |
| `spi_bus_initialize()` | Set up the shared SPI bus |
| `spi_bus_add_device()` | Add a device (with its CS pin and speed) |
| `spi_device_polling_transmit()` | Send/receive data (blocks until done) |
| `pre_cb` | Callback before transaction — used to set DC pin |

**Now go do Exercise 1. I'll wait.** 🏫


