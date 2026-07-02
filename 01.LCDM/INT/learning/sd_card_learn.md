# SD Card Knowledge Plan - From Zero to Manual SPI Driver

## Goal
Understand an SD card well enough to write a manual SD-over-SPI driver that shares the SPI bus with the LCD.

This project intentionally avoids `esp_vfs_fat_sdspi_mount()` and normal file APIs at the beginning. The first goal is to talk to the card at the block level: initialize it, read sectors, and later understand how FAT maps files onto those sectors.

---

## Current Direction

You already finished the LCD driver. The SD card should now be developed as another device on the same SPI bus:

- LCD and SD share MOSI, MISO, and SCK.
- LCD and SD must use different CS pins.
- Only one device should have CS low during a transaction.
- SD card initialization must start slowly, normally around 100-400 kHz.
- After initialization, SD SPI clock can be increased, for example to 10-20 MHz if the wiring and card are stable.

Do not use the VFS/FAT wrapper yet. For learning, build this stack in layers:

```text
Your code
  |
  v
Manual SD block API: sd_read_sector(), sd_write_sector()
  |
  v
Manual SD SPI protocol: CMD0, CMD8, CMD55, ACMD41, CMD58, CMD17, CMD24
  |
  v
ESP-IDF SPI master driver: spi_device_transmit()
  |
  v
Shared SPI bus pins
```

---

## Phase A: Physical Understanding

### A1. What is inside an SD card?

- [ ] Learn: SD card = flash memory + controller chip.
- [ ] Learn: Full-size SD has 9 pins; microSD has 8 pins.
- [ ] Learn: SPI mode uses MOSI, MISO, SCK, CS, VCC, and GND.

### A2. Two communication modes

- [ ] Learn: SD bus mode can use 1-bit or 4-bit data.
- [ ] Learn: SPI mode is slower but can share a bus with other SPI devices.
- [ ] Understand: This project uses SPI mode because LCD and SD card share the same SPI host.

### A3. SPI mode pin connections

- [ ] Draw the wiring diagram from memory.

```text
ESP32 GPIO11 (MOSI) -> SD card DI / MOSI
ESP32 GPIO12 (MISO) <- SD card DO / MISO
ESP32 GPIO10 (SCK)  -> SD card CLK / SCK
ESP32 GPIO13 (CS)   -> SD card CS
ESP32 3.3V          -> SD card VDD
ESP32 GND           -> SD card GND
```

Checkpoint A:

- Can you draw the wiring without looking?
- Can you explain why LCD CS and SD CS must not be low at the same time?

---

## Phase B: SPI Basics For SD Card

### B1. SPI review

- [ ] SPI master provides the clock.
- [ ] MOSI means Master Out, Slave In.
- [ ] MISO means Master In, Slave Out.
- [ ] CS low selects a device.
- [ ] CS high deselects a device.

### B2. Shared bus rules

Before talking to the SD card:

1. Make LCD CS high.
2. Make SD CS low.
3. Send the SD command or data transfer.
4. Make SD CS high.
5. Send at least one extra `0xFF` byte after some SD operations so the card can release MISO.

Checkpoint B:

- Can you explain what happens if LCD CS and SD CS are both low?
- Can you explain why idle MOSI bytes are usually `0xFF` for SD cards?

---

## Phase C: SD Card SPI Protocol

### C1. Command packet format

Every SD command packet is 6 bytes:

```text
Byte 0: 0x40 | command_index
Byte 1: argument bits 31..24
Byte 2: argument bits 23..16
Byte 3: argument bits 15..8
Byte 4: argument bits 7..0
Byte 5: CRC7 + stop bit
```

Important examples:

| Command | Packet byte 0 | Purpose |
|---------|---------------|---------|
| CMD0    | 0x40          | Reset card and enter SPI idle state |
| CMD8    | 0x48          | Check voltage range and SD v2 support |
| CMD55   | 0x77          | Prefix before an application command |
| ACMD41  | 0x69          | Ask card to finish initialization |
| CMD58   | 0x7A          | Read OCR register and capacity flags |
| CMD17   | 0x51          | Read one 512-byte block |
| CMD24   | 0x58          | Write one 512-byte block |

Only CMD0 and CMD8 need fixed CRC values in SPI mode before CRC is enabled:

- CMD0 CRC byte: `0x95`
- CMD8 CRC byte: `0x87`
- Most other commands can use `0xFF` while CRC is disabled.

### C2. Response types

- R1 response: 1 status byte.
- R3/R7 response: R1 byte plus 4 data bytes.
- A valid response is not `0xFF`; poll by reading `0xFF` until the card answers or timeout expires.

