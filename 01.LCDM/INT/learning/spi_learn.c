#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

#define PIN_MISO 12
#define PIN_MOSI 11
#define PIN_SCK  10
#define PIN_CS   9


spi_device_handle_t spi_dev;

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(5000)); // For IDF MONITOR to connect before printing logs
    // ===== SETUP =====
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCK,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE, // Max transfer size in bytes
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO)); // Initialize SPI bus pins, Master IO, Auto DMA channel
    printf("SPI bus is initialized...\n");

    //Add SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,          // Clock out at 1 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = PIN_CS,             // CS pin
        .queue_size = 1,                    
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev));
    printf("SPI device is added...\n");


    // ===== TEST =====
    uint8_t send_data = 0xA5;
    uint8_t recv_data = 0x00;
    spi_transaction_t transaction1 = {
        .length = 8,
        .tx_buffer = &send_data,
        .rx_buffer = &recv_data,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_dev, &transaction1));

    if (send_data == recv_data) {
        printf("TX = 0x%02X\n", send_data);
        printf("RX = 0x%02X\n", recv_data);
        printf("✅ SUCCESS! SPI loopback works!\n");
    } else {
        printf("TX = 0x%02X\n", send_data);
        printf("RX = 0x%02X\n", recv_data);
        printf("❌ FAIL! Data mismatch. Check MOSI-MISO jumper wire.\n");
    }

    while(1)
    {
        printf("alive\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

