#ifndef SPI_BUS_MANAGER_H
#define SPI_BUS_MANAGER_H

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ──────────────────────────── Shared SPI Bus Pin Definitions ────────────────────────────
// These pins are SHARED by LCD and SD card on the same SPI2 bus
#define SPI_MOSI_PIN    11      // Channel 2 of Logic Analyzer
#define SPI_MISO_PIN    12      // Channel 5 of Logic Analyzer
#define SPI_SCK_PIN     10      // Channel 0 of Logic Analyzer

// ──────────────────────────── Device CS Pin Definitions ────────────────────────────
// Each device has its OWN CS pin. The SPI driver manages CS automatically.
// ⚠️ Do NOT toggle CS manually — let spi_bus_add_device() + SPI driver handle it!
#define SPI_LCD_CS_PIN      9       // Channel 1 of Logic Analyzer
#define SPI_SD_CS_PIN       13      // Channel 4 of Logic Analyzer
#define SPI_TOUCH_CS_PIN    14      // Reserved for future touch screen

// ──────────────────────────── LCD-specific Pins ────────────────────────────
#define SPI_LCD_DC_PIN      8       // Channel 3 of Logic Analyzer
#define LCD_RESET_PIN       7

// ──────────────────────────── Public API ────────────────────────────

/**
 * @brief Initialize the shared SPI bus.
 *
 * Must be called ONCE before adding any devices (LCD, SD card).
 * Call this FIRST in app_main().
 *
 * ⚠️ This function does NOT configure CS pins.
 *    Each device driver configures its own CS via spi_bus_add_device()
 *    (LCD) or esp_vfs_fat_sdspi_mount() (SD card).
 *
 * @param host  SPI host to use (e.g. SPI2_HOST)
 */
void spi_bus_init(spi_host_device_t host);

#endif // SPI_BUS_MANAGER_H