Common R1 bits:

```text
0x01 = idle state
0x00 = ready / no error
0x04 = illegal command
```

### C3. Initialization sequence

Use this sequence manually:

```text
1. SD CS high.
2. Send at least 80 clocks by transmitting ten bytes of 0xFF.
3. SD CS low.
4. Send CMD0, argument 0, CRC 0x95.
5. Expect R1 = 0x01.
6. Send CMD8, argument 0x000001AA, CRC 0x87.
7. Expect R7 response ending in 0x01AA for modern SDHC/SDXC cards.
8. Loop:
   a. Send CMD55.
   b. Send ACMD41 with HCS bit set: argument 0x40000000.
   c. Stop when R1 = 0x00.
9. Send CMD58.
10. Read OCR and check CCS bit to know if the card uses block addressing.
11. Increase SPI clock after the card is initialized.
```

### C4. Why 80+ clocks with CS high?

When power is first applied, the card needs clocks before it is ready to receive commands. With CS high, the card is not selected, but it still receives clock edges. Sending at least 80 clocks gives the internal controller time to leave power-up state.

Then CMD0 is sent with CS low. This is the command that resets the card and puts it into SPI idle state. A correct early response is `0x01`.

Checkpoint C:

- What does CMD0 do?
- What does CMD8 prove?
- Why do we send CMD55 before ACMD41?
- What does CMD58 tell us?

---

## Phase D: Block Read And Write

### D1. Sector size

SD cards are normally accessed in 512-byte logical blocks.

```text
Sector 0 -> 512 bytes
Sector 1 -> 512 bytes
Sector 2 -> 512 bytes
...
```

For SDHC/SDXC cards, CMD17/CMD24 arguments are block numbers.
For older SDSC cards, arguments may be byte addresses. That means sector `N` uses argument `N * 512`.

### D2. Read one block with CMD17

Read flow:

```text
1. Send CMD17 with sector address.
2. Wait for R1 = 0x00.
3. Wait for data token 0xFE.
4. Read 512 data bytes.
5. Read 2 CRC bytes and ignore them for now.
6. Deselect card and send one extra 0xFF.
```

### D3. Write one block with CMD24

Write flow:

```text
1. Send CMD24 with sector address.
2. Wait for R1 = 0x00.
3. Send data token 0xFE.
4. Send 512 data bytes.
5. Send 2 dummy CRC bytes.
6. Read data response token.
7. Wait while card is busy.
8. Deselect card and send one extra 0xFF.
```

For the first milestone, implement read before write. Reading sector 0 is enough to prove the card works.

Checkpoint D:

- Can you read sector 0 into a 512-byte buffer?
- Can you print the first 16 bytes in hex?
- Can you detect timeout if the card never sends data token `0xFE`?

---

## Phase E: FAT Filesystem Understanding

Do this after raw block read works.

### E1. What is a filesystem?

- [ ] A filesystem organizes raw sectors into files and directories.
- [ ] Without a filesystem, the SD card is only numbered 512-byte sectors.
- [ ] FAT32 is common on SD cards and simple enough to study.

### E2. FAT32 layout, simplified

```text
MBR or boot sector
  |
  v
FAT32 boot sector / BPB
  |
  v
FAT tables
  |
  v
Root directory cluster
  |
  v
Data clusters
```

### E3. How a file is read manually

```text
1. Read sector 0.
2. Detect whether sector 0 is an MBR or a FAT boot sector.
3. Parse the FAT32 BPB fields.
4. Calculate first data sector.
5. Locate the root directory cluster.
6. Search directory entries for a file name.
7. Follow the FAT cluster chain.
8. Read the file data clusters.
```

This is a later learning step. It is much easier after you trust `sd_read_sector()`.

---

## Phase F: What Not To Use Yet

Do not use these for the learning driver:

- `esp_vfs_fat_sdspi_mount()`
- `fopen()`, `fread()`, `fwrite()`
- `opendir()`, `readdir()`

Those functions are good for production convenience, but they hide the exact protocol and FAT behavior you want to learn.

It is still OK to use ESP-IDF's SPI master driver:

- `spi_bus_initialize()`
- `spi_bus_add_device()`
- `spi_device_transmit()`

The learning target is not "bit-bang SPI from scratch"; the learning target is "implement the SD card protocol and block access yourself."

---

## Hands-On Coding Roadmap

### Milestone 1: SPI device setup

- [ ] Add SD card as a device on the existing SPI bus.
- [ ] Configure SD SPI clock initially to 400 kHz or lower.
- [ ] Use SPI mode 0.
- [ ] Keep LCD CS high when SD is active.
- [ ] Verify MISO is pulled high (returns 0xFF) when card is idle.

