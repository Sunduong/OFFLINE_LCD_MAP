#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "map_define.h"

esp_err_t map_renderer_init(map_handle_t *map, lcd_t *lcd, sd_card_t *sd, int zoom);
void mao_renderer_deinit(map_handle_t *map);
void map_render(map_handle_t *map, double lat, double lon);
void map_draw_marker(map_handle_t *map);
#endif // MAP_RENDERER_H
