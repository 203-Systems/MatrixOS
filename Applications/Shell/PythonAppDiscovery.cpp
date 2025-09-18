#include "PythonAppDiscovery.h"
#include "System/ReleaseConfig.h"

#if DEVICE_STORAGE == 1

namespace PythonAppDiscovery {

bool ParseAppInfo(const string& json_content, AppInfoJson& result) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_content);

    if (error) {
        MLOGE("PythonAppDiscovery", "JSON parsing failed: %s", error.c_str());
        return false;
    }

    // Parse required string fields
    if (doc["name"].isNull() || doc["author"].isNull() || doc["appMainFile"].isNull()) {
        MLOGE("PythonAppDiscovery", "Missing required fields (name, author, or appMainFile)");
        return false;
    }
    result.name = doc["name"].as<string>();
    result.author = doc["author"].as<string>();
    result.appMainFile = doc["appMainFile"].as<string>();

    // Parse version
    if (doc["version"].isNull() || !doc["version"].is<int>()) {
        MLOGE("PythonAppDiscovery", "Missing or invalid version field");
        return false;
    }
    result.version = doc["version"].as<uint32_t>();

    // Parse color - can be hex string or RGB array
    if (doc["color"].isNull()) {
        MLOGE("PythonAppDiscovery", "Missing color field");
        return false;
    }

    JsonArrayConst color_array = doc["color"].as<JsonArrayConst>();
    if (!color_array.isNull()) {
        // Parse color array [r, g, b]
        if (color_array.size() != 3) {
            MLOGE("PythonAppDiscovery", "Color array must have 3 elements");
            return false;
        }
        result.color = ParseColorArray(color_array);
    } else {
        // Parse hex string like "0xFF5722" or "#FF5722"
        string color_str = doc["color"].as<string>();
        if (color_str.empty()) {
            MLOGE("PythonAppDiscovery", "Invalid color string");
            return false;
        }
        result.color = Color(strtol(color_str.c_str(), nullptr, 0));
    }

    // Parse osMinimalVer array [major, minor, patch]
    JsonArrayConst version_array = doc["osMinimalVer"].as<JsonArrayConst>();
    if (version_array.isNull() || version_array.size() != 3) {
        MLOGE("PythonAppDiscovery", "osMinimalVer must be an array with 3 elements");
        return false;
    }
    result.osMinimalVer[0] = version_array[0].as<uint32_t>();
    result.osMinimalVer[1] = version_array[1].as<uint32_t>();
    result.osMinimalVer[2] = version_array[2].as<uint32_t>();

    result.valid = true;
    return true;
}


bool ValidateVersionCompatibility(const uint32_t required_version[3]) {
    // Get current MatrixOS version
    uint32_t current_major = MATRIXOS_MAJOR_VER;
    uint32_t current_minor = MATRIXOS_MINOR_VER;
    uint32_t current_patch = MATRIXOS_PATCH_VER;

    // Check if current version >= required version
    if (current_major > required_version[0]) return true;
    if (current_major < required_version[0]) return false;

    if (current_minor > required_version[1]) return true;
    if (current_minor < required_version[1]) return false;

    return (current_patch >= required_version[2]);
}

bool ValidatePythonFile(const string& directory_path, const string& python_filename) {
    // Check if filename ends with .py
    if (python_filename.length() < 3 ||
        python_filename.substr(python_filename.length() - 3) != ".py") {
        MLOGE("PythonAppDiscovery", "appMainFile must end with .py: %s", python_filename.c_str());
        return false;
    }

    // Check if file exists
    string full_path = directory_path + "/" + python_filename;
    if (!MatrixOS::FileSystem::Exists(full_path)) {
        MLOGE("PythonAppDiscovery", "Python file not found: %s", full_path.c_str());
        return false;
    }

    return true;
}

Color ParseColorArray(JsonArrayConst color_array) {
    uint8_t r = color_array[0].as<uint8_t>();
    uint8_t g = color_array[1].as<uint8_t>();
    uint8_t b = color_array[2].as<uint8_t>();
    return Color(r, g, b);
}

