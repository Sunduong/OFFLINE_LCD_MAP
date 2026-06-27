1. What happens if LCD task tries to acquire SPI mutex but SD card task already has it?
--> LCD task blocks and waits for SD card task to release the mutex. If SD card task crashes/hangs while holding it, LCD task will also hang forever (or until watchdog timeout resets the system). This is a potential system failure point.

2. Why can't we use portMAX_DELAY in a production system?
--> portMAX_DELAY causes tasks to wait indefinitely. In production, if another task crashes/gets stuck holding the mutex, your task hangs forever, potentially causing system lockup or watchdog timeout. This violates real-time guarantees.

3. What's the difference between xSemaphoreTake() and xSemaphoreTakeFromISR()?
--> xSemaphoreTake() is for tasks and CAN BLOCK if the semaphore is unavailable. 
xSemaphoreTakeFromISR() does NOT exist because ISRs cannot block (they run in 
interrupt context and must finish quickly). Instead, ISRs use xSemaphoreGiveFromISR() 
to signal without blocking. This function sets xHigherPriorityTaskWoken to indicate 
if a higher-priority task was woken. You MUST check this flag and call 
portYIELD_FROM_ISR() to let that task run immediately, otherwise it waits for the 
next context switch.