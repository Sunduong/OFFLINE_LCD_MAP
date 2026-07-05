#include "button_driver.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stddef.h>
#include <string.h>

static const char *TAG = "BUTTON_DRIVER";

static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_driver_t *btn = (button_driver_t *) arg;
    button_raw_event_t raw_event;
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (btn == NULL || btn->raw_queue == NULL)
    {
        ESP_LOGE(TAG, "Button driver pointer or raw queue is NULL in ISR");
        return;
    }

    raw_event.pressed = (gpio_get_level(btn->pin) == 0); // Assuming active low button
    raw_event.tick = xTaskGetTickCountFromISR();

    xQueueSendFromISR(btn->raw_queue, &raw_event, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void button_task(void *arg)
{
    button_driver_t *btn = (button_driver_t *) arg;
    button_raw_event_t raw_event;

    while(1)
    {
        if (xQueueReceive(btn->raw_queue, &raw_event, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(20));

        bool current_level = (gpio_get_level(btn->pin) == 0);
        if (current_level != raw_event.pressed)
        {
            continue; // Ignore if the state has changed during debounce delay
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - btn->last_transition_tick) < pdMS_TO_TICKS(btn->debounce_ms))
        {
            continue; // Ignore if within debounce period
        }
        btn->last_transition_tick = now;

        if (raw_event.pressed)
        {
            btn->is_pressed = true;

            button_raw_event_t release_event;
            BaseType_t got_release = xQueueReceive(btn->raw_queue, &release_event, pdMS_TO_TICKS(btn->long_press_ms));
            if (got_release == pdTRUE && !release_event.pressed)
            {
                button_event_t event = ((release_event.tick - raw_event.tick) >= pdMS_TO_TICKS(btn->long_press_ms)) ? BUTTON_EVENT_LONG_PRESS : BUTTON_EVENT_SHORT_PRESS;
                xQueueSend(btn->event_queue, &event, portMAX_DELAY);
            }
            else if (got_release != pdTRUE) // Timeout, treat as long press
            {
                button_event_t event = BUTTON_EVENT_LONG_PRESS;
                xQueueSend(btn->event_queue, &event, portMAX_DELAY);
            }

            btn->is_pressed = false;
        }
        else
        {
            btn->is_pressed = false;
        }
    }
}

esp_err_t button_driver_init(button_driver_t *btn, gpio_num_t pin, QueueHandle_t event_queue, uint32_t debounce_ms, uint32_t long_press_ms)
{
    bool isr_handler_added_flag = false;
    bool task_created_flag = false;
    esp_err_t err = ESP_ERR_NO_MEM;

    if (btn == NULL)
    {
        ESP_LOGE(TAG, "Button driver pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    memset(btn, 0, sizeof(button_driver_t)); // Clear the structure
    btn->pin = pin;
    btn->debounce_ms = (debounce_ms > 0) ? debounce_ms : BUTTON_DEBOUNCE_MS;
    btn->long_press_ms = (long_press_ms > 0) ? long_press_ms : BUTTON_LONG_PRESS_MS;
    btn->last_transition_tick = 0;

    btn->raw_queue = xQueueCreate(10, sizeof(button_raw_event_t));
    if (btn->raw_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create raw event queue");
        return ESP_ERR_NO_MEM;
    }

    if (event_queue != NULL)
    {
        btn->event_queue = event_queue;
        btn->owns_event_queue = false;
    }
    else
    {
        btn->event_queue = xQueueCreate(10, sizeof(button_event_t));
        btn->owns_event_queue = (btn->event_queue != NULL);
    }
    
    if (btn->event_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event queue");
        goto clean_up;
    }

    err = gpio_reset_pin(pin);
    if (err != ESP_OK && err != ESP_ERR_INVALID_ARG)
    {
        ESP_LOGW(TAG, "Failed to reset GPIO pin %d: %s", pin, esp_err_to_name(err));
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d: %s", pin, esp_err_to_name(err));
        goto clean_up;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(err));
        goto clean_up;
    }
    
    err = gpio_isr_handler_add(pin, button_isr_handler, (void *) btn);
    if (err == ESP_OK)
    {
        isr_handler_added_flag = true;
    }
    else
    {
        ESP_LOGE(TAG, "gpio_isr_handler_add failed: %s", esp_err_to_name(err));
        goto clean_up;
    }

    BaseType_t task_created = xTaskCreate(button_task, "button_task", 2048, (void *) btn, 10, &btn->task_handle);
    if (task_created != pdTRUE)
    {
        err = ESP_ERR_NO_MEM;
        ESP_LOGE(TAG, "Failed to create button task");
        goto clean_up;
    }
    task_created_flag = true;
    btn->initialized = true;
    ESP_LOGI(TAG, "Button driver initialized successfully on GPIO pin %d", pin);
    return ESP_OK;

clean_up:
    if (task_created_flag)
    {
        vTaskDelete(btn->task_handle);
        btn->task_handle = NULL;
    }
    if (isr_handler_added_flag)
    {
        gpio_isr_handler_remove(pin);
    }
    if (btn->owns_event_queue && btn->event_queue != NULL)
    {
        vQueueDelete(btn->event_queue);
        btn->event_queue = NULL;
        
    }
    if (btn->raw_queue != NULL)
    {
        vQueueDelete(btn->raw_queue);
        btn->raw_queue = NULL;
    }
    btn->initialized = false;
    ESP_LOGE(TAG, "Button driver initialized failed: %s", esp_err_to_name(err));
    return err;
}

void button_driver_deinit(button_driver_t *btn)
{
    // Do nothing
}

bool button_driver_is_pressed(const button_driver_t *btn)
{
    return ((btn != NULL) && btn->is_pressed); 
}

bool button_driver_get_event(button_driver_t *btn, button_event_t *event, TickType_t timeout)
{
    if (btn == NULL || btn->event_queue == NULL || event == NULL)
    {
        return false;
    }
    return (xQueueReceive(btn->event_queue, event, timeout) == pdTRUE);
}
