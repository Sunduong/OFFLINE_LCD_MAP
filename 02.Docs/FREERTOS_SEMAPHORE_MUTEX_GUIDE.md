# FreeRTOS Semaphore & Mutex Learning Guide
## For Portfolio Project + Samsung Job Preparation

---

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Core Concepts](#core-concepts)
3. [Semaphore Basics](#semaphore-basics)
4. [Mutex Basics](#mutex-basics)
5. [Semaphore vs Mutex](#semaphore-vs-mutex)
6. [Practical Examples](#practical-examples)
7. [Your SPI Bus Manager Design](#your-spi-bus-manager-design)

---

## Prerequisites

You already know:
- ✅ FreeRTOS Tasks (create, suspend, resume)
- ✅ Software Timers
- ✅ Event Groups
- ✅ GPIO ISRs

**Key concept you need:** Tasks can run concurrently. If two tasks access the same resource at the same time, BAD THINGS happen.

---

## Core Concepts

### The Problem: Resource Contention

**Scenario:** Two tasks trying to use SPI bus simultaneously

```
Task_LCD:                          Task_SD:
│                                  │
├─ Lock SPI? No, go ahead!        │
│  (writes to MOSI)               │
│                                  ├─ Lock SPI? No, go ahead!
│  (reads from MISO)              │  (writes to MOSI at same time!)
│  Gets corrupted data! ❌         │  (LCD data gets corrupted!) ❌
│                                  │
```

**Solution:** Only ONE task can use the SPI bus at a time → Use a **MUTEX**.

### Hai loại Semaphore chính

#### 1. Binary Semaphore (giá trị 0 hoặc 1)

```
 0 ←──────────────────────────────────→ 1

 Chỉ có 1 "cờ" duy nhất
 Dùng cho: BÁO HIỆU (signaling)
```

- Giống như **cửa hàng đóng/mở**: chỉ cần biết mở hay đóng
- Ứng dụng: "Data đã sẵn sàng chưa?" → chỉ cần ĐÚNG/SAI

#### 2. Counting Semaphore (giá trị 0, 1, 2, ... N)

```
 0 ←── 1 ←── 2 ←── 3 ←── ... ←── N

 Có thể đếm nhiều lần
 Dùng cho: QUẢN LÝ SỐ LƯỢNG (resource counting)
```

- Giống như **bãi đỗ xe**: biết còn bao nhiêu chỗ trống
- Ứng dụng: "Còn bao nhiêu buffer trống?" → cần đếm số lượng

> **Trong project ESP LCD của bạn, Binary Semaphore là đủ.** Counting Semaphore ít khi cần.

---

### Binary Semaphore — Code chi tiết từng dòng

```c
hashtag#include "freertos/semphr.h"

// BƯỚC 1: Tạo binary semaphore
// Lưu ý: xSemaphoreCreateBinary() tạo ra semaphore với giá trị = 0 (chưa có tín hiệu)
// Khác với xSemaphoreCreateMutex() — mutex tạo ra với giá trị = 1 (sẵn sàng)
SemaphoreHandle_t data_ready = xSemaphoreCreateBinary();
// data_ready → [ value = 0 ] ← chưa ai Give, cờ đang hạ

// ─────────────────────────────────────────
// BƯỚC 2: Task A — CHỜ tín hiệu
// ─────────────────────────────────────────
void waiting_task(void *arg) {
 while (1) {
 // xSemaphoreTake() làm gì?
 // 1. Kiểm tra semaphore value
 // 2. Nếu value > 0: giảm value đi 1, return pdTRUE ngay lập tức
 // 3. Nếu value = 0: BLOCK task này (cho task ngủ) cho đến khi ai đó Give
 // 4. portMAX_DELAY = chờ mãi mãi (không timeout)

 if (xSemaphoreTake(data_ready, portMAX_DELAY) == pdTRUE) {
 // ✅ Đến đây = đã nhận được tín hiệu
 // Task A được đánh thức và tiếp tục chạy
 printf("Tín hiệu nhận được! Xử lý data...\n");
 process_data();
 }
 // Nếu xSemaphoreTake() fail (timeout) → bỏ qua if, vòng lặp lại
 }
}

// ─────────────────────────────────────────
// BƯỚC 3: Task B — GỬI tín hiệu
// ─────────────────────────────────────────
void signaling_task(void *arg) {
 while (1) {
 // Làm gì đó... ví dụ đọc sensor
 read_sensor_data();

 // xSemaphoreGive() làm gì?
 // 1. Tăng semaphore value lên 1
 // 2. Nếu có task nào đang chờ (blocked) ở xSemaphoreTake()
 // → Đánh thức task đó dậy
 // 3. Value lại giảm về 0 (vì task A Take rồi)

 xSemaphoreGive(data_ready); // 🚩 Cờ lên! Báo cho Task A biết
 // Task A sẽ được đánh thức tại đây

 vTaskDelay(pdMS_TO_TICKS(1000)); // Đợi 1 giây rồi đọc lại
 }
}
```

#### Timeline — Xem thứ tự xảy ra như thế nào

```
Thời gian →
─────────────────────────────────────────────────────────────

Task B (signaling): ── read_sensor() ── Give() ────────── read_sensor() ── Give() ──
 ↓ ↓
Semaphore value: 0 ──────────────────── 1 → 0 ──────────────────────── 1 → 0 ───────
 ↓ ↓
Task A (waiting): ── Take() 💤💤💤💤 ⏰ tỉnh! ── process() ── Take() 💤💤 ⏰ tỉnh! ──

Giải thích:
- Ban đầu Sem = 0, Task A gọi Take() → bị block 💤
- Task B gọi Give() → Sem = 1, Task A được đánh thức ⏰
- Task A Take() thành công → Sem giảm về 0, Task A chạy process()
- Task A gọi Take() lần nữa → Sem = 0 → lại bị block 💤
- Task B Give() tiếp → Sem = 1 → Task A lại tỉnh ⏰
- ... lặp lại
```

---

### Tại sao gọi là "Binary"?

Vì giá trị chỉ có 2 trạng thái: **0** hoặc **1**

| Giá trị | Ý nghĩa | Giống như |
|---------|---------|-----------|
| **1** | Có tín hiệu (đã Give) | Đèn xanh 🟢 — đi tiếp! |
| **0** | Chưa có tín hiệu (chưa Give / đã Take) | Đèn đỏ 🔴 — phải chờ! |

---

### Ví dụ thực tế: ISR báo data UART đã đến

Đây là pattern **rất phổ biến** trong embedded: ISR nhận data → báo cho task xử lý.

```c
SemaphoreHandle_t uart_data_ready;

// Khởi tạo
void app_main(void) {
 uart_data_ready = xSemaphoreCreateBinary(); // value = 0
 xTaskCreate(uart_handler_task, "uart_task", 4096, NULL, 5, NULL);
}

// ─────────────────────────────────────────
// ISR: Chạy khi UART nhận được data
// ⚠️ Trong ISR PHẢI dùng xSemaphoreGiveFromISR()
// KHÔNG được dùng xSemaphoreGive() thường!
// ─────────────────────────────────────────
void uart_isr(void) {
 BaseType_t xHigherPriorityTaskWoken = pdFALSE;

 // Give từ ISR — báo hiệu "data đã sẵn sàng"
 xSemaphoreGiveFromISR(uart_data_ready, &xHigherPriorityTaskWoken);
 // ↑ semaphore ↑ biến này sẽ = pdTRUE
 // nếu task ưu tiên cao hơn bị đánh thức

 // Tại sao cần xHigherPriorityTaskWoken?
 // → ISR có thể đánh thức task ưu tiên CAO hơn task đang chạy
 // → Cần yield (nhường CPU) ngay để task ưu tiên cao chạy tiếp
 // → Nếu không yield, task ưu tiên cao phải chờ đến lần context switch tiếp theo
 if (xHigherPriorityTaskWoken == pdTRUE) {
 portYIELD_FROM_ISR(); // Nhường CPU cho task ưu tiên cao hơn
 }
}

// ─────────────────────────────────────────
// Task: Xử lý UART data khi có tín hiệu
// ─────────────────────────────────────────
void uart_handler_task(void *arg) {
 while (1) {
 // Chờ ISR Give → nếu chưa có, task ngủ 💤
 if (xSemaphoreTake(uart_data_ready, portMAX_DELAY) == pdTRUE) {
 // ✅ ISR đã báo data sẵn sàng
 printf("Data received!\n");
 process_uart_data();
 }
 // Xử lý xong → vòng lặp → lại Take() → lại chờ 💤
 }
}
```

#### Tại sao ISR phải dùng `GiveFromISR` còn Task dùng `Give`?

| | `xSemaphoreGive()` | `xSemaphoreGiveFromISR()` |
|---|---|---|
| **Gọi từ** | Task (bình thường) | ISR (ngắt) |
| **Tại sao khác?** | Task có thể bị preempt | ISR không thể bị preempt |
| **Context switch** | Tự động | Phải chỉ định qua `xHigherPriorityTaskWoken` |
| **Nếu dùng sai** | Gọi `Give()` trong ISR → CRASH 💥 | Gọi `GiveFromISR()` trong task → vẫn hoạt động nhưng chậm hơn |

> **Quy tắc vàng:** Trong ISR → luôn dùng hàm có hậu tố `...FromISR()`

---

### Tóm tắt Semaphore

```
┌─────────────────────────────────────────────────────┐
│ BINARY SEMAPHORE — TÓM TẮT │
├─────────────────────────────────────────────────────┤
│ │
│ Mục đích: BÁO HIỆU giữa các task/ISR │
│ │
│ Give() = "Tôi đã xong, ai cần thì lấy đi!" 🚩 │
│ Take() = "Tôi chờ đến khi có tín hiệu" 💤 │
│ │
│ Dùng cho: │
│ ✅ ISR → Task signaling │
│ ✅ Task A chờ Task B hoàn thành │                                    
│ ✅ "Data ready", "Event occurred" │
│ │
│ KHÔNG dùng cho: │
│ ❌ Bảo vệ tài nguyên dùng chung (→ dùng Mutex) │
│ ❌ Chặn 2 task truy cập cùng lúc (→ dùng Mutex) │
│ │
└─────────────────────────────────────────────────────┘

---

## Semaphore Basics

### What is a Semaphore?

A **counter-based synchronization mechanism**. Think of it like a parking lot:
- Semaphore value = number of available parking spots
- Task wants to park = `xSemaphoreTake()`
- Task leaves = `xSemaphoreGive()`

### Binary Semaphore (most common for you)

```c
#include "freertos/semphr.h"

// Create a binary semaphore (initial value = 1, max value = 1)
SemaphoreHandle_t sema = xSemaphoreCreateBinary();

// In one task: Wait for signal
xSemaphoreTake(sema, portMAX_DELAY);  // Block until signal arrives
printf("Semaphore received!\n");

// In another task: Send signal
xSemaphoreGive(sema);  // Wake up the waiting task
```

### Why "Binary"?

Binary semaphore value is 0 or 1:
- 1 = "parking spot available" (semaphore given)
- 0 = "no spots available" (semaphore taken)

### Real Example: Task Synchronization

```c
SemaphoreHandle_t uart_data_ready;

// Task 1: UART ISR (runs when data arrives)
void uart_isr(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Signal that data is available
    xSemaphoreGiveFromISR(uart_data_ready, &xHigherPriorityTaskWoken);
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();  // Yield to waiting task
    }
}

// Task 2: Process UART data
void uart_handler_task(void *arg) {
    while (1) {
        // Wait for data to arrive
        if (xSemaphoreTake(uart_data_ready, portMAX_DELAY) == pdTRUE) {
            printf("Data received!\n");
            process_uart_data();
        }
    }
}
```

---

## Mutex Basics

### What is a Mutex?

A **Mutual Exclusion lock**. Only ONE owner at a time.

```c
#include "freertos/semphr.h"

// Create a mutex
SemaphoreHandle_t spi_mutex = xSemaphoreCreateMutex();

// Task A:
xSemaphoreTake(spi_mutex, portMAX_DELAY);   // Acquire lock
spi_transmit_data(data_a);                  // Use SPI
xSemaphoreGive(spi_mutex);                  // Release lock

// Task B: (must wait while Task A has lock)
xSemaphoreTake(spi_mutex, portMAX_DELAY);   // Blocked until Task A gives
spi_transmit_data(data_b);                  // Use SPI
xSemaphoreGive(spi_mutex);                  // Release lock
```

### Key Difference from Binary Semaphore

| Aspect | Binary Semaphore | Mutex |
|--------|------------------|-------|
| **Purpose** | Task synchronization | Resource protection |
| **Ownership** | No owner | Has owner (task that took it) |
| **Recursion** | Can't take twice | Can take multiple times (recursive) |
| **Priority Inversion** | None | ✅ Priority Inheritance (advanced) |
| **Can use in ISR?** | ✅ Yes (xSemaphoreGiveFromISR) | ❌ No (takes time) |

---

## Semaphore vs Mutex

### Use Semaphore When:
- Signaling between tasks (task A waits for task B event)
- Task-to-ISR communication
- One task signals many tasks

**Example:**
```c
// GPS task signals when new location available
xSemaphoreGive(location_ready_sema);  // Many tasks might be waiting
```

### Use Mutex When:
- Protecting shared resources (SPI bus, memory buffer, etc.)
- Only one task should access at a time
- Need to prevent race conditions

**Example:**
```c
// Protect SPI bus access
xSemaphoreTake(spi_mutex, portMAX_DELAY);
lcd_write_data(...);
sd_read_file(...);  // Not actually—if you're sharing bus
xSemaphoreGive(spi_mutex);
```

---

## Practical Examples

### Example 1: Simple Mutex Protection (Your SPI Case)

```c
#include "freertos/semphr.h"
#include "freertos/task.h"

// Global SPI mutex
SemaphoreHandle_t spi_mutex;

void spi_bus_init(void) {
    // Create mutex for SPI bus protection
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex!");
        return;
    }
    
    // Initialize SPI bus (shared by LCD and SD)
    spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
    ESP_LOGI(TAG, "SPI mutex created, bus initialized");
}

// LCD Task
void lcd_task(void *arg) {
    while (1) {
        // Acquire SPI mutex
        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) {
            // Now we have exclusive access to SPI
            lcd_fill_rect(...);  // Safe to use SPI
            lcd_draw_line(...);
            
            // Release for other tasks
            xSemaphoreGive(spi_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // Give other tasks chance
    }
}

// SD Card Task
void sd_card_task(void *arg) {
    while (1) {
        // Acquire SPI mutex
        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) {
            // Now we have exclusive access to SPI
            sd_read_file(...);  // Safe to use SPI
            ư(...);
            
            // Release for other tasks
            xSemaphoreGive(spi_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // Give other tasks chance
    }
}
```

### Example 2: Timeout (What if task hangs?)

```c
TickType_t timeout = pdMS_TO_TICKS(1000);  // 1 second timeout

if (xSemaphoreTake(spi_mutex, timeout) == pdTRUE) {
    // Got the lock within 1 second
    lcd_write_data(...);
    xSemaphoreGive(spi_mutex);
} else {
    // Timeout! Couldn't get lock after 1 second
    ESP_LOGW(TAG, "SPI mutex timeout - another task may be stuck!");
}
```

### Example 3: Binary Semaphore for Task Signaling

```c
// GPS data ready signal
SemaphoreHandle_t gps_data_ready;

// GPS UART ISR
void gps_uart_isr(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Signal navigation task that new GPS data arrived
    xSemaphoreGiveFromISR(gps_data_ready, &xHigherPriorityTaskWoken);
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Navigation Task
void navigation_task(void *arg) {
    while (1) {
        // Wait for GPS data
        if (xSemaphoreTake(gps_data_ready, portMAX_DELAY) == pdTRUE) {
            printf("New GPS position: lat=%f, lon=%f\n", 
                   gps_data.latitude, gps_data.longitude);
            
            // Update map display
            update_map_position();
        }
    }
}
```

### Example 4: COMMON MISTAKE - Deadlock

```c
// ❌ WRONG - This causes DEADLOCK
void lcd_and_sd_write(void) {
    // Get LCD lock
    xSemaphoreTake(lcd_mutex, portMAX_DELAY);
    
    // Wait for SD card (but another task has it!)
    xSemaphoreTake(sd_mutex, portMAX_DELAY);  // ❌ DEADLOCK!
    
    xSemaphoreGive(sd_mutex);
    xSemaphoreGive(lcd_mutex);
}

// ✅ CORRECT - Always acquire in same order
void lcd_and_sd_write_correct(void) {
    xSemaphoreTake(lcd_mutex, portMAX_DELAY);   // Always first
    xSemaphoreTake(sd_mutex, portMAX_DELAY);    // Always second
    
    // Do work...
    
    xSemaphoreGive(sd_mutex);                   // Release reverse order
    xSemaphoreGive(lcd_mutex);
}
```

---

## Your SPI Bus Manager Design

### Current Problem

You have:
- LCD on SPI2 (GPIO 10, 11, 12 shared)
- SD Card MUST use SPI2 (can't use different pins)
- Both cannot access SPI2 simultaneously

### Solution Architecture

```c
// ═══════════════════════════════════════════════════════
// FILE: driver/spi_bus_manager.h
// ═══════════════════════════════════════════════════════

typedef struct {
    spi_host_device_t host;           // SPI2_HOST
    SemaphoreHandle_t bus_mutex;      // Protects access
    spi_device_handle_t lcd_device;   // LCD SPI device handle
    spi_device_handle_t sd_device;    // SD card SPI device handle
} spi_bus_manager_t;

// Public API
void spi_bus_manager_init(spi_bus_manager_t *manager);
void spi_bus_manager_acquire(spi_bus_manager_t *manager, TickType_t timeout);
void spi_bus_manager_release(spi_bus_manager_t *manager);

// ═══════════════════════════════════════════════════════
// FILE: driver/spi_bus_manager.c
// ═══════════════════════════════════════════════════════

static const char *TAG = "SPI_BUS_MANAGER";

void spi_bus_manager_init(spi_bus_manager_t *manager) {
    // Create mutex to protect bus
    manager->bus_mutex = xSemaphoreCreateMutex();
    if (manager->bus_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI bus mutex!");
        return;
    }
    
    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = LCD_MISO_PIN,
        .sclk_io_num = LCD_SCK_PIN,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(manager->host, &buscfg, SPI_DMA_CH_AUTO));
    
    ESP_LOGI(TAG, "SPI bus manager initialized with mutex protection");
}

void spi_bus_manager_acquire(spi_bus_manager_t *manager, TickType_t timeout) {
    if (xSemaphoreTake(manager->bus_mutex, timeout) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire SPI bus (timeout)!");
    }
}

void spi_bus_manager_release(spi_bus_manager_t *manager) {
    xSemaphoreGive(manager->bus_mutex);
}

// ═══════════════════════════════════════════════════════
// USAGE IN YOUR TASKS
// ═══════════════════════════════════════════════════════

spi_bus_manager_t spi_manager;

void lcd_task(void *arg) {
    while (1) {
        // Acquire SPI bus
        spi_bus_manager_acquire(&spi_manager, portMAX_DELAY);
        
        // Now we have exclusive access
        lcd_fill_screen(lcd, COLOR_BLACK);
        lcd_draw_rect(lcd, 10, 10, 100, 100, COLOR_WHITE);
        
        // Release for other tasks
        spi_bus_manager_release(&spi_manager);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void sd_card_task(void *arg) {
    while (1) {
        // Acquire SPI bus
        spi_bus_manager_acquire(&spi_manager, pdMS_TO_TICKS(5000));
        
        // Now we have exclusive access
        sd_read_file("map_tile.bin", buffer);
        
        // Release for other tasks
        spi_bus_manager_release(&spi_manager);
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## Key Takeaways for Your Samsung Job

1. **Always protect shared resources** — SPI bus, memory buffers, hardware peripherals
2. **Use Mutex for resources, Semaphore for signals**
3. **Avoid deadlocks** — acquire locks in consistent order
4. **Use timeouts** — never `portMAX_DELAY` in production (could hang)
5. **Profile under load** — test with multiple tasks contending

---

## Your Assignment

After reading this guide:

1. **Write a simple test program** (not hardware-dependent):
   - Create 2 tasks
   - Share a buffer with a MUTEX
   - Task A writes, Task B reads
   - Verify no race conditions

2. **Review your LCD driver**:
   - Currently it doesn't use a mutex for SPI access
   - In Phase 2.2, we'll refactor to use SPI bus manager

3. **Answer these questions**:
   - What happens if LCD task tries to acquire SPI mutex but SD card task already has it?
   - Why can't we use `portMAX_DELAY` in a production system?
   - What's the difference between `xSemaphoreTake()` and `xSemaphoreTakeFromISR()`?

---

## Next Steps

Once you understand this guide:
1. I'll show you how to refactor LCD driver to use SPI bus manager
2. Phase 2.2: SD card driver with proper SPI mutex protection
3. Then we move to Phase 2.3 (GPS)

Questions? Ask them now before we code Phase 2.2.
