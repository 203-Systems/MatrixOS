#include <functional>
#include "MatrixOS.h"
#include "UI/UI.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"
#include "../PikaCallbackUtils.h"


extern "C" {
    // Forward declaration for KeyEvent constructor
    PikaObj* New__MatrixOS_KeyEvent_KeyEvent(Args *args);

    // UI constructor - supports optional parameters with progressive fill
    void _MatrixOS_UI_UI___init__(PikaObj *self, PikaTuple* val) {
        int arg_count = pikaTuple_getSize(val);

        // Start with defaults
        string name = "";
        Color color = Color(0xFFFFFF);
        bool newLEDLayer = true;

        // Progressive fill based on available arguments
        if (arg_count >= 1) {
            char* name_arg = pikaTuple_getStr(val, 0);
            name = string(name_arg);
        }

        if (arg_count >= 2) {
            Arg* colorArg = pikaTuple_getArg(val, 1);
            PikaObj* colorObj = arg_getObj(colorArg);
            Color* color_ptr = getCppObjPtrInPikaObj<Color>(colorObj);
            if (color_ptr) {
                color = *color_ptr;
            }
        }

        if (arg_count == 3) {
            newLEDLayer = pikaTuple_getBool(val, 2);
        }

        // Create object with filled parameters
        createCppObjPtrInPikaObj<UI>(self, name, color, newLEDLayer);
    }

    // Start method
    void _MatrixOS_UI_UI_Start(PikaObj *self) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->Start();
    }

    // SetName method
    void _MatrixOS_UI_UI_SetName(PikaObj *self, char* name) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->SetName(string(name));
    }

    // SetColor method
    void _MatrixOS_UI_UI_SetColor(PikaObj *self, PikaObj* color) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        Color* colorPtr = getCppObjPtrInPikaObj<Color>(color);
        if (!colorPtr) return;

        ui->SetColor(*colorPtr);
    }

    // ShouldCreatenewLEDLayer method
    void _MatrixOS_UI_UI_ShouldCreatenewLEDLayer(PikaObj *self, pika_bool create) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->ShouldCreatenewLEDLayer(create);
    }

    // SetSetupFunc method
    pika_bool _MatrixOS_UI_UI_SetSetupFunc(PikaObj *self, Arg* setupFunc) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"setupFunc", setupFunc);

        ui->SetSetupFunc([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"setupFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetLoopFunc method
    pika_bool _MatrixOS_UI_UI_SetLoopFunc(PikaObj *self, Arg* loopFunc) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"loopFunc", loopFunc);

        ui->SetLoopFunc([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"loopFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetEndFunc method
    pika_bool _MatrixOS_UI_UI_SetEndFunc(PikaObj *self, Arg* endFunc) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"endFunc", endFunc);

        ui->SetEndFunc([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"endFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetPreRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPreRenderFunc(PikaObj *self, Arg* pre_renderFunc) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"preRenderFunc", pre_renderFunc);

        ui->SetPreRenderFunc([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"preRenderFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetPostRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPostRenderFunc(PikaObj *self, Arg* post_renderFunc) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"postRenderFunc", post_renderFunc);

        ui->SetPostRenderFunc([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"postRenderFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetKeyEventHandler method
    pika_bool _MatrixOS_UI_UI_SetKeyEventHandler(PikaObj *self, Arg* key_event_handler) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"keyEventHandler", key_event_handler);

        ui->SetKeyEventHandler([self](KeyEvent* keyEvent) -> bool {
            // Create a KeyEvent Python object and pass it to the callback
            PikaObj* keyEventObj = newNormalObj(New__MatrixOS_KeyEvent_KeyEvent);
            copyCppObjIntoPikaObj<KeyEvent>(keyEventObj, *keyEvent);

            // Pack the object into an Arg for the callback
            Arg* keyEventArg = arg_newObj(keyEventObj);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"keyEventHandler", keyEventArg);

            // Convert result to bool
            bool retval = false;
            if (result) {
                if (arg_getType(result) == ARG_TYPE_BOOL) {
                    retval = arg_getBool(result);
                }
                arg_deinit(result);
            }

            arg_deinit(keyEventArg);
            obj_deinit(keyEventObj);

            return retval;
        });

        return true;
    }

    // AddUIComponent method
    void _MatrixOS_UI_UI_AddUIComponent(PikaObj *self, PikaObj* uiComponent, PikaObj* xy) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        UIComponent* component = getCppObjPtrInPikaObj<UIComponent>(uiComponent);
        if (!component) return;

        Point* point = getCppObjPtrInPikaObj<Point>(xy);
        if (!point) return;

        ui->AddUIComponent(component, *point);
    }

    // ClearUIComponents method
    void _MatrixOS_UI_UI_ClearUIComponents(PikaObj *self) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->ClearUIComponents();
    }

    // AllowExit method
    void _MatrixOS_UI_UI_AllowExit(PikaObj *self, pika_bool allow) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->AllowExit(allow);
    }

    // SetFPS method
    void _MatrixOS_UI_UI_SetFPS(PikaObj *self, int fps) {
        UI* ui = getCppObjPtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->SetFPS(fps);
    }
}