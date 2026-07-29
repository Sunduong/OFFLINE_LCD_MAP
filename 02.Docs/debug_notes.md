# 2026-06-11
## ILI9488 LCD Bug
I follow AI chatbot to dev this LCD driver. I choose Color Mode like RGB565 - 16 bit

```
lcd_send_cmd(lcd, 0x3A); // COLMOD
uint8_t pixel_format = 0x55; // 16-bit RGB565
lcd_send_data(lcd, &pixel_format, 1);
ESP_LOGI(TAG, "LCD color mode set to RGB565...");
```
Then my module always show white screen. It's an issue.
I doubt and debug many things. I check my SPI protocol can initialize successfully.  So what is the reason why?
I changed the color mode, I see it can work well.

# 2026-06-29
## SD CARD Bug
I wrote the SD card SPI driver. I control LCD CS and SD CS pin manually.
When I wrote done. I check, the LOG always come out: 

```
I (354) SD_CARD_DRIVER: SD card device added to SPI bus... 
I (354) SD_CARD_DRIVER: Response[0] = 0x00 
I (364) SD_CARD_DRIVER: CMD0 response: 0x00 
I (364) LCD_DRIVER: Initializing LCD... 
I (374) LCD_DRIVER: LCD device added to SPI bus
```
I don't know the reason why. I need CMD0 response 0x01. 
I debug on Logic Analyzer. I see that whenever SPI SCK is at LOW level, LCD_CS and SD_CS pins are all pulled down.  
--> Actually, this is normal behaviour. Might be some hardware make any effects on these pins when Reset ESP32.

I doubt that SD_CS pin might has any problems because it is pulled down at the beginer of my program.
So, I tried to disable all LCD execution, but SD Card execution. Issue still appears, I see that the pull down of SD_CS pin is the normal behaviour.

I tried to check the SD_MOSI and the MOSI at LCD side on the "LCD 3.5" SPI Ili9488 TFT" module by VOM. I see it's not connected as I know before. And sometimes I thought I connect the pin row on the module wrongly, but it's not kkkkk

I see that SD_CS and MOSI can show normally on Logic Analyzer regarding SCK pulse (LCD side). So I think the bug comes from HARDWARE.
I disconnect MOSI (LCD side) and connect to SD_MOSI (SD side). I got the log below:

```
I (354) SD_CARD_DRIVER: Initializing SD card on SPI bus... 
I (364) SD_CARD_DRIVER: Idle[0] = 0xFF 
I (364) SD_CARD_DRIVER: Idle[1] = 0xFF 
I (364) SD_CARD_DRIVER: Idle[2] = 0xFF 
I (374) SD_CARD_DRIVER: Idle[3] = 0xFF 
I (374) SD_CARD_DRIVER: Idle[4] = 0xFF 
I (384) SD_CARD_DRIVER: Idle[5] = 0xFF 
I (384) SD_CARD_DRIVER: Idle[6] = 0xFF 
I (394) SD_CARD_DRIVER: Idle[7] = 0xFF 
I (394) SD_CARD_DRIVER: Idle[8] = 0xFF 
I (404) SD_CARD_DRIVER: Idle[9] = 0xFF 
I (404) SD_CARD_DRIVER: SD card device added to SPI bus... 
I (414) SD_CARD_DRIVER: Response[0] = 0xFF 
I (414) SD_CARD_DRIVER: Response[1] = 0xFF 
I (424) SD_CARD_DRIVER: Response[2] = 0xFF 
I (424) SD_CARD_DRIVER: Response[3] = 0xFF 
I (424) SD_CARD_DRIVER: Response[4] = 0xFF 
I (434) SD_CARD_DRIVER: Response[5] = 0xFF 
I (434) SD_CARD_DRIVER: Response[6] = 0xFF 
I (444) SD_CARD_DRIVER: Response[7] = 0xFF 
I (444) SD_CARD_DRIVER: CMD0 response: 0xFF 
I (454) main_task: Returned from app_main()
```

I thought something wrong in the Hardware. So I found a tutourial using Arduino IDE for developing SD card on this module. I see I have to connect SD side to LCD side. It works.

Nguyên nhân:
SPI bus của LCD và SD trên module không được nối như mình tưởng.
Phải nối chung các đường SCK/MOSI/MISO giữa header LCD và header SD.

Triệu chứng:
- Logic Analyzer thấy CMD0 gửi đúng.
- SD không phản hồi.
- Sau khi nối chung bus thì CMD0 trả 0x01.



# 2026-07-23
## Bottle neck when map is loaded on LCD


