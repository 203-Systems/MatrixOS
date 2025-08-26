#include "Shell.h"


namespace MatrixOS::SYS
{
  void ExecuteAPP(uint32_t app_id);
  uint16_t GetApplicationCount();
}  // Use non exposed Matrix OS API

void Shell::Setup()
{
  #ifdef configUSE_FREERTOS_PROVIDED_HEAP
    MLOGD("Shell", "Matrix OS Free Heap Size: %.2fkb (%d%%)", xPortGetFreeHeapSize() / 1024.0f, xPortGetFreeHeapSize() * 100 / configTOTAL_HEAP_SIZE);
    MLOGD("Shell", "Matrix OS Minimum Free Heap Size: %.2fkb (%d%%)", xPortGetMinimumEverFreeHeapSize() / 1024.0f, xPortGetMinimumEverFreeHeapSize() * 100 / configTOTAL_HEAP_SIZE);
  #endif
  #ifdef ESP_PLATFORM
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    uint32_t total_heap_size = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);  
    MLOGD("Shell", "Matrix OS Free Heap Size: %d (%.2fkb) (%d%%)", info.total_free_bytes, info.total_free_bytes / 1024.0f, info.total_free_bytes * 100 / total_heap_size); 
    MLOGD("Shell", "Matrix OS Lifetime Minimum Free Heap Size: %d (%.2fkb) (%d%%)", info.minimum_free_bytes, info.minimum_free_bytes / 1024.0f, info.minimum_free_bytes * 100 / total_heap_size);
    MLOGD("Shell", "Matrix OS Total Heap Size: %d (%.2fkb)", total_heap_size, total_heap_size / 1024.0f);
    MLOGD("Shell", "Matrix OS Free Blocks: %d", info.free_blocks);
    MLOGD("Shell", "Matrix OS Largest Free Block: %.2fkb", info.largest_free_block / 1024.0f);
    MLOGD("Shell", "Matrix OS Total Blocks: %d", info.total_blocks);
  #endif
}

void Shell::Loop() {
    ApplicationLauncher();
}

void Shell::ApplicationLauncher() {
  uint8_t tap_counter = 0;
  uint32_t last_tap = 0;

  UI applicationLauncher("Application Launcher", Color(0x00FFAA), false);

  applicationLauncher.disableExit = true;

  MLOGD("Shell", "%d apps detected", MatrixOS::SYS::GetApplicationCount());

  uint16_t visible_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    (void)app_id;
    if (application->visibility)
    { visible_app_count++; }
  }
  
  UIButton appBtns[visible_app_count];

  // Iterate though vector 
  uint8_t btnIndex = 0;
  for (const auto& [order_id, app_id]: application_ids)
  {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end())
    {
      continue;
    }
    Application_Info* application = application_it->second;

    if (application->visibility)
    {
      uint8_t x = btnIndex % 8;
      uint8_t y = btnIndex / 8;

      Color app_color = application->color;

      appBtns[btnIndex].SetName(application->name);
      appBtns[btnIndex].SetColor(app_color);
      appBtns[btnIndex].OnPress([&, app_id, x, y, app_color]() -> void {
        MLOGD("Shell", "Launching App ID: %d", app_id);
        LaunchAnimation(Point(x, y), app_color);
        MatrixOS::SYS::ExecuteAPP(app_id);
      });

      applicationLauncher.AddUIComponent(appBtns[btnIndex], Point(x, y));
      MLOGD("Shell", "App #%d [%d] %s-%s loaded.", btnIndex, order_id, application->author.c_str(),
                                  application->name.c_str());
      btnIndex++;
    }
    else
    { MLOGD("Shell", "%s not visible, skip.", application->name.c_str()); }
  }

  #if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG  // Logging Mode Indicator
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFBF00)
  #elif MATRIXOS_LOG_LEVEL == LOG_LEVEL_VERBOSE
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFF007F)
  #else
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFFFFF)
  #endif

  UIButton applicationLauncherBtn;
  applicationLauncherBtn.SetName("Application Launcher");
  applicationLauncherBtn.SetColor(Color(0x00FFAA));
  applicationLauncherBtn.OnPress([&]() -> void {
    // Tap on the application launcher button 10 times to show hidden apps
    if (MatrixOS::SYS::Millis() - last_tap < 1000)
    {
      tap_counter++;
    }

    last_tap = MatrixOS::SYS::Millis();

    // MLOGI("Hidden Launcher", "Tap %d", tap_counter);

    if (tap_counter >= 10)
    {
      tap_counter = 0;
      MLOGI("Hidden Launcher", "Enter");
      HiddenApplicationLauncher();
    }
  });
  applicationLauncher.AddUIComponent(applicationLauncherBtn, Point(0, 7));

  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(SHELL_SYSTEM_SETTING_COLOR);
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });

  applicationLauncher.AddUIComponent(systemSettingBtn, Point(7, 7));
  

  applicationLauncher.AllowExit(false);  // So nothing happens
  applicationLauncher.Start();
}

