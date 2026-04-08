#include "PythonAppDiscovery.h"
#include "System/ReleaseConfig.h"

#if DEVICE_STORAGE == 1

namespace PythonAppDiscovery
{

bool ParseAppInfo(const string& jsonContent, AppInfoJson& result) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonContent);

  if (error)
  {
    MLOGE("Shell", "JSON parsing failed: %s", error.c_str());
    return false;
  }

  // Parse required string fields
  if (doc["name"].isNull() || doc["author"].isNull() || doc["appMainFile"].isNull())
  {
    MLOGE("Shell", "Missing required fields (name, author, or appMainFile)");
    return false;
  }
  result.name = doc["name"].as<string>();
  result.author = doc["author"].as<string>();
  result.appMainFile = doc["appMainFile"].as<string>();

  // Parse version
  if (doc["version"].isNull() || !doc["version"].is<int>())
  {
    MLOGE("Shell", "Missing or invalid version field");
    return false;
  }
  result.version = doc["version"].as<uint32_t>();

  // Parse color - can be hex string or RGB array
  if (doc["color"].isNull())
  {
    MLOGE("Shell", "Missing color field");
    return false;
  }

  JsonArrayConst colorArray = doc["color"].as<JsonArrayConst>();
  if (!colorArray.isNull())
  {
    // Parse color array [r, g, b]
    if (colorArray.size() != 3)
    {
      MLOGE("Shell", "Color array must have 3 elements");
      return false;
    }
    result.color = ParseColorArray(colorArray);
  }
  else
  {
    // Parse hex string like "0xFF5722" or "#FF5722"
    string colorStr = doc["color"].as<string>();
    if (colorStr.empty())
    {
      MLOGE("Shell", "Invalid color string");
      return false;
    }
    result.color = Color(strtol(colorStr.c_str(), nullptr, 0));
  }

  // Parse osMinimalVer array [major, minor, patch]
  JsonArrayConst versionArray = doc["osMinimalVer"].as<JsonArrayConst>();
  if (versionArray.isNull() || versionArray.size() != 3)
  {
    MLOGE("Shell", "osMinimalVer must be an array with 3 elements");
    return false;
  }
  result.osMinimalVer[0] = versionArray[0].as<uint32_t>();
  result.osMinimalVer[1] = versionArray[1].as<uint32_t>();
  result.osMinimalVer[2] = versionArray[2].as<uint32_t>();

  result.valid = true;
  return true;
}

bool ValidateVersionCompatibility(const uint32_t requiredVersion[3]) {
  // Get current MatrixOS version
  uint32_t currentMajor = MATRIXOS_MAJOR_VER;
  uint32_t currentMinor = MATRIXOS_MINOR_VER;
  uint32_t currentPatch = MATRIXOS_PATCH_VER;

  // Check if current version >= required version
  if (currentMajor > requiredVersion[0])
    return true;
  if (currentMajor < requiredVersion[0])
    return false;

  if (currentMinor > requiredVersion[1])
    return true;
  if (currentMinor < requiredVersion[1])
    return false;

  return (currentPatch >= requiredVersion[2]);
}

bool ValidatePythonFile(const string& directoryPath, const string& pythonFilename) {
  // Check if filename ends with .py or .py.a
  bool endsWithPy = (pythonFilename.length() >= 3 && pythonFilename.substr(pythonFilename.length() - 3) == ".py");
  bool endsWithPya = (pythonFilename.length() >= 5 && pythonFilename.substr(pythonFilename.length() - 5) == ".py.a");

  if (!endsWithPy && !endsWithPya)
  {
    MLOGE("Shell", "appMainFile must end with .py or .py.a: %s", pythonFilename.c_str());
    return false;
  }

  // Check if file exists
  string fullPath = directoryPath + "/" + pythonFilename;
  if (!MatrixOS::FileSystem::Exists(fullPath))
  {
    MLOGE("Shell", "Python file not found: %s", fullPath.c_str());
    return false;
  }

  return true;
}

Color ParseColorArray(JsonArrayConst colorArray) {
  uint8_t r = colorArray[0].as<uint8_t>();
  uint8_t g = colorArray[1].as<uint8_t>();
  uint8_t b = colorArray[2].as<uint8_t>();
  return Color(r, g, b);
}

