#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* ==================== Configuration ==================== */
#define GPS_UART_NUM        UART_NUM_1
#define GPS_UART_BUF        1024
#define GPS_BAUD_RATE       38400
#define GPS_NMEA_MAX_LEN    128 // Max NMEA sentence length (NMEA 0183 spec = 82 chars)
#define GPS_TASK_STACK      4096
#define GPS_TASK_PRIORITY   3
#define GPS_READ_TIMEOUT_MS 100

/**
 * @brief GPS data structure — holds all parsed information from NMEA sentences.
 *
 * Filled by parsing $GPRMC (position, speed, heading, time, fix status)
 * and $GPGGA (satellites, altitude, HDOP, fix quality).
 */
typedef struct {
    /* --- Position --- */
    double latitude;   // Decimal degrees
    double longitude;  // Decimal degrees

    /* --- Movement --- */
    float speed_kmh;   // Speed over ground in km/h
    float heading;     // Course over ground in degrees (0-359.99)

    /* --- Time & Date --- */
    int hour;          // UTC hours (0-23)
    int minute;        // UTC minutes (0-59)
    int second;        // UTC seconds (0-59)
    int day;           // Day (1-31)
    int month;         // Month (1-12)
    int year;          // Year (e.g. 2025)

    /* --- Fix Quality --- */
    bool fix_valid;    // true = active fix (RMC status 'A'), false = void ('V')
    int satellites;    // Number of satellites in use (from $GPGGA)
    float hdop;        // Horizontal dilution of precision (from $GPGGA, lower = better)
    float altitude;    // Altitude above mean sea level in meters (from $GPGGA)

    /* --- Internal state --- */
    bool data_updated; // true when new data was parsed since last read
} gps_data_t;

esp_err_t gps_driver_init(void);

/**
 * @brief Deinitialize GPS driver — stop task, remove UART driver.
 */
void gps_driver_deinit(void);

/**
 * @brief Get the latest GPS data (thread-safe).
 *
 * Copies the internal gps_data_t to the caller's struct.
 * Use this from other tasks (LCD, SD card, etc.) to read GPS info.
 *
 * @param data Pointer to caller's gps_data_t to fill.
 * @return true if data was updated since last read, false if no new data.
 */
bool gps_driver_get_data(gps_data_t *data);

/**
 * @brief Check if GPS has a valid fix.
 *
 * @return true if fix is valid (RMC status = 'A').
 */
bool gps_driver_has_fix(void);

#endif // GPS_DRIVER_H
