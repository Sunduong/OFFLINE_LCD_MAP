#include "sd_card_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include <string.h>

static const char *TAG = "SD_CARD_DRIVER";

void sd_card_init(sd_card_t *sd, spi_host_device_t host)
{
    sd->mounted = false;
    ESP_LOGI(TAG, "Initializing SD card on SPI bus...");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, //20Mhz
        .mode = 0,
        .spics_io_num = SPI_SD_CS_PIN,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &sd->spi));
    ESP_LOGI(TAG, "SD card device added to SPI bus...");
}
