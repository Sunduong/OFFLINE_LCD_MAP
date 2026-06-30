#ifndef SD_CARD_DRIVER_H
#define SD_CARD_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "driver/spi_master.h"
#include "spi_bus_manager.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// ──────────────────────────── SD Card Device Structure ────────────────────────────
typedef struct {
    sdmmc_card_t *card;     // SD card info (allocated by mount, freed by deinit)
    bool           mounted;  // Is filesystem mounted?
} sd_card_t;

// ──────────────────────────── Public API ────────────────────────────

/**
 * @brief Initialize SD card and mount FAT filesystem on shared SPI bus.
 *
 * ⚠️ This function does NOT call spi_bus_add_device() manually.
 *    esp_vfs_fat_sdspi_mount() handles that internally.
 *    The SPI bus must already be initialized via spi_bus_init().
 *
 * @param sd    Pointer to sd_card_t struct
 * @param host  SPI host (must be same as LCD, e.g. SPI2_HOST)
 */
void sd_card_init(sd_card_t *sd, spi_host_device_t host);

/**
 * @brief Unmount FAT filesystem and cleanup SD card resources.
 * @param sd  Pointer to sd_card_t struct
 */
void sd_card_deinit(sd_card_t *sd);

/**
 * @brief Check if SD card filesystem is mounted.
 * @return true if mounted, false otherwise
 */
bool sd_card_is_mounted(sd_card_t *sd);

/**
 * @brief Read a file from SD card into buffer.
 *
 * @param sd       Pointer to sd_card_t struct
 * @param path     Full path starting with /sdcard/ (e.g. "/sdcard/test.txt")
 * @param buffer   Destination buffer
 * @param max_len  Maximum bytes to read
 * @return Number of bytes read, or -1 on error
 */
int sd_card_read_file(sd_card_t *sd, const char *path, uint8_t *buffer, size_t max_len);

/**
 * @brief List files in a directory on SD card.
 *
 * @param sd       Pointer to sd_card_t struct
 * @param dir_path Directory path (e.g. "/sdcard")
 */
void sd_card_list_dir(sd_card_t *sd, const char *dir_path);

#endif // SD_CARD_DRIVER_H
