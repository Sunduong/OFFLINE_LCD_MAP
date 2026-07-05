#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"

typedef enum {
    BUTTON_UP = 0,
    BUTTON_DOWN,
    BUTTON_SELECT,
    BUTTON_BACK,
    BUTTON_MAX,
} button_id_t;

extern const gpio_num_t button_pins[];

#endif //BOARD_CONFIG_H
