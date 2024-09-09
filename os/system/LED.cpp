#include "MatrixOS.h"

namespace MatrixOS::LED
{
  // static timer
  StaticTimer_t led_tmdef;
  TimerHandle_t led_tm;

  SemaphoreHandle_t activeBufferSemaphore;
  vector<Color*> frameBuffers; //0 is the active layer
  // Render to layer 0 will render directly to the active buffer without buffer swap operation. Very efficent for real time rendering.
  // Otherwise, render to layer 255 (Top layer). Content will be updated on the next Update();
  // If directly write to the active buffer, before NewLayer, CopyLayer(0, currentLayer) need to be called to resync the buffer.

  vector<float> ledBrightnessMultiplyer;
  vector<uint8_t> ledPartitionBrightness;

  bool needUpdate = false;

  void LEDTimerCallback(TimerHandle_t xTimer) {
    if (needUpdate)
    {
      xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
      // MLOGD("LED", "Update");
      needUpdate = false;
      // MLOGD("LED", "Update (Brightness size: %d)", ledPartitionBrightness.size());
      Device::LED::Update(frameBuffers[0], ledPartitionBrightness);
      xSemaphoreGive(activeBufferSemaphore);
    }
  }

  void UpdateBrightness() {
    for (uint8_t i = 0; i < Device::led_partitions.size(); i++)
    {
      float brightness_multiplied = ledBrightnessMultiplyer[i] * MatrixOS::UserVar::brightness;

      if (brightness_multiplied >= 255.0)
      { 
        ledPartitionBrightness[i] = 255; 
      }
      else if (brightness_multiplied <= 0.0)
      { 
        ledPartitionBrightness[i] = 0; 
      }
      else
      {
        ledPartitionBrightness[i] = (uint8_t)brightness_multiplied;
      }

      // MLOGD("LED", "Partition %s Brightness %d (%d * %f = %f)", Device::led_partitions[i].name.c_str(), ledPartitionBrightness[i], MatrixOS::UserVar::brightness, ledBrightnessMultiplyer[i], brightness_multiplied);
    }
  }

