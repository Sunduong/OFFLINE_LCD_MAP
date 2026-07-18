#ifndef MAP_DEFINE_H
#define MAP_DEFINE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lcd_driver.h"
#include "sd_card_driver.h"

// ──────────────────────────── Tile Constants ────────────────────────────
#define TILE_SIZE               256
#define TILE_PIXELS             (TILE_SIZE * TILE_SIZE)
#define TILE_BYTES_RGB666       (TILE_PIXELS * 3)

// ──────────────────────────── Map Handle ────────────────────────────
typedef struct {
    lcd_t       *lcd;           // CLD driver handle
    sd_card_t   *sd_card;            // SD card drvier handle
    int         zoom;           // current zoom level
    uint8_t     *tile_buf;      // Buffer for loading one tile (192KB)
} map_handle_t;

// ──────────────────────────── Map Position ────────────────────────────
typedef struct {
    int zoom;
    double tile_x;
    double tile_y;
    int base_tile_x;
    int base_tile_y;
    int pixel_offset_x;
    int pixel_offset_y;
} map_position_t;

#endif // MAP_DEFINE_H
