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
        uint32_t privileged : 1;   // Bit 0: System privilege (file system, etc.)
        uint32_t reserved : 31;    // Bits 1-31: Reserved for future use
      };
      uint32_t raw;  // Direct access to all flags
    };

    // Constructor
    TaskPermissions(uint32_t value = 0) : raw(value) {}
  };

  inline StackType_t application_stack[APPLICATION_STACK_SIZE];
  inline StaticTask_t application_taskdef;

  inline StackType_t supervisor_stack[configMINIMAL_STACK_SIZE * 4];
  inline StaticTask_t supervisor_taskdef;

  inline bool inited = false;
  inline Application* active_app = NULL;
  inline TaskHandle_t active_app_task = NULL;
  inline uint32_t active_app_id = 0;
  inline Application_Info* active_app_info = NULL;
  inline uint32_t next_app_id = 0;

  inline SavedVar<uint32_t> prev_system_version = SavedVar<uint32_t>(SYSTEM_VAR_NAMESPACE, "PREV_OS_VERSION", MATRIXOS_VERSION_ID);

  void Begin(void);

  void InitSysModules(void);

  uint32_t GenerateAPPID(string author, string app_name);

  uint16_t GetApplicationCount();

  void UpdateSystemNVS();

  // Permission APIs
  bool IsTaskPrivileged(TaskHandle_t task = nullptr);
  TaskPermissions GetTaskPermissions(TaskHandle_t task = nullptr);
  void SetTaskPermissions(TaskPermissions permissions, TaskHandle_t task = nullptr);
}