### Milestone 2: Low-level byte helpers

- [ ] Implement transfer-one-byte helper.
- [ ] Implement transfer-many-bytes helper.
- [ ] Implement select/deselect helpers for SD CS handling.
- [ ] Implement timeout loops for response polling.
- [ ] CRC7 helper (optional for CMD0/CMD8 only)

### Milestone 3: Command helper

- [ ] Implement `sd_send_cmd(cmd, arg, crc)`.
- [ ] Read and return R1 response.
- [ ] Support reading the extra 4 bytes for CMD8 and CMD58.

### Milestone 4: Card initialization

- [ ] Send 80+ clocks with CS high.
- [ ] Send CMD0 and check for `0x01`.
- [ ] Send CMD8 and verify `0x01AA`.
- [ ] Loop CMD55 + ACMD41 until ready.
- [ ] Send CMD58 and detect SDHC/SDXC addressing.
- [ ] Increase SPI clock after init succeeds.

### Milestone 5: Read sector

- [ ] Implement `sd_read_sector(uint32_t sector, uint8_t buffer[512])`.
- [ ] Read sector 0.
- [ ] Print the first 16 or 32 bytes.
- [ ] Check for `0x55 0xAA` signature at bytes 510 and 511.

### Milestone 6: Optional write sector

- [ ] Only test writing on a spare SD card.
- [ ] Prefer writing to a known test sector, not sector 0.
- [ ] Verify by reading the sector back.

### Milestone 7: FAT32 parser

- [ ] Parse MBR partition table.
- [ ] Parse FAT32 boot sector.
- [ ] Locate root directory.
- [ ] Read one short-name file.

---

## Knowledge Check Before Coding The Next Step

Answer these before implementing initialization:

1. What are the 6 required SD card SPI signals?
2. Why is SPI mode useful in this project?
3. Why must SD initialization start at a slow clock speed?
4. What does CMD0 do?
5. What does CMD8 check?
6. Why does ACMD41 need CMD55 before it?
7. What is the difference between byte addressing and block addressing?
8. What does CMD17 return after the `0xFE` data token?
9. Why should raw write testing use a spare SD card?
10. Which ESP-IDF SPI APIs are still allowed in this learning path?

---

## Immediate Next Step

Your next coding task should be small:

```text
Implement manual SD initialization only.
Do not implement FAT.
Do not implement file read/write.
Do not call VFS mount.
```

Success means:

- CMD0 returns `0x01`.
- CMD8 returns a valid R7 response.
- ACMD41 eventually returns `0x00`.
- CMD58 returns OCR bytes.
- The driver logs whether the card is SDHC/SDXC.

After that, the next step is `sd_read_sector(0, buffer)` and printing the first bytes.

# SD Card Knowledge Plan - Using VFS Library

# Chat 1

## 🧑‍🏫 Thầy khen em đã nhận ra điều này — Đó là dấu hiệu của developer giỏi

**Biết mình chưa hiểu = quan trọng hơn là giả vờ hiểu.**

Em ĐÚNG. Code tôi viết dùng nhiều API mà em chưa từng thấy:
- `SDSPI_HOST_DEFAULT()`, `SDSPI_DEVICE_CONFIG_DEFAULT()` — macro gì?
- `esp_vfs_fat_sdspi_mount()` — 5 tham số, mỗi tham số ý nghĩa gì?
- `sdmmc_card_t`, `sdmmc_host_t`, `sdspi_device_config_t` — struct nào cho việc gì?

→ Nếu không hiểu các struct/macro này, em chỉ đang **copy-paste**, không phải **code**.

---

## 📚 Kế hoạch học: ESP-IDF SD Card Examples

### Bước 1: Tìm examples trong ESP-IDF

ESP-IDF có sẵn examples. Mở terminal:

```bash
# Tìm thư mục examples của ESP-IDF
# Thường ở: %USERPROFILE%\esp\esp-idf\examples\storage\sd_card
```

Có 2 examples quan trọng:
```
esp-idf/examples/storage/sd_card/        ← SPI mode (CẦN XEM)
esp-idf/examples/storage/sdmmc/          ← SDMMC mode (1-bit/4-bit, skip)
```

### Bước 2: Đọc theo thứ tự này

```
1. sd_card/README.md          ← Hiểu tổng quan
2. sd_card/main/sd_card_example_main.c  ← Đọc từng dòng
3. sd_card/CMakeLists.txt     ← Hiểu component dependencies
4. sd_card/sdkconfig.defaults ← Hiểu config cần bật
```

