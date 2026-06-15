You've completed the LCD driver — that's Phase 2.1 done! 🎉

According to your `plan.md`, here's what's next in **Phase 2: Core Drivers Development**:

### Next Up: SD Card & Filesystem (Phase 2.2)

This is the critical next step because your map tiles will live on the SD card. You need to:

1. **Initialize SD card in SPI mode** — shares the same SPI bus as the LCD (same MOSI/MISO/SCK, separate CS pin on GPIO13)
2. **Mount FAT filesystem** — using ESP-IDF's `esp_vfs_fat_sdspi_mount()`
3. **Implement file read/write** — test by reading a file from SD and printing its contents
4. **SPI bus sharing with mutex** — LCD and SD card share the same SPI bus, so you need a mutex to prevent both from accessing it simultaneously

**Key concept to learn:** The SD card and LCD are on the **same SPI bus** but with different CS pins. Only one device can use the bus at a time — this is where FreeRTOS mutexes become essential (which you already learned in `practice.c`!).

After SD card, the order continues:
- **Phase 2.3:** GPS Driver (UART + NMEA parsing)
- **Phase 2.4:** Button Handler (GPIO interrupts + debounce)

Want me to create a lesson guide for the SD card driver, similar to what you had for the ILI9488?
