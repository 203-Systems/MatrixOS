#include "MatrixOS.h"

namespace MatrixOS::LED
{
  bool led_inited = false;
  uint32_t led_count = 0;

  // static timer
  StaticTimer_t led_tmdef;
  TimerHandle_t led_tm;

  SemaphoreHandle_t activeBufferSemaphore;
  vector<Color*> frameBuffers; //0 is the active layer
  // Render to layer 0 will render directly to the active buffer without buffer swap operation. Very efficient for real time rendering.
  // Otherwise, render to layer 255 (Top layer). Content will be updated on the next Update();
  // If directly write to the active buffer, before NewLayer, CopyLayer(0, currentLayer) need to be called to resync the buffer.

  vector<float> ledBrightnessMultiplier;
  vector<uint8_t> ledPartitionBrightness;

  bool needUpdate = false;

  bool crossfade_active = false;
  uint32_t crossfade_start_time = 0;
  uint16_t crossfade_duration = 0;
  Color* crossfade_source_buffer = nullptr;
  bool crossfade_destroy_source_buffer = false;
  Color* crossfade_buffer = nullptr;

  void RenderCrossfade();

  IRAM_ATTR void LEDTimerCallback(TimerHandle_t xTimer) {
    xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
    if(crossfade_active)
    {
      RenderCrossfade();
    }

    if (needUpdate)
    {
      // MLOGD("LED", "Update");
      needUpdate = false;

      // MLOGD("LED", "Update (Brightness size: %d)", ledPartitionBrightness.size());
      Device::LED::Update(crossfade_active ? crossfade_buffer : frameBuffers[0], ledPartitionBrightness);
    }
    xSemaphoreGive(activeBufferSemaphore);
  }

  void UpdateBrightness() {
    for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
    {
      float brightness_multiplied = ledBrightnessMultiplier[i] * MatrixOS::UserVar::brightness;

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

      // MLOGD("LED", "Partition %s Brightness %d (%d * %f = %f)", Device::LED::partitions[i].name.c_str(), ledPartitionBrightness[i], MatrixOS::UserVar::brightness, ledBrightnessMultiplier[i], brightness_multiplied);
    }
    
    needUpdate = true;
  }

  void Init() {

    if(led_inited == false)
    {
      // Generate brightness level map
      ledBrightnessMultiplier.resize(Device::LED::partitions.size());
      ledPartitionBrightness.resize(Device::LED::partitions.size());

      led_count = 0;
      for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
      {
        MLOGD("LED", "Partition#%d %s Size %d", i, Device::LED::partitions[i].name.c_str(), Device::LED::partitions[i].size);
        ledBrightnessMultiplier[i] = Device::LED::partitions[i].default_multiplier;
        MatrixOS::NVS::GetVariable(StringHash("system_led_brightness_multiplier_" + Device::LED::partitions[i].name), &ledBrightnessMultiplier[i], sizeof(float));
        led_count += Device::LED::partitions[i].size;
      }

      UpdateBrightness();

      activeBufferSemaphore = xSemaphoreCreateMutex();
    }

    for (Color* buffer : frameBuffers)
    {
      if(buffer)
      {
        vPortFree(buffer);
      }
    }

    frameBuffers.clear();
    

    CreateLayer(0); //Create Layer 0 - The active layer
    CreateLayer(0); //Create Layer 1 - The base layer

    if(led_inited == false)
    {
      led_tm = xTimerCreateStatic("LED Timer", 1000 / Device::LED::fps, pdTRUE, (void*)0, LEDTimerCallback, &led_tmdef);
      xTimerStart(led_tm, 0);
    }

    led_inited = true;
  }


  void NextBrightness() {
    MLOGD("LED", "Next Brightness");
    MLOGD("LED", "Current Brightness %d", UserVar::brightness.value);
    for (uint8_t new_brightness : Device::LED::brightness_level)
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
    SetBrightness(Device::LED::brightness_level[0]);
  }

  void SetBrightness(uint8_t brightness) {
    MLOGD("LED", "Set Brightness %d", brightness);
    MatrixOS::UserVar::brightness.Set(brightness);
    UpdateBrightness();
  }

