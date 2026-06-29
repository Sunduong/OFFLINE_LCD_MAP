#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h"
#include "sd_card_driver.h"
#include "spi_bus_manager.h"

lcd_t lcd;
sd_card_t sd_card;

void app_main(void)
{
    spi_bus_init(SPI2_HOST); // Initialize the shared SPI bus for both LCD and SD card
    lcd_init(&lcd, SPI2_HOST);
    sd_card_init(&sd_card, SPI2_HOST);

    printf("LCD initialized! Starting color test...\n");
    printf("SD Card initialized!\n");

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

