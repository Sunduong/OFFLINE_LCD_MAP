#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(3000)); // For IDF MONITOR to connect before printing logs

    while(1)
    {
        printf("alive\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

