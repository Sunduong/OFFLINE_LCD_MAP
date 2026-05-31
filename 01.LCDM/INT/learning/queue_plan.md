### Exercise 2: ISR + Queue (Medium) ⭐ MOST IMPORTANT

**Goal:** ISR sends button presses to queue, task processes

```c
hashtag#include "freertos/FreeRTOS.h"
hashtag#include "freertos/task.h"
hashtag#include "freertos/queue.h"
hashtag#include "driver/gpio.h"

hashtag#define BUTTON_GPIO GPIO_NUM_X // Your button GPIO

QueueHandle_t button_queue = NULL;

// ISR: Button pressed
void button_isr_handler(void *arg) {
 BaseType_t xHigherPriorityTaskWoken = pdFALSE;

 uint32_t button_press_count = 1;

 // TODO: Send button press to queue (ISR-safe)
 // Use: xQueueSendFromISR(button_queue, &button_press_count, &xHigherPriorityTaskWoken);

 printf("Button pressed in ISR!\n");
}

// Task: Process button presses
void button_handler_task(void *pvParameters) {
 uint32_t press_count;

 while (1) {
 // TODO: Receive from queue (blocks if empty)
 // Use: if (xQueueReceive(button_queue, &press_count, portMAX_DELAY))

 printf("Task got button press: %d\n", press_count);
 // Do something with press (toggle LED, etc)
 }
}

void app_main(void) {
 // TODO 1: Create queue (size: 10, item size: 4 bytes)
 button_queue = xQueueCreate(10, sizeof(uint32_t));

 // TODO 2: Setup button GPIO with ISR
 gpio_config_t io_conf = {
 .pin_bit_mask = (1ULL << BUTTON_GPIO),
 .mode = GPIO_MODE_INPUT,
 .intr_type = GPIO_INTR_POSEDGE, // Rising edge
 };
 gpio_config(&io_conf);
 gpio_install_isr_service(0);
 gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

 // TODO 3: Create button handler task
 xTaskCreate(button_handler_task, "button_task", 2048, NULL, 5, NULL);
}
```

**Expected Behavior:**
```
Press button → [ISR sends to queue] → [Task receives] → Prints "Button press!"
```

---

### Exercise 3: Multiple Producers, One Consumer (Hard) 🔥

**Goal:** Two tasks send to same queue, one task receives

```c
void producer1_task(void *pvParameters) {
 while (1) {
 uint32_t data = 100;
 xQueueSend(myQueue, &data, portMAX_DELAY);
 printf("Producer 1 sent: 100\n");
 vTaskDelay(pdMS_TO_TICKS(500));
 }
}

void producer2_task(void *pvParameters) {
 while (1) {
 uint32_t data = 200;
 xQueueSend(myQueue, &data, portMAX_DELAY);
 printf("Producer 2 sent: 200\n");
 vTaskDelay(pdMS_TO_TICKS(700));
 }
}

void consumer_task(void *pvParameters) {
 uint32_t data;
 while (1) {
 if (xQueueReceive(myQueue, &data, portMAX_DELAY)) {
 printf("Consumer received: %d\n", data);
 }
 }
}

void app_main(void) {
 myQueue = xQueueCreate(10, sizeof(uint32_t));

 xTaskCreate(producer1_task, "prod1", 2048, NULL, 5, NULL);
 xTaskCreate(producer2_task, "prod2", 2048, NULL, 5, NULL);
 xTaskCreate(consumer_task, "consumer", 2048, NULL, 5, NULL);
}
```

**Expected Output:**
```
Producer 1 sent: 100
Producer 2 sent: 200
Consumer received: 100
Producer 1 sent: 100
Consumer received: 200
...
```

---

## Homework Challenge 📝

**Pick ONE of the 3 exercises above and:**
1. Implement it on your ESP32
2. Get it compiling and running
3. Show me the serial output
4. Tell me: *"What happened? Did it work as expected?"*

**Start with Exercise 1 (easy)** to build confidence. Then move to **Exercise 2 (ISR version)** - that's what you'll use for GPS + buttons!

---

## Key Takeaways

✅ **Queues are FIFO** - First in, first out 
✅ **ISR uses `xQueueSendFromISR()`** - Never blocking! 
✅ **Tasks use `xQueueReceive()`** - Can block and sleep 
✅ **Protects data** - No race conditions 
✅ **Saves power** - Task sleeps if queue empty 

---

**Which exercise appeals to you most?** Start there! 🚀

View Nguyen Xuan’s profileNguyen Xuan Duong
Nguyen Xuan Duong   11:58 PM
**✅ CORRECT!** 👏 

Your answer is solid:
- ✅ Correct definition
- ✅ Good real-world analogy
- ✅ You explained the WHY (ensures order)

---

## But Here's My Strict Teacher Challenge 🎓

Your answer is textbook perfect. Now **apply it to your actual code:**

**Question 1b (Harder):**

Look at your **Day2 button code** - in `vTask1()`:

```c
EventBits_t uxBits = xEventGroupWaitBits(xEventGroup, BIT_EVENT_BUTTON_PRESSED_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
if (uxBits & BIT_EVENT_BUTTON_PRESSED_DONE) {
 TickType_t delta_time = time_released - time_pressed;
 if (delta_time < pdMS_TO_TICKS(1000)) {
 printf("Short press detected...\n");
 }
}
```

You're **NOT using a queue here** - you're using **event groups** instead. 

**My question:** Why didn't your code use a queue for button events? What's the difference between using a queue vs an event group for this?

---

## Next: Answer These (No Looking at My Notes!)

2. **What's the difference between `xQueueSend()` and `xQueueSendFromISR()`?**
 - Why can't ISR use `xQueueSend()`?
 - What happens if you try? (Hint: think about blocking)

3. **In your Day2 code, find where a queue is used:**
 - Give me the variable name
 - Give me the line number
 - What data goes into that queue?

4. **What's `portMAX_DELAY`?**
 - When would you use it?
 - When would you use a small timeout like `100`?

---

**Answer 2, 3, and 4 now.** Don't rush. Take your time. 📝
