#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#if CONFIG_SPIRAM
    #include "esp_psram.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "sd_card_driver.h"
#include "spi_bus_manager.h"
#include "button_driver.h"
#include "board_config.h"
#include "gps_driver.h"
#include "compass_driver.h"
#include "map_renderer.h"
#include "map_cache.h"

static const char *TAG = "APP_MAIN";

/*
1. Init cache
2. Tự đặt entries[3].key
3. Đặt entries[3].valid = true
4. Gọi find với key giống → phải trả về &entries[3]
5. Gọi find với key khác → phải trả về NULL
6. Deinit
7. Kiểm tra storage == NULL 
*/

void app_main(void)
{
    map_cache_t cache = {0};
    assert(map_cache_init(&cache) == ESP_OK);

    map_tile_key_t key = {
        .zoom = 16,
        .tile_x = 52195,
        .tile_y = 30779,
    };

    cache.entries[3].key = key;
    cache.entries[3].valid = true;

    map_cache_entry_t *find_entry = map_cache_find(&cache, &key);
    if (find_entry == &cache.entries[3])
    {
        ESP_LOGI(TAG, "Test 1: map_cache_find API works");
    }

    map_tile_key_t key2 = {
        .zoom = 17,
        .tile_x = 52195,
        .tile_y = 30779,
    };
    find_entry = map_cache_find(&cache, &key2);
    if (find_entry == NULL)
    {
        ESP_LOGI(TAG, "Test 2: map_cache_find API works");
    }

    map_cache_deinit(&cache);
    if (cache.storage == NULL)
    {
        ESP_LOGI(TAG, "Testt 3: map_cache_deinit API works");
    }
}