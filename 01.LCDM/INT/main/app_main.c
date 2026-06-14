#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_driver.h" 

void app_main(void)
{
    lcd_t lcd;

    lcd_init(&lcd, SPI2_HOST);
    printf("LCD initialized! Starting color test...\n");

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
    // lcd_fill_screen(&lcd, colors[0]);
    while(1)
    {
        for (int i = 0; i < (sizeof(colors) / sizeof(uint32_t)); i++)
        {
            printf("Filling screen: %s\n", color_name[i]);
            lcd_fill_screen(&lcd, colors[i]);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
    }
}

