#include "spi_bus_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SPI_BUS_MANAGER";

void spi_bus_init(spi_host_device_t host)
{
    ESP_LOGI(TAG, "Initializing shared SPI bus...");

    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN, // MISO required for SD card reads!
        .sclk_io_num = SPI_SCK_PIN,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized (MOSI=%d, MISO=%d, SCK=%d)",
    SPI_MOSI_PIN, SPI_MISO_PIN, SPI_SCK_PIN);
}

void spi_bus_all_cs_high(void)
{
    gpio_set_level(SPI_LCD_CS_PIN, 1);
    gpio_set_level(SPI_SD_CS_PIN, 1);
}

void spi_bus_select_lcd(void)
{
    gpio_set_level(SPI_LCD_CS_PIN, 0);
    gpio_set_level(SPI_SD_CS_PIN, 1);
}

void spi_bus_select_sd(void)
{
    gpio_set_level(SPI_LCD_CS_PIN, 1);
    gpio_set_level(SPI_SD_CS_PIN, 0);
}
