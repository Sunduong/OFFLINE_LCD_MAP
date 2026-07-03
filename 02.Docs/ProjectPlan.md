# ESP32-S3 Local Map Device - Development Roadmap

## Project Overview
A portable offline map device for motorbike travel with ESP32-S3, featuring local map storage, navigation support, and phone connectivity.

---

## Phase 1: Foundation & Hardware Setup (Week 1-2)

### 1.1 Project Structure Setup
```
esp32-map-device/
├── main/
│ ├── CMakeLists.txt
│ ├── main.c
│ ├── app/
│ │ ├── app_main.c
│ │ ├── app_config.h
│ │ └── app_events.c
│ ├── drivers/
│ │ ├── lcd/
│ │ │ ├── lcd_driver.c
│ │ │ ├── lcd_driver.h
│ │ │ └── lcd_config.h
│ │ ├── gps/
│ │ │ ├── gps_driver.c
│ │ │ └── gps_driver.h
│ │ ├── storage/
│ │ │ ├── sd_card.c
│ │ │ └── sd_card.h
│ │ └── input/
│ │ ├── button.c
│ │ └── button.h
│ ├── core/
│ │ ├── map_renderer/
│ │ │ ├── map_renderer.c
│ │ │ └── map_renderer.h
│ │ ├── map_data/
│ │ │ ├── map_parser.c
│ │ │ ├── map_parser.h
│ │ │ └── map_types.h
│ │ ├── navigation/
│ │ │ ├── nav_engine.c
│ │ │ └── nav_engine.h
│ │ └── location/
│ │ ├── location_manager.c
│ │ └── location_manager.h
│ ├── comm/
│ │ ├── ble/
│ │ │ ├── ble_server.c
│ │ │ └── ble_server.h
│ │ └── wifi/
│ │ ├── wifi_manager.c
│ │ └── wifi_manager.h
│ └── utils/
│ ├── logger.c
│ ├── logger.h
│ ├── file_utils.c
│ └── file_utils.h
├── components/
│ └── [3rd party components]
├── spi_flash/
├── sd_card/
│ └── maps/
│ └── [offline map tiles]
├── CMakeLists.txt
├── sdkconfig
└── README.md
```

### 1.2 Hardware Components Checklist
- [x] ESP32-S3 DevKit
- [x] LCD Display (specify: SPI/Parallel? Resolution?)
- [x] Buttons
- [ ] GPS Module (recommend: NEO-6M or NEO-M8N)
- [ ] SD Card Module (for map storage)
- [ ] Power Supply (battery + BMS for motorbike)
- [ ] Enclosure (weatherproof for motorbike)

### 1.3 Hardware Interface Design
```
ESP32-S3 Pin Assignment (Draft):
┌─────────────────────────────────────┐
│ LCD (SPI): │
│ - MOSI: GPIO11 │
│ - MISO: GPIO12 │
│ - SCK: GPIO10 │
│ - CS: GPIO9 │
│ - DC: GPIO8 │
│ - RST: GPIO7 │
├─────────────────────────────────────┤
│ SD Card (SPI): │
│ - MOSI: GPIO11 (shared with LCD) │
│ - MISO: GPIO12 (shared with LCD) │
│ - SCK: GPIO10 (shared with LCD) │
│ - CS: GPIO13 │
├─────────────────────────────────────┤
│ GPS (UART): │
│ - TX: GPIO4 │
│ - RX: GPIO5 │
├─────────────────────────────────────┤
│ Buttons: │
│ - BTN_UP: GPIO1 │
│ - BTN_DOWN: GPIO2 │
│ - BTN_SELECT: GPIO3 │
│ - BTN_BACK: GPIO6 │
└─────────────────────────────────────┘
```

---

## Phase 2: Core Drivers Development (Week 3-4)

### 2.1 LCD Driver
- [ ] Initialize SPI communication
- [ ] Implement basic LCD commands (init, clear, set pixel)
- [ ] Implement drawing primitives (line, rectangle, circle, text)
- [ ] Implement image/tile rendering
- [ ] Optimize for performance (DMA if available)

### 2.2 SD Card & Filesystem
- [ ] Initialize SD card in SPI mode
- [ ] Mount FAT filesystem
- [ ] Implement file read/write operations
- [ ] Test with sample map files

### 2.3 GPS Driver
- [ ] Initialize UART for GPS module
- [ ] Parse NMEA sentences (GPRMC, GPGGA)
- [ ] Extract latitude, longitude, speed, heading
- [ ] Implement position update callback

### 2.4 Button Handler
- [ ] GPIO initialization with interrupts
- [ ] Debounce implementation
- [ ] Long press / short press detection
- [ ] Button event queue

