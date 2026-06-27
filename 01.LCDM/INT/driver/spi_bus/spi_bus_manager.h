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

/**
 * @brief Acquire (lock) the SPI bus for exclusive access
 *
 * @param manager Pointer to manager structure
 * @param timeout Maximum time to wait (in FreeRTOS ticks)
 *                Use portMAX_DELAY for infinite wait (NOT recommended in production)
 *                Use pdMS_TO_TICKS(1000) for 1-second timeout (RECOMMENDED)
 *
 * @return pdTRUE if lock acquired within timeout
 *         pdFALSE if timeout expired (bus is held by another task)
 *
 * Usage:
 *     if (spi_bus_manager_acquire(&spi_manager, pdMS_TO_TICKS(1000)) == pdTRUE) {
 *         lcd_fill_screen(...);  // Safe SPI access
 *         spi_bus_manager_release(&spi_manager);
 *     } else {
 *         ESP_LOGE(TAG, "SPI bus timeout!");
 *     }
 */
BaseType_t spi_bus_manager_acquire(spi_bus_manager_t *manager, TickType_t timeout);

/**
 * @brief Release (unlock) the SPI bus for other tasks
 *
 * @param manager Pointer to manager structure
 *
 * CRITICAL: Always call this after you're done with SPI access!
 *           If you forget, other tasks will deadlock.
 *           In practice, use try-finally pattern or scope guards.
 */
void spi_bus_manager_release(spi_bus_manager_t *manager);

#endif // SPI_BUS_MANAGER_H
