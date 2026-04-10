#include "MatrixOS.h"

namespace MatrixOS::LED
{
bool ledInited = false;
uint32_t ledCount = 0;

// static timer
StaticTimer_t ledTmdef;
TimerHandle_t ledTm;

SemaphoreHandle_t activeBufferSemaphore;
vector<Color*> frameBuffers; // 0 is the active layer
// Render to layer 0 will render directly to the active buffer without buffer swap operation. Very efficient for real time rendering.
// Otherwise, render to layer 255 (Top layer). Content will be updated on the next Update();
// If directly write to the active buffer, before NewLayer, CopyLayer(0, currentLayer) need to be called to resync the buffer.

vector<float> ledBrightnessMultiplier;
vector<uint8_t> ledPartitionBrightness;

bool needUpdate = false;

bool crossfadeActive = false;
uint32_t crossfadeStartTime = 0;
uint16_t crossfadeDuration = 0;
Color* crossfadeSourceBuffer = nullptr;
bool crossfadeDestroySourceBuffer = false;
Color* crossfadeBuffer = nullptr;

void RenderCrossfade();

static Color* AllocateColorBuffer(const char* errorMessage) {
  Color* buffer = (Color*)pvPortMalloc(ledCount * sizeof(Color));
  if (buffer == nullptr)
  {
    MatrixOS::SYS::ErrorHandler(errorMessage);
  }
  return buffer;
}

IRAM_ATTR void LEDTimerCallback(TimerHandle_t xTimer) {
  xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
  if (crossfadeActive)
  {
    RenderCrossfade();
  }

  if (needUpdate)
  {
    // MLOGD("LED", "Update");
    needUpdate = false;

    // MLOGD("LED", "Update (Brightness size: %d)", ledPartitionBrightness.size());
    Device::LED::Update(crossfadeActive ? crossfadeBuffer : frameBuffers[0], ledPartitionBrightness);
  }
  xSemaphoreGive(activeBufferSemaphore);
}

void UpdateBrightness() {
  for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
  {
    float brightnessMultiplied = ledBrightnessMultiplier[i] * MatrixOS::UserVar::brightness;

    if (brightnessMultiplied >= 255.0)
    {
      ledPartitionBrightness[i] = 255;
    }
    else if (brightnessMultiplied <= 0.0)
    {
      ledPartitionBrightness[i] = 0;
    }
    else
    {
      ledPartitionBrightness[i] = (uint8_t)brightnessMultiplied;
    }

    // MLOGD("LED", "Partition %s Brightness %d (%d * %f = %f)", Device::LED::partitions[i].name.c_str(), ledPartitionBrightness[i],
    // MatrixOS::UserVar::brightness, ledBrightnessMultiplier[i], brightnessMultiplied);
  }

  needUpdate = true;
}

void Reset() {
  if (activeBufferSemaphore == nullptr)
  {
    return;
  }

  xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);

  crossfadeActive = false;
  crossfadeStartTime = 0;
  crossfadeDuration = 0;

  if (crossfadeDestroySourceBuffer && crossfadeSourceBuffer != nullptr)
  {
    vPortFree(crossfadeSourceBuffer);
  }
  crossfadeSourceBuffer = nullptr;
  crossfadeDestroySourceBuffer = false;

  if (crossfadeBuffer != nullptr)
  {
    vPortFree(crossfadeBuffer);
    crossfadeBuffer = nullptr;
  }

  for (Color* buffer : frameBuffers)
  {
    if (buffer)
    {
      vPortFree(buffer);
    }
  }

  frameBuffers.clear();

  CreateLayer(0); // Create Layer 0 - The active layer
  CreateLayer(0); // Create Layer 1 - The base layer
  xSemaphoreGive(activeBufferSemaphore);
}

void Init() {

  if (ledInited == false)
  {
    // Generate brightness level map
    ledBrightnessMultiplier.resize(Device::LED::partitions.size());
    ledPartitionBrightness.resize(Device::LED::partitions.size());

    ledCount = 0;
    for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
    {
      MLOGD("LED", "Partition#%d %s Size %d", i, Device::LED::partitions[i].name.c_str(), Device::LED::partitions[i].size);
      ledBrightnessMultiplier[i] = Device::LED::partitions[i].default_multiplier;
      MatrixOS::NVS::GetVariable(StringHash("system_led_brightness_multiplier_" + Device::LED::partitions[i].name),
                                 &ledBrightnessMultiplier[i], sizeof(float));
      ledCount += Device::LED::partitions[i].size;
    }

    UpdateBrightness();
  }

  if (activeBufferSemaphore == nullptr)
  {
    activeBufferSemaphore = xSemaphoreCreateMutex();
  }

  Reset();

  if (ledInited == false)
  {
    ledTm = xTimerCreateStatic("LED Timer", 1000 / Device::LED::fps, pdTRUE, (void*)0, LEDTimerCallback, &ledTmdef);
    xTimerStart(ledTm, 0);
  }

  ledInited = true;
}

