#include <stdio.h>
#include "esp_log.h"
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

static const char *TAG = "APP_MAIN";

lcd_t lcd;
sd_card_t sd_card;
button_driver_t button[BUTTON_MAX];
map_handle_t map;

static const char *button_name(button_id_t id)
{
    switch (id)
    {
        case BUTTON_UP:
            return "UP";
        case BUTTON_DOWN:
            return "DOWN";
        case BUTTON_SELECT:
            return "SELECT";
        case BUTTON_BACK:
            return "BACK";
        default:
            return "UNKNOWN";
    }
}

void vTaskButton(void *pvParameters)
{
    button_event_t event;

    while(1)
    {
        for (button_id_t id = BUTTON_UP; id < BUTTON_MAX; id++)
        {
            if (button_driver_get_event(&button[id], &event, 0))
            {
                const char *event_name = (event == BUTTON_EVENT_LONG_PRESS) ? "LONG PRESS" : "SHORT PRESS";
                ESP_LOGI(TAG, "Button %s - Type: %s", button_name(id), event_name);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// void vTaskGPS(void *pvParameters)
// {
//     gps_data_t gps_data;
//     while (1)
//     {
//         if (gps_driver_get_data(&gps_data))
//         {
//         ESP_LOGI(TAG, "GPS: lat = %.6f lon = %.6f speed = %.1fkm/h heading = %.1f° sats = %d fix %s",
//         gps_data.latitude, gps_data.longitude,
//         gps_data.speed_kmh, gps_data.heading,
//         gps_data.satellites, gps_data.fix_valid ? "YES" : "NO");
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000)); // Update GPS Log after 1 second
//     }
// }

// void vTaskCompass(void *pvParameters)
// {
//     compass_data_t compass_data;
//     while (1)
//     {
//         if (compass_driver_get_data(&compass_data))
//         {
//             ESP_LOGI(TAG, "Compass: heading = %.1f° raw = (%d, %d, %d) cal = %s",
//             compass_data.heading, compass_data.raw_x, compass_data.raw_y, compass_data.raw_z,
//             compass_data.data_uploaded ? "YES" : "NO");
//         }
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

void vTaskMap(void *pvParameters)
{
    // gps_data_t gps_data;
    while (1)
    {
        // Only render if GPS is fixed
        // if (gps_driver_get_data(&gps_data) && gps_driver_has_fix())
        {
            // map_render(&map, gps_data.latitude, gps_data.longitude);
            map_render(&map, 10.85629, 106.72055);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void)
{
    esp_err_t err;
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card
    ESP_LOGI(TAG, "SPI Shared Bus initialized!");

    lcd_init(&lcd, SPI2_HOST);
    ESP_LOGI(TAG, "LCD initialized!");

    // esp_log_level_set("sdmmc_cmd", ESP_LOG_VERBOSE);
    // esp_log_level_set("sdspi_host", ESP_LOG_VERBOSE);
    // esp_log_level_set("diskio_sdmmc", ESP_LOG_VERBOSE);
    sd_card_init(&sd_card, SPI2_HOST);
    ESP_LOGI(TAG, "SD Card initialized!");

    // for (int i = 0; i < BUTTON_MAX; i++)
    // {
    //     button_driver_init(&button[i], button_pins[i], NULL, 0, 0);
    // }
    
    // err = gps_driver_init();
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "GPS initialized failed");
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "GPS initialized!");
    // }

    // err = compass_driver_init();
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Compass initialized failed");
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Compass initialized!");
    // }

    err = map_renderer_init(&map, &lcd, &sd_card, 16);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Map renderer init failed: %s", esp_err_to_name(err));
    }
    
    // xTaskCreate(vTaskButton, "Button Task", 4096 * 2, NULL, 3, NULL);
    // xTaskCreate(vTaskGPS, "GPS Task", 4096 * 2, NULL, 4, NULL);
    // xTaskCreate(vTaskCompass, "Compass Task", 4096 * 2, NULL, 4, NULL);
    xTaskCreate(vTaskMap, "Map task", 4096 * 2, NULL, 5, NULL);
}
