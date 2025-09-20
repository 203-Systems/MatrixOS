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
        Point* point_ptr = getCppObjPtrInPikaObj<Point>(xy);
        if (!point_ptr) return;

        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
        if (!color_ptr) return;

        MatrixOS::LED::SetColor(*point_ptr, *color_ptr, layer);
    }

    void _MatrixOS_LED_SetColorByID(PikaObj *self, int id, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
        if (!color_ptr) return;
        
        MatrixOS::LED::SetColor(id, *color_ptr, layer);
    }

    void _MatrixOS_LED_Fill(PikaObj *self, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
        if (!color_ptr) return;
        
        MatrixOS::LED::Fill(*color_ptr, layer);
    }

    pika_bool _MatrixOS_LED_FillPartition(PikaObj *self, char* partition, PikaObj* color, int layer) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
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
}