#include <functional>
#include "MatrixOS.h"
#include "UI/Component/UISelector.h"
#include "UI/UI.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {

    // UISelector constructor — heap-allocated via handle wrapper
    void _MatrixOS_UISelector_UISelector___init__(PikaObj *self) {
        UISelector* sel = createCppHandleInPikaObj<UISelector>(self);
        obj_setPtr(self, (char*)"_component", static_cast<UIComponent*>(sel));
        InitCallbackContext(self);
    }

    // Close method — deterministic teardown.
    // Returns false if the native object is still referenced by a live UI.
    pika_bool _MatrixOS_UISelector_UISelector_Close(PikaObj *self) {
        UISelector* sel = getCppHandlePtrInPikaObj<UISelector>(self);
        if (sel && UI::IsComponentAttached(static_cast<UIComponent*>(sel))) {
            return false;
        }

        InvalidateCallbackContext(self);
        destroyCppHandleInPikaObj<UISelector>(self);
        obj_setPtr(self, (char*)"_component", nullptr);
        ClearCallbackInPikaObj(self, (char*)"getValueFunc");
        ClearCallbackInPikaObj(self, (char*)"changeCallback");
        ClearCallbackInPikaObj(self, (char*)"colorFunc");
        ClearCallbackInPikaObj(self, (char*)"nameFunc");
        DestroyCallbackContext(self);
        return true;
    }

    // SetValueFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetValueFunc(PikaObj *self, Arg* getValueFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"getValueFunc", getValueFunc);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        selector->SetValueFunc([ctx]() -> uint16_t {
            uint16_t retval = 0;
            Arg* result = SafeCallCallback0(ctx, (char*)"getValueFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_INT) {
                    retval = (uint16_t)arg_getInt(result);
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // SetLitMode method
    pika_bool _MatrixOS_UISelector_UISelector_SetLitMode(PikaObj *self, int litMode) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetLitMode((UISelectorLitMode)litMode);
        return true;
    }

    // SetDimension method
    pika_bool _MatrixOS_UISelector_UISelector_SetDimension(PikaObj *self, PikaObj* dimension) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        Dimension* dimPtr = getCppValuePtrInPikaObj<Dimension>(dimension);
        if (!dimPtr) return false;

        selector->SetDimension(*dimPtr);
        return true;
    }

    // SetName method
    pika_bool _MatrixOS_UISelector_UISelector_SetName(PikaObj *self, char* name) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetName(string(name));
        return true;
    }

    // SetCount method
    pika_bool _MatrixOS_UISelector_UISelector_SetCount(PikaObj *self, int count) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetCount((uint16_t)count);
        return true;
    }

    // SetDirection method
    pika_bool _MatrixOS_UISelector_UISelector_SetDirection(PikaObj *self, int direction) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetDirection((UISelectorDirection)direction);
        return true;
    }

    // SetColor method
    pika_bool _MatrixOS_UISelector_UISelector_SetColor(PikaObj *self, PikaObj* color) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        Color* colorPtr = getCppValuePtrInPikaObj<Color>(color);
        if (!colorPtr) return false;

        selector->SetColor(*colorPtr);
        return true;
    }

    // OnChange method
    pika_bool _MatrixOS_UISelector_UISelector_OnChange(PikaObj *self, Arg* changeCallback) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"changeCallback", changeCallback);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        selector->OnChange([ctx](uint16_t value) {
            Arg* valueArg = arg_newInt(value);
            Arg* result = SafeCallCallback1(ctx, (char*)"changeCallback", valueArg);

            if (result) arg_deinit(result);
            arg_deinit(valueArg);
        });

        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        selector->SetColorFunc([ctx]() -> Color {
            Color retval = Color::White;
            Arg* result = SafeCallCallback0(ctx, (char*)"colorFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_OBJECT) {
                    PikaObj* colorObj = arg_getObj(result);
                    Color* color = getCppValuePtrInPikaObj<Color>(colorObj);
                    if (color) {
                        retval = *color;
                    }
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // SetIndividualColorFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetIndividualColorFunc(PikaObj *self, Arg* colorFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        selector->SetIndividualColorFunc([ctx](uint16_t index) -> Color {
            Color retval = Color::White;
            Arg* indexArg = arg_newInt(index);
            Arg* result = SafeCallCallback1(ctx, (char*)"colorFunc", indexArg);

            if (result) {
                if (arg_getType(result) == ARG_TYPE_OBJECT) {
                    PikaObj* colorObj = arg_getObj(result);
                    Color* color = getCppValuePtrInPikaObj<Color>(colorObj);
                    if (color) {
                        retval = *color;
                    }
                }
                arg_deinit(result);
            }
            arg_deinit(indexArg);

            return retval;
        });

        return true;
    }

    // SetNameFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetNameFunc(PikaObj *self, Arg* nameFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"nameFunc", nameFunc);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        selector->SetIndividualNameFunc([ctx](uint16_t index) -> string {
            string retval = string("");
            Arg* indexArg = arg_newInt(index);
            Arg* result = SafeCallCallback1(ctx, (char*)"nameFunc", indexArg);

            if (result) {
                if (arg_getType(result) == ARG_TYPE_STRING) {
                    retval = string(arg_getStr(result));
                }
                arg_deinit(result);
            }
            arg_deinit(indexArg);

            return retval;
        });

        return true;
    }
}