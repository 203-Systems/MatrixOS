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

// Structure for discovered Python apps
struct DiscoveredPythonApp {
    Application_Info info;
    string python_file_path;
};

// Directory scanning functions
vector<DiscoveredPythonApp> ScanPythonApplications();
void ScanDirectory(const string& directory_path, vector<DiscoveredPythonApp>& discovered_apps);
bool LoadApp(const string& directory_path, const string& json_filepath, vector<DiscoveredPythonApp>& discovered_apps);


}  // namespace PythonAppDiscovery