### Bước 3: Khi đọc example, trả lời các câu hỏi:

> **Q1**: Example gọi `spi_bus_initialize()` không? Hay để `esp_vfs_fat_sdspi_mount()` tự làm?
>
> **Q2**: `SDSPI_HOST_DEFAULT()` expand ra struct gì? (Hint: click vào nó trong VS Code)
>
> **Q3**: `SDSPI_DEVICE_CONFIG_DEFAULT()` set `gpio_cs` = gì mặc định?
>
> **Q4**: Example có dùng `spi_bus_add_device()` thủ công không?
>
> **Q5**: `esp_vfs_fat_sdmmc_mount_config_t` có field nào? Ý nghĩa mỗi field?

### Bước 4: Chạy example trên board thật

```
- Copy example ra chỗ khác
- Sửa pin definitions cho đúng board em
- idf.py build flash monitor
- Thấy log: "SD card mounted, size = XXX MB"
- → Lúc này em ĐÃ HIỂU flow thực tế
```

### Bước 5: Quay lại code của mình

Sau khi hiểu example, quay lại đọc `sd_card_driver.c` tôi viết:
- Em sẽ thấy code tôi viết = example + shared bus adaptation
- Lúc đó code sẽ có ý nghĩa, không phải "phép thuật"

---

## 🎯 Tóm tắt

```
Bây giờ:    ❌ Không hiểu code → copy-paste → không học được gì
Học example: ✅ Hiểu từng API → biết struct nào làm gì
Quay lại:    ✅ Code có ý nghĩa → tự debug được → tự viết được
```

**Đi đọc example đi. Khi nào hiểu example, quay lại báo thầy.** 🧑‍🏫

# Chat 2

## 🗺️ Lộ trình tiếp theo — Từ hiểu example đến hoàn thành Phase 2.2

### Giai đoạn 1: HỌC (Hiểu ESP-IDF SD card API)
```
1. Mở esp-idf/examples/storage/sd_card/
2. Đọc README.md
3. Đọc sd_card_example_main.c từng dòng
4. Trả lời 5 câu hỏi thầy đưa ở trên
5. Chạy example trên board thật (sửa pin cho đúng)
6. Thấy log: "SD card mounted, size = XXX MB"
```
→ **Mục tiêu**: Hiểu `esp_vfs_fat_sdspi_mount()` làm gì, struct nào cần gì

### Giai đoạn 2: ÁP DỤNG (Quay lại code của mình)
```
1. Đọc lại sd_card_driver.c thầy viết
2. Hiểu tại sao bỏ spi_bus_add_device()
3. Hiểu tại sao malloc card info
4. Sửa 3 bug trong lcd_driver.c:
   - Bug #1: Deadlock khi malloc fail (THIẾT MẠNG!)
   - Bug #2: COLMOD comment (0x55 → 0x66)
   - Bug #3: MADCTL inconsistency (0x48 vs 0x00)
5. Tạo main/CMakeLists.txt
6. Thêm sdkconfig.defaults: CONFIG_FATFS_SUPPORT_FATFS=y
```
→ **Mục tiêu**: Code compile + LCD vẫn hoạt động + SD card mount thành công

### Giai đoạn 3: TEST (Bước 4-5 trong lesson)
```
1. Tạo file test.txt trên SD card
2. Gọi sd_card_read_file() trong app_main()
3. Thấy log: nội dung file
4. Tạo 2 task: LCD task + SD task
5. LCD nhấp nháy màu + SD đọc file cùng lúc
6. LCD KHÔNG bị giật/lag
```
→ **Mục tiêu**: 2 device chia sẻ SPI bus, hoạt động song song

### Giai đoạn 4: ỨNG DỤNG (Bước 6 trong lesson)
```
1. Tạo file test.rgb565 (ảnh gradient) bằng Python script
2. Đọc ảnh từ SD card
3. Hiển thị lên LCD
4. Ảnh không bị nhiễu, không lệch màu
```
→ **Mục tiêu**: Đọc file ảnh từ SD → hiển thị lên LCD

### Giai đoạn 5: KIỂM TRA CUỐI PHASE
```
Trả lời 5 câu kiểm tra trong lesson:
1. SPI Bus Sharing — khi LCD đang gửi, SD chen vào thì sao?
2. acquire_bus vs Mutex — tại sao dùng acquire_bus?
3. Task Priority — đổi priority thì LCD có giật không?
4. Starvation — LCD không delay thì SD có chạy được không?
5. Real-world — tại sao dùng timeout thay vì portMAX_DELAY?
```
→ **Mục tiêu**: Hiểu sâu enough để giải thích cho người khác

---



