#ifndef MAP_CACHE_H
#define MAP_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "map_define.h"

#define MAP_CACHE_CAPACITY 9U

// Identify exactly the tile
typedef struct {
    int zoom;
    int tile_x;
    int tile_y;
} map_tile_key_t;

// Cache status and the place cache is storage
typedef struct {
    map_tile_key_t key;
    uint8_t *pixels;
    uint32_t last_used; // used this for Least Recently Used (LRU) algorithm
    bool valid;
    bool pinned;
} map_cache_entry_t;

/* typedef struct {
 // Do later 
} map_cache_stats_t; */

typedef struct {
    uint8_t *storage;
    size_t storage_size;
    map_cache_entry_t entries[MAP_CACHE_CAPACITY];
    uint32_t access_clock; // used this for Least Recently Used (LRU) algorithm
} map_cache_t;

esp_err_t map_cache_init(map_cache_t *cache);
void map_cache_deinit(map_cache_t *cache);

map_cache_entry_t *map_cache_find(map_cache_t *cache, const map_tile_key_t *key);

#endif // MAP_CACHE_H