  bool SetBrightnessMultiplier(string partition_name, float multiplier) {
    for (uint8_t i = 0; i < Device::LED::partitions.size(); i++)
    {
      if (Device::LED::partitions[i].name == partition_name)
      {

        if (i == 0 && multiplier < 0.1)
        {
          MLOGW("LED", "Main Partition Multiplier can not be less than 0.1");
          return false;
        }
        ledBrightnessMultiplier[i] = multiplier;
        MatrixOS::NVS::SetVariable(StringHash("system_led_brightness_multiplier_" + partition_name), &multiplier, sizeof(float));
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

    xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
    uint16_t index = Device::LED::XY2Index(xy);
    // MLOGI("LED", "Set Color #%.2X%.2X%.2X to %d %d at Layer %d (index %d)", color.R, color.G, color.B, xy.x, xy.y, layer, index);
    if (index == UINT16_MAX)return;

    frameBuffers[layer][index] = color;

    if(layer == 0)
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
    if (index == UINT16_MAX) return;
      
    frameBuffers[layer][index] = color;

    if(layer == 0)
    { needUpdate = true; }
  }

  void Fill(Color color, uint8_t layer) {
    if (layer == 255)
    { layer = CurrentLayer(); }
    else if (layer >= frameBuffers.size() || frameBuffers[layer] == nullptr)
    {
      MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
      return;
    }

    vTaskSuspendAll();
    // MLOGV("LED", "Fill Layer %d", layer);

    uint16_t start = 0;
    uint16_t end = led_count;

    for (uint16_t index = start; index < end; index++)
    { frameBuffers[layer][index] = color; }

    xTaskResumeAll();

    if(layer == 0)
    { needUpdate = true; }
  }

  bool FillPartition(string partition, Color color, uint8_t layer)
  {

    if(partition.empty())
    {
      Fill(color, layer);
      return true;
    }

    if (layer == 255)
    { layer = CurrentLayer(); }
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

    if(layer == 0)
    { needUpdate = true; }

    return true;
  }

   int8_t CurrentLayer() {
     return frameBuffers.size() - 1;
  }

  int8_t CreateLayer(uint16_t crossfade)
  {
    int8_t oldLayer = CurrentLayer();
    if (oldLayer >= MAX_LED_LAYERS)
    {
      MatrixOS::SYS::ErrorHandler("Max LED Layer Exceded");
      return -1;
    }
    Color* frameBuffer = (Color*)pvPortMalloc(led_count * sizeof(Color));
    if (frameBuffer == nullptr)
    {
      MatrixOS::SYS::ErrorHandler("Failed to allocate new led buffer");
      return -1;
    }
    frameBuffers.push_back(frameBuffer);
    int8_t newLayer = CurrentLayer();
    Fill(0, newLayer);
    MLOGD("LED Layer", "Layer Created - %d", newLayer);

    if(crossfade)
    {
      Fade(crossfade, frameBuffers[oldLayer]);
    }


    return newLayer;
  }

  bool DestroyLayer(uint16_t crossfade)
  {
    if (frameBuffers.size() > 2)
    {
      if(crossfade)
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
      if(crossfade)
      {
        Fade(crossfade);
      }
      Fill(0, CurrentLayer());
      MLOGW("LED Layer", "Already at layer 1, can not delete layer");
      return false;
    }
    return false;
  }

  void CopyLayer(uint8_t dest, uint8_t src)
  {
    memcpy((void*)frameBuffers[dest], (void*)frameBuffers[src], led_count * sizeof(Color));
  }


  void Update(uint8_t layer)
  {
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


  void Fade(uint16_t crossfade, Color* source_buffer)
  {
    if(!UserVar::ui_animation){return;}

    // MLOGD("LED", "Fade %d", crossfade);

    uint16_t crossfade_delay = 1000 / Device::LED::fps; // Delay by one frame
    
    if(crossfade == 0) // Stop crossfade
    {
      crossfade_start_time = 0; 
      return;
    }

    if(crossfade_active)
    {
      crossfade_active = false;
      xSemaphoreTake(activeBufferSemaphore, portMAX_DELAY);
    
      // Crossfade already active
      // MLOGW("LED", "Crossfade already active");

      // If existing crossfade require to delete the source buffer, then delete it right now
      if(crossfade_destroy_source_buffer) { vPortFree(crossfade_source_buffer); }

      crossfade_source_buffer = crossfade_buffer; // Start crossfade from the crossfade buffer
      crossfade_buffer = nullptr; // Let the RenderCrossfade() to create a new buffer
      crossfade_destroy_source_buffer = true;
      xSemaphoreGive(activeBufferSemaphore);

    }
    else if(source_buffer == nullptr)
    {
      // Create a copy of the current buffer
      crossfade_source_buffer = (Color*)pvPortMalloc(led_count * sizeof(Color));
      if(crossfade_source_buffer == nullptr)
      {
        MatrixOS::SYS::ErrorHandler("Failed to allocate crossfade buffer");
        return;
      }
      memcpy((void*)crossfade_source_buffer, (void*)frameBuffers[0], led_count * sizeof(Color));
      crossfade_destroy_source_buffer = true;
    }
    else
    { 
      // This is used in the case of creating a new layer
      // No New buffer is created, the source buffer is used directly
      crossfade_source_buffer = source_buffer;
      crossfade_destroy_source_buffer = false;
    }

    crossfade_start_time = MatrixOS::SYS::Millis() + crossfade_delay;
    crossfade_duration = crossfade;
    crossfade_active = true;
  }

  // If any layer is 0, it will be show up as black（or lightless)
  // If layer 2 is 255, it will be using the top layer
  IRAM_ATTR void RenderCrossfade() {
    Fract16 ratio = 0;

    uint32_t currentTime = MatrixOS::SYS::Millis();

    if(crossfade_buffer == nullptr)
    {
      crossfade_buffer = (Color*)pvPortMalloc(led_count * sizeof(Color));
    }

    if(currentTime <= crossfade_start_time)
    {
      ratio = 0;
    }
    else if(currentTime < crossfade_start_time + crossfade_duration)
    {
      ratio = (currentTime - crossfade_start_time) * FRACT16_MAX / crossfade_duration;
    }
    else
    {
      ratio = FRACT16_MAX;
    }

    // MLOGD("LED", "Crossfade %d %d %d", currentTime - crossfade_start_time, crossfade_duration, ratio);

    if(ratio < FRACT16_MAX)
    {
      for (uint16_t index = 0; index < led_count; index++)
      {
        crossfade_buffer[index] = Color::Crossfade(
              crossfade_source_buffer == nullptr ? Color(0) : crossfade_source_buffer[index], 
              frameBuffers[0][index],
              ratio
            );
      }
    }
    else if(ratio == FRACT16_MAX)
    {
      if(crossfade_destroy_source_buffer) { vPortFree(crossfade_source_buffer); }
      vPortFree(crossfade_buffer);
      crossfade_buffer = nullptr;
      crossfade_active = false;
      // MLOGD("LED", "Crossfade Done");
    }

    needUpdate = true;
  }

  void PauseUpdate(bool pause) {
    if (pause)
    { xTimerStop(led_tm, 0); }
    else
    { xTimerStart(led_tm, 0); }
  }

  uint32_t GetLEDCount(void) {
    return led_count;
  }
}