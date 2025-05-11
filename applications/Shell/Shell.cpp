#include "Shell.h"

void Shell::Setup()
{
  #ifdef configUSE_FREERTOS_PROVIDED_HEAP
  // HeapStats_t heapStats;
  // vPortGetHeapStats(&heapStats);
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
  switch (current_page)
  {
    case 0:
      ApplicationLauncher();
      break;
  }
}

std::vector<UIButton> commonBarBtns;  // Because the UI in the common bar need to exist after the function ends
uint8_t tap_counter = 0;
uint32_t last_tap = 0;

void Shell::AddCommonBarInUI(UI* ui) {
  commonBarBtns.clear();
  commonBarBtns.reserve(2);  // Make sure to change this after adding more stuffs, if vector content got relocated, UI
                             // will not be able to find it and will hardfault!
  commonBarBtns.push_back(UIButton());
  commonBarBtns.back().SetName("Application Launcher");
  commonBarBtns.back().SetColorFunc([&]() -> Color { return Color(0x00FFAA).DimIfNot(current_page == 0); });
  commonBarBtns.back().OnPress([&]() -> void {
    if (current_page != 0)
    {
      current_page = 0;
      ui->Exit();
    }
    else
    {
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
    }
  });
  ui->AddUIComponent(commonBarBtns.back(), Point(0, 7));

#if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG  // Logging Mode Indicator
#define SHELL_SYSTEM_SETTING_COLOR Color(0xFFBF00)
#elif MATRIXOS_LOG_LEVEL == LOG_LEVEL_VERBOSE
#define SHELL_SYSTEM_SETTING_COLOR Color(0xFF007F)
#else
#define SHELL_SYSTEM_SETTING_COLOR Color(0xFFFFFF)
#endif

  commonBarBtns.push_back(UIButton());

  commonBarBtns.back().SetName("System Setting");
  commonBarBtns.back().SetColor(SHELL_SYSTEM_SETTING_COLOR);
  commonBarBtns.back().OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });

  ui->AddUIComponent(commonBarBtns.back(), Point(7, 7));
  ui->AllowExit(false);  // So nothing happens
}

namespace MatrixOS::SYS
{
  void ExecuteAPP(uint32_t app_id);
  uint16_t GetApplicationCount();
}  // Use non exposed Matrix OS API

void Shell::ApplicationLauncher() {
  UI applicationLauncher("Application Launcher", Color(0x00FFAA), false);

  applicationLauncher.disableExit = true;
  AddCommonBarInUI(&applicationLauncher);

  MLOGD("Shell", "%d apps detected", MatrixOS::SYS::GetApplicationCount());

  uint16_t visible_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    (void)app_id;
    if (application->visibility)
    { visible_app_count++; }
  }

  appBtns.clear();
  appBtns.reserve(visible_app_count);

  // Iterate though vector 
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
      uint8_t x = appBtns.size() % 8;
      uint8_t y = appBtns.size() / 8;

      Color app_color = application->color;

      appBtns.push_back(UIButton());
      appBtns.back().SetName(application->name);
      appBtns.back().SetColor(app_color);
      appBtns.back().OnPress([&, app_id, x, y, app_color]() -> void {
        LaunchAnimation(Point(x, y), app_color);
        MatrixOS::SYS::ExecuteAPP(app_id);
      });

      applicationLauncher.AddUIComponent(appBtns.back(), Point(x, y));
      MLOGD("Shell", "App #%d [%d] %s-%s loaded.", appBtns.size() - 1, order_id, application->author.c_str(),
                                  application->name.c_str());
    }
    else
    { MLOGD("Shell", "%s not visible, skip.", application->name.c_str()); }
  }
  applicationLauncher.Start();
}

void Shell::HiddenApplicationLauncher() {
  UI hiddenApplicationLauncher("Hidden Application Launcher", Color(0xFFFFFF));

  uint16_t invisible_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    if (application->visibility)
    { invisible_app_count++; }
  }

  hiddenAppBtns.clear();
  hiddenAppBtns.reserve(invisible_app_count);
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
      uint8_t x = hiddenAppBtns.size() % 8;
      uint8_t y = hiddenAppBtns.size() / 8;

      Color app_color = application->color;

      hiddenAppBtns.push_back(UIButton());
      hiddenAppBtns.back().SetName(application->name);
      hiddenAppBtns.back().SetColor(app_color);
      hiddenAppBtns.back().OnPress([&, app_id, x, y, app_color]() -> void {
        LaunchAnimation(Point(x, y), app_color);
        MatrixOS::SYS::ExecuteAPP(app_id);
      });
      hiddenApplicationLauncher.AddUIComponent(hiddenAppBtns.back(), Point(x, y));
      MLOGD("Shell (invisible)", "App #%d [%d] %s-%s loaded.", hiddenAppBtns.size() - 1, order_id, application->author.c_str(),
                                  application->name.c_str());
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

    for (uint8_t i = 0; i < Device::led_count; i++)
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