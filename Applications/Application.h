#pragma once
#include "MatrixOS.h"
#include <functional>
#include <unordered_map>
#include <deque>
#include <cstdarg>

class Application;

struct Application_Info {
  string name;
  string author;
  Color color;
  uint32_t version;
  bool visibility = true;
  bool is_system = false;  // System privilege flag
  std::function<Application*()> factory = nullptr;
  std::function<void(Application*)> destructor = nullptr;
};

class Application {
 public:
  void Start(const vector<string>& args) {
    Setup(args);
    while (true)
    {
      Loop();
      taskYIELD();
    }
  }

  void Exit() { // Call this to exit the application
    MatrixOS::SYS::ExitAPP();
  };

  // Override these functions to implement your application
  virtual void Setup(const vector<string>& args) {};
  virtual void Loop() { Exit(); }; //If the Loop func didn't get overriden, it will just exit. This prevents infinity loop.
  virtual void End() {};
  
  virtual ~Application() = default;
};

#define APPID(author, name) StaticHash(author "-" name)

#define APPLICATION_HELPER_CLASS_IMPL(CLASS) CLASS##_HELPER
#define APPLICATION_HELPER_CLASS(CLASS) APPLICATION_HELPER_CLASS_IMPL(CLASS)

// Stores all the applications - Optimal for ID lookup
// Using function-local static to ensure proper initialization order
inline std::unordered_map<uint32_t, Application_Info*>& GetApplications() {
  static std::unordered_map<uint32_t, Application_Info*> applications;
  return applications;
}

// Store all the application id in order of registration - Preserves order and is optimal for iteration
inline std::map<uint32_t, uint32_t>& GetApplicationIDs() {
  static std::map<uint32_t, uint32_t> application_ids;
  return application_ids;
}

template <typename APPLICATION_CLASS>
static inline void register_application(uint32_t order, bool is_system) {
    APPLICATION_CLASS::info.is_system = is_system;  // Set system flag
    APPLICATION_CLASS::info.factory = []() -> Application* {                                       \
      return new APPLICATION_CLASS();                                                              \
    };                                                                                             \
    APPLICATION_CLASS::info.destructor = [](Application* app) {                                 \
      delete (APPLICATION_CLASS*)app;                                                                   \
    };                                                                                             \
    MLOGI("Application", "Registering application: %s%s", APPLICATION_CLASS::info.name.c_str(), is_system ? " (system)" : "");  \
    uint32_t app_id = StringHash(APPLICATION_CLASS::info.author + '-' + APPLICATION_CLASS::info.name); \
    auto& applications = GetApplications(); \
    if (applications.find(app_id) != applications.end()) { \
      return; \
    } \
    applications.insert({app_id, &APPLICATION_CLASS::info}); \
    GetApplicationIDs()[order] = app_id; \
}


#define REGISTER_APPLICATION(APPLICATION_CLASS, IS_SYSTEM)                                         \
  __attribute__((__constructor__)) inline void APPLICATION_HELPER_CLASS(APPLICATION_CLASS)(void) { \
    register_application<APPLICATION_CLASS>(__COUNTER__, IS_SYSTEM);      \
  }