---

## Phase 3: Map System Development (Week 5-7)

### 3.1 Map Data Format
**Option A: Custom Binary Format (Recommended for simplicity)**
```
Map File Structure:
├── Header (16 bytes)
│ - Magic number: 0x4D415053 ("MAPS")
│ - Version: 1 byte
│ - Zoom levels: 1 byte
│ - Tile size: 2 bytes (e.g., 256x256)
│ - Total tiles: 4 bytes
│ - Reserved: 6 bytes
├── Index Table
│ - Tile coordinates (x, y, zoom)
│ - Offset to tile data
│ - Tile size
├── Tile Data
│ - Compressed tile images (RLE or LZ4)
```

**Option B: Use existing format (MBTiles/Mapbox)**
- SQLite-based format
- Requires SQLite library integration

### 3.2 Map Tile System
```
Tile Coordinate System:
┌────────────────────────────────────┐
│ Zoom Level 0: 1 tile (world) │
│ Zoom Level 1: 4 tiles │
│ Zoom Level 2: 16 tiles │
│ ... │
│ Zoom Level n: 4^n tiles │
└────────────────────────────────────┘

Tile naming: z/x/y.tile
Example: 15/12345/6789.tile
```

### 3.3 Map Renderer
- [ ] Implement tile loading from SD card
- [ ] Implement tile caching (RAM buffer for visible tiles)
- [ ] Implement map rendering with current position
- [ ] Implement zoom in/out functionality
- [ ] Implement pan functionality

### 3.4 Map Data Preparation Tools
Create PC-side tool for map download:
```python
# map_downloader.py
- Download tiles from OpenStreetMap
- Convert to custom binary format
- Store on SD card
```

---

## Phase 4: Navigation System (Week 8-9)

### 4.1 Bluetooth Communication
- [ ] Set up ESP32-S3 BLE server
- [ ] Define GATT service for navigation
- [ ] Receive navigation data from phone app

**BLE Service Structure:**
```
Service UUID: 0x1800 (custom for MapNav)

Characteristics:
├── Nav Data (UUID: 0x2A01)
│ - Write: Receive route from phone
│ - Format: JSON/Protocol Buffer
├── Position (UUID: 0x2A02)
│ - Notify: Send current GPS position
├── Status (UUID: 0x2A03)
│ - Read/Notify: Device status
└── Command (UUID: 0x2A04)
 - Write: Remote commands
```

### 4.2 Navigation Data Protocol
```c
// Navigation route data structure
typedef struct {
 float start_lat;
 float start_lon;
 float end_lat;
 float end_lon;
 uint16_t waypoint_count;
 waypoint_t waypoints[MAX_WAYPOINTS];
} route_t;

typedef struct {
 float lat;
 float lon;
 char instruction[64]; // "Turn left in 100m"
 uint16_t distance; // Distance to next waypoint
} waypoint_t;
```

### 4.3 Navigation Engine
- [ ] Parse route data from BLE
- [ ] Calculate distance to waypoints
- [ ] Trigger navigation instructions
- [ ] Visual feedback on LCD (direction arrow, distance)

---

## Phase 5: User Interface (Week 10-11)

### 5.1 UI States
```
┌─────────────┐
│ BOOT │ ──> Show splash screen
└──────┬──────┘
 │
 v
┌─────────────┐
│ MAP VIEW │ ──> Main map display with GPS position
└──────┬──────┘
 │
 ├──── [SELECT] ───> MENU
 │ │
 │ ├── Zoom In/Out
 │ ├── Navigate to...
 │ ├── Settings
 │ └── About
 │
 └──── [Long Press] ──> GPS Status
```

### 5.2 Main Screen Layout
```
┌────────────────────────────┐
│ ┌────────────────────────┐ │
│ │ │ │
│ │ MAP TILES │ │
│ │ │ │
│ │ 📍 │ │ <- Current position
│ │ │ │
│ │ ────── │ │ <- Route line
│ │ │ │
│ └────────────────────────┘ │
│ │
│ ┌──────────┐ ┌──────────┐│
│ │ Zoom: 15 │ │ ↑ 200m │ │ <- Navigation info
│ └──────────┘ └──────────┘│
└────────────────────────────┘
```

---

## Phase 6: Power Management & Optimization (Week 12)

### 6.1 Power Optimization
- [ ] Implement sleep modes when idle
- [ ] Reduce GPS update rate when stationary
- [ ] LCD brightness control
- [ ] Power consumption profiling

### 6.2 Performance Optimization
- [ ] Profile rendering performance
- [ ] Optimize tile loading (prefetch)
- [ ] Memory usage optimization
- [ ] Cache frequently used tiles

