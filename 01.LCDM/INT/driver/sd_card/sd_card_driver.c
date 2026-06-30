#include "sd_card_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "SD_CARD_DRIVER";

// ──────────────────────────── Static Variables ────────────────────────────
// ⚠️ These are static because they are only used within this file.
//    We keep slot_config to reuse it during deinit (sdspi_host_remove_device).
static sdmmc_host_t             sd_host;
static sdspi_device_config_t    slot_config;
static const char               *mount_point = "/sdcard";

// ═══════════════════════════════════════════════════════
// PUBLIC: Init + Mount
// ═══════════════════════════════════════════════════════
/*
┌─────────────────────────────────────────────────────────────────┐
│  WHAT esp_vfs_fat_sdspi_mount() DOES INTERNALLY (5 steps):      │
│                                                                 │
│  Step 1: sdspi_host_init()                                      │
│          → Initialize SDSPI driver layer                        │
│                                                                 │
│  Step 2: sdspi_host_init_device(&slot_config)                   │
│          → ⚠️ Calls spi_bus_add_device() internally!            │
│          → This is why we do NOT call it ourselves!             │
│                                                                 │
│  Step 3: sdmmc_card_init(&host, &card)                          │
│          → Send CMD0, CMD8, ACMD41, CMD2, CMD9...               │
│          → Probe card, read CID/CSD, determine capacity         │
│                                                                 │
│  Step 4: esp_vfs_fat_register(mount_point, ...)                 │
│          → Register "/sdcard/" prefix into VFS                  │
│          → Allocate FAT drive number (pdrv)                     │
│                                                                 │
│  Step 5: ff_diskio_register_sdmmc() + f_mount()                 │
│          → Attach disk I/O functions (read_sector/write_sector) │
│          → Mount FAT: read BPB, FAT table, root directory       │
│                                                                 │
│  After mount succeeds → fopen("/sdcard/test.txt") works!        │
└─────────────────────────────────────────────────────────────────┘
*/
void sd_card_init(sd_card_t *sd, spi_host_device_t host)
{
    sd->mounted = false;
    sd->card    = NULL;

    ESP_LOGI(TAG, "Initializing SD card on shared SPI bus...");

    // ── Step 1: Configure SD host (SPI mode) ──
    // SDSPI_HOST_DEFAULT() sets up function pointers for SPI-based SD communication.
    // We override .slot to use the SAME SPI host as LCD (SPI2_HOST).
    sd_host = (sdmmc_host_t)SDSPI_HOST_DEFAULT();
    sd_host.slot = host;

    // ── Step 2: Configure SD slot (CS pin) ──
    // SDSPI_DEVICE_CONFIG_DEFAULT() gives default slot config.
    // We set gpio_cs to our SD CS pin and host_id to the shared bus.
    slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs  = SPI_SD_CS_PIN;   // GPIO13
    slot_config.host_id  = host;

    // ── Step 3: Configure FAT filesystem mount ──
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,   // ⚠️ NEVER format! Data will be lost!
        .max_files              = 5,        // Max 5 files open simultaneously
        .allocation_unit_size   = 16 * 1024,
    };

    // ── Step 4: Allocate card info structure ──
    // esp_vfs_fat_sdspi_mount() needs a pointer to fill in card info.
    // We malloc it so it persists beyond this function's stack.
    sdmmc_card_t *card = malloc(sizeof(sdmmc_card_t));
    if (card == NULL) {
        ESP_LOGE(TAG, "Failed to allocate card info structure!");
        return;
    }

    // ── Step 5: Mount! ──
    // This single call does all 5 internal steps (see comment block above).
    // It will add the SD device to the SPI bus, probe the card, and mount FAT.
    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &sd_host, &slot_config,
                                             &mount_config, card);

    if (ret == ESP_OK) {
        sd->mounted = true;
        sd->card    = card;
        ESP_LOGI(TAG, "SD card mounted at %s", mount_point);

        // Print SD card info (read from CSD register)
        ESP_LOGI(TAG, "SD Card Info:");
        ESP_LOGI(TAG, "  Type:   %s", card->is_sdio ? "SDIO" : "SPI");
        ESP_LOGI(TAG, "  Size:   %llu MB", ((uint64_t)card->csd.capacity) * 512 / (1024 * 1024));
        ESP_LOGI(TAG, "  Speed:  %d MHz", card->csd.tr_speed / 1000000);
        ESP_LOGI(TAG, "  RCA:    0x%08X", card->rca);

    } else if (ret == ESP_ERR_INVALID_STATE) {
        // Card was already mounted (e.g. from a previous init call)
        ESP_LOGW(TAG, "SD card already mounted");
        sd->mounted = true;
        sd->card    = card;

    } else {
        // Mount failed — print error and cleanup
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check: SD card inserted? Wiring correct? Format = FAT32?");
        free(card);  // Cleanup on failure
    }
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Deinit + Unmount
// ═══════════════════════════════════════════════════════
void sd_card_deinit(sd_card_t *sd)
{
    if (!sd->mounted) {
        return;
    }

    // 1. Unmount FAT filesystem from VFS
    esp_vfs_fat_sdcard_unmount(mount_point, sd->card);

    // 2. Remove SDSPI device from bus + deinit SDSPI host
    //    ⚠️ If we skip this, re-mounting will fail with ESP_ERR_INVALID_STATE
    sdspi_host_remove_device(&slot_config);
    sdspi_host_deinit();

    // 3. Free allocated card info
    free(sd->card);
    sd->card    = NULL;
    sd->mounted = false;

    ESP_LOGI(TAG, "SD card unmounted and cleaned up");
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Status Check
// ═══════════════════════════════════════════════════════
bool sd_card_is_mounted(sd_card_t *sd)
{
    return sd->mounted;
}

// ═══════════════════════════════════════════════════════
// PUBLIC: Read File
// ═══════════════════════════════════════════════════════
/*
┌─────────────────────────────────────────────────────────────────┐
│  DATA FLOW: fread() → ??? → SPI hardware                        │
│                                                                 │
│  fread()                                                        │
│    → VFS (redirect "/sdcard/" prefix to FatFS)                  │
│      → FatFS (f_read: find cluster chain, read directory)        │
│        → ff_diskio (disk_read: convert to sector read)           │
│          → SDSPI driver (send CMD17 via spi_device_polling)      │
│            → SPI Master driver (MOSI/MISO/SCK/CS hardware)       │
│              → SD card hardware                                  │
└─────────────────────────────────────────────────────────────────┘
*/
int sd_card_read_file(sd_card_t *sd, const char *path, uint8_t *buffer, size_t max_len)
{
    if (!sd->mounted) {
        ESP_LOGE(TAG, "SD card not mounted!");
        return -1;
    }

    // Open file in binary read mode
    // Path must start with /sdcard/ (e.g. "/sdcard/test.txt")
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return -1;
    }

    // Read data into buffer
    size_t bytes_read = fread(buffer, 1, max_len, f);
    fclose(f);

    ESP_LOGI(TAG, "Read %d bytes from %s", bytes_read, path);
    return (int)bytes_read;
}

// ═══════════════════════════════════════════════════════
// PUBLIC: List Directory
// ═══════════════════════════════════════════════════════
void sd_card_list_dir(sd_card_t *sd, const char *dir_path)
{
    if (!sd->mounted) {
        ESP_LOGE(TAG, "SD card not mounted!");
        return;
    }

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return;
    }

    ESP_LOGI(TAG, "Listing directory: %s", dir_path);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        ESP_LOGI(TAG, "  %s (%s)", entry->d_name,
                 entry->d_type == DT_DIR ? "DIR" : "FILE");
    }
    closedir(dir);
}
