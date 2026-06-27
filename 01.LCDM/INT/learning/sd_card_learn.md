# SD Card Knowledge Plan — From Zero to Driver

## Goal
Understand SD card well enough to write a driver that shares SPI bus with LCD.

---

## Phase A: Physical Understanding (What is SD Card?)

### A1. What does an SD card look like inside?
- [ ] Learn: SD card = Flash memory + Controller chip
- [ ] Learn: 9 pins on full-size SD, 8 pins on microSD
- [ ] Learn: Which pins are for SPI mode? (MOSI, MISO, SCK, CS, VCC, GND)

### A2. Two communication modes
- [ ] Learn: SD Bus mode (1-bit / 4-bit) — uses dedicated pins, faster
- [ ] Learn: SPI mode — uses standard SPI bus, slower, but shares bus with other devices
- [ ] Understand: Why we use SPI mode? → Because we share the bus with LCD!

### A3. SPI mode pin connections
- [ ] Draw the wiring diagram: ESP32 ↔ SD card
 ```
 ESP32 GPIO11 (MOSI) → SD card MOSI (pin 2)
 ESP32 GPIO12 (MISO) ← SD card MISO (pin 7)
 ESP32 GPIO10 (SCK) → SD card SCK (pin 5)
 ESP32 GPIO13 (CS) → SD card CS (pin 1)
 ESP32 3.3V → SD card VDD (pin 4)
 ESP32 GND → SD card GND (pin 3,6)
 ```

**Checkpoint A:** Can you draw the wiring diagram from memory? If yes → proceed to Phase B.

---

## Phase B: SD Card SPI Protocol (How to talk to SD card)

### B1. SPI basics review
- [ ] Remember: SPI = Master sends clock, Master selects slave via CS
- [ ] Remember: MOSI = Master Out Slave In, MISO = Master In Slave Out
- [ ] Remember: CS low = selected, CS high = not selected

### B2. SD card SPI command format
- [ ] Learn: Every SD card command is 6 bytes:
 ```
 Byte 0: 01 + Command index (e.g. CMD0 = 0x40, CMD1 = 0x41)
 Byte 1-4: Argument (4 bytes, big-endian)
 Byte 5: CRC7 + stop bit
 ```
- [ ] Learn: After each command, SD card sends a response (1-5 bytes)

### B3. SD card initialization sequence
- [ ] Learn the startup sequence:
 ```
 1. Send 80+ clock cycles with CS high (SD card wakes up)
 2. CMD0 (GO_IDLE_STATE) → Card enters SPI mode
 3. CMD8 (SEND_IF_COND) → Check voltage (SD v2 only)
 4. CMD55 + ACMD41 (SD_SEND_OP_COND) → Wait until card is ready
 5. CMD58 (READ_OCR) → Read capacity info
 ```
- [ ] Understand: ESP-IDF handles ALL of this automatically via `esp_vfs_fat_sdspi_mount()`
- [ ] You do NOT need to send these commands manually!


### XuanDuong comments:
Question: Why send 80+ clock cycles with CS HIGH to wake up SD card?
 --> The SD card has TWO modes: SD Bus mode and SPI mode

When power is first applied, the SD card starts in __SD Bus mode__ (its default mode). To switch to __SPI mode__, you must follow a specific sequence.
The wake-up sequence explained

Step 1: CS HIGH + 80 clock cycles
 ┌──────────────────────────────────────────────────────────────┐
 │ CS HIGH = "I'm NOT talking to you yet" │
 │ │
 │ Why 80 clocks? │
 │ - SD card needs clock cycles to stabilize its internal │
 │ circuits after power-on │
 │ - 80 clocks = 10 bytes of dummy data │
 │ - With CS high, the card IGNORES the data but RECEIVES │
 │ the clock signal → internal state machine resets │
 │ - This is like knocking on the door before entering │
 │ │
 │ MOSI: ▒▒▒▒▒▒▒▒▒▒ (dummy data, card ignores it) │
 │ SCK: ┌┐┌┐┌┐┌┐┌┐┌┐┌┐┌┐┌┐ (80+ clock pulses) │
 │ CS: ──────────────────── (HIGH = not selected) │
 │ MISO: ?????????? (card not responding yet) │
 └──────────────────────────────────────────────────────────────┘