---

## Phase 7: Integration & Testing (Week 13-14)

### 7.1 Integration Testing
- [ ] End-to-end navigation test
- [ ] Offline functionality test
- [ ] GPS accuracy test
- [ ] BLE connectivity test

### 7.2 Field Testing
- [ ] Motorbike mounting test
- [ ] Vibration resistance
- [ ] Sunlight readability
- [ ] Battery life test

---

## Phase 8: Documentation & Showcase (Week 15)

### 8.1 Documentation
- [ ] README with setup instructions
- [ ] Hardware schematic diagram
- [ ] API documentation
- [ ] User manual

### 8.2 Portfolio Materials
- [ ] Demo video
- [ ] Project blog post
- [ ] GitHub repository cleanup
- [ ] Presentation slides

---

## Technology Stack Summary

| Component | Technology/Library |
|-----------|-------------------|
| Framework | ESP-IDF v5.x |
| Language | C (C11) |
| Build System | CMake + Ninja |
| LCD Driver | Custom SPI driver |
| Filesystem | FAT + Wear Leveling |
| GPS | Custom NMEA parser |
| BLE | ESP-IDF Bluedroid |
| Map Format | Custom binary (or MBTiles) |
| Compression | miniz / LZ4 |

---

## Hardware BOM (Bill of Materials)

