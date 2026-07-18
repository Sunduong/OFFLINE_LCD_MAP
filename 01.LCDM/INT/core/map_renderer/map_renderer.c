#include "map_renderer.h"
#include "map_parser.h"
#include "map_math.h"
#include <math.h>

static const char *TAG = "MAP_RENDERER";

#define SCREEN_CENTER_X         LCD_WIDTH / 2
#define SCREEN_CENTER_Y         LCD_HEIGHT / 2

/**
 * @brief  Initialize the map renderer.
 *
 * Allocates the 192KB tile buffer. 
 * 
 * @param map       Map handle
 * @param lcd       LCD driver handle
 * @param sd        SD card driver handle
 * @param zoom      Default zoom level (e.g. 15)
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer allocation fails
 */
esp_err_t map_renderer_init(map_handle_t *map, lcd_t *lcd, sd_card_t *sd, int zoom)
{
    if (map == NULL || lcd == NULL || sd == NULL)
        return ESP_ERR_INVALID_ARG;
    
    map->lcd = lcd;
    map->sd_card = sd;
    map->zoom = zoom;
    map->tile_buf = heap_caps_malloc(TILE_BYTES_RGB666, MALLOC_CAP_DMA);
    if (map->tile_buf == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate tile buffer (%d bytes)", TILE_BYTES_RGB666);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "Map render initialized (zoom=%d, tile_buf=%dKB)", zoom, TILE_BYTES_RGB666/1024);
    return ESP_OK;
}

/**
 * @brief  Deinitialize map renderer - free tile buffer.
 *
 */
void mao_renderer_deinit(map_handle_t *map)
{
    // Do later
}

/**
 * @brief  Render map at the given GPS position.
 *
 * Load 6 tiles around the GPS position and draws them on LCD.
 * Then draws the position marker at center of screen.
 * 
 * @param map       Map handle
 * @param lat       Current latitude
 * @param lon       Current longitude
 */
void map_render(map_handle_t *map, double lat, double lon)
{
    if (map == NULL || map->tile_buf == NULL)
    {
        ESP_LOGW(TAG, "No tile data to render!");
        return;
    }

    // Step 1: Convert GPS to tile coordinates
    map_position_t pos;
    gps_to_map_position(lat, lon, map->zoom, &pos);

    // Step 2: Calculate screen position for each tile
    // the GPS marker is always at screen center (160, 240)
    int base_screen_x = SCREEN_CENTER_X - pos.pixel_offset_x;
    int base_screen_y = SCREEN_CENTER_Y - pos.pixel_offset_y;

    ESP_LOGI(TAG, "Render: lat=%.6f lon=%.6f tile=(%d,%d) offset=(%d,%d) screen_base=(%d,%d)",
                 lat, lon, (int)pos.tile_x, (int)pos.tile_y, pos.pixel_offset_x, pos.pixel_offset_y,
                 base_screen_x, base_screen_y);
    
    // Step 3: Clear the screen first (in case tiles are missing)
    lcd_fill_screen(map->lcd, COLOR_BLACK);

    // Step 4: Load and draw 9 tiles (3x3 grid)
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int tile_x = pos.base_tile_x + dx;
            int tile_y = pos.base_tile_y + dy;

            // Select screen position for this tile
            int screen_x = base_screen_x + dx * TILE_SIZE;
            int screen_y = base_screen_y + dy * TILE_SIZE;

            // Skip if entirely off-screen
            if (screen_x >= LCD_WIDTH || screen_y >= LCD_HEIGHT)    continue;
            if (screen_x + TILE_SIZE <= 0 || screen_y + TILE_SIZE <= 0) continue;

            // Load tile from SD card
            esp_err_t err = map_load_tile(map->sd_card, map->zoom, tile_x, tile_y, map->tile_buf);
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG, "Missing tile (%d,%d) - drawing gray", tile_x, tile_y);
                // Draw green rectangle for missing tile
                int draw_x = (screen_x <= 0) ? 0 : screen_x;
                int draw_y = (screen_y <= 0) ? 0 : screen_y;
                int draw_x2 = ((screen_x + TILE_SIZE) >= LCD_WIDTH) ? (LCD_WIDTH - 1) : (screen_x + TILE_SIZE - 1);
                int draw_y2 = ((screen_y + TILE_SIZE) >= LCD_HEIGHT) ? (LCD_HEIGHT - 1) : (screen_y + TILE_SIZE - 1);
                lcd_fill_rect(map->lcd, draw_x, draw_y, draw_x2, draw_y2, COLOR_GREEN);
                continue;
            }

            // Draw tile on LCD
            lcd_draw_bitmap(map->lcd, screen_x, screen_y, screen_x + TILE_SIZE - 1, screen_y + TILE_SIZE - 1, map->tile_buf);
        }
    }

    // Step 5: Draw position marker at center
    map_draw_marker(map);
}

/**
 * @brief  Draw the position marker at center of screen.
 *
 * Draw a red circle with crosshair at LCD center (160, 240).
 * 
 * @param map       Map handle
 */
void map_draw_marker(map_handle_t *map)
{
    if (map == NULL || map->lcd == NULL)    return;

    int center_x =  SCREEN_CENTER_X;
    int center_y = SCREEN_CENTER_Y;
    int R = 6;

    // Draw filled circle (radius 5) using rects
    for (int dy = -R; dy <= R; dy ++)
    {
        int half_width = (int)sqrt(R * R - dy * dy);
        lcd_draw_hline(map->lcd, center_x - half_width, center_y + dy, half_width * 2 + 1, COLOR_RED);
    }

    lcd_draw_hline(map->lcd, center_x - 10, center_y, 21, COLOR_WHITE);
    lcd_draw_vline(map->lcd, center_x, center_y - 10, 21, COLOR_WHITE);
    lcd_draw_rect(map->lcd, center_x - R - 1, center_y - R, center_x + R + 1, center_y + R + 1, COLOR_BLACK);
}
