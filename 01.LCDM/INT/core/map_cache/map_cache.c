#include "map_cache.h"
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "MAP_CACHE";

static bool map_cache_is_tile_key_equal(const map_tile_key_t *tileA, const map_tile_key_t *tileB)
{
    return ((tileA->zoom == tileB->zoom) && (tileA->tile_x == tileB->tile_x) && (tileA->tile_y == tileB->tile_y));
} 

/**
 * @brief Allocate the map cache on PSRAM
 * Reset metadata
 * Allocates the 1.6875 MiB on PSRAM for 9 tile slots
 * @param cache Map Cache
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer allocation fails
 */
esp_err_t map_cache_init(map_cache_t *cache)
{
    if (cache == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(cache, 0, sizeof(*cache)); // Reset struct, if call map_cache_init() at the second time, 1.6875 MiB is leaked !!!
    cache->storage_size = MAP_CACHE_CAPACITY * TILE_BYTES_RGB666;
    cache->storage = heap_caps_malloc(cache->storage_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (cache->storage == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes in PSRAM", cache->storage_size);
        memset(cache, 0, sizeof(*cache));
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "PSRAM DMA allocate OK at %p", cache->storage);

    for (size_t i = 0; i < MAP_CACHE_CAPACITY; i++)
    {
        cache->entries[i].pixels = cache->storage + i * TILE_BYTES_RGB666;
        cache->entries[i].valid = false;
        cache->entries[i].pinned = false;
        cache->entries[i].last_used = 0;
    }

    return ESP_OK;
}

/**
 * @brief Free the storage on PSRAM
 * Free space for the 1.6875 MiB on PSRAM for 9 tile slots
 * @param cache Map Cache
 */
void map_cache_deinit(map_cache_t *cache)
{
    if ((cache == NULL) || (cache->storage == NULL))
        return;

    heap_caps_free(cache->storage);
    memset(cache, 0, sizeof(*cache));
}

/**
 * @brief map_cache_find
 * Find exactly the map tile in 9 tiles map cache
 * @param cache Map Cache
 * @param key map tile key need to find
 * @return pointer to map_cache_entry_t datatype
 */
map_cache_entry_t *map_cache_find(map_cache_t *cache, const map_tile_key_t *key)
{
    if ((cache == NULL) || (key == NULL))
    {
        return NULL;
    }

    for (size_t i = 0; i < MAP_CACHE_CAPACITY; i++)
    {
        map_cache_entry_t *entry = &cache->entries[i];
        if (!entry->valid)
            continue;

        if (!map_cache_is_tile_key_equal(&entry->key, key))
            continue;
        entry->last_used = ++cache->access_clock;
        return entry;
    }
    return NULL;
}
