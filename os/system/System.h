#pragma once

namespace MatrixOS::SYS
{
    StaticTimer_t device_task_tmdef;
    TimerHandle_t device_task_tm;

    StackType_t  application_stack[APPLICATION_STACK_SIZE];
    StaticTask_t application_taskdef;

    StackType_t  supervisor_stack[configMINIMAL_STACK_SIZE];
    StaticTask_t supervisor_taskdef;

    inline TaskHandle_t active_app_task = NULL;
    inline uint32_t active_app_id = 0;

    void LoadVariables();
    void SaveVariables();
    
    void ExecuteAPP(uint32_t app_id);
    uint32_t GenerateAPPID(string author, string app_name);
}