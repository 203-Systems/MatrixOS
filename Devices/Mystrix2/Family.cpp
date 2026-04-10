#include "Device.h"
#include "MatrixOS.h"
#include "UI/UI.h"

#include "esp_private/system_internal.h" // For esp_reset_reason_set_hint
#include "esp_timer.h"                   // esp_timer_get_time

#include "esp_efuse.h"
#include "esp_efuse_table.h"

namespace Device
{
Direction rotation = TOP;

namespace KeyPad
{
void Scan();
}

void DeviceInit() {
  MLOGI("Mystrix2", "DeviceInit: LoadDeviceInfo start");
  LoadDeviceInfo();
  MLOGI("Mystrix2", "DeviceInit: LoadDeviceInfo done");
  MLOGI("Mystrix2", "DeviceInit: USB init start");
  USB::Init();
  MLOGI("Mystrix2", "DeviceInit: USB init done");
  MLOGI("Mystrix2", "DeviceInit: NVS init start");
  NVS::Init();
  MLOGI("Mystrix2", "DeviceInit: NVS init done");
  MLOGI("Mystrix2", "DeviceInit: LED init start");
  LED::Init();
  MLOGI("Mystrix2", "DeviceInit: LED init done");
  MLOGI("Mystrix2", "DeviceInit: KeyPad init start");
  KeyPad::Init();
  MLOGI("Mystrix2", "DeviceInit: KeyPad init done");

  MLOGI("Mystrix2", "DeviceInit: Storage init start");
  Storage::Init();
  MLOGI("Mystrix2", "DeviceInit: Storage init done");

  MLOGI("Mystrix2", "DeviceInit: BLEMIDI init start");
  BLEMIDI::Init(name);
  MLOGI("Mystrix2", "DeviceInit: BLEMIDI init done");
}

void RegisterInputClusters(); // forward declaration

namespace Input
{
vector<InputCluster> clusters;

static const InputCluster* FindCluster(uint8_t clusterId) {
  for (const auto& c : clusters)
  {
    if (c.clusterId == clusterId) return &c;
  }
  return nullptr;
}

// Grid2D mapping handler
static bool Grid2D_GetPosition(const InputCluster& cluster, uint16_t memberId, Point* point) {
  if (memberId >= cluster.inputCount) return false;
  int16_t localX = memberId % cluster.dimension.x;
  int16_t localY = memberId / cluster.dimension.x;
  Point hwPoint(cluster.rootPoint.x + localX, cluster.rootPoint.y + localY);
  if (cluster.rotation != TOP)
  {
    hwPoint = hwPoint.Rotate(cluster.rotation, Point(cluster.rotationDimension.x, cluster.rotationDimension.y));
  }
  *point = hwPoint;
  return true;
}

static bool Grid2D_TryGetMemberId(const InputCluster& cluster, Point point, uint16_t* memberId) {
  Point hwPoint = point;
  if (cluster.rotation != TOP)
  {
    hwPoint = point.Rotate(cluster.rotation, Point(cluster.rotationDimension.x, cluster.rotationDimension.y), true);
  }
  if (!cluster.Contains(hwPoint)) return false;
  Point local = hwPoint - cluster.rootPoint;
  uint16_t id = static_cast<uint16_t>(local.y) * cluster.dimension.x + local.x;
  if (id >= cluster.inputCount) return false;
  *memberId = id;
  return true;
}

// Linear1D mapping handler
static bool Linear1D_GetPosition(const InputCluster& cluster, uint16_t memberId, Point* point) {
  if (memberId >= cluster.inputCount) return false;
  Point hwPoint;
  if (cluster.dimension.x > 1)
  {
    hwPoint = Point(cluster.rootPoint.x + memberId, cluster.rootPoint.y);
  }
  else
  {
    hwPoint = Point(cluster.rootPoint.x, cluster.rootPoint.y + memberId);
  }
  if (cluster.rotation != TOP)
  {
    hwPoint = hwPoint.Rotate(cluster.rotation, Point(cluster.rotationDimension.x, cluster.rotationDimension.y));
  }
  *point = hwPoint;
  return true;
}

static bool Linear1D_TryGetMemberId(const InputCluster& cluster, Point point, uint16_t* memberId) {
  Point hwPoint = point;
  if (cluster.rotation != TOP)
  {
    hwPoint = point.Rotate(cluster.rotation, Point(cluster.rotationDimension.x, cluster.rotationDimension.y), true);
  }
  if (!cluster.Contains(hwPoint)) return false;
  Point local = hwPoint - cluster.rootPoint;
  uint16_t id = (cluster.dimension.x > 1) ? local.x : local.y;
  if (id >= cluster.inputCount) return false;
  *memberId = id;
  return true;
}

bool GetPosition(uint8_t clusterId, uint16_t memberId, Point* point) {
  const InputCluster* cluster = FindCluster(clusterId);
  if (!cluster || !cluster->HasCoordinates()) return false;
  if (!cluster->getPosition) return false;
  return cluster->getPosition(*cluster, memberId, point);
}

bool TryGetMemberId(uint8_t clusterId, Point point, uint16_t* memberId) {
  const InputCluster* cluster = FindCluster(clusterId);
  if (!cluster || !cluster->HasCoordinates()) return false;
  if (!cluster->tryGetMemberId) return false;
  return cluster->tryGetMemberId(*cluster, point, memberId);
}
} // namespace Input

void Rotate(Direction newRotation, bool absolute) {
  if (newRotation != 0 && newRotation != 90 && newRotation != 180 && newRotation != 270)
  {
    return;
  }
  if (newRotation == 0 && !absolute)
  {
    return;
  }

  // Clear LED layers
  for (uint8_t ledLayer = 0; ledLayer <= MatrixOS::LED::CurrentLayer(); ledLayer++)
  {
    MatrixOS::LED::Fill(0, ledLayer);
  }

  // Update device-owned rotation state and persist
  rotation = (Direction)((rotation * !absolute + newRotation) % 360);
  MatrixOS::UserVar::rotation = rotation;

  // Rebuild input clusters with new rotation
  RegisterInputClusters();

  // Clear stale input events and suppress active device-side inputs
  MatrixOS::Input::ClearInputBuffer();
  Device::Input::SuppressActiveInputs();
}

void RegisterInputClusters() {
  Dimension rotDim = Dimension(X_SIZE, Y_SIZE);

  Input::clusters.clear();

  // Cluster 0: System (FN button) — no coordinates, no rotation
  InputCluster fnCluster;
  fnCluster.clusterId = 0;
  fnCluster.name = "System";
  fnCluster.inputClass = InputClass::Keypad;
  fnCluster.shape = InputClusterShape::Scalar;
  fnCluster.rootPoint = Point::Invalid();
  fnCluster.dimension = Dimension(1, 1);
  fnCluster.inputCount = 1;
  Input::clusters.push_back(fnCluster);

  // Cluster 1: Main 8x8 Grid
  InputCluster gridCluster;
  gridCluster.clusterId = 1;
  gridCluster.name = "Grid";
  gridCluster.inputClass = InputClass::Keypad;
  gridCluster.shape = InputClusterShape::Grid2D;
  gridCluster.rootPoint = Point(0, 0);
  gridCluster.dimension = Dimension(X_SIZE, Y_SIZE);
  gridCluster.inputCount = X_SIZE * Y_SIZE;
  gridCluster.rotation = rotation;
  gridCluster.rotationDimension = rotDim;
  gridCluster.getPosition = Input::Grid2D_GetPosition;
  gridCluster.tryGetMemberId = Input::Grid2D_TryGetMemberId;
  Input::clusters.push_back(gridCluster);

  // Cluster 2: TouchBar Left (8 keys along left edge, x = -1 in hardware space)
  InputCluster touchbarLeftCluster;
  touchbarLeftCluster.clusterId = 2;
  touchbarLeftCluster.name = "TouchBarLeft";
  touchbarLeftCluster.inputClass = InputClass::Keypad;
  touchbarLeftCluster.shape = InputClusterShape::Linear1D;
  touchbarLeftCluster.rootPoint = Point(-1, 0);
  touchbarLeftCluster.dimension = Dimension(1, Y_SIZE);
  touchbarLeftCluster.inputCount = TOUCHBAR_SIZE / 2;
  touchbarLeftCluster.rotation = rotation;
  touchbarLeftCluster.rotationDimension = rotDim;
  touchbarLeftCluster.getPosition = Input::Linear1D_GetPosition;
  touchbarLeftCluster.tryGetMemberId = Input::Linear1D_TryGetMemberId;
  Input::clusters.push_back(touchbarLeftCluster);

  // Cluster 3: TouchBar Right (8 keys along right edge, x = X_SIZE in hardware space)
  InputCluster touchbarRightCluster;
  touchbarRightCluster.clusterId = 3;
  touchbarRightCluster.name = "TouchBarRight";
  touchbarRightCluster.inputClass = InputClass::Keypad;
  touchbarRightCluster.shape = InputClusterShape::Linear1D;
  touchbarRightCluster.rootPoint = Point(X_SIZE, 0);
  touchbarRightCluster.dimension = Dimension(1, Y_SIZE);
  touchbarRightCluster.inputCount = TOUCHBAR_SIZE / 2;
  touchbarRightCluster.rotation = rotation;
  touchbarRightCluster.rotationDimension = rotDim;
  touchbarRightCluster.getPosition = Input::Linear1D_GetPosition;
  touchbarRightCluster.tryGetMemberId = Input::Linear1D_TryGetMemberId;
  Input::clusters.push_back(touchbarRightCluster);
}

void DeviceStart() {
  rotation = MatrixOS::UserVar::rotation;
  RegisterInputClusters();
  Device::KeyPad::Start();
  Device::LED::Start();

  if (bluetooth)
  {
    BLEMIDI::Start();
  }

#ifdef FACTORY_CONFIG
  if (esp_efuse_block_is_empty(EFUSE_BLK3))
  {
    MatrixOS::UIUtility::TextScroll("Factory Test", Color(0xFF00FF));
    MatrixOS::SYS::ExecuteAPP("203 Systems", "Mystrix Factory Menu");
  }
#endif
  Device::KeyPad::Scan();
  // Use keyState->pressure instead of Active() because it might still be debouncing
  if (KeyPad::keypadState[0][0].pressure && KeyPad::keypadState[1][1].pressure)
  {
    MatrixOS::SYS::ExecuteAPP("203 Systems", "Mystrix Factory Menu");
  }
  else if (KeyPad::keypadState[6][6].pressure && KeyPad::keypadState[7][7].pressure)
  {
    Device::Input::SuppressActiveInputs();
    MatrixOS::UserVar::brightness.Set(Device::LED::brightnessLevel[0]);
  }
  else if (KeyPad::keypadState[0][5].pressure && KeyPad::keypadState[1][6].pressure &&
           KeyPad::keypadState[0][7].pressure)
  {
    MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(2, 5), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(5, 5), Color(0xFF00FF));
    MatrixOS::LED::Update();
    MatrixOS::SYS::DelayMs(1500);
    Device::NVS::Clear();
    MatrixOS::SYS::Reboot();
  }
}

