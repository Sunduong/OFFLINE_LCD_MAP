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
    for (int i = 0; i < 10; i++)
    {
        sd_card_spi_transfer_byte(sd, 0xFF);
    }
    ESP_LOGI(TAG, "SD card device added to SPI bus...");
}