void ScanPythonApplications(vector<PythonAppInfo>& pythonAppInfos) {
  if (MatrixOS::FileSystem::Available() == false)
  {
    return;
  }

  if (!MatrixOS::FileSystem::Exists("rootfs:/MatrixOS/Applications"))
  {
    MLOGW("Shell", "Applications directory not found, creating it");
    MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS");
    MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS/Applications");
    return;
  }

  ScanDirectory("rootfs:/MatrixOS/Applications", pythonAppInfos);
}

void ScanDirectory(const string& directoryPath, vector<PythonAppInfo>& pythonAppInfos) {
  vector<string> entries = MatrixOS::FileSystem::ListDir(directoryPath);

  for (const string& entry : entries)
  {
    string fullPath = directoryPath + "/" + entry;

    if (entry == "AppInfo.json")
    {
      // Found AppInfo.json, process it
      LoadApp(directoryPath, fullPath, pythonAppInfos);
    }
    else
    {
      // Check if it's a directory by trying to list it
      vector<string> sub_entries = MatrixOS::FileSystem::ListDir(fullPath);
      if (!sub_entries.empty() || MatrixOS::FileSystem::Exists(fullPath + "/AppInfo.json"))
      {
        // It's a directory, scan recursively
        ScanDirectory(fullPath, pythonAppInfos);
      }
    }
  }
}

bool LoadApp(const string& directoryPath, const string& jsonFilepath, vector<PythonAppInfo>& pythonAppInfos) {
  // Read JSON file
  File file = MatrixOS::FileSystem::Open(jsonFilepath, "r");
  if (!file.Available())
  {
    MLOGE("Shell", "Failed to open AppInfo.json: %s", jsonFilepath.c_str());
    return false;
  }

  // Read file content
  string jsonContent;
  char buffer[512];
  size_t bytesRead;
  while ((bytesRead = file.Read(buffer, sizeof(buffer) - 1)) > 0)
  {
    buffer[bytesRead] = '\0';
    jsonContent += buffer;
  }

  file.Close();

  // Parse JSON
  AppInfoJson appInfo;
  if (!ParseAppInfo(jsonContent, appInfo))
  {
    MLOGE("Shell", "Failed to parse AppInfo.json in %s", directoryPath.c_str());
    return false;
  }

  // Validate version compatibility
  if (!ValidateVersionCompatibility(appInfo.osMinimalVer))
  {
    MLOGW("Shell", "App %s requires newer MatrixOS version [%d.%d.%d]", appInfo.name.c_str(), appInfo.osMinimalVer[0],
          appInfo.osMinimalVer[1], appInfo.osMinimalVer[2]);
    return false;
  }

  // Validate Python file exists
  if (!ValidatePythonFile(directoryPath, appInfo.appMainFile))
  {
    MLOGW("Shell", "App %s Missing python file %s", appInfo.name.c_str(), directoryPath);
    return false;
  }

  // Create Application_Info structure
  Application_Info info;
  info.name = appInfo.name;
  info.author = appInfo.author;
  info.color = appInfo.color;
  info.version = appInfo.version;
  info.visibility = true;    // Python apps are visible by default
  info.is_system = false;    // Python apps are not system apps
  info.factory = nullptr;    // Not used for Python apps
  info.destructor = nullptr; // Not used for Python apps

  // Full path to Python file
  string pythonFilePath = directoryPath + "/" + appInfo.appMainFile;

  // Add to python app infos
  PythonAppInfo appData;
  appData.info = info;
  appData.file_path = pythonFilePath;
  pythonAppInfos.push_back(appData);

  MLOGI("Shell", "Found Python app: %s by %s", info.name.c_str(), info.author.c_str());
  return true;
}

} // namespace PythonAppDiscovery

#else
// Filesystem not available, provide stub functions
namespace PythonAppDiscovery
{
void ScanPythonApplications() {
  MLOGW("Shell", "Filesystem not available, skipping Python app discovery");
}
} // namespace PythonAppDiscovery
#endif