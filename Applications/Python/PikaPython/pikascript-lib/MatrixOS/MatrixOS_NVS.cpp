#include "MatrixOS.h"
#include "pikaScript.h"

extern "C" {
    int _MatrixOS_NVS_GetSize(PikaObj *self, int hash) {
        return MatrixOS::NVS::GetSize((uint32_t)hash);
    }

    Arg* _MatrixOS_NVS_GetVariable(PikaObj *self, int hash) {
        auto data = MatrixOS::NVS::GetVariable((uint32_t)hash);

        // Create bytes argument with the NVS data
        Arg* result = arg_newBytes((uint8_t*)data.data(), data.size());
        
        return result;
    }

    pika_bool _MatrixOS_NVS_SetVariable(PikaObj *self, int hash, char* data, int length) {
        return MatrixOS::NVS::SetVariable((uint32_t)hash, (void*)data, (uint16_t)length);
    }

    pika_bool _MatrixOS_NVS_DeleteVariable(PikaObj *self, int hash) {
        return MatrixOS::NVS::DeleteVariable((uint32_t)hash);
    }
}