void LoadDeviceInfo() {
#ifndef FACTORY_CONFIG
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA, &deviceInfo, sizeof(deviceInfo) * 8);
#endif
  LoadVariantInfo();
}

void DeviceSettings() {
  UI deviceSettings("Device Settings", Color(0x00FFAA));

  UIButton bluetoothToggle;
  bluetoothToggle.SetName("Bluetooth");
  bluetoothToggle.SetColorFunc([&]() -> Color { return Color(0x0082fc).DimIfNot(Device::BLEMIDI::started); });
  bluetoothToggle.OnPress([&]() -> void {
    if (Device::BLEMIDI::started)
    {
      Device::BLEMIDI::Stop();
      Device::bluetooth = false;
    }
    else
    {
      Device::BLEMIDI::Start();
      Device::bluetooth = true;
    }
  });
  bluetoothToggle.OnHold([&]() -> void {
    MatrixOS::UIUtility::TextScroll(bluetoothToggle.name + " " + (Device::BLEMIDI::started ? "Enabled" : "Disabled"),
                                    bluetoothToggle.GetColor());
  });
  deviceSettings.AddUIComponent(bluetoothToggle, Point(0, 0));

  UIToggle touchbarToggle;
  touchbarToggle.SetName("Touchbar");
  touchbarToggle.SetColor(Color(0x7957FB));
  touchbarToggle.SetValuePointer(&Device::touchbarEnable);
  touchbarToggle.OnPress([&]() -> void { Device::touchbarEnable.Save(); });
  deviceSettings.AddUIComponent(touchbarToggle, Point(1, 0));

  UIButton keypadCalibrationBtn;
  keypadCalibrationBtn.SetName("Keypad Calibration");
  keypadCalibrationBtn.SetColor(Color::White);
  keypadCalibrationBtn.OnPress([]() -> void { MatrixOS::SYS::ExecuteAPP("203 Systems", "Force Calibration"); });
  keypadCalibrationBtn.SetEnabled(Device::KeyPad::velocitySensitivity);
  deviceSettings.AddUIComponent(keypadCalibrationBtn, Point(7, 0));

  // Infomation
  UIButton deviceNameBtn;
  deviceNameBtn.SetName("Device Name");
  deviceNameBtn.SetColor(Color(0x00FFD0));
  deviceNameBtn.OnPress([]() -> void { MatrixOS::UIUtility::TextScroll(Device::name, Color(0x00FFD0)); });
  deviceSettings.AddUIComponent(deviceNameBtn, Point(0, 7));

  UIButton deviceSerialBtn;
  deviceSerialBtn.SetName("Device Serial");
  deviceSerialBtn.SetColor(Color(0x00FF30));
  deviceSerialBtn.OnPress([]() -> void { MatrixOS::UIUtility::TextScroll(Device::GetSerial(), Color(0x00FF30)); });
  deviceSettings.AddUIComponent(deviceSerialBtn, Point(1, 7));

  deviceSettings.Start();
}

