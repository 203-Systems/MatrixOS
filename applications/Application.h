#pragma once
#include "MatrixOS.h"
#include <functional>
#include <unordered_map>
#include <deque>

struct Application_Info {
  string name;
  string author;
  Color color;
  uint32_t version;
  bool visibility = true;
  std::function<Application*()> factory = nullptr;
};

class Application {
 public:
  void* args;

  void Start(void* args = NULL);

  virtual void Setup(){};
  virtual void Loop() { Exit(); }; //If the Loop func didn't get overrided, it will just exit. This prevents infinity loop.
  virtual void End(){};

  void Exit();
};

#define APPID(author, name) StaticHash(author "-" name)

#define APPLICATION_HELPER_CLASS_IMPL(CLASS) CLASS##_HELPER
#define APPLICATION_HELPER_CLASS(CLASS) APPLICATION_HELPER_CLASS_IMPL(CLASS)

// Stores all the application info - Optimal for ID lookup
inline std::unordered_map<uint32_t, Application_Info*> applications;

// Store all the application id in order of registration - Perserves order and is optimal for iteration
inline std::vector<uint32_t> application_ids;

#define REGISTER_APPLICATION(APPLICATION_CLASS)                                                    \
  __attribute__((__constructor__)) inline void APPLICATION_HELPER_CLASS(APPLICATION_CLASS)(void) { \
    APPLICATION_CLASS::info.factory = []() -> Application* {                                       \
      return new APPLICATION_CLASS();                                                              \
    };                                                                                             \
    ESP_LOGI("Application", "Registering application: %s", APPLICATION_CLASS::info.name.c_str());  \
    uint32_t app_id = Hash(APPLICATION_CLASS::info.author + '-' + APPLICATION_CLASS::info.name); \
    if (applications.find(app_id) != applications.end()) { \
      return; \
    } \
    applications.insert({app_id, &APPLICATION_CLASS::info}); \
    application_ids.push_back(app_id); \
  }
