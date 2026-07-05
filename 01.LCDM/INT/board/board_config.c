#include "board_config.h"

const gpio_num_t button_pins[BUTTON_MAX] = {
    GPIO_NUM_35, // BUTTON_UP_PIN
    GPIO_NUM_36, // BUTTON_DOWN_PIN
    GPIO_NUM_37, // BUTTON_SELECT_PIN
    GPIO_NUM_38  // BUTTON_BACK_PIN
};
