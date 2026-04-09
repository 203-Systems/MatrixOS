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
    }

    // Close method — deterministic teardown.
    // Refuses to destroy if the native object is still referenced by a live UI.
    void _MatrixOS_UISelector_UISelector_Close(PikaObj *self) {
        UISelector* sel = getCppHandlePtrInPikaObj<UISelector>(self);
        if (sel && UI::IsComponentAttached(static_cast<UIComponent*>(sel))) {
            return;  // still owned by a UI
        }

        destroyCppHandleInPikaObj<UISelector>(self);
        obj_setPtr(self, (char*)"_component", nullptr);
        ClearCallbackInPikaObj(self, (char*)"getValueFunc");
        ClearCallbackInPikaObj(self, (char*)"changeCallback");
        ClearCallbackInPikaObj(self, (char*)"colorFunc");
        ClearCallbackInPikaObj(self, (char*)"nameFunc");
    }

    // SetValueFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetValueFunc(PikaObj *self, Arg* getValueFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"getValueFunc", getValueFunc);

        selector->SetValueFunc([self]() -> uint16_t {
            if (!hasCppHandleInPikaObj(self)) return 0;
            uint16_t retval = 0;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"getValueFunc");

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

        selector->OnChange([self](uint16_t value) {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* valueArg = arg_newInt(value);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"changeCallback", valueArg);

            if (result) {
                arg_deinit(result);
            }
            arg_deinit(valueArg);
        });

        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UISelector* selector = getCppHandlePtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        selector->SetColorFunc([self]() -> Color {
            if (!hasCppHandleInPikaObj(self)) return Color::White;
            Color retval = Color::White;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"colorFunc");

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

        selector->SetIndividualColorFunc([self](uint16_t index) -> Color {
            if (!hasCppHandleInPikaObj(self)) return Color::White;
            Color retval = Color::White;
            Arg* indexArg = arg_newInt(index);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"colorFunc", indexArg);

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

        selector->SetIndividualNameFunc([self](uint16_t index) -> string {
            if (!hasCppHandleInPikaObj(self)) return string("");
            string retval = string("");
            Arg* indexArg = arg_newInt(index);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"nameFunc", indexArg);

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