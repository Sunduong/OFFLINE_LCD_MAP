#include "sd_card_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include "spi_bus_manager.h"

static const char *TAG = "SD_CARD_DRIVER";

static uint8_t sd_card_spi_transfer_byte(sd_card_t *sd, uint8_t data)
{
    uint8_t rx = 0xFF;

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = &rx,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(sd->spi, &t));
    return rx;
}

static uint8_t sd_card_send_cmd(sd_card_t *sd, uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t response = 0xFF;
    sd_card_spi_transfer_byte(sd, 0xFF);   // one idle byte
    
    sd_card_spi_transfer_byte(sd, 0x40 | cmd);
    sd_card_spi_transfer_byte(sd, (arg >> 24) & 0xFF);
    sd_card_spi_transfer_byte(sd, (arg >> 16) & 0xFF);
    sd_card_spi_transfer_byte(sd, (arg >> 8) & 0xFF);
    sd_card_spi_transfer_byte(sd, arg & 0xFF);
    sd_card_spi_transfer_byte(sd, crc);

    // Wait for a valid response
    for (int i = 0; i < 8; i++) {
        response = sd_card_spi_transfer_byte(sd, 0xFF);
        ESP_LOGI(TAG, "Response[%d] = 0x%02X", i, response);
        if (response != 0xFF) {
            break;
        }
    }
    return response;
}

void sd_card_init(sd_card_t *sd, spi_host_device_t host)
{
    sd->mounted = false;
    ESP_LOGI(TAG, "Initializing SD card on SPI bus...");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 400 * 1000, //400Khz
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &sd->spi));

    // send 80 clock cycles while SD CS is still High level. This is the required "wake up/prepare" step before CMD0 
    spi_bus_all_cs_high();
    // spi_bus_select_sd();
    for (int i = 0; i < 10; i++)
    {
        uint8_t b = sd_card_spi_transfer_byte(sd, 0xFF);
        ESP_LOGI(TAG, "Idle[%d] = 0x%02X", i, b);
    }
    ESP_LOGI(TAG, "SD card device added to SPI bus...");

    spi_bus_select_sd();
    uint8_t r1 = sd_card_send_cmd(sd, SD_CMD0, 0x00000000, SD_CMD0_CRC);
    spi_bus_all_cs_high();
    sd_card_spi_transfer_byte(sd, 0xFF);
    ESP_LOGI(TAG, "CMD0 response: 0x%02X", r1);
}






// // Online C compiler to run C program online
// #include <stdio.h>

// int compare(const void *a, const void *b)
// {
//     return (*(int*) a - *(int*) b);
// }

// int solution(int *A, int A_length, int *B, int B_length)
// {
//     int n = A_length;
//     int m = B_length;
//     qsort(A, n, sizeof(int), compare);
//     qsort(B, m, sizeof(int), compare);
//     int i = 0;
//     int k;
//     for (k = 0; k < n; k++)
//     {
//         // if (i < m - 1 && B[i] < A[k]) 
//         for (i; i < m; i++)
//         {
//             if (A[k] == B[i])
//                 return A[k];  
//         }
//     }
//     return -1;
// }

// int main() {
//     // Write C code here
//     printf("Start small. Ship something.");

//     return 0;
// }