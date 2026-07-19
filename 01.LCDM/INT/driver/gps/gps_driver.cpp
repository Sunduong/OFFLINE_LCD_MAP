#include "gps_driver.h"
#include <TinyGPS++.h>

static const char *TAG = "GPS_DRIVER";

/* ==================== Internal State ==================== */
static TinyGPSPlus          s_gps;              // TinyGPSPlus object (parser)
static gps_data_t           s_gps_data;         // Internal GPS data (written by GPS task)
static SemaphoreHandle_t    s_data_mutex;       // Protects s_gps_data
static QueueHandle_t        s_uart_event_queue;  // UART event queue
static TaskHandle_t         s_gps_task_handle;

/* ==================== Private: UART ==================== */

/**
 * @brief Configure and install UART driver for GPS module.
 *
 * Order: param_config → set_pin → driver_install
 * (driver_install must be LAST, after config and pins are set)
 */
static esp_err_t gps_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = 0,
    };

    esp_err_t err = uart_param_config(GPS_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_set_pin(GPS_UART_NUM, GPIO_NUM_48, GPIO_NUM_47, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_driver_install(GPS_UART_NUM, GPS_UART_BUF * 2, GPS_UART_BUF * 2, 20, &s_uart_event_queue, 0);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "UART initialized: baud=%d, TX=GPIO48, RX=GPIO47", GPS_BAUD_RATE);
    return ESP_OK;
}

/* ==================== Private: GPS Task ==================== */

/**
 * @brief GPS FreeRTOS task — reads and parses NMEA continuously.
 *
 * Flow:
 * 1. Wait for UART event (if BLOCKED - task sleep)
 * 2. If UART_DATA -> read bytes from FIFO
 * 3. Feed each byte into TinyGPSPlus (auto parse + checksum)
 * 4. If data is updated -> take mutex, copy to s_gps_data, release mutex
 * 5. If UART_FIFO_OVF / UART_BUFFER_FULL → flush + reset queue
 * 6. Loop
 */
static void gps_task(void *arg)
{
    uart_event_t event;
    uint8_t data[256];
    // char buf[257];

    while (1) {
        if (xQueueReceive(s_uart_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
            case UART_DATA: {
                int len = uart_read_bytes(GPS_UART_NUM, data, event.size, pdMS_TO_TICKS(10));
                // if (len > 0)
                // {
                //     memcpy(buf, data, len);
                //     buf[len] = 0;

                //     ESP_LOGI(TAG, "%s", buf);
                // }

                if (len <= 0) {
                    break;
                }

                for (int i = 0; i < len; i++) {
                    s_gps.encode(data[i]);
                }

                if (s_gps.location.isUpdated() || s_gps.speed.isUpdated() ||
                    s_gps.altitude.isUpdated() || s_gps.satellites.isUpdated()) {
                    xSemaphoreTake(s_data_mutex, portMAX_DELAY);

                    if (s_gps.location.isValid()) {
                        s_gps_data.latitude = s_gps.location.lat();
                        s_gps_data.longitude = s_gps.location.lng();
                        s_gps_data.fix_valid = true;
                    } else {
                        s_gps_data.fix_valid = false;
                    }

                    if (s_gps.speed.isValid()) {
                        s_gps_data.speed_kmh = s_gps.speed.kmph();
                    }

                    if (s_gps.course.isValid()) {
                        s_gps_data.heading = s_gps.course.deg();
                    }

                    if (s_gps.date.isValid()) {
                        s_gps_data.year = s_gps.date.year();
                        s_gps_data.month = s_gps.date.month();
                        s_gps_data.day = s_gps.date.day();
                    }
                    if (s_gps.time.isValid()) {
                        s_gps_data.hour = s_gps.time.hour();
                        s_gps_data.minute = s_gps.time.minute();
                        s_gps_data.second = s_gps.time.second();
                    }

                    if (s_gps.satellites.isValid()) {
                        s_gps_data.satellites = s_gps.satellites.value();
                    }

                    if (s_gps.hdop.isValid()) {
                        s_gps_data.hdop = s_gps.hdop.value();
                    }

                    if (s_gps.altitude.isValid()) {
                        s_gps_data.altitude = s_gps.altitude.meters();
                    }

                    s_gps_data.data_updated = true;
                    xSemaphoreGive(s_data_mutex);
                }
                break;
            }

            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "hw fifo overflow");
                uart_flush_input(GPS_UART_NUM);
                xQueueReset(s_uart_event_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "ring buffer full");
                uart_flush_input(GPS_UART_NUM);
                xQueueReset(s_uart_event_queue);
                break;

            case UART_FRAME_ERR:
                ESP_LOGW(TAG, "uart frame error");
                break;

            default:
                ESP_LOGW(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
}

esp_err_t gps_driver_init(void)
{
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create data mutex");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = gps_uart_init();
    if (err != ESP_OK) {
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
        return err;
    }

    memset(&s_gps_data, 0, sizeof(gps_data_t));

    BaseType_t task_created = xTaskCreate(gps_task, "gps_task", GPS_TASK_STACK, NULL, GPS_TASK_PRIORITY, &s_gps_task_handle);
    if (task_created != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create GPS task");
        uart_driver_delete(GPS_UART_NUM);
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "GPS driver initialized successfully");
    return ESP_OK;
}

void gps_driver_deinit(void)
{
    uart_driver_delete(GPS_UART_NUM);

    if (s_gps_task_handle != NULL) {
        vTaskDelete(s_gps_task_handle);
        s_gps_task_handle = NULL;
    }

    if (s_data_mutex != NULL) {
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
    }

    ESP_LOGI(TAG, "GPS driver deinitialized");
}

bool gps_driver_get_data(gps_data_t *data)
{
    if (data == NULL || s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to get GPS data");
        return false;
    }

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    memcpy(data, &s_gps_data, sizeof(gps_data_t));
    bool updated = s_gps_data.data_updated;
    s_gps_data.data_updated = false; // Clear flag after read
    xSemaphoreGive(s_data_mutex);

    return updated;
}

bool gps_driver_has_fix(void)
{
    if (s_data_mutex == NULL) {
        return false;
    }

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    bool fix = s_gps_data.fix_valid;
    xSemaphoreGive(s_data_mutex);

    return fix;
}