  void Init() {
    if(activeBufferSemaphore)
    {
      vSemaphoreDelete(activeBufferSemaphore);
    }

    if(led_tm)
    {
      xTimerDelete(led_tm, 0);
    }

    frameBuffers.clear();
    
    // Generate brightness level map
    ledBrightnessMultiplyer.resize(Device::led_partitions.size());
    ledPartitionBrightness.resize(Device::led_partitions.size());
    for (uint8_t i = 0; i < Device::led_partitions.size(); i++)
    {
      ledBrightnessMultiplyer[i] = Device::led_partitions[i].default_multiplier;
      MatrixOS::NVS::GetVariable(Hash("system_led_brightness_multiplyer_" + Device::led_partitions[i].name), &ledBrightnessMultiplyer[i], sizeof(float));
    }

    UpdateBrightness();

    activeBufferSemaphore = xSemaphoreCreateMutex();
    CreateLayer(); //Create Layer 0 - The active layer
    CreateLayer(); //Create Layer 1 - Swap Layer 1
    led_tm = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::fps, true, NULL, LEDTimerCallback, &led_tmdef);
    xTimerStart(led_tm, 0);
  }


  void NextBrightness() {
    MLOGD("LED", "Next Brightness");
    MLOGD("LED", "Current Brightness %d", UserVar::brightness.value);
    for (uint8_t new_brightness : Device::led_brightness_level)
    {
      MLOGD("LED", "Check Brightness Level  %d", new_brightness);
      if (new_brightness > UserVar::brightness)
      {
        MLOGD("LED", "Brightness Level Selected");
        SetBrightness(new_brightness);
        return;
      }
    }
    MLOGD("LED", "Lowest Level Selected");
    SetBrightness(Device::led_brightness_level[0]);
  }

  void SetBrightness(uint8_t brightness) {
    MLOGD("LED", "Set Brightness %d", brightness);
    MatrixOS::UserVar::brightness.Set(brightness);
    UpdateBrightness();
  }

  void SetBrightnessMultiplier(string partiton_name, float multiplier) {
    for (uint8_t i = 0; i < Device::led_partitions.size(); i++)
    {
      if (Device::led_partitions[i].name == partiton_name)
      {

        if (i == 0 && multiplier < 0.1)
        {
          MLOGW("LED", "Main Partition Multiplier can not be less than 0.1");
          return;
        }
        ledBrightnessMultiplyer[i] = multiplier;
        MatrixOS::NVS::SetVariable(Hash("system_led_brightness_multiplyer_" + partiton_name), &multiplier, sizeof(float));
        UpdateBrightness();
        return;
      }
    }
    MLOGW("LED", "Partition Not Found");
  }

  void SetColor(Point xy, Color color, uint8_t layer) {
    if (layer == 255)
    { layer = CurrentLayer(); }
    else if (layer > CurrentLayer())
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }
    // MLOGV("LED", "Set Color #%.2X%.2X%.2X to %d %d at Layer %d", color.R, color.G, color.B, xy.x, xy.y, layer);
    xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
    uint16_t index = Device::LED::XY2Index(xy);
    if (index == UINT16_MAX)return;

    frameBuffers[layer][index] = color;

    if(layer == 0)
    { needUpdate = true; }
  }

  void SetColor(uint16_t ID, Color color, uint8_t layer) {
    if (layer == 255)
    { layer = CurrentLayer(); }
    else if (layer > CurrentLayer())
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }

    uint16_t index = Device::LED::ID2Index(ID);
    if (index == UINT16_MAX) return;
      
    frameBuffers[layer][index] = color;

    if(layer == 0)
    { needUpdate = true; }
  }

  void Fill(Color color, uint8_t layer) {
    if (layer == 255)
    { layer = CurrentLayer(); }
    else if (layer > CurrentLayer())
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }
    vTaskSuspendAll();
    // MLOGV("LED", "Fill Layer %d", layer);

    uint16_t start = 0;
    uint16_t end = Device::led_count;

    for (uint16_t index = start; index < end; index++)
    { frameBuffers[layer][index] = color; }

    xTaskResumeAll();

    if(layer == 0)
    { needUpdate = true; }
  }

  void FillPartition(string partition, Color color, uint8_t layer)
  {

    if(partition.empty())
    {
      Fill(color, layer);
      return;
    }

    if (layer == 255)
    { layer = CurrentLayer(); }
    else if (layer > CurrentLayer())
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }
    vTaskSuspendAll();
    // MLOGV("LED", "Fill Layer %d", layer);

    uint16_t start = 0;
    uint16_t end = 0;

    for (uint8_t i = 0; i < Device::led_partitions.size(); i++)
    {
      if (Device::led_partitions[i].name == partition)
      {
        start = Device::led_partitions[i].start;
        end = start + Device::led_partitions[i].size;
        break;
      }
    }

    if (end == 0)
    {
      xTaskResumeAll();
      MLOGW("LED", "Partition Not Found");
      return;
    }

    for (uint16_t index = start; index < end; index++)
    { frameBuffers[layer][index] = color; }

    xTaskResumeAll();

    if(layer == 0)
    { needUpdate = true; }
  }

  void CopyLayer(uint8_t dest, uint8_t src)
  {
    memcpy((void*)frameBuffers[dest], (void*)frameBuffers[src], Device::led_count * sizeof(Color));
  }

  void Update(uint8_t layer) {
    if (layer == 255 || layer == CurrentLayer())
    {
      xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
      Color* swapBufferPtr = frameBuffers[0];
      frameBuffers[0] = frameBuffers.back();
      frameBuffers.back() = swapBufferPtr;
      CopyLayer(CurrentLayer(), 0);
      needUpdate = true;
      xSemaphoreGive(activeBufferSemaphore);
    }
  }


  // If any layer is 0, it will be show up as black（or lightless)
  // If layer 2 is 255, it will be using the top layer
  void Crossfade(uint8_t layer1, uint8_t layer2, Fract16 ratio) {
    if (layer2 == 255) { layer2 = CurrentLayer(); }

    if (layer1 > CurrentLayer() || layer2 > CurrentLayer())
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }

    if (ratio == 0)
    {
      Update(layer1);
      return;
    }
    else if (ratio == FRACT16_MAX)
    {
      Update(layer2);
      return;
    }

    xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
    for (uint16_t index = 0; index < Device::led_count; index++)
    {
      Color sourceColor = layer1 == 0 ? Color(0) : frameBuffers[layer1][index];
      Color targetColor = layer2 == 0 ? Color(0) : frameBuffers[layer2][index];
      
      frameBuffers[0][index] = Color::Crossfade(sourceColor, targetColor, ratio);
    }
    needUpdate = true;
    xSemaphoreGive(activeBufferSemaphore);
  }

  int8_t CurrentLayer() {
    return frameBuffers.size() - 1;
  }

  int8_t CreateLayer() {
    if (CurrentLayer() >= MAX_LED_LAYERS - 1)
    {
      MatrixOS::SYS::ErrorHandler("Max LED Layer Exceded");
      return -1;
    }
    Color* frameBuffer = (Color*)pvPortMalloc(Device::led_count * sizeof(Color));
    if (frameBuffer == nullptr)
    {
      MatrixOS::SYS::ErrorHandler("Failed to allocate new led buffer");
      return -1;
    }
    frameBuffers.push_back(frameBuffer);
    Fill(0, CurrentLayer());
    MLOGD("LED Layer", "Layer Created - %d", CurrentLayer());
    return CurrentLayer();
  }

  bool DestroyLayer() {
    if (CurrentLayer() > 1)
    {
      vPortFree(frameBuffers.back());
      frameBuffers.pop_back();
      MLOGD("LED Layer", "Layer Destoried - %d", CurrentLayer());
      Update();
      return true;
    }
    else
    {
      Fill(0, 1);
      Update();
      MLOGD("LED Layer", "Already at layer 1, can not delete layer");
      return false;
    }
  }

  void ShiftCanvas(EDirection direction, int8_t distance, uint8_t layer) {
    // Color[] tempBuffer;
  }

  void RotateCanvas(EDirection direction, uint8_t layer) {
    // TODO
  }

  void PauseUpdate(bool pause) {
    if (pause)
    { xTimerStop(led_tm, 0); }
    else
    { xTimerStart(led_tm, 0); }
  }
}