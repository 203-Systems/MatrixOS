#pragma once

#define SYSTEM_VAR_NAMESPACE "SYSTEM_VAR"

class Application;
class Application_Info;

namespace MatrixOS::SYS
{
// Task permission flags bitmap
struct TaskPermissions {
  union {
    struct {
      uint32_t privileged : 1; // Bit 0: System privilege (file system, etc.)
      uint32_t reserved : 31;  // Bits 1-31: Reserved for future use
    };
    uint32_t raw; // Direct access to all flags
  };

  // Constructor
  TaskPermissions(uint32_t value = 0) : raw(value) {}
};

inline StackType_t applicationStack[APPLICATION_STACK_SIZE];
inline StaticTask_t applicationTaskdef;

inline StackType_t supervisorStack[configMINIMAL_STACK_SIZE * 4];
inline StaticTask_t supervisorTaskdef;

inline bool inited = false;
inline Application* activeApp = NULL;
inline TaskHandle_t activeAppTask = NULL;
inline uint32_t activeAppId = 0;
inline Application_Info* activeAppInfo = NULL;
inline uint32_t nextAppId = 0;

inline SavedVar<uint32_t> prevSystemVersion = SavedVar<uint32_t>(SYSTEM_VAR_NAMESPACE, "PREV_OS_VERSION", MATRIXOS_VERSION_ID);

void Begin(void);

void InitSysModules(void);

uint32_t GenerateAPPID(string author, string appName);

uint16_t GetApplicationCount();

void UpdateSystemNVS();

// Permission APIs
bool IsTaskPrivileged(TaskHandle_t task = nullptr);
TaskPermissions GetTaskPermissions(TaskHandle_t task = nullptr);
void SetTaskPermissions(TaskPermissions permissions, TaskHandle_t task = nullptr);
} // namespace MatrixOS::SYS