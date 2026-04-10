#include "FreeRTOS.h"
#include "message_buffer.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
  struct TaskControl {
    TaskFunction_t function = nullptr;
    void* parameter = nullptr;
    std::atomic<bool> running{false};
    std::atomic<bool> deleted{false};
    std::atomic<bool> suspended{false};

    std::mutex suspend_mutex;
    std::condition_variable suspend_cv;

    std::mutex notify_mutex;
    std::condition_variable notify_cv;
    uint32_t notify_count = 0;

    void* tls[configNUM_THREAD_LOCAL_STORAGE_POINTERS] = {};
  };

  struct QueueControl {
    size_t item_size = 0;
    size_t capacity = 0;
    std::deque<std::vector<uint8_t>> items;
    std::mutex mutex;
    std::condition_variable cv;
  };

  struct SemaphoreControl {
    std::mutex mutex;
    std::condition_variable cv;
    int count = 1;
  };

  struct TimerControl {
    TickType_t period_ticks = 0;
    BaseType_t auto_reload = pdFALSE;
    void* id = nullptr;
    TimerCallbackFunction_t callback = nullptr;
    std::atomic<bool> active{false};
    std::thread worker;
  };

  struct MessageBufferControl {
    size_t capacity = 0;
    size_t used = 0;
    std::deque<std::vector<uint8_t>> messages;
    std::mutex mutex;
    std::condition_variable cv;
  };

  std::mutex g_task_mutex;
  std::unordered_map<std::thread::id, TaskControl*> g_tasks;
  thread_local TaskControl* g_current_task = nullptr;

  std::atomic<eTaskSchedulerState> g_scheduler_state{taskSCHEDULER_NOT_STARTED};
  auto g_start_time = std::chrono::steady_clock::now();

  TaskControl* ResolveTask(TaskHandle_t task)
  {
    if (task)
    {
      return static_cast<TaskControl*>(task);
    }
    return g_current_task;
  }

  TickType_t MsToTicks(uint32_t ms)
  {
    return static_cast<TickType_t>(ms / portTICK_PERIOD_MS);
  }
}

extern "C" {

void* pvPortMalloc(size_t size)
{
  return std::malloc(size);
}

void vPortFree(void* ptr)
{
  std::free(ptr);
}

size_t xPortGetFreeHeapSize(void)
{
  return configTOTAL_HEAP_SIZE;
}

size_t xPortGetMinimumEverFreeHeapSize(void)
{
  return configTOTAL_HEAP_SIZE;
}

TaskHandle_t xTaskCreateStatic(TaskFunction_t task, const char* name, uint32_t stackDepth, void* params,
                               UBaseType_t priority, StackType_t* stackBuffer, StaticTask_t* taskBuffer)
{
  (void)name;
  (void)stackDepth;
  (void)priority;
  (void)stackBuffer;
  (void)taskBuffer;

  TaskHandle_t handle = nullptr;
  if (xTaskCreate(task, name, static_cast<uint16_t>(stackDepth), params, priority, &handle) != pdPASS)
  {
    return nullptr;
  }
  return handle;
}

BaseType_t xTaskCreate(TaskFunction_t task, const char* name, uint16_t stackDepth, void* params, UBaseType_t priority,
                       TaskHandle_t* taskHandle)
{
  (void)name;
  (void)stackDepth;
  (void)priority;

  if (!task)
  {
    return pdFAIL;
  }

  auto* control = new TaskControl();
  control->function = task;
  control->parameter = params;
  control->running = true;

  std::thread worker([control]() {
    g_current_task = control;
    {
      std::lock_guard<std::mutex> lock(g_task_mutex);
      g_tasks[std::this_thread::get_id()] = control;
    }

    control->function(control->parameter);

    control->running = false;
    {
      std::lock_guard<std::mutex> lock(g_task_mutex);
      g_tasks.erase(std::this_thread::get_id());
    }
    g_current_task = nullptr;
  });
  worker.detach();

  if (taskHandle)
  {
    *taskHandle = control;
  }
  return pdPASS;
}

void vTaskDelete(TaskHandle_t task)
{
  TaskControl* control = ResolveTask(task);
  if (!control)
  {
    return;
  }
  control->deleted = true;
  control->running = false;
}

void vTaskDelay(TickType_t ticks)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ticks * portTICK_PERIOD_MS));
}