10 giây cho `fread(192KB)` — tôi cần đo chi tiết để tìm bottleneck chính xác. Thay vì đoán, hãy thêm code đo thời gian vào từng bước.

### 🔬 Đề xuất: Thêm timing vào `sd_card_read_file`

```c
int sd_card_read_file(sd_card_t *sd, const char *path, uint8_t *buffer, size_t max_len)
{
 if (!sd->mounted) {
 ESP_LOGE(TAG, "SD card not mounted!");
 return -1;
 }

 int64_t t0 = esp_timer_get_time();

 FILE *f = fopen(path, "rb");
 int64_t t1 = esp_timer_get_time();
 if (f == NULL) {
 ESP_LOGE(TAG, "Failed to open file: %s", path);
 return -1;
 }

 size_t bytes_read = fread(buffer, 1, max_len, f);
 int64_t t2 = esp_timer_get_time();

 fclose(f);
 int64_t t3 = esp_timer_get_time();

 ESP_LOGI(TAG, "fopen: %lld ms | fread: %lld ms | fclose: %lld ms | total: %lld ms | bytes: %u",
 (t1 - t0) / 1000,
 (t2 - t1) / 1000,
 (t3 - t2) / 1000,
 (t3 - t0) / 1000,
 bytes_read);

 return (int)bytes_read;
}
```

Cần thêm `hashtag#include "esp_timer.h"` ở đầu file.

### Kết quả sẽ cho biết:

| Kịch bản | fopen | fread | fclose | Nguyên nhân |
|----------|-------|-------|--------|-------------|
| A | ~10s | ~0ms | ~0ms | fopen bottleneck (directory parse) |
| B | ~0ms | ~10s | ~0ms | fread bottleneck (sector-by-sector read) |
| C | ~0ms | ~0ms | ~10s | fclose bottleneck (unlikely) |
| D | ~3s | ~4s | ~3s | Tất cả đều chậm |

### Nếu fread là bottleneck (Kịch bản B — khả năng cao nhất):

Thêm đo chi tiết hơn trong `map_load_tile`:

```c
esp_err_t map_load_tile(sd_card_t *sd, int zoom, int tile_x, int tile_y, uint8_t *buffer)
{
 // ... existing code ...
 
 int64_t t0 = esp_timer_get_time();
 int bytes_read = sd_card_read_file(sd, path, buffer, TILE_BYTES_RGB666);
 int64_t t1 = esp_timer_get_time();
 
 ESP_LOGI(TAG, "map_load_tile total: %lld ms", (t1 - t0) / 1000);
 // ...
}
```

### Nếu muốn đo chi tiết hơn trong fread:

Chia fread thành nhiều lần đọc nhỏ để xem tốc độ đọc có đều không:

```c
// Thay vì fread 192KB 1 lần:
size_t chunk_size = 4096; // 4KB
size_t total_read = 0;
for (int i = 0; i < 48; i++) // 48 chunks × 4KB = 192KB
{
 int64_t tc0 = esp_timer_get_time();
 size_t r = fread(buffer + total_read, 1, chunk_size, f);
 int64_t tc1 = esp_timer_get_time();
 
 if (i < 3 || i >= 45) // Log first 3 + last 3 chunks
 ESP_LOGI(TAG, "chunk %d: %lld ms (%u bytes)", i, (tc1-tc0)/1000, r);
 
 total_read += r;
}
```

→ Biết đọc chunk đầu có chậm hơn chunk cuối không (FAT chain lookup vs sequential read).

Bạn thêm code đo thời gian vào `sd_card_read_file` rồi chạy thử được không? Kết quả sẽ cho biết chính xác bottleneck ở đâu.





I (136231) SD_CARD_DRIVER: 123456789
I (136231) SD_CARD_DRIVER: fopen: 501 ms | fread: 16058 ms | fclose: 0 ms | total: 16560 ms | bytes: 196608
I (136231) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30778.bin


