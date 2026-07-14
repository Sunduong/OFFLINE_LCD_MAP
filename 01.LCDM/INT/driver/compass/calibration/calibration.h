#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float offset_x;
    float offset_y;
    float offset_z;

    float scale_x;
    float scale_y;
    float scale_z;

    bool calibrated;
} compass_calibration_t;

void calibration_apply(const compass_calibration_t *calib, int16_t raw_x, int16_t raw_y, int16_t raw_z, 
                        float *calibrated_x, float *calibrated_y, float *calibrated_z);
float calibration_calculate_heading(float calibrated_x, float calibrated_y);
#endif // CALIBRATION_H
