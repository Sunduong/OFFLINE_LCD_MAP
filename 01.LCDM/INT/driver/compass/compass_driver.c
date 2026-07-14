#include "compass_driver.h"
#include "calibration.h"

static const char *TAG = "COMPASS_DRIVER";

static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;
static i2c_master_dev_handle_t s_i2c_dev_handle = NULL;

static compass_data_t s_compass_data;
static SemaphoreHandle_t s_data_mutex;
static TaskHandle_t s_compass_task_handle;

static compass_calibration_t s_compass_calibration_data = {
    .offset_x = 0.0f,
    .offset_y = 0.0f,
    .offset_z = 0.0f,
    .scale_x = 1.0f,
    .scale_y = 1.0f,
    .scale_z = 1.0f,
    .calibrated = false
};

#define COMPASS_REG_DATA_OUT_X_LSB   0x00
#define COMPASS_REG_STATUS           0x06
#define COMPASS_REG_CONTROL_1        0x09
#define COMPASS_REG_CONTROL_2        0x0A
#define COMPASS_REG_SET_RESET        0x0B
#define MAGNETIC_DECLINATION         0.5f   // Vietnam ~0.5° East

static void compass_task(void *arg);

/* 
Register                Address         Mô tả
Data Output X LSB       0x00            X data low byte
Data Output X MSB       0x01            X data high byte
Data Output Y LSB       0x02            Y data low byte
Data Output Y MSB       0x03            Y data high byte
Data Output Z LSB       0x04            Z data low byte
Data Output Z MSB       0x05            Z data high byte
Status                  0x06            DRDY, OVL, DOR flags
Temperature LSB         0x07            Temperature low byte
Temperature MSB         0x08            Temperature high byte
Control Register 1      0x09            Mode, ODR, RNG, OSR
Control Register 2      0x0A            Soft reset, ROL_PNT
Set/Reset Period        0x0B            Default 0x01
 */
static esp_err_t compass_i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
    .i2c_port = COMPASS_I2C_NUM,
    .sda_io_num = COMPASS_I2C_SDA,
    .scl_io_num = COMPASS_I2C_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &s_i2c_bus_handle);
    if (err != ESP_OK) {
        return err;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = COMPASS_I2C_ADDR,
        .scl_speed_hz = COMPASS_I2C_FREQ,
    };
    err = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_config, &s_i2c_dev_handle);

    return err;
}

static esp_err_t compass_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_TIMEOUT_MS);
}

static esp_err_t compass_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_TIMEOUT_MS);
}

// QMC5883L Init Sequence: Power ON → Soft Reset → Set/Reset Period → Config Control Reg 1 → Config Control Reg 2 → Ready
static esp_err_t compass_init(void)
{
    esp_err_t err;

    // Soft Reset
    err = compass_register_write_byte(s_i2c_dev_handle, COMPASS_REG_CONTROL_2, 0x80);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    err = compass_register_write_byte(s_i2c_dev_handle, COMPASS_REG_SET_RESET, 0x01);
    if (err != ESP_OK) {
        return err;
    }

    err = compass_register_write_byte(s_i2c_dev_handle, COMPASS_REG_CONTROL_1, 0x11);
    if (err != ESP_OK) {
        return err;
    }

    err = compass_register_write_byte(s_i2c_dev_handle, COMPASS_REG_CONTROL_2, 0x40);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

static esp_err_t compass_detect(void)
{
    // Read identification register by trying to read any register and check received ACK
    uint8_t status;
    esp_err_t err = compass_register_read(s_i2c_dev_handle, COMPASS_REG_STATUS, &status, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "QMC5883L is not found at address 0x%02X", COMPASS_I2C_ADDR);
        return err;
    }

    ESP_LOGI(TAG, "QMC5883L is detected at address 0x%02X", COMPASS_I2C_ADDR);
    return ESP_OK;
}