vector<DiscoveredPythonApp> ScanPythonApplications() {
    MLOGI("PythonAppDiscovery", "Starting Python application discovery");
    vector<DiscoveredPythonApp> discovered_apps;

    if (!MatrixOS::FileSystem::Exists("rootfs:/MatrixOS/Applications")) {
        MLOGW("PythonAppDiscovery", "Applications directory not found, creating it");
        MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS");
        MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS/Applications");
        return discovered_apps;
    }

    ScanDirectory("rootfs:/MatrixOS/Applications", discovered_apps);
    MLOGI("PythonAppDiscovery", "Python application discovery completed: %d apps found", discovered_apps.size());
    return discovered_apps;
}

void ScanDirectory(const string& directory_path, vector<DiscoveredPythonApp>& discovered_apps) {
    MLOGD("PythonAppDiscovery", "Scanning directory: %s", directory_path.c_str());

    vector<string> entries = MatrixOS::FileSystem::ListDir(directory_path);

    for (const string& entry : entries) {
        string full_path = directory_path + "/" + entry;

        if (entry == "AppInfo.json") {
            // Found AppInfo.json, process it
            LoadApp(directory_path, full_path, discovered_apps);
        } else {
            // Check if it's a directory by trying to list it
            vector<string> sub_entries = MatrixOS::FileSystem::ListDir(full_path);
            if (!sub_entries.empty() || MatrixOS::FileSystem::Exists(full_path + "/AppInfo.json")) {
                // It's a directory, scan recursively
                ScanDirectory(full_path, discovered_apps);
            }
        }
    }
}

bool LoadApp(const string& directory_path, const string& json_filepath, vector<DiscoveredPythonApp>& discovered_apps) {
    MLOGD("PythonAppDiscovery", "Processing AppInfo.json: %s", json_filepath.c_str());

    // Read JSON file
    File* file = MatrixOS::FileSystem::Open(json_filepath, "r");
    if (!file) {
        MLOGE("PythonAppDiscovery", "Failed to open AppInfo.json: %s", json_filepath.c_str());
        return false;
    }

    // Read file content
    string json_content;
    char buffer[512];
    size_t bytes_read;
    while ((bytes_read = file->Read(buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        json_content += buffer;
    }

    file->Close();
    delete file;

    // Parse JSON
    AppInfoJson app_info;
    if (!ParseAppInfo(json_content, app_info)) {
        MLOGE("PythonAppDiscovery", "Failed to parse AppInfo.json in %s", directory_path.c_str());
        return false;
    }

    // Validate version compatibility
    if (!ValidateVersionCompatibility(app_info.osMinimalVer)) {
        MLOGW("PythonAppDiscovery", "App %s requires newer MatrixOS version [%d.%d.%d]",
              app_info.name.c_str(), app_info.osMinimalVer[0],
              app_info.osMinimalVer[1], app_info.osMinimalVer[2]);
        return false;
    }

    // Validate Python file exists
    if (!ValidatePythonFile(directory_path, app_info.appMainFile)) {
        return false;
    }

    // Create Application_Info structure
    Application_Info info;
    info.name = app_info.name;
    info.author = app_info.author;
    info.color = app_info.color;
    info.version = app_info.version;
    info.visibility = true;  // Python apps are visible by default
    info.is_system = false;  // Python apps are not system apps
    info.factory = nullptr;  // Not used for Python apps
    info.destructor = nullptr;  // Not used for Python apps

    // Full path to Python file
    string python_file_path = directory_path + "/" + app_info.appMainFile;

    // Add to discovered apps
    DiscoveredPythonApp discovered_app;
    discovered_app.info = info;
    discovered_app.python_file_path = python_file_path;
    discovered_apps.push_back(discovered_app);

    MLOGI("PythonAppDiscovery", "Successfully discovered Python app: %s by %s",
          info.name.c_str(), info.author.c_str());
    return true;
}


}  // namespace PythonAppDiscovery

#else
// Filesystem not available, provide stub functions
namespace PythonAppDiscovery {
    void ScanPythonApplications() {
        MLOGW("PythonAppDiscovery", "Filesystem not available, skipping Python app discovery");
    }
}
#endif