#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    void _MatrixOS_SYS_Reboot(PikaObj *self) {
        MatrixOS::SYS::Reboot();
    }

    void _MatrixOS_SYS_Bootloader(PikaObj *self) {
        MatrixOS::SYS::Bootloader();
    }

    void _MatrixOS_SYS_DelayMs(PikaObj *self, int ms) {
        MatrixOS::SYS::DelayMs(ms);
    }

    int _MatrixOS_SYS_Millis(PikaObj *self) {
        return MatrixOS::SYS::Millis();
    }

    void _MatrixOS_SYS_OpenSetting(PikaObj *self) {
        MatrixOS::SYS::OpenSetting();
    }

    void _MatrixOS_SYS_ExecuteAPP(PikaObj *self, char* author, char* app_name) {
        MatrixOS::SYS::ExecuteAPP(string(author), string(app_name));
    }

    void _MatrixOS_SYS_ExecuteAPPByID(PikaObj *self, int app_id) {
        MatrixOS::SYS::ExecuteAPP(app_id);
    }

    // NVS functions
    int _MatrixOS_NVS_GetSize(PikaObj *self, int hash) {
        return MatrixOS::NVS::GetSize((uint32_t)hash);
    }

    char* _MatrixOS_NVS_GetVariable(PikaObj *self, int hash, int length) {
        auto data = MatrixOS::NVS::GetVariable((uint32_t)hash);
        if (data.empty()) {
            return nullptr;
        }
        
        // // Create a new buffer for the return value
        // char* result = (char*)malloc(data.size() + 1);
        // memcpy(result, data.data(), data.size());
        // result[data.size()] = '\0';
        // return result;

        // TODO Fix Malloc
        return NULL;
    }

    pika_bool _MatrixOS_NVS_SetVariable(PikaObj *self, int hash, char* data, int length) {
        return MatrixOS::NVS::SetVariable((uint32_t)hash, (void*)data, (uint16_t)length);
    }

    pika_bool _MatrixOS_NVS_DeleteVariable(PikaObj *self, int hash) {
        return MatrixOS::NVS::DeleteVariable((uint32_t)hash);
    }
}