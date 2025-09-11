#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // LED class implementation
    void _MatrixOS_LED_NextBrightness(PikaObj *self) {
        MatrixOS::LED::NextBrightness();
    }

    void _MatrixOS_LED_SetBrightness(PikaObj *self, int brightness) {
        MatrixOS::LED::SetBrightness(brightness);
    }

    void _MatrixOS_LED_SetBrightnessMultiplier(PikaObj *self, char* partition_name, pika_float multiplier) {
        MatrixOS::LED::SetBrightnessMultiplier(string(partition_name), multiplier);
    }

    void _MatrixOS_LED_SetColor(PikaObj *self, PikaObj* xy, PikaObj* color) {
        int x = obj_getInt(xy, (char*)"x");
        int y = obj_getInt(xy, (char*)"y");
        Point point(x, y);
        
        // Extract color components
        int r = obj_getInt(color, (char*)"r");
        int g = obj_getInt(color, (char*)"g");
        int b = obj_getInt(color, (char*)"b");
        int w = obj_getInt(color, (char*)"w");
        Color color_obj(r, g, b, w);

        MatrixOS::LED::SetColor(point, color_obj);
    }

    void _MatrixOS_LED_SetColorByID(PikaObj *self, int id, PikaObj* color) {
        // Extract color components
        int r = obj_getInt(color, (char*)"r");
        int g = obj_getInt(color, (char*)"g");
        int b = obj_getInt(color, (char*)"b");
        int w = obj_getInt(color, (char*)"w");
        Color color_obj(r, g, b, w);
        
        MatrixOS::LED::SetColor(id, color_obj);
    }

    void _MatrixOS_LED_Fill(PikaObj *self, PikaObj* color) {
        // Extract color components
        int r = obj_getInt(color, (char*)"r");
        int g = obj_getInt(color, (char*)"g");
        int b = obj_getInt(color, (char*)"b");
        int w = obj_getInt(color, (char*)"w");
        Color color_obj(r, g, b, w);
        
        MatrixOS::LED::Fill(color_obj);
    }

    void _MatrixOS_LED_FillPartition(PikaObj *self, char* partition, PikaObj* color) {
        // Extract color components
        int r = obj_getInt(color, (char*)"r");
        int g = obj_getInt(color, (char*)"g");
        int b = obj_getInt(color, (char*)"b");
        int w = obj_getInt(color, (char*)"w");
        Color color_obj(r, g, b, w);
        
        MatrixOS::LED::FillPartition(string(partition), color_obj);
    }

    void _MatrixOS_LED_Update(PikaObj *self) {
        MatrixOS::LED::Update();
    }

    int _MatrixOS_LED_CurrentLayer(PikaObj *self) {
        return MatrixOS::LED::CurrentLayer();
    }

    int _MatrixOS_LED_CreateLayer(PikaObj *self) {
        return MatrixOS::LED::CreateLayer();
    }

    void _MatrixOS_LED_CopyLayer(PikaObj *self, int dest, int src) {
        MatrixOS::LED::CopyLayer(dest, src);
    }

    pika_bool _MatrixOS_LED_DestroyLayer(PikaObj *self) {
        return MatrixOS::LED::DestroyLayer() ? pika_true : pika_false;
    }

    void _MatrixOS_LED_Fade(PikaObj *self) {
        MatrixOS::LED::Fade();
    }

    void _MatrixOS_LED_PauseUpdate(PikaObj *self, PikaObj* pause) {
        pika_bool pause_val = obj_getBool(pause, (char*)"value");
        MatrixOS::LED::PauseUpdate(pause_val != 0);
    }

    int _MatrixOS_LED_GetLedCount(PikaObj *self) {
        return MatrixOS::LED::GetLedCount();
    }
}