void taskYIELD(void)
{
  std::this_thread::yield();
}

TickType_t xTaskGetTickCount(void)
{
  auto now = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_start_time).count();
  return static_cast<TickType_t>(MsToTicks(static_cast<uint32_t>(ms)));
}

eTaskSchedulerState xTaskGetSchedulerState(void)
{
  return g_scheduler_state.load();
}

void vTaskStartScheduler(void)
{
  g_scheduler_state.store(taskSCHEDULER_RUNNING);
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
  return g_current_task;
}

void vTaskSuspend(TaskHandle_t task)
{
  TaskControl* control = ResolveTask(task);
  if (!control)
  {
    return;
  }
  std::unique_lock<std::mutex> lock(control->suspend_mutex);
  control->suspended = true;
  if (control == g_current_task)
  {
    control->suspend_cv.wait(lock, [control]() { return !control->suspended.load() || control->deleted.load(); });
  }
}

void vTaskResume(TaskHandle_t task)
{
  TaskControl* control = ResolveTask(task);
  if (!control)
  {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(control->suspend_mutex);
    control->suspended = false;
  }
  control->suspend_cv.notify_all();
}

void vTaskSuspendAll(void)
{
}

BaseType_t xTaskResumeAll(void)
{
  return pdTRUE;
}

eTaskState eTaskGetState(TaskHandle_t task)
{
  TaskControl* control = ResolveTask(task);
  if (!control)
  {
    return eInvalid;
  }
  if (control->deleted || !control->running)
  {
    return eDeleted;
  }
  return eRunning;
}

uint32_t ulTaskNotifyTake(BaseType_t clearOnExit, TickType_t ticksToWait)
{
  TaskControl* control = ResolveTask(nullptr);
  if (!control)
  {
    return 0;
  }

  std::unique_lock<std::mutex> lock(control->notify_mutex);
  if (control->notify_count == 0)
  {
    if (ticksToWait == 0)
    {
      return 0;
    }
    if (ticksToWait == portMAX_DELAY)
    {
      control->notify_cv.wait(lock, [control]() { return control->notify_count > 0 || control->deleted.load(); });
    }
    else
    {
      control->notify_cv.wait_for(lock, std::chrono::milliseconds(ticksToWait * portTICK_PERIOD_MS),
                                  [control]() { return control->notify_count > 0 || control->deleted.load(); });
    }
  }

  uint32_t count = control->notify_count;
  if (count > 0)
  {
    if (clearOnExit)
    {
      control->notify_count = 0;
    }
    else
    {
      control->notify_count--;
    }
  }
  return count;
}

void xTaskNotifyGive(TaskHandle_t task)
{
  TaskControl* control = ResolveTask(task);
  if (!control)
  {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(control->notify_mutex);
    control->notify_count++;
  }
  control->notify_cv.notify_all();
}

void* pvTaskGetThreadLocalStoragePointer(TaskHandle_t task, BaseType_t index)
{
  TaskControl* control = ResolveTask(task);
  if (!control || index < 0 || index >= configNUM_THREAD_LOCAL_STORAGE_POINTERS)
  {
    return nullptr;
  }
  return control->tls[index];
}

void vTaskSetThreadLocalStoragePointer(TaskHandle_t task, BaseType_t index, void* value)
{
  TaskControl* control = ResolveTask(task);
  if (!control || index < 0 || index >= configNUM_THREAD_LOCAL_STORAGE_POINTERS)
  {
    return;
  }
  control->tls[index] = value;
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t itemSize)
{
  auto* queue = new QueueControl();
  queue->capacity = length;
  queue->item_size = itemSize;
  return queue;
}