| Component | Quantity | Est. Cost | Notes |
|-----------|----------|-----------|-------|
| ESP32-S3 DevKit | 1 | ~$8 | Already have |
| LCD Display (3.5" ILI9488) | 1 | ~$12 | SPI interface |
| GPS Module (NEO-M8N) | 1 | ~$10 | High accuracy |
| MicroSD Card (16GB) | 1 | ~$5 | Class 10 |
| SD Card Module | 1 | ~$2 | SPI interface |
| Buttons (tactile) | 4 | ~$1 | |
| Battery (18650 x2) | 2 | ~$8 | 3.7V Li-Ion |
| BMS Module | 1 | ~$3 | Battery management |
| USB-C Charging Module | 1 | ~$2 | TP4056 |
| Enclosure | 1 | ~$5 | 3D printed |
| **Total** | | **~$56** | |

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Limited RAM for map tiles | High | Use tile streaming, limit cache size |
| Slow SD card read | Medium | Use Class 10 card, optimize SPI speed |
| GPS signal loss in tunnels | Low | Dead reckoning with last known position |
| LCD sunlight readability | Medium | Use high brightness display, shade |
| Battery drain | Medium | Power management, efficient rendering |

---

## Success Criteria

- [ ] Device boots and displays map within 5 seconds
- [ ] GPS position updates every 1 second
- [ ] Smooth map panning (30 FPS minimum)
- [ ] Successfully receives navigation from phone via BLE
- [ ] Minimum 4 hours battery life
- [ ] Weatherproof for motorbike use

---

## Getting Started - First Steps

1. **Set up ESP-IDF environment** (if not already)
2. **Create project structure** using template above
3. **Test LCD display** with simple graphics
4. **Test SD card** read/write
5. **Test GPS** module with serial output

Let me know when you're ready to start, and I'll help you set up the initial project structure!



Status Update

## 🖼️ Hiển thị ảnh .jpg từ SD card lên LCD — Cách làm

Đây là bước chuyển từ "vẽ màu" sang "hiển thị ảnh thật". Có 2 phần:

---

### 📐 Kiến trúc tổng quan

```
SD Card          RAM (PSRAM)           LCD
┌──────────┐    ┌──────────────┐    ┌──────────┐
│ image.jpg│──→│ JPEG Decoder │──→│ LCD SPI  │
│ (compressed)│ │ (tjpgd)     │   │ (RGB666) │
└──────────┘    │ → RGB888    │   └──────────┘
                │ → RGB666    │
                └──────────────┘
```

**3 bước:**
1. Đọc file `.jpg` từ SD card vào RAM
2. **Decode JPEG** → pixel data (RGB888 → RGB666)
3. Gửi pixel data lên LCD qua SPI

---

### 🔑 Thách thức lớn nhất: RAM

```
Ảnh 320×480 pixel:
  - RGB666: 320 × 480 × 3 bytes = 460,800 bytes (~450KB)
  - ESP32-S3 SRAM: ~512KB → KHÔNG ĐỦ!
  - ESP32-S3 PSRAM: 2-8MB → ĐỦ!

→ Phải dùng PSRAM (heap_caps_malloc với MALLOC_CAP_SPIRAM)
→ Hoặc decode từng dòng (line-by-line) để tiết kiệm RAM
```

---

### 📚 Thư viện JPEG decode cho ESP-IDF

Có 2 lựa chọn:

| Thư viện | Ưu điểm | Nhược điểm |
|----------|---------|------------|
| **`tjpgd`** (Tiny JPEG Decoder) | Nhẹ (~6KB), decode line-by-line, có sẵn trong ESP-IDF | Chỉ hỗ trợ JPEG baseline |
| **`esp_jpeg`** (ESP-IDF component) | Hỗ trợ nhiều format, tối ưu cho ESP32 | Lớn hơn, cần thêm component |

**Thầy khuyên `tjpgd`** vì:
- Có sẵn trong ESP-IDF (`components/` hoặc IDF component registry)
- Decode line-by-line → tiết kiệm RAM (chỉ cần 1 dòng buffer)
- Đã được dùng trong ESP-IDF `lcd_jpeg` example

---

### 📋 Các bước cần làm

#### Bước 1: Thêm `tjpgd` component
```
# Trong idf_component.yml hoặc CMakeLists.txt
idf.py add-dependency "espressif/tjpgd"
```

Hoặc copy `tjpgd` source vào project (2 file: `tjpgd.c` + `tjpgd.h`).

#### Bước 2: Cấu hình PSRAM
```
# sdkconfig.defaults
CONFIG_SPIRAM=y
CONFIG_SPIRAM_USE_MALLOC=y
```

#### Bước 3: Flow decode JPEG → LCD

```c
#include "tjpgd.h"

// tjpgd cần input function (đọc data từ buffer)
static uint16_t tjpgd_data_in(JDEC *jd, uint8_t *buf, uint16_t nbytes) {
    // Copy nbytes từ JPEG buffer vào buf
    memcpy(buf, jpeg_data + jd->device_ptr, nbytes);
    jd->device_ptr += nbytes;
    return nbytes;
}

// tjpgd cần output function (nhận pixel data đã decode)
static uint16_t tjpgd_data_out(JDEC *jd, void *bitmap, JRECT *rect) {
    // bitmap = 1 dòng pixel RGB888
    // rect = tọa độ vùng pixel
    
    // Convert RGB888 → RGB666
    uint8_t *src = (uint8_t *)bitmap;
    uint8_t *line_buf = heap_caps_malloc(width * 3, MALLOC_CAP_DMA);
    
    for (int i = 0; i < width; i++) {
        line_buf[i * 3 + 0] = src[i * 3 + 0] & 0xFC;  // R
        line_buf[i * 3 + 1] = src[i * 3 + 1] & 0xFC;  // G
        line_buf[i * 3 + 2] = src[i * 3 + 2] & 0xFC;  // B
    }
    
    // Gửi lên LCD
    lcd_send_data(&lcd, line_buf, width * 3);
    free(line_buf);
    
    return 1;  // Success
}

void display_jpeg(const char *path) {
    // 1. Đọc JPEG file từ SD card
    uint8_t *jpeg_data = malloc(file_size);
    sd_card_read_file(&sd_card, path, jpeg_data, file_size);
    
    // 2. Decode JPEG → gửi từng dòng lên LCD
    JDEC jd;
    jd.device_ptr = 0;  // Offset trong jpeg_data
    
    // Acquire SPI bus cho toàn bộ thao tác
    spi_device_acquire_bus(lcd.spi, portMAX_DELAY);
    lcd_set_window(&lcd, 0, 0, 319, 479);  // Full screen
    
    // tjpgd tự gọi tjpgd_data_out cho mỗi dòng
    jd_decomp(&jd, tjpgd_data_out, 0);
    
    spi_device_release_bus(lcd.spi);
    free(jpeg_data);
}
```

---

### ⚠️ Lưu ý quan trọng

1. **Kích thước ảnh phải đúng 320×480** (hoặc nhỏ hơn) — tjpgd không resize
2. **JPEG phải là baseline** (không progressive)
3. **Cần PSRAM** — ảnh decode cần nhiều RAM
4. **RGB888 → RGB666**: mask mỗi byte với `& 0xFC`
5. **Acquire bus** cho toàn bộ quá trình decode + gửi — tránh SD task chen ngang

---

### 🎯 Bước tiếp theo

Em muốn:
1. **Thầy hướng dẫn setup `tjpgd` + code đầy đủ?** (cần thêm component + PSRAM config)
2. **Hay tự đọc ESP-IDF `lcd_jpeg` example trước?** (có sẵn trong `esp-idf/examples/peripherals/lcd/jpeg/`)

**Thầy khuyên đọc example trước** — giống như SD card, hiểu example rồi mới code. 🖼️

