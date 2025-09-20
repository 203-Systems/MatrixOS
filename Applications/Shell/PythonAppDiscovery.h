#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include "ArduinoJson.h"

namespace PythonAppDiscovery {

// Structure to hold parsed JSON data
struct AppInfoJson {
    string name;
    string author;
    Color color;
    uint32_t version;
    string appMainFile;
    uint32_t osMinimalVer[3];  // [major, minor, patch]
    bool valid = false;
};

// JSON parsing and validation functions
bool ParseAppInfo(const string& json_content, AppInfoJson& result);
bool ValidateVersionCompatibility(const uint32_t required_version[3]);
bool ValidatePythonFile(const string& directory_path, const string& python_filename);
Color ParseColorArray(JsonArrayConst color_array);

// Simple struct to store Python app data
struct PythonAppInfo {
    Application_Info info;
    string file_path;
};

// Directory scanning functions
void ScanPythonApplications(vector<PythonAppInfo>& python_app_infos);
void ScanDirectory(const string& directory_path, vector<PythonAppInfo>& python_app_infos);
bool LoadApp(const string& directory_path, const string& json_filepath, vector<PythonAppInfo>& python_app_infos);


}  // namespace PythonAppDiscovery