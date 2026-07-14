#include "calibration.h"


static const char *TAG = "CALIBRATION";

#define CALIBRATION_DURATION_MS 15000 // 15 seconds

// void calibration_reset(compass_calibration_t *cal)
// {
//     cal->offset_x = 0.0f;
//     cal->offset_y = 0.0f;
//     cal->offset_z = 0.0f;

//     cal->scale_x = 1.0f;
//     cal->scale_y = 1.0f;
//     cal->scale_z = 1.0f;

//     cal->calibrated = false;
// }

void calibration_apply(const compass_calibration_t *calib, int16_t raw_x, int16_t raw_y, int16_t raw_z, 
                        float *calibrated_x, float *calibrated_y, float *calibrated_z)
{
    if (!calib->calibrated) {
        *calibrated_x = (float)raw_x;
        *calibrated_y = (float)raw_y;
        *calibrated_z = (float)raw_z;
        return;
    }
    
    *calibrated_x = (raw_x - calib->offset_x) / calib->scale_x;
    *calibrated_y = (raw_y - calib->offset_y) / calib->scale_y;
    *calibrated_z = (raw_z - calib->offset_z) / calib->scale_z;
}

float calibration_calculate_heading(float calibrated_x, float calibrated_y)
{
    // atan2 returns radians: -π to +π
    float heading_rad = atan2f(calibrated_y, calibrated_x);
    // convert to degrees: -180 to +180
    float heading_deg = heading_rad * 180.0f / M_PI;
    // Normalize to 0-360
    if (heading_deg < 0)
    {
        heading_deg += 360.0f;
    }
    return heading_deg;
}
