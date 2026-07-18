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

void vTaskLCD(void *pvParameters)
{
    //Test: Cycle through colors
    uint32_t colors[] = {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_YELLOW,
        COLOR_CYAN,
        COLOR_MAGENTA,
        COLOR_WHITE,
        COLOR_BLACK
    };

    const char *color_name[] = {
        "RED", "GREEN", "BLUE",
        "YELLOW", "CYAN", "MAGENTA",
        "WHITE", "BLACK"
    };

    while(1)
    {
        for (int i = 0; i < (sizeof(colors) / sizeof(uint32_t)); i++)
        {
            printf("Filling screen: %s\n", color_name[i]);
            lcd_fill_screen(&lcd, colors[i]);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        lcd_fill_rect(&lcd, 50, 50, 320 - 50, 480 - 50, COLOR_MAGENTA);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_fill_screen(&lcd, colors[7]);


        lcd_draw_pixel(&lcd, 10, 300, COLOR_GREEN);
        lcd_draw_pixel(&lcd, 11, 300, COLOR_GREEN);
        lcd_draw_pixel(&lcd, 10, 301, COLOR_GREEN);
        lcd_draw_pixel(&lcd, 11, 301, COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        lcd_draw_pixel(&lcd, 160, 240, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_draw_pixel(&lcd, 161, 240, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_draw_pixel(&lcd, 160, 241, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd_draw_pixel(&lcd, 161, 241, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));

        lcd_draw_hline(&lcd, 160/2, 240, 160, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));

        lcd_draw_vline(&lcd, 160, 240/2, 240, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(2000));

        lcd_draw_rect(&lcd, 10, 10, 310, 470, COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void vTaskSDCard(void *pvParameters)
{
    uint8_t buffer[512];
    while(1)
    {
        if (sd_card_is_mounted(&sd_card))
        {
            int len = sd_card_read_file(&sd_card, "/sdcard/Nihao.txt", buffer, sizeof(buffer) - 1);
            if (len > 0)
            {
                buffer[len] = '\0';
                ESP_LOGI(TAG, "File content: %s", buffer);
            }
            sd_card_list_dir(&sd_card, "/sdcard");
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

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

void vTaskGPS(void *pvParameters)
{
    gps_data_t gps_data;
    while (1)
    {
        if (gps_driver_get_data(&gps_data))
        {
        ESP_LOGI(TAG, "GPS: lat = %.6f lon = %.6f speed = %.1fkm/h heading = %.1f° sats = %d fix %s",
        gps_data.latitude, gps_data.longitude,
        gps_data.speed_kmh, gps_data.heading,
        gps_data.satellites, gps_data.fix_valid ? "YES" : "NO");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update GPS Log after 1 second
    }
}

void vTaskCompass(void *pvParameters)
{
    compass_data_t compass_data;
    while (1)
    {
        if (compass_driver_get_data(&compass_data))
        {
            ESP_LOGI(TAG, "Compass: heading = %.1f° raw = (%d, %d, %d) cal = %s",
            compass_data.heading, compass_data.raw_x, compass_data.raw_y, compass_data.raw_z,
            compass_data.data_uploaded ? "YES" : "NO");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTaskMap(void *pvParameters)
{
    gps_data_t gps_data;
    while (1)
    {
        // Only render if GPS is fixed
        if (gps_driver_get_data(&gps_data) && gps_driver_has_fix())
        {
            map_render(&map, gps_data.latitude, gps_data.longitude);
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

    sd_card_init(&sd_card, SPI2_HOST);
    ESP_LOGI(TAG, "SD Card initialized!");

    for (int i = 0; i < BUTTON_MAX; i++)
    {
        button_driver_init(&button[i], button_pins[i], NULL, 0, 0);
    }
    
    err = gps_driver_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPS initialized failed");
    }
    else
    {
        ESP_LOGI(TAG, "GPS initialized!");
    }

    err = compass_driver_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Compass initialized failed");
    }
    else
    {
        ESP_LOGI(TAG, "Compass initialized!");
    }

    err = map_renderer_init(&map, &lcd, &sd_card, 15);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Map renderer init failed: %s", esp_err_to_name(err));
    }

    xTaskCreate(vTaskSDCard, "SD Card Task", 4096 * 2, NULL, 2, NULL);
    xTaskCreate(vTaskButton, "Button Task", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(vTaskGPS, "GPS Task", 4096 * 2, NULL, 4, NULL);
    xTaskCreate(vTaskCompass, "Compass Task", 4096 * 2, NULL, 4, NULL);
    xTaskCreate(vTaskMap, "Map task", 4096 * 2, NULL, 5, NULL);
}