BaseType_t xQueueSend(QueueHandle_t queueHandle, const void* item, TickType_t ticksToWait)
{
  auto* queue = static_cast<QueueControl*>(queueHandle);
  if (!queue || !item)
  {
    return pdFALSE;
  }

  std::unique_lock<std::mutex> lock(queue->mutex);
  auto has_space = [queue]() { return queue->items.size() < queue->capacity; };

  if (!has_space())
  {
    if (ticksToWait == 0)
    {
      return pdFALSE;
    }
    if (ticksToWait == portMAX_DELAY)
    {
      queue->cv.wait(lock, has_space);
    }
    else
    {
      queue->cv.wait_for(lock, std::chrono::milliseconds(ticksToWait * portTICK_PERIOD_MS), has_space);
    }
  }

  if (!has_space())
  {
    return pdFALSE;
  }

  std::vector<uint8_t> payload(queue->item_size);
  std::memcpy(payload.data(), item, queue->item_size);
  queue->items.push_back(std::move(payload));
  lock.unlock();
  queue->cv.notify_all();
  return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t queueHandle, void* buffer, TickType_t ticksToWait)
{
  auto* queue = static_cast<QueueControl*>(queueHandle);
  if (!queue || !buffer)
  {
    return pdFALSE;
  }

  std::unique_lock<std::mutex> lock(queue->mutex);
  auto has_items = [queue]() { return !queue->items.empty(); };

  if (!has_items())
  {
    if (ticksToWait == 0)
    {
      return pdFALSE;
    }
    if (ticksToWait == portMAX_DELAY)
    {
      queue->cv.wait(lock, has_items);
    }
    else
    {
      queue->cv.wait_for(lock, std::chrono::milliseconds(ticksToWait * portTICK_PERIOD_MS), has_items);
    }
  }

  if (!has_items())
  {
    return pdFALSE;
  }

  std::vector<uint8_t> payload = std::move(queue->items.front());
  queue->items.pop_front();
  lock.unlock();
  queue->cv.notify_all();

  std::memcpy(buffer, payload.data(), std::min(payload.size(), queue->item_size));
  return pdTRUE;
}

BaseType_t xQueueReset(QueueHandle_t queueHandle)
{
  auto* queue = static_cast<QueueControl*>(queueHandle);
  if (!queue)
  {
    return pdFALSE;
  }
  std::lock_guard<std::mutex> lock(queue->mutex);
  queue->items.clear();
  return pdTRUE;
}

UBaseType_t uxQueueSpacesAvailable(QueueHandle_t queueHandle)
{
  auto* queue = static_cast<QueueControl*>(queueHandle);
  if (!queue)
  {
    return 0;
  }
  std::lock_guard<std::mutex> lock(queue->mutex);
  if (queue->capacity < queue->items.size())
  {
    return 0;
  }
  return static_cast<UBaseType_t>(queue->capacity - queue->items.size());
}

void vQueueDelete(QueueHandle_t queueHandle)
{
  auto* queue = static_cast<QueueControl*>(queueHandle);
  delete queue;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
  auto* sem = new SemaphoreControl();
  sem->count = 1;
  return sem;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t semHandle, TickType_t ticksToWait)
{
  auto* sem = static_cast<SemaphoreControl*>(semHandle);
  if (!sem)
  {
    return pdFALSE;
  }

  std::unique_lock<std::mutex> lock(sem->mutex);
  auto can_take = [sem]() { return sem->count > 0; };

  if (!can_take())
  {
    if (ticksToWait == 0)
    {
      return pdFALSE;
    }
    if (ticksToWait == portMAX_DELAY)
    {
      sem->cv.wait(lock, can_take);
    }
    else
    {
      sem->cv.wait_for(lock, std::chrono::milliseconds(ticksToWait * portTICK_PERIOD_MS), can_take);
    }
  }

  if (!can_take())
  {
    return pdFALSE;
  }

  sem->count--;
  return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t semHandle)
{
  auto* sem = static_cast<SemaphoreControl*>(semHandle);
  if (!sem)
  {
    return pdFALSE;
  }

  {
    std::lock_guard<std::mutex> lock(sem->mutex);
    if (sem->count < 1)
    {
      sem->count++;
    }
  }
  sem->cv.notify_all();
  return pdTRUE;
}

void vSemaphoreDelete(SemaphoreHandle_t semHandle)
{
  auto* sem = static_cast<SemaphoreControl*>(semHandle);
  delete sem;
}

