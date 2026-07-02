#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "sd_card_driver.h"
#include "spi_bus_manager.h"

static const char *TAG = "APP_MAIN";

lcd_t lcd;
sd_card_t sd_card;

void app_main(void)
{
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card
    ESP_LOGI(TAG, "SPI Shared Bus initialized!");
    lcd_init(&lcd, SPI2_HOST);
    ESP_LOGI(TAG, "LCD initialized!");

    if (sd_card_is_mounted(&sd_card)) {
        ESP_LOGI(TAG, "SD card already mounted");
    } else {
        ESP_LOGI(TAG, "SD card not mounted, initializing...");
    }
    sd_card_init(&sd_card, SPI2_HOST);
    ESP_LOGI(TAG, "SD Card initialized!");

    uint8_t buffer[512];
    int len = sd_card_read_file(&sd_card, "/sdcard/Nihao.txt", buffer, sizeof(buffer));
    if (len > 0) 
    {
        for (int i = 0; i < len; i++)
        {
            ESP_LOGI(TAG, "Byte %d = 0x%02X ('%c)", 
                        i, 
                        buffer[i], 
                        (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.'
                    );
        }
    }

    sd_card_list_dir(&sd_card, "/sdcard");

}