Step 2: CS LOW + CMD0 → Switch to SPI mode
 ┌──────────────────────────────────────────────────────────────┐
 │ CS LOW = "Now I AM talking to you!" │
 │ │
 │ CMD0 (GO_IDLE_STATE) = 0x40 0x00 0x00 0x00 0x00 0x95 │
 │ │
 │ This command tells the card: "Reset and enter SPI mode" │
 │ After CMD0, the card switches from SD Bus mode → SPI mode │
 │ │
 │ MOSI: [CMD0 bytes] │
 │ SCK: ┌┐┌┐┌┐┌┐┌┐┌┐ │
 │ CS: ─────┐ ┌────── (LOW = selected!) │
 │ └─────┘ │
 │ MISO: [0x01 response] ← Card says "I'm in SPI mode now!" │
 └──────────────────────────────────────────────────────────────┘





### B4. Reading/writing data blocks
- [ ] Learn: Data is read/written in 512-byte blocks (sectors)
- [ ] Learn: CMD17 reads one block, CMD24 writes one block
- [ ] Understand: FAT filesystem translates files → block numbers → CMD17/CMD24

**Checkpoint B:** Can you explain what CMD0, CMD8, ACMD41 do? Do you understand why ESP-IDF handles this for you? If yes → proceed to Phase C.

---



## Phase C: FAT Filesystem (How files are stored)



### C1. What is a filesystem?

- [ ] Learn: A filesystem organizes data on storage into files and folders

- [ ] Learn: Without filesystem, SD card is just raw 512-byte blocks

- [ ] Learn: Common filesystems: FAT16, FAT32, exFAT, NTFS, ext4



### C2. Why FAT32?

- [ ] Learn: FAT32 = simple, widely supported, works on all OS

- [ ] Learn: Most SD cards come formatted as FAT32 from factory

- [ ] Learn: ESP-IDF supports FAT16 and FAT32 (not exFAT or NTFS)



### C3. FAT32 structure (simplified)

- [ ] Learn the 3 key areas:

```

┌──────────────────────────────────┐

│ Boot Sector / MBR │ ← Where filesystem info lives

├──────────────────────────────────┤

│ FAT (File Allocation Table) │ ← Which clusters are used/free

│ (2 copies for safety) │

├──────────────────────────────────┤

│ Root Directory │ ← List of files/folders

├──────────────────────────────────┤

│ Data Area (clusters) │ ← Actual file data

└──────────────────────────────────┘

```

- [ ] Learn: Cluster = smallest unit of allocation (typically 4KB-32KB)

- [ ] Learn: FAT table = linked list of clusters (file = chain of clusters)



### C4. How a file is read (simplified)

- [ ] Learn the flow:

```

1. Read root directory → find "test.txt" entry → get starting cluster number

2. Read FAT table → follow chain: cluster 5 → 6 → 7 → EOF

3. Read data from clusters 5, 6, 7 → that's your file content

```



**Checkpoint C:** Can you explain what FAT table does? Can you explain how a file is found on disk? If yes → proceed to Phase D.



---



## Phase D: VFS — Virtual File System (How ESP-IDF makes it easy)



### D1. What is VFS?

- [ ] Learn: VFS = Virtual File System — a layer that makes all storage look the same

- [ ] Learn: After mounting, you use standard C functions:

```c

FILE *f = fopen("/sdcard/test.txt", "r"); // Open file

fread(buffer, 1, 100, f); // Read file

fclose(f); // Close file

```

- [ ] Learn: VFS translates fopen/fread → FAT operations → SD card SPI commands

- [ ] You NEVER deal with sectors, clusters, or CMD17 directly!



### D2. Mount point

- [ ] Learn: Mount point = where the filesystem appears in the path

- [ ] Learn: `/sdcard` is the mount point → all files are at `/sdcard/filename`

- [ ] Learn: `esp_vfs_fat_sdspi_mount()` registers this path



### D3. The one function that does everything

- [ ] Learn: `esp_vfs_fat_sdspi_mount()` does ALL of this:

```

1. Adds SD card device to SPI bus (spi_bus_add_device internally)

2. Sends SD card init commands (CMD0, CMD8, ACMD41...)

3. Reads FAT32 boot sector

4. Mounts FAT filesystem

5. Registers /sdcard path in VFS

```

- [ ] After calling this ONE function → you can use fopen/fread/fwrite!



**Checkpoint D:** Can you explain the full path from `fopen("/sdcard/test.txt", "r")` to actual SPI bytes on the wire? If yes → you're ready to code!



---



## Phase E: Hands-on Coding (Apply the knowledge)



### E1. Create SD card driver files

- [ ] Create `SDcard_driver.h` with struct and function declarations

- [ ] Create `SDcard_driver.c` with `sd_card_init()` using `esp_vfs_fat_sdspi_mount()`



### E2. Add file read/write functions

- [ ] Add `sd_card_read_file()` — uses fopen/fread/fclose

- [ ] Add `sd_card_list_dir()` — uses opendir/readdir/closedir



### E3. Test with LCD sharing SPI bus

