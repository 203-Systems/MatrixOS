// Declares shared conversion, object construction, and callback helpers for MatrixOS bindings.
#pragma once

#ifndef NO_QSTR
#include "MatrixOS.h"
#endif

extern "C" {
#include "py/obj.h"
}

namespace MatrixOSPython
{
uint32_t ColorToRgb(const Color& color);
mp_obj_t MakeTick(uint64_t value);

Color ObjectToColor(mp_obj_t colorObj);
Dimension ObjectToDimension(mp_obj_t dimensionObj);
Point ObjectToPoint(mp_obj_t pointObj);
InputId ObjectToInputId(mp_obj_t inputIdObj);
uint32_t ObjectToHash(mp_obj_t hashObj);

mp_obj_t MakeInputId(const InputId& id);
mp_obj_t MakePartition(const LEDPartition& partition);
mp_obj_t MakePoint(const Point& point);
mp_obj_t MakeKeypadInfo(const KeypadInfo& keypad);
mp_obj_t MakeInputEvent(const InputEvent& event);
mp_obj_t MakeInputCluster(const InputCluster& cluster);
mp_obj_t MakeInputSnapshot(const InputSnapshot& snapshot);
mp_obj_t MakeKeypadCapabilities(const KeypadCapabilities& capabilities);

mp_obj_t ProtectedCall(size_t argc, const mp_obj_t* args);
Color CallColorCallback(mp_obj_t callback, Color fallback);
Color CallIndexedColorCallback(mp_obj_t callback, uint16_t index, Color fallback);
int32_t CallIntCallback(mp_obj_t callback, int32_t fallback);
string CallIndexedStringCallback(mp_obj_t callback, uint16_t index, const string& fallback);
} // namespace MatrixOSPython
