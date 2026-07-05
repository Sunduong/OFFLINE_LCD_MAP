#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>

// Default button configuration
#define BUTTON_UP_PIN       GPIO_NUM_35
#define BUTTON_DOWN_PIN     GPIO_NUM_36
#define BUTTON_SELECT_PIN   GPIO_NUM_37
#define BUTTON_BACK_PIN     GPIO_NUM_38

#define BUTTON_DEBOUNCE_MS      30U
#define BUTTON_LONG_PRESS_MS    1000U

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
} button_event_t;

typedef struct {
    bool pressed;
    TickType_t tick;
} button_raw_event_t;

typedef struct {
    gpio_num_t pin;
    uint32_t debounce_ms;
    uint32_t long_press_ms;

    QueueHandle_t raw_queue;
    QueueHandle_t event_queue;
    TaskHandle_t task_handle;

    bool initialized;
    bool owns_event_queue;
    bool is_pressed;
    TickType_t last_transition_tick;
} button_driver_t;

esp_err_t button_driver_init(button_driver_t *btn, gpio_num_t pin, QueueHandle_t event_queue, uint32_t debounce_ms, uint32_t long_press_ms);
void button_driver_deinit(button_driver_t *btn);
bool button_driver_is_pressed(const button_driver_t *btn);
bool button_driver_get_event(button_driver_t *btn, button_event_t *event, TickType_t timeout);

#endif //BUTTON_DRIVER_H