void NextBrightness() {
  MLOGD("LED", "Next Brightness");
  MLOGD("LED", "Current Brightness %d", UserVar::brightness.value);
  for (uint8_t newBrightness : Device::LED::brightnessLevel)
  {
    MLOGD("LED", "Check Brightness Level  %d", newBrightness);
    if (newBrightness > UserVar::brightness)
    {
      MLOGD("LED", "Brightness Level Selected");
      SetBrightness(newBrightness);
      return;
    }
  }
  MLOGD("LED", "Lowest Level Selected");
  SetBrightness(Device::LED::brightnessLevel[0]);
}

void SetBrightness(uint8_t brightness) {
  MLOGD("LED", "Set Brightness %d", brightness);
  MatrixOS::UserVar::brightness.Set(brightness);
  UpdateBrightness();
}

bool SetBrightnessMultiplier(string partitionName, float multiplier) {
  for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
  {
    if (Device::LED::partitions[i].name == partitionName)
    {

      if (i == 0 && multiplier < 0.1)
      {
        MLOGW("LED", "Main Partition Multiplier can not be less than 0.1");
        return false;
      }
      ledBrightnessMultiplier[i] = multiplier;
      MatrixOS::NVS::SetVariable(StringHash("system_led_brightness_multiplier_" + partitionName), &multiplier, sizeof(float));
      UpdateBrightness();
      return true;
    }
  }
  MLOGW("LED", "Partition Not Found");
  return false;
}

void SetColor(Point xy, Color color, uint8_t layer) {
  if (layer == 255)
  {
    layer = CurrentLayer();
  }
  else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    return;
  }

  uint16_t index = Device::LED::XY2Index(xy);
  // MLOGI("LED", "Set Color #%.2X%.2X%.2X to %d %d at Layer %d (index %d)", color.R, color.G, color.B, xy.x, xy.y, layer, index);
  if (index == UINT16_MAX)
    return;

  frameBuffers[layer][index] = color;

  if (layer == 0)
  {
    needUpdate = true;
  }
}

void SetColor(uint16_t ID, Color color, uint8_t layer) {
  if (layer == 255)
  {
    layer = CurrentLayer();
  }
  else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    return;
  }

  uint16_t index = Device::LED::ID2Index(ID);
  if (index == UINT16_MAX)
    return;

  frameBuffers[layer][index] = color;

  if (layer == 0)
  {
    needUpdate = true;
  }
}

void Fill(Color color, uint8_t layer) {
  if (layer == 255)
  {
    layer = CurrentLayer();
  }
  else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    return;
  }

  vTaskSuspendAll();
  // MLOGV("LED", "Fill Layer %d", layer);

  uint16_t start = 0;
  uint16_t end = ledCount;

  for (uint16_t index = start; index < end; index++)
  {
    frameBuffers[layer][index] = color;
  }

  xTaskResumeAll();

  if (layer == 0)
  {
    needUpdate = true;
  }
}

bool FillPartition(string partition, Color color, uint8_t layer) {

  if (partition.empty())
  {
    Fill(color, layer);
    return true;
  }

  if (layer == 255)
  {
    layer = CurrentLayer();
  }
  else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    return false;
  }

  vTaskSuspendAll();
  // MLOGV("LED", "Fill Layer %d", layer);

  uint16_t start = 0;
  uint16_t end = 0;

  for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
  {
    if (Device::LED::partitions[i].name == partition)
    {
      start = Device::LED::partitions[i].start;
      end = start + Device::LED::partitions[i].size;
      break;
    }
  }

  if (end == 0)
  {
    xTaskResumeAll();
    MLOGV("LED", "Partition Not Found");
    return false;
  }

  std::fill(frameBuffers[layer] + start, frameBuffers[layer] + end, color);

  xTaskResumeAll();

  if (layer == 0)
  {
    needUpdate = true;
  }

  return true;
}

int8_t CurrentLayer() {
  return frameBuffers.size() - 1;
}

int8_t CreateLayer(uint16_t crossfade) {
  int8_t oldLayer = CurrentLayer();
  if (oldLayer >= MAX_LED_LAYERS)
  {
    MatrixOS::SYS::ErrorHandler("Max LED Layer Exceded");
    return -1;
  }
  Color* frameBuffer = AllocateColorBuffer("Failed to allocate new led buffer");
  if (frameBuffer == nullptr)
  {
    return -1;
  }
  frameBuffers.push_back(frameBuffer);
  int8_t newLayer = CurrentLayer();
  Fill(0, newLayer);
  MLOGD("LED Layer", "Layer Created - %d", newLayer);

  if (crossfade)
  {
    Fade(crossfade, frameBuffers[oldLayer]);
  }

  return newLayer;
}

bool DestroyLayer(uint16_t crossfade) {
  if (frameBuffers.size() > 2)
  {
    if (crossfade)
    {
      Fade(crossfade);
    }

    vPortFree(frameBuffers.back());
    frameBuffers.pop_back();
    Update();

    MLOGD("LED Layer", "Layer Destoried - %d", frameBuffers.size());
    return true;
  }
  else
  {
    if (crossfade)
    {
      Fade(crossfade);
    }
    Fill(0, CurrentLayer());
    MLOGW("LED Layer", "Already at layer 1, can not delete layer");
    return false;
  }
  return false;
}

