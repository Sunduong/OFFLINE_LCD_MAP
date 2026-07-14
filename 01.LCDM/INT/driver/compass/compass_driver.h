#ifndef COMPASS_DRIVER_H
#define COMPASS_DRIVER_H

#include <driver/i2c_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define COMPASS_I2C_NUM    I2C_NUM_0
#define COMPASS_I2C_SDA    21   // GPIO_NUM_21
#define COMPASS_I2C_SCL    22   // GPIO_NUM_22
#define COMPASS_I2C_FREQ   100000
#define COMPASS_I2C_ADDR   0x0D
#define I2C_TIMEOUT_MS     1000

typedef struct {
    float heading;

    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;

    float calibrated_x;
    float calibrated_y;
    float calibrated_z;

    bool data_uploaded;
} compass_data_t;

esp_err_t compass_read_raw(int16_t *x, int16_t *y, int16_t *z);
esp_err_t compass_driver_init(void);
void compass_driver_deinit(void);
bool compass_driver_get_data(compass_data_t *data);
float compass_driver_get_heading(void);

#endif // COMPASS_DRIVER_H
