#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "PikaObjUtils.h"

extern "C" {
    // LED class implementation
    void _MatrixOS_LED_NextBrightness(PikaObj *self) {
        MatrixOS::LED::NextBrightness();
    }

    void _MatrixOS_LED_SetBrightness(PikaObj *self, int brightness) {
        MatrixOS::LED::SetBrightness(brightness);
    }

    pika_bool _MatrixOS_LED_SetBrightnessMultiplier(PikaObj *self, char* partition_name, pika_float multiplier) {
        return MatrixOS::LED::SetBrightnessMultiplier(string(partition_name), multiplier);
    }

    void _MatrixOS_LED_SetColor(PikaObj *self, PikaObj* xy, PikaObj* color, int layer) {
        // Get Point object from PikaObj
        Point* point_ptr = getCppValuePtrInPikaObj<Point>(xy);
        if (!point_ptr) return;

        // Get Color object from PikaObj
        Color* color_ptr = getCppValuePtrInPikaObj<Color>(color);
        if (!color_ptr) return;

        MatrixOS::LED::SetColor(*point_ptr, *color_ptr, layer);
    }

    void _MatrixOS_LED_SetColorByID(PikaObj *self, int id, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppValuePtrInPikaObj<Color>(color);
        if (!color_ptr) return;
        
        MatrixOS::LED::SetColor(id, *color_ptr, layer);
    }

    void _MatrixOS_LED_Fill(PikaObj *self, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppValuePtrInPikaObj<Color>(color);
        if (!color_ptr) return;
        
        MatrixOS::LED::Fill(*color_ptr, layer);
    }

    pika_bool _MatrixOS_LED_FillPartition(PikaObj *self, char* partition, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppValuePtrInPikaObj<Color>(color);
        if (!color_ptr) return false;
        
        return MatrixOS::LED::FillPartition(string(partition), *color_ptr, layer);
    }

    void _MatrixOS_LED_Update(PikaObj *self, int layer) {
        MatrixOS::LED::Update(layer);
    }

    int _MatrixOS_LED_CurrentLayer(PikaObj *self) {
        return MatrixOS::LED::CurrentLayer();
    }

    int _MatrixOS_LED_CreateLayer(PikaObj *self, int crossfade) {
        return MatrixOS::LED::CreateLayer(crossfade);
    }

    void _MatrixOS_LED_CopyLayer(PikaObj *self, int dest, int src) {
        MatrixOS::LED::CopyLayer(dest, src);
    }

    pika_bool _MatrixOS_LED_DestroyLayer(PikaObj *self, int crossfade) {
        return MatrixOS::LED::DestroyLayer(crossfade);
    }

    void _MatrixOS_LED_Fade(PikaObj *self, int crossfade) {
        MatrixOS::LED::Fade(crossfade);
    }

    void _MatrixOS_LED_PauseUpdate(PikaObj *self, pika_bool pause) {
        MatrixOS::LED::PauseUpdate(pause != 0);
    }

    int _MatrixOS_LED_GetLEDCount(PikaObj *self) {
        return MatrixOS::LED::GetLEDCount();
    }

    int _MatrixOS_LED_GetPartitionCount(PikaObj *self) {
        return (int)Device::LED::partitions.size();
    }

    char* _MatrixOS_LED_GetPartitionName(PikaObj *self, int index) {
        if (index < 0 || index >= (int)Device::LED::partitions.size()) {
            return (char*)"";
        }
        return (char*)Device::LED::partitions[index].name.c_str();
    }

    int _MatrixOS_LED_GetPartitionStart(PikaObj *self, int index) {
        if (index < 0 || index >= (int)Device::LED::partitions.size()) {
            return -1;
        }
        return Device::LED::partitions[index].start;
    }

    int _MatrixOS_LED_GetPartitionSize(PikaObj *self, int index) {
        if (index < 0 || index >= (int)Device::LED::partitions.size()) {
            return 0;
        }
        return Device::LED::partitions[index].size;
    }

    int _MatrixOS_LED_GetPartitionType(PikaObj *self, int index) {
        if (index < 0 || index >= (int)Device::LED::partitions.size()) {
            return 0;
        }
        return (int)Device::LED::partitions[index].type;
    }

    pika_float _MatrixOS_LED_GetPartitionDefaultMultiplier(PikaObj *self, int index) {
        if (index < 0 || index >= (int)Device::LED::partitions.size()) {
            return 0.0f;
        }
        return Device::LED::partitions[index].default_multiplier;
    }

    int _MatrixOS_LED_GetPartitionIndex(PikaObj *self, char* name) {
        if (!name) {
            return -1;
        }
        string target(name);
        for (uint16_t i = 0; i < Device::LED::partitions.size(); i++) {
            if (Device::LED::partitions[i].name == target) {
                return i;
            }
        }
        return -1;
    }
}
