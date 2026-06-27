#include "driver/spi_master.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "spi_bus_manager.h"

// ──────────────────────────── SD Card Device Structure ────────────────────────────
typedef struct {
    spi_device_handle_t spi;    //SPI device handle for SD card
    sdmmc_card_t *card;         //SD card info
    bool mounted;               //Is filesystem mounted?
} sd_card_t;

void sd_card_init(sd_card_t *sd, spi_host_device_t host);
