#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

QueueHandle_t myQueue = NULL;

void producerTask(void *pvParameters)
{
    for (int i = 0; i < 10; i++)
    {
        printf("Sending: %d\n", i);
        if (xQueueSend(myQueue, &i, portMAX_DELAY) != pdPASS)
        {   
            printf("Failed to send to queue\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("Producer task ended\n");
    vTaskDelete(NULL);
}

void consumerTask(void *pvParameters)
{
    int receivedValue;
    for (int i = 0; i < 10; i++)
    {
        printf("[Consumer] Waiting for data...\n");
        if (xQueueReceive(myQueue, &receivedValue, portMAX_DELAY) == pdPASS)
        {
            printf("Received: %d\n", receivedValue);
        }
        else
        {
            printf("Failed to receive from queue\n");
        }
    }
    printf("Consumer task ended\n");
    vTaskDelete(NULL);
}

void app_main(void)
{
    //Create a queue to hold 5 items, item size: 4 bytes
    myQueue = xQueueCreate(5, sizeof(int));

    if (myQueue == NULL)
    {
        printf("Failed to create queue\n");
        return;
    }

    //Create producer task
    xTaskCreate(producerTask, "producer", 2048, NULL, 5, NULL);

    //Create consumer task
    xTaskCreate(consumerTask, "consumer", 2048, NULL, 5, NULL);
}