void Bootloader() {
// Check out esp_reset_reason_t for other Espressif pre-defined values
#define APP_REQUEST_UF2_RESET_HINT (esp_reset_reason_t)0x11F2

  // call esp_reset_reason() is required for idf.py to properly links esp_reset_reason_set_hint()
  (void)esp_reset_reason();
  esp_reset_reason_set_hint(APP_REQUEST_UF2_RESET_HINT);
  esp_restart();
}

void Reboot() {
  esp_restart();
}

void Delay(uint32_t interval) {
  vTaskDelay(pdMS_TO_TICKS(interval));
}

uint32_t Millis() {
  return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
  // return 0;
}

void Log(string& format, va_list& valst) {
  // ESP_LOG_LEVEL((esp_log_level_t)level, tag.c_str(), format.c_str(), valst);
  // esp_log_writev(ESP_LOG_INFO, format.c_str(), valst);

  vprintf(format.c_str(), valst);
}

string GetSerial() {
  uint8_t uuid[16];
  esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, (void*)uuid, 128);
  string uuid_str;
  uuid_str.reserve(33);
  const char char_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  for (uint8_t i = 0; i < 16; i++)
  {
    uuid_str += char_table[uuid[i] >> 4];
    uuid_str += char_table[uuid[i] & 0x0F];
  }
  return uuid_str; // TODO
}

void ErrorHandler() {}

uint64_t Micros() {
  return (uint64_t)esp_timer_get_time();
}
} // namespace Device

namespace MatrixOS::SYS
{
void ErrorHandler(char const* error);
}
