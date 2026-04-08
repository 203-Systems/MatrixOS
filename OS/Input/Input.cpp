#include "MatrixOS.h"
#include "Input.h"

static const char* TAG = "Input";

namespace MatrixOS::Input
{

QueueHandle_t inputEventQueue = nullptr;

void Init() {
  if (!inputEventQueue)
  {
    inputEventQueue = xQueueCreate(INPUT_EVENT_QUEUE_SIZE, sizeof(InputEvent));
  }
  else
  {
    xQueueReset(inputEventQueue);
  }
  MLOGI(TAG, "Input system initialized");
}

bool Get(InputEvent* event, uint32_t timeoutMs) {
  if (!inputEventQueue)
  {
    return false;
  }
  return xQueueReceive(inputEventQueue, event, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

} // namespace MatrixOS::Input
