#pragma once
#include "MatrixOS.h"
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
  bool isSystem = false; // System privilege flag
  Application* (*factory)() = nullptr;
  void (*destructor)(Application*) = nullptr;
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
  virtual void Loop() {
    Exit();
  }; // If the Loop func didn't get overriden, it will just exit. This prevents infinity loop.
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
  static std::map<uint32_t, uint32_t> applicationIds;
  return applicationIds;
}

// Named template functions for app creation/destruction.
// Using real function addresses avoids WASM indirect-call-table issues
// that arise when lambdas are stored in std::function during static init.
template <typename T>
static Application* CreateApp() {
  void* mem = pvPortMalloc(sizeof(T));
  if (mem == nullptr)
  {
    return nullptr;
  }
  return new (mem) T();
}

template <typename T>
static void DestroyApp(Application* app) {
  if (app != nullptr)
  {
    static_cast<T*>(app)->~T();
    vPortFree(app);
  }
}

template <typename APPLICATION_CLASS> static inline void RegisterApplication(uint32_t order, bool isSystem) {
  APPLICATION_CLASS::info.isSystem = isSystem; // Set system flag
  APPLICATION_CLASS::info.factory = &CreateApp<APPLICATION_CLASS>;
  APPLICATION_CLASS::info.destructor = &DestroyApp<APPLICATION_CLASS>;
  MLOGI("Application", "Registering application: %s%s", APPLICATION_CLASS::info.name.c_str(), isSystem ? " (system)" : "");
  uint32_t app_id = StringHash(APPLICATION_CLASS::info.author + '-' + APPLICATION_CLASS::info.name);
  auto& applications = GetApplications();
  if (applications.find(app_id) != applications.end())
  {
    return;
  }
  applications.insert({app_id, &APPLICATION_CLASS::info});
  GetApplicationIDs()[order] = app_id;
}

#define REGISTER_APPLICATION(APPLICATION_CLASS, IS_SYSTEM)                                                                                 \
  __attribute__((__constructor__)) inline void APPLICATION_HELPER_CLASS(APPLICATION_CLASS)(void) {                                         \
    RegisterApplication<APPLICATION_CLASS>(__COUNTER__, IS_SYSTEM);                                                                       \
  }
