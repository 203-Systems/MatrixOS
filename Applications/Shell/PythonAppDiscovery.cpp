#include "PythonAppDiscovery.h"
#include "System/ReleaseConfig.h"

#if DEVICE_STORAGE == 1

namespace PythonAppDiscovery {

bool ParseAppInfo(const string& json_content, AppInfoJson& result) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_content);

    if (error) {
        MLOGE("Shell", "JSON parsing failed: %s", error.c_str());
        return false;
    }

    // Parse required string fields
    if (doc["name"].isNull() || doc["author"].isNull() || doc["appMainFile"].isNull()) {
        MLOGE("Shell", "Missing required fields (name, author, or appMainFile)");
        return false;
    }
    result.name = doc["name"].as<string>();
    result.author = doc["author"].as<string>();
    result.appMainFile = doc["appMainFile"].as<string>();

    // Parse version
    if (doc["version"].isNull() || !doc["version"].is<int>()) {
        MLOGE("Shell", "Missing or invalid version field");
        return false;
    }
    result.version = doc["version"].as<uint32_t>();

    // Parse color - can be hex string or RGB array
    if (doc["color"].isNull()) {
        MLOGE("Shell", "Missing color field");
        return false;
    }

    JsonArrayConst color_array = doc["color"].as<JsonArrayConst>();
    if (!color_array.isNull()) {
        // Parse color array [r, g, b]
        if (color_array.size() != 3) {
            MLOGE("Shell", "Color array must have 3 elements");
            return false;
        }
        result.color = ParseColorArray(color_array);
    } else {
        // Parse hex string like "0xFF5722" or "#FF5722"
        string color_str = doc["color"].as<string>();
        if (color_str.empty()) {
            MLOGE("Shell", "Invalid color string");
            return false;
        }
        result.color = Color(strtol(color_str.c_str(), nullptr, 0));
    }

    // Parse osMinimalVer array [major, minor, patch]
    JsonArrayConst version_array = doc["osMinimalVer"].as<JsonArrayConst>();
    if (version_array.isNull() || version_array.size() != 3) {
        MLOGE("Shell", "osMinimalVer must be an array with 3 elements");
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
    // Check if filename ends with .py or .py.a
    bool ends_with_py = (python_filename.length() >= 3 &&
                         python_filename.substr(python_filename.length() - 3) == ".py");
    bool ends_with_pya = (python_filename.length() >= 5 &&
                          python_filename.substr(python_filename.length() - 5) == ".py.a");

    if (!ends_with_py && !ends_with_pya) {
        MLOGE("Shell", "appMainFile must end with .py or .py.a: %s", python_filename.c_str());
        return false;
    }

    // Check if file exists
    string full_path = directory_path + "/" + python_filename;
    if (!MatrixOS::FileSystem::Exists(full_path)) {
        MLOGE("Shell", "Python file not found: %s", full_path.c_str());
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

void ScanPythonApplications(vector<PythonAppInfo>& python_app_infos) {
    if(MatrixOS::FileSystem::Available() == false)
    {
        return;
    }

    if (!MatrixOS::FileSystem::Exists("rootfs:/MatrixOS/Applications")) {
        MLOGW("Shell", "Applications directory not found, creating it");
        MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS");
        MatrixOS::FileSystem::MakeDir("rootfs:/MatrixOS/Applications");
        return;
    }

    ScanDirectory("rootfs:/MatrixOS/Applications", python_app_infos);
}

void ScanDirectory(const string& directory_path, vector<PythonAppInfo>& python_app_infos) {
    vector<string> entries = MatrixOS::FileSystem::ListDir(directory_path);

    for (const string& entry : entries) {
        string full_path = directory_path + "/" + entry;

        if (entry == "AppInfo.json") {
            // Found AppInfo.json, process it
            LoadApp(directory_path, full_path, python_app_infos);
        } else {
            // Check if it's a directory by trying to list it
            vector<string> sub_entries = MatrixOS::FileSystem::ListDir(full_path);
            if (!sub_entries.empty() || MatrixOS::FileSystem::Exists(full_path + "/AppInfo.json")) {
                // It's a directory, scan recursively
                ScanDirectory(full_path, python_app_infos);
            }
        }
    }
}

bool LoadApp(const string& directory_path, const string& json_filepath, vector<PythonAppInfo>& python_app_infos) {
    // Read JSON file
    File file = MatrixOS::FileSystem::Open(json_filepath, "r");
    if (!file.Available()) {
        MLOGE("Shell", "Failed to open AppInfo.json: %s", json_filepath.c_str());
        return false;
    }

    // Read file content
    string json_content;
    char buffer[512];
    size_t bytes_read;
    while ((bytes_read = file.Read(buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        json_content += buffer;
    }

    file.Close();

    // Parse JSON
    AppInfoJson app_info;
    if (!ParseAppInfo(json_content, app_info)) {
        MLOGE("Shell", "Failed to parse AppInfo.json in %s", directory_path.c_str());
        return false;
    }

    // Validate version compatibility
    if (!ValidateVersionCompatibility(app_info.osMinimalVer)) {
        MLOGW("Shell", "App %s requires newer MatrixOS version [%d.%d.%d]",
              app_info.name.c_str(), app_info.osMinimalVer[0],
              app_info.osMinimalVer[1], app_info.osMinimalVer[2]);
        return false;
    }

    // Validate Python file exists
    if (!ValidatePythonFile(directory_path, app_info.appMainFile)) {
        MLOGW("Shell", "App %s Missing python file %s", app_info.name.c_str(), directory_path);
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

    // Add to python app infos
    PythonAppInfo app_data;
    app_data.info = info;
    app_data.file_path = python_file_path;
    python_app_infos.push_back(app_data);

    MLOGI("Shell", "Found Python app: %s by %s",
          info.name.c_str(), info.author.c_str());
    return true;
}


}  // namespace PythonAppDiscovery

#else
// Filesystem not available, provide stub functions
namespace PythonAppDiscovery {
    void ScanPythonApplications() {
        MLOGW("Shell", "Filesystem not available, skipping Python app discovery");
    }
}
#endif