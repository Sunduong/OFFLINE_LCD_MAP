#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "map_define.h"

esp_err_t map_load_tile(sd_card_t *sd, int zoom, int tile_x, int tile_y, uint8_t *buffer);

#endif  // MAP_PARSER_H
