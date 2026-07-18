#include "map_parser.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "sd_card_driver.h"

static const char *TAG = "MAP_PARSER";

/**
 * @brief  Build the SD card file path for a specific tile.
 *
 * Path format: /sdcard/maps/{zoom}/{tile_x}/{tile_y}.bin
 * 
 * @param zoom      Zoom level
 * @param tile_x    Tile X coordinate
 * @param tile_y    Tile Y coordinate
 * @param path      Output buffer for path string
 * @param path_len  Size of path buffer
 */
static void map_tile_path(int zoom, int tile_x, int tile_y, char *path, size_t path_len)
{
    snprintf(path, path_len, "/sdcard/maps/%d/%d/%d.bin", zoom, tile_x, tile_y);
}

/**
 * @brief  Load a single tile data from SD card into buffer on internal RAM ESP32S3.
 *
 * Tile file is raw RGB666: 256*256*3 = 192KB.
 * Buffer must be at least TILE_BYTES_RGB666 bytes.
 * 
 * @param sd        SD card handle
 * @param zoom      Zoom level
 * @param tile_x    Tile X coordinate
 * @param tile_y    Tile Y coordinate
 * @param buffer    Output buffer
 * @return ESP_OK on success, ESP_FAIL if file not found or read error.
 */
esp_err_t map_load_tile(sd_card_t *sd, int zoom, int tile_x, int tile_y, uint8_t *buffer)
{
    if (sd == NULL || buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_card_is_mounted(sd))
    {
        ESP_LOGE(TAG, "SD Card not mounted!");
        return ESP_ERR_INVALID_STATE;
    }
// int sd_card_read_file(sd_card_t *sd, const char *path, uint8_t *buffer, size_t max_len)
    char path[64];
    map_tile_path(zoom, tile_x, tile_y, path, sizeof(path));

    // Read file from SD card
    int bytes_read = sd_card_read_file(sd, path, buffer, TILE_BYTES_RGB666);
    if (bytes_read < 0)
    {
        ESP_LOGW(TAG, "Tile not found: %s", path);
        return ESP_FAIL;
    }
    if (bytes_read < TILE_BYTES_RGB666)
    {
        ESP_LOGW(TAG, "Tile incomplete: %s (%d/%d bytes)", path, bytes_read, TILE_BYTES_RGB666);
        return ESP_FAIL;
    }
    return ESP_OK;
}