## Solution
1. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/sdspi_host.html
2. Bật Log Verbose để check các log ở tầng dưới của esp idf library
Cách bật
```
esp_log_level_set("sdspi_host", ESP_LOG_VERBOSE);
esp_log_level_set("sdmmc_cmd", ESP_LOG_VERBOSE);
esp_log_level_set("diskio_sdmmc", ESP_LOG_VERBOSE);
```
### When the issue appear, this is the LOG:
```
D (127683) sdspi_host: poll_busy: timeout
V (127693) sdmmc_cmd: cmd response 00000000 00000000 00000000 00000000 err=0x0 state=0
V (127693) sdmmc_cmd: sending cmd slot=1 op=17 arg=15ede flags=1c50 data=0x3fc9e4b0 blklen=512 datalen=512 timeout=1000
V (127703) sdspi_host: sdspi_host_start_command: slot=1, CMD17, arg=0x00015ede flags=0x5, data=0x3fc9e4b0, data_size=512 crc=0x71
D (127753) sdspi_host: poll_busy: timeout
V (127753) sdmmc_cmd: cmd response 00000000 00000000 00000000 00000000 err=0x0 state=0
V (127753) sdmmc_cmd: sending cmd slot=1 op=17 arg=15edf flags=1c50 data=0x3fc9e4b0 blklen=512 datalen=512 timeout=1000
V (127763) sdspi_host: sdspi_host_start_command: slot=1, CMD17, arg=0x00015edf flags=0x5, data=0x3fc9e4b0, data_size=512 crc=0x78
D (127813) sdspi_host: poll_busy: timeout
V (127813) sdmmc_cmd: cmd response 00000000 00000000 00000000 00000000 err=0x0 state=0
I (127813) SD_CARD_DRIVER: fopen: 827 ms | fread: 24493 ms | fclose: 0 ms | total: 25321 ms | bytes: 196608
I (127823) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30780.bin
```

### When issue is fixed, this is the LOG:
1. Fix by pull up 10k resitor on MISO pin:
```
I (30819) SD_CARD_DRIVER: fopen: 28 ms | fread: 773 ms | fclose: 0 ms | total: 801 ms | bytes: 196608 | speed: 248 KB/s | err: 0
I (30819) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30779.bin
I (31679) SD_CARD_DRIVER: fopen: 28 ms | fread: 772 ms | fclose: 0 ms | total: 800 ms | bytes: 196608 | speed: 248 KB/s | err: 0
I (31679) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30780.bin
I (32549) SD_CARD_DRIVER: fopen: 32 ms | fread: 776 ms | fclose: 0 ms | total: 809 ms | bytes: 196608 | speed: 247 KB/s | err: 0
I (32549) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30780.bin
I (33609) MAP_RENDERER: Render: lat=10.856290 lon=106.720550 tile=(52195,30779) offset=(226,189) screen_base=(-66,51)
I (34409) SD_CARD_DRIVER: fopen: 26 ms | fread: 775 ms | fclose: 0 ms | total: 802 ms | bytes: 196608 | speed: 247 KB/s | err: 0
I (34409) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30778.bin
I (35229) SD_CARD_DRIVER: fopen: 26 ms | fread: 773 ms | fclose: 0 ms | total: 799 ms | bytes: 196608 | speed: 248 KB/s | err: 0
I (35229) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30778.bin
I (36059) SD_CARD_DRIVER: fopen: 28 ms | fread: 774 ms | fclose: 0 ms | total: 802 ms | bytes: 196608 | speed: 248 KB/s | err: 0
I (36059) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30779.bin
```

2. Fix by setvbuf:
```
                                            Số lệnh	        Chi phí/lệnh	Tổng (ước tính)	        Số đo thực tế
Ban đầu (không setvbuf, không pull-up)      ~384	        ~40ms	        ~15.4s	                ~16s ✅
Chỉ thêm pull-up	                        ~384	        ~2ms	        ~0.8s	                ~0.8s ✅
Chỉ thêm setvbuf (trường hợp bạn vừa test)	~24	            ~40ms	        ~1s	                    ~1.5s ✅
Cả 2 cùng lúc	                            ~24	            ~2ms	        ~50-100ms	            (chưa test)
```

3. Fix by both 1 and 2
```
Giai đoạn	                        fopen	        fread	        Tổng
Ban đầu (chưa fix gì)	            501ms	        16058ms	        16560ms
Chỉ pull-up MISO	                26ms	        772ms	        800ms
Chỉ setvbuf	                        28ms	        ~772ms	        ~800ms
Cả 2 kết hợp	                    26-28ms	        262-265ms	    ~290ms
```
```
I (924) MAP_RENDERER: Render: lat=10.856290 lon=106.720550 tile=(52195,30779) offset=(226,189) screen_base=(-66,51)
I (934) main_task: Returned from app_main()
I (1424) SD_CARD_DRIVER: fopen: 26 ms | fread: 263 ms | fclose: 0 ms | total: 289 ms | bytes: 196608 | speed: 729 KB/s | err: 0
I (1434) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30778.bin
I (1744) SD_CARD_DRIVER: fopen: 26 ms | fread: 263 ms | fclose: 0 ms | total: 290 ms | bytes: 196608 | speed: 727 KB/s | err: 0
I (1744) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30778.bin
I (2054) SD_CARD_DRIVER: fopen: 28 ms | fread: 262 ms | fclose: 0 ms | total: 291 ms | bytes: 196608 | speed: 730 KB/s | err: 0
I (2054) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30779.bin
I (2434) SD_CARD_DRIVER: fopen: 28 ms | fread: 263 ms | fclose: 0 ms | total: 292 ms | bytes: 196608 | speed: 727 KB/s | err: 0
I (2434) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52196/30779.bin
I (2784) SD_CARD_DRIVER: fopen: 28 ms | fread: 262 ms | fclose: 0 ms | total: 291 ms | bytes: 196608 | speed: 730 KB/s | err: 0
I (2784) SD_CARD_DRIVER: Read 196608 bytes from /sdcard/maps/16/52195/30780.bin
I (3134) SD_CARD_DRIVER: fopen: 28 ms | fread: 265 ms | fclose: 0 ms | total: 294 ms
```