void Shell::HiddenApplicationLauncher() {
  UI hiddenApplicationLauncher("Hidden Application Launcher", Color(0xFFFFFF));

  uint16_t invisible_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    if (application->visibility == false)
    { invisible_app_count++; }
  }

  UIButton hiddenAppBtns[invisible_app_count];
  uint8_t btnIndex = 0;
  
  for (const auto& [order_id, app_id]: application_ids)
  {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end())
    {
      continue;
    }
    Application_Info* application = application_it->second;

    if (application->visibility == false)
    {
      uint8_t x = btnIndex % 8;
      uint8_t y = btnIndex / 8;

      Color app_color = application->color;

      hiddenAppBtns[btnIndex].SetName(application->name);
      hiddenAppBtns[btnIndex].SetColor(app_color);
      hiddenAppBtns[btnIndex].OnPress([&, app_id, x, y, app_color]() -> void {
        LaunchAnimation(Point(x, y), app_color);
        MatrixOS::SYS::ExecuteAPP(app_id);
      });
      hiddenApplicationLauncher.AddUIComponent(hiddenAppBtns[btnIndex], Point(x, y));
      MLOGD("Shell (invisible)", "App #%d [%d] %s-%s loaded.", btnIndex, order_id, application->author.c_str(),
                                  application->name.c_str());
      btnIndex++;
    }
    else
    { MLOGD("Shell", "%s visible, skip.", application->name.c_str()); }
  }
  hiddenApplicationLauncher.Start();
}

void Shell::LaunchAnimation(Point origin, Color color)
{
  if(!MatrixOS::UserVar::ui_animation) { return; }

  uint32_t startTime = MatrixOS::SYS::Millis();

  const float speed = 30; // base mills per pixel
  const float edgeInnerWidth = -8;
  const float edgeWidth = 1;
  const float edgeOuterWidth = 1;
  const float endDistance = 20;
  Timer animTimer;
  uint16_t frameTime = 1000 / Device::LED::fps;

  while(true)
  {
    if(!animTimer.Tick(frameTime)) { continue; }
    float r = (MatrixOS::SYS::Millis() - startTime) / speed - edgeWidth;

    if(r > endDistance) { break; }

    for (uint16_t i = 0; i < MatrixOS::LED::GetLedCount(); i++)
    {
      Point xy = Device::LED::Index2XY(i);

      if(!xy) { continue; }

      float distanceDiff = sqrt(pow(xy.x - origin.x, 2) + pow(xy.y - origin.y, 2)) - r;

      Color pixelColor = Color(0);
      if(distanceDiff > (edgeWidth + edgeOuterWidth)) // Outside of render range
      {
        continue; // Do not change existing color
      }
      else if(distanceDiff < (edgeInnerWidth - edgeWidth)) // Inside of render range
      {
        pixelColor = Color(0);
      }
      else if(abs(distanceDiff) <= edgeWidth) // In the edge width
      {
        pixelColor = color;
      }
      else if(distanceDiff > edgeWidth) // In the outer edge
      {

        pixelColor = color.Scale((1 - (distanceDiff - edgeWidth) / edgeOuterWidth) * 255);
      }
      else if(distanceDiff < -edgeWidth) // In the inner edge
      {
        pixelColor = color.Scale((1 - (distanceDiff + edgeWidth) / edgeInnerWidth) * 255);
      }
      MatrixOS::LED::SetColor(xy, pixelColor);
    }
    MatrixOS::LED::Update();
  }
  MatrixOS::LED::Fill(Color(0));
}