void CopyLayer(uint8_t dest, uint8_t src) {
  memcpy((void*)frameBuffers[dest], (void*)frameBuffers[src], ledCount * sizeof(Color));
}

void Update(uint8_t layer) {
  if (layer == 255)
  {
    layer = CurrentLayer();
  }
  else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    return;
  }

  xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
  CopyLayer(0, layer);
  needUpdate = true;
  xSemaphoreGive(activeBufferSemaphore);
}

void Fade(uint16_t crossfade, Color* sourceBuffer) {
  if (!UserVar::uiAnimation)
  {
    return;
  }

  // MLOGD("LED", "Fade %d", crossfade);

  uint16_t crossfadeDelay = 1000 / Device::LED::fps; // Delay by one frame

  if (crossfade == 0) // Stop crossfade
  {
    crossfadeStartTime = 0;
    return;
  }

  Color* nextCrossfadeBuffer = nullptr;
  if (crossfadeActive)
  {
    nextCrossfadeBuffer = AllocateColorBuffer("Failed to allocate crossfade buffer");
    if (nextCrossfadeBuffer == nullptr)
    {
      return;
    }
  }
  else if (crossfadeBuffer == nullptr)
  {
    crossfadeBuffer = AllocateColorBuffer("Failed to allocate crossfade buffer");
    if (crossfadeBuffer == nullptr)
    {
      return;
    }
  }

  if (crossfadeActive)
  {
    crossfadeActive = false;
    xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);

    // Crossfade already active
    // MLOGW("LED", "Crossfade already active");

    // If existing crossfade require to delete the source buffer, then delete it right now
    if (crossfadeDestroySourceBuffer)
    {
      vPortFree(crossfadeSourceBuffer);
    }

    crossfadeSourceBuffer = crossfadeBuffer; // Start crossfade from the crossfade buffer
    crossfadeBuffer = nextCrossfadeBuffer;
    crossfadeDestroySourceBuffer = true;
    xSemaphoreGive(activeBufferSemaphore);
  }
  else if (sourceBuffer == nullptr)
  {
    // Create a copy of the current buffer
    crossfadeSourceBuffer = AllocateColorBuffer("Failed to allocate crossfade buffer");
    if (crossfadeSourceBuffer == nullptr)
    {
      return;
    }
    memcpy((void*)crossfadeSourceBuffer, (void*)frameBuffers[0], ledCount * sizeof(Color));
    crossfadeDestroySourceBuffer = true;
  }
  else
  {
    // This is used in the case of creating a new layer
    // No New buffer is created, the source buffer is used directly
    crossfadeSourceBuffer = sourceBuffer;
    crossfadeDestroySourceBuffer = false;
  }

  crossfadeStartTime = MatrixOS::SYS::Millis() + crossfadeDelay;
  crossfadeDuration = crossfade;
  crossfadeActive = true;
}

// If any layer is 0, it will be show up as blackï¼ˆor lightless)
// If layer 2 is 255, it will be using the top layer
IRAM_ATTR void RenderCrossfade() {
  Fract16 ratio = 0;

  uint32_t currentTime = MatrixOS::SYS::Millis();

  if (currentTime <= crossfadeStartTime)
  {
    ratio = 0;
  }
  else if (currentTime < crossfadeStartTime + crossfadeDuration)
  {
    ratio = (currentTime - crossfadeStartTime) * FRACT16_MAX / crossfadeDuration;
  }
  else
  {
    ratio = FRACT16_MAX;
  }

  // MLOGD("LED", "Crossfade %d %d %d", currentTime - crossfadeStartTime, crossfadeDuration, ratio);

  if (ratio < FRACT16_MAX)
  {
    for (uint16_t index = 0; index < ledCount; index++)
    {
      crossfadeBuffer[index] =
          Color::Crossfade(crossfadeSourceBuffer == nullptr ? Color(0) : crossfadeSourceBuffer[index], frameBuffers[0][index], ratio);
    }
  }
  else if (ratio == FRACT16_MAX)
  {
    if (crossfadeDestroySourceBuffer)
    {
      vPortFree(crossfadeSourceBuffer);
    }
    crossfadeSourceBuffer = nullptr;
    crossfadeDestroySourceBuffer = false;
    vPortFree(crossfadeBuffer);
    crossfadeBuffer = nullptr;
    crossfadeActive = false;
    // MLOGD("LED", "Crossfade Done");
  }

  needUpdate = true;
}

void PauseUpdate(bool pause) {
  if (pause)
  {
    xTimerStop(ledTm, 0);
  }
  else
  {
    xTimerStart(ledTm, 0);
  }
}

uint32_t GetLEDCount(void) {
  return ledCount;
}
} // namespace MatrixOS::LED