# 2026-07-29

## Title: Enable PSRAM for map cache using, my program gets crash

1. Cause
- I enable Heap memory debugging → Heap corruption detection → chọn "Comprehensive". I want to debug on Heap Memory
- This is the LOG
```
 (1579) main_task: Calling app_main()
I (1589) APP_MAIN: PSRAM size: 8388608 bytes
I (1589) SPI_BUS_MANAGER: Initializing shared SPI bus...
I (1599) SPI_BUS_MANAGER: SPI bus initialized (MOSI=11, MISO=12, SCK=10)
I (1609) APP_MAIN: SPI Shared Bus initialized!
I (1609) LCD_DRIVER: Initializing LCD...
I (1619) LCD_DRIVER: LCD device added to SPI bus...
I (1729) LCD_DRIVER: LCD hardware reset complete...
I (1849) LCD_DRIVER: LCD software reset complete...
I (1969) LCD_DRIVER: LCD sleep out complete...
I (1969) LCD_DRIVER: LCD color mode set to RGB666...
I (1969) LCD_DRIVER: LCD rotation set to portrait...
I (1969) LCD_DRIVER: LCD normal display mode set...
I (2079) LCD_DRIVER: LCD display turned on...
I (2079) LCD_DRIVER: LCD initialization complete!
I (2079) APP_MAIN: LCD initialized!
I (2079) SD_CARD_DRIVER: Initializing SD card on shared SPI bus...
I (2089) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (2099) sdspi_transaction: cmd=52, R1 response: command not supported
I (2139) sdspi_transaction: cmd=5, R1 response: command not supported
I (2169) SD_CARD_DRIVER: SD card mounted at /sdcard
Name: SD16G
Type: SDHC/SDXC
Speed: 10.00 MHz (limit: 10.00 MHz)
Size: 15360MB
CSD: ver=2, sector_size=512, capacity=31457280 read_bl_len=9
SSR: bus_width=1
I (2169) APP_MAIN: SD Card initialized!
I (2179) gpio: GPIO[35]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (2189) gpio: GPIO[35]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3
CORRUPT HEAP: Invalid data at 0x3c0675a0. Expected 0xfefefefe got 0x4d684d68
CORRUPT HEAP: Invalid data at 0x3c0675a4. Expected 0xfefefefe got 0x4d684d68
CORRUPT HEAP: Invalid data at 0x3c0675a8. Expected 0xfefefefe got 0x4d684d68
CORRUPT HEAP: Invalid data at 0x3c0675ac. Expected 0xfefefefe got 0x4d684d68
```

- Heap memory error appear while Button pin is initialized. I checked the esp hardware design and see that these button pins are also used for PSRAM connections.

```
ESP-IDF GPIO docs (ESP32-S3): "When using Octal flash or Octal PSRAM or both, GPIO33 ~ GPIO37 are connected to SPIIO4 ~ SPIIO7 and SPIDQS. Therefore, on boards embedded with ESP32-S3R8/R8V chip, GPIO33~37 are also not recommended for other uses."
```
2. Solution

Cách fix — đổi 3 pin nút bấm sang GPIO an toàn

Không dùng các dải GPIO sau trên ESP32-S3 (đặc biệt board có Octal PSRAM như của bạn):

GPIO26-32 — luôn luôn cấm (dùng cho SPI flash/PSRAM chuẩn, mọi board S3)
GPIO33-37 — cấm thêm khi có Octal PSRAM (board bạn đang dùng)
GPIO0, 3, 45, 46 — chân bootstrap, tránh dùng làm input thường trừ khi hiểu rõ ảnh hưởng
GPIO19, 20 — mặc định dùng cho USB-JTAG (vẫn dùng được nhưng sẽ tắt tính năng debug qua USB)