TimerHandle_t xTimerCreateStatic(const char* name, TickType_t period, BaseType_t autoReload, void* timerId,
                                 TimerCallbackFunction_t callback, StaticTimer_t* buffer)
{
  (void)name;
  (void)buffer;

  auto* timer = new TimerControl();
  timer->period_ticks = period == 0 ? 1 : period;
  timer->auto_reload = autoReload;
  timer->id = timerId;
  timer->callback = callback;
  return timer;
}

BaseType_t xTimerStart(TimerHandle_t timerHandle, TickType_t ticksToWait)
{
  (void)ticksToWait;
  auto* timer = static_cast<TimerControl*>(timerHandle);
  if (!timer || !timer->callback)
  {
    return pdFALSE;
  }

  if (timer->active.exchange(true))
  {
    return pdTRUE;
  }

  timer->worker = std::thread([timer]() {
    while (timer->active)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(timer->period_ticks * portTICK_PERIOD_MS));
      if (!timer->active)
      {
        break;
      }
      timer->callback(timer);
      if (!timer->auto_reload)
      {
        timer->active = false;
        break;
      }
    }
  });
  timer->worker.detach();
  return pdTRUE;
}

BaseType_t xTimerStop(TimerHandle_t timerHandle, TickType_t ticksToWait)
{
  (void)ticksToWait;
  auto* timer = static_cast<TimerControl*>(timerHandle);
  if (!timer)
  {
    return pdFALSE;
  }
  timer->active = false;
  return pdTRUE;
}

void* pvTimerGetTimerID(TimerHandle_t timerHandle)
{
  auto* timer = static_cast<TimerControl*>(timerHandle);
  if (!timer)
  {
    return nullptr;
  }
  return timer->id;
}

MessageBufferHandle_t xMessageBufferCreate(size_t size)
{
  auto* buffer = new MessageBufferControl();
  buffer->capacity = size;
  return buffer;
}

size_t xMessageBufferSendFromISR(MessageBufferHandle_t bufferHandle, const void* data, size_t size, BaseType_t* taskWoken)
{
  (void)taskWoken;
  auto* buffer = static_cast<MessageBufferControl*>(bufferHandle);
  if (!buffer || !data)
  {
    return 0;
  }

  std::lock_guard<std::mutex> lock(buffer->mutex);
  if (size > buffer->capacity || buffer->used + size > buffer->capacity)
  {
    return 0;
  }
  std::vector<uint8_t> payload(size);
  std::memcpy(payload.data(), data, size);
  buffer->messages.push_back(std::move(payload));
  buffer->used += size;
  buffer->cv.notify_all();
  return size;
}

size_t xMessageBufferReceive(MessageBufferHandle_t bufferHandle, void* outBuffer, size_t bufferSize, TickType_t ticksToWait)
{
  auto* buffer = static_cast<MessageBufferControl*>(bufferHandle);
  if (!buffer || !outBuffer)
  {
    return 0;
  }

  std::unique_lock<std::mutex> lock(buffer->mutex);
  auto has_messages = [buffer]() { return !buffer->messages.empty(); };

  if (!has_messages())
  {
    if (ticksToWait == 0)
    {
      return 0;
    }
    if (ticksToWait == portMAX_DELAY)
    {
      buffer->cv.wait(lock, has_messages);
    }
    else
    {
      buffer->cv.wait_for(lock, std::chrono::milliseconds(ticksToWait * portTICK_PERIOD_MS), has_messages);
    }
  }

  if (!has_messages())
  {
    return 0;
  }

  std::vector<uint8_t> payload = std::move(buffer->messages.front());
  buffer->messages.pop_front();
  buffer->used -= payload.size();

  size_t to_copy = std::min(payload.size(), bufferSize);
  std::memcpy(outBuffer, payload.data(), to_copy);
  return to_copy;
}

BaseType_t xMessageBufferReset(MessageBufferHandle_t bufferHandle)
{
  auto* buffer = static_cast<MessageBufferControl*>(bufferHandle);
  if (!buffer)
  {
    return pdFALSE;
  }
  std::lock_guard<std::mutex> lock(buffer->mutex);
  buffer->messages.clear();
  buffer->used = 0;
  return pdTRUE;
}

}  // extern "C"