- [ ] Call `spi_bus_init()` → `lcd_init()` → `sd_card_init()` in app_main

- [ ] Create 2 tasks sharing the bus

- [ ] Verify LCD doesn't glitch when SD card reads



---



## Knowledge Check — Must answer ALL before coding



1. What are the 6 pins needed to connect SD card in SPI mode?

2. Why do we use SPI mode instead of SD Bus mode?

3. What does CMD0 do? Do you need to send it manually?

4. What is FAT32? Why not NTFS or ext4?

5. What does the FAT table do?

6. What does `esp_vfs_fat_sdspi_mount()` do? (List all 5 steps)

7. After mounting, how do you read a file? (Show the C code)

8. Why don't you need to call `spi_bus_add_device()` for SD card?



















































## B4 Explained: Reading/Writing Data Blocks on SD Card

### What does "512-byte blocks (sectors)" mean?

SD card storage is divided into **fixed-size blocks** called **sectors**. Each sector = **512 bytes**. You cannot read or write less than 512 bytes at a time.

```
SD Card storage (e.g. 8GB):

┌──────────┬──────────┬──────────┬──────────┬─────┐
│ Sector 0 │ Sector 1 │ Sector 2 │ Sector 3 │ ... │
│ 512 bytes│ 512 bytes│ 512 bytes│ 512 bytes│ │
└──────────┴──────────┴──────────┴──────────┴─────┘
 Address 0 Address 1 Address 2 Address 3

You can ONLY read/write ONE sector at a time.
You CANNOT read just 10 bytes — you must read the full 512-byte sector.
```

### What are CMD17 and CMD24?

These are SD card SPI commands to read/write one sector:

| Command | Name | What it does |
|---------|------|-------------|
| **CMD17** | READ_SINGLE_BLOCK | Read 512 bytes from a sector address |
| **CMD24** | WRITE_BLOCK | Write 512 bytes to a sector address |

### How CMD17 works (read one sector):

```
ESP32 sends: CMD17 + sector_address (e.g. "read sector 5")
SD card responds: 512 bytes of data

Timeline:
 ESP32: [CMD17][addr=5] ──────────────────── [receive 512 bytes] ──→ done
 SD card: [processing...] ── [send data data data...] ──→ done
```

### How CMD24 works (write one sector):

```
ESP32 sends: CMD24 + sector_address + 512 bytes of data
SD card responds: "OK, written"

Timeline:
 ESP32: [CMD24][addr=5][send 512 bytes] ──→ done
 SD card: [receive data] [writing to flash...] ── [OK]
```

### What does "FAT translates files → block numbers → CMD17/CMD24" mean?

This is the key concept! When you call `fopen("/sdcard/test.txt", "r")`, here's what happens:

```
Your code: fopen("/sdcard/test.txt", "r")
 │
 ▼
VFS layer: "Open file at /sdcard/test.txt"
 │
 ▼
FAT layer: Look up "test.txt" in directory
 Found! Starting cluster = 5
 FAT table says: cluster 5 → sector 200-207
 │
 ▼
SD card layer: CMD17 sector 200 → 512 bytes
 CMD17 sector 201 → 512 bytes
 CMD17 sector 202 → 512 bytes
 ...
 │
 ▼
Your code: fread(buffer, 1, 100, f) → gets first 100 bytes
```

### Real-world analogy

```
Think of SD card like a library:

 Sector = One page in a book (512 characters per page)
 CMD17 = "Give me page #200"
 CMD24 = "Write this on page #200"
 FAT = Library card catalog (which pages belong to which book)
 fopen() = "Find me the book called test.txt"
 fread() = "Read me some pages from that book"

 You NEVER read half a page — you always read a full page (512 bytes).
 The library (FAT) figures out which pages to read.
```

### The important takeaway

```
┌──────────────────────────────────────────────────────────────┐
│ You do NOT need to use CMD17/CMD24 directly! │
│ │
│ ESP-IDF chain: │
│ fopen/fread → VFS → FAT → CMD17/CMD24 → SPI bus │
│ │
│ You only write: fopen("/sdcard/test.txt", "r") │
│ ESP-IDF handles everything else automatically! │
│ │
│ But understanding this helps you debug when things break. │
└──────────────────────────────────────────────────────────────┘
```

Now, can you answer the Checkpoint B questions?
1. What does CMD0 do? → Switches SD card to SPI mode
2. What does CMD8 do? → Checks voltage compatibility (SD v2 cards)
3. What does ACMD41 do? → Tells SD card to start up and get ready
4. Why does ESP-IDF handle this for you? → So you don't have to write low-level SPI commands manually

If you understand these, proceed to Phase C (FAT filesystem)!