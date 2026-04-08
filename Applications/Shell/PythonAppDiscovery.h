#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include "ArduinoJson.h"

namespace PythonAppDiscovery
{

// Structure to hold parsed JSON data
struct AppInfoJson {
  string name;
  string author;
  Color color;
  uint32_t version;
  string appMainFile;
  uint32_t osMinimalVer[3]; // [major, minor, patch]
  bool valid = false;
};

// JSON parsing and validation functions
bool ParseAppInfo(const string& jsonContent, AppInfoJson& result);
bool ValidateVersionCompatibility(const uint32_t requiredVersion[3]);
bool ValidatePythonFile(const string& directoryPath, const string& pythonFilename);
Color ParseColorArray(JsonArrayConst colorArray);

// Simple struct to store Python app data
struct PythonAppInfo {
  Application_Info info;
  string file_path;
};

// Directory scanning functions
void ScanPythonApplications(vector<PythonAppInfo>& pythonAppInfos);
void ScanDirectory(const string& directoryPath, vector<PythonAppInfo>& pythonAppInfos);
bool LoadApp(const string& directoryPath, const string& jsonFilepath, vector<PythonAppInfo>& pythonAppInfos);

} // namespace PythonAppDiscovery