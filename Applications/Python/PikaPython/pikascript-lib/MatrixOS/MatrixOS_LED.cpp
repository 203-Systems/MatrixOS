#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    void _MatrixOS_LED_CopyLayer(PikaObj *self, int dest, int src);
    int _MatrixOS_LED_CreateLayer(PikaObj *self);
    int _MatrixOS_LED_CurrentLayer(PikaObj *self);
    pika_bool _MatrixOS_LED_DestroyLayer(PikaObj *self);
    void _MatrixOS_LED_Fade(PikaObj *self);
    void _MatrixOS_LED_Fill(PikaObj *self, PikaObj* color);
    void _MatrixOS_LED_FillPartition(PikaObj *self, char* partition, PikaObj* color);
    int _MatrixOS_LED_GetLedCount(PikaObj *self);
    void _MatrixOS_LED_NextBrightness(PikaObj *self);
    void _MatrixOS_LED_PauseUpdate(PikaObj *self, PikaObj* pause);
    void _MatrixOS_LED_SetBrightness(PikaObj *self, int brightness);
    void _MatrixOS_LED_SetBrightnessMultiplier(PikaObj *self, char* partition_name, pika_float multiplier);
    void _MatrixOS_LED_SetColor(PikaObj *self, PikaObj* xy, PikaObj* color);
    void _MatrixOS_LED_SetColorByID(PikaObj *self, int id, PikaObj* color);
    void _MatrixOS_LED_Update(PikaObj *self);
}