esp_err_t compass_read_raw(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t data[6];
    esp_err_t err = compass_register_read(s_i2c_dev_handle, COMPASS_REG_DATA_OUT_X_LSB, data, sizeof(data));
    if (err != ESP_OK) {
        return err;
    }

    *x = (int16_t)((data[1] << 8) | data[0]);
    *y = (int16_t)((data[3] << 8) | data[2]);
    *z = (int16_t)((data[5] << 8) | data[4]);

    return ESP_OK;
}

static bool compass_data_ready(void)
{
    uint8_t status;
    esp_err_t err = compass_register_read(s_i2c_dev_handle, 0x06, &status, 1);
    if (err != ESP_OK) return false;
    return (status & 0x01);
}

static void compass_task(void *arg)
{
    while (1)
    {
        // Wait 100ms (10Hz update rate)
        vTaskDelay(pdMS_TO_TICKS(100));

        // Check if data is ready
        if (!compass_data_ready())
        {
            continue; // Skip this iteration if data is not ready
        }

        // Read raw data
        int16_t raw_x, raw_y, raw_z;
        if (compass_read_raw(&raw_x, &raw_y, &raw_z) != ESP_OK)
        {
            continue; // Skip this iteration if read fails
        }

        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
        s_compass_data.raw_x = raw_x;
        s_compass_data.raw_y = raw_y;
        s_compass_data.raw_z = raw_z;
        calibration_apply(&s_compass_calibration_data, s_compass_data.raw_x, s_compass_data.raw_y, s_compass_data.raw_z,
                          &s_compass_data.calibrated_x, &s_compass_data.calibrated_y, &s_compass_data.calibrated_z);
        s_compass_data.heading = calibration_calculate_heading(s_compass_data.calibrated_x, s_compass_data. calibrated_y) + MAGNETIC_DECLINATION;
        if (s_compass_data.heading >= 360.0f)
        {
            s_compass_data.heading -= 360.0f;
        }
        s_compass_data.data_uploaded = true;
        xSemaphoreGive(s_data_mutex);
    }
}

esp_err_t compass_driver_init(void)
{
    // 1. Create mutex for thread-safe data access
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create data mutex");
        return ESP_ERR_NO_MEM;
    }

    // 2. Initialize I2C
    esp_err_t err = compass_i2c_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return err;
    }

    // 3. QMC5883L detect + init
    err = compass_detect();
    if (err != ESP_OK) return err;
    
    err = compass_init();
    if (err != ESP_OK) return err;

    // 4. Load Calibration from NVS (dont need to calib after each booting time)
    // compass_driver_load_calibration();

    // 5. Create Compass task
    xTaskCreate(compass_task, "compass_task", 4096, NULL, 5, &s_compass_task_handle);
    ESP_LOGI(TAG, "Compass driver initialized successfully");
    return ESP_OK;
}

void compass_driver_deinit(void)
{
    if (s_compass_task_handle != NULL)
    {
        vTaskDelete(s_compass_task_handle);
        s_compass_task_handle = NULL;
    }

     // Remove device + delete bus (new API)
    if (s_i2c_dev_handle) {
        i2c_master_bus_rm_device(s_i2c_dev_handle);
        s_i2c_dev_handle = NULL;
    }
    if (s_i2c_bus_handle) {
        i2c_del_master_bus(s_i2c_bus_handle);
        s_i2c_bus_handle = NULL;
    }

    if (s_data_mutex != NULL)
    {
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
    }
}

bool compass_driver_get_data(compass_data_t *data)
{
    if (data == NULL || s_data_mutex == NULL)
    {
        return false;
    }

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    *data = s_compass_data;
    bool updated = s_compass_data.data_uploaded;
    s_compass_data.data_uploaded = false; // Reset the flag after reading
    xSemaphoreGive(s_data_mutex);
    return updated;
}

float compass_driver_get_heading(void)
{
    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    float heading = s_compass_data.heading;
    xSemaphoreGive(s_data_mutex);
    return heading;
}