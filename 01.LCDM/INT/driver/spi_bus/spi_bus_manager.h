#ifndef SPI_BUS_MANAGER_H
#define SPI_BUS_MANAGER_H

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ──────────────────────────── Shared SPI Bus Pin Definitions ────────────────────────────
// These pins are SHARED by LCD and SD card on the same SPI2 bus
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 12
#define SPI_SCK_PIN 10

// ──────────────────────────── Device CS Pin Definitions ────────────────────────────
#define SPI_LCD_CS_PIN 9
#define SPI_SD_CS_PIN 13

// ──────────────────────────── LCD-specific Pins ────────────────────────────
#define SPI_LCD_DC_PIN 8
#define LCD_RESET_PIN 7

// ──────────────────────────── Public API ────────────────────────────

/**
 * @brief Initialize the shared SPI bus.
 * Must be called ONCE before adding any devices (LCD, SD card).
 * Call this FIRST in app_main().
 *
 * @param host SPI host to use (e.g. SPI2_HOST)
 */
void spi_bus_init(spi_host_device_t host);
void spi_bus_all_cs_high(void);
void spi_bus_select_lcd(void);
void spi_bus_select_sd(void);

