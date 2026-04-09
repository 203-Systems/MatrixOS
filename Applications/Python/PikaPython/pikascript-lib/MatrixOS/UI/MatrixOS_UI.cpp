#include <functional>
#include "MatrixOS.h"
#include "UI/UI.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"
#include "../PikaCallbackUtils.h"

extern "C" {

    // UI constructor - supports optional parameters with progressive fill
    // Native UI object is heap-allocated via handle wrapper.
    void _MatrixOS_UI_UI___init__(PikaObj *self, PikaTuple* val) {
        int arg_count = pikaTuple_getSize(val);

        // Start with defaults
        string name = "";
        Color color = Color::White;
        bool newLEDLayer = true;

        // Progressive fill based on available arguments
        if (arg_count >= 1) {
            char* name_arg = pikaTuple_getStr(val, 0);
            name = string(name_arg);
        }

        if (arg_count >= 2) {
            Arg* colorArg = pikaTuple_getArg(val, 1);
            PikaObj* colorObj = arg_getObj(colorArg);
            Color* color_ptr = getCppValuePtrInPikaObj<Color>(colorObj);
            if (color_ptr) {
                color = *color_ptr;
            }
        }

        if (arg_count == 3) {
            newLEDLayer = pikaTuple_getBool(val, 2);
        }

        createCppHandleInPikaObj<UI>(self, name, color, newLEDLayer);
    }

    // Close method - deterministic teardown.
    // Destroys the native UI object, invalidates callbacks.
    void _MatrixOS_UI_UI_Close(PikaObj *self) {
        destroyCppHandleInPikaObj<UI>(self);

        // Clear all stored callbacks so they cannot be invoked after close
        ClearCallbackInPikaObj(self, (char*)"setupFunc");
        ClearCallbackInPikaObj(self, (char*)"loopFunc");
        ClearCallbackInPikaObj(self, (char*)"endFunc");
        ClearCallbackInPikaObj(self, (char*)"preRenderFunc");
        ClearCallbackInPikaObj(self, (char*)"postRenderFunc");
        ClearCallbackInPikaObj(self, (char*)"keyEventHandler");
    }

    // Start method
    void _MatrixOS_UI_UI_Start(PikaObj *self) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->Start();
    }

    // SetName method
    void _MatrixOS_UI_UI_SetName(PikaObj *self, char* name) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->SetName(string(name));
    }

    // SetColor method
    void _MatrixOS_UI_UI_SetColor(PikaObj *self, PikaObj* color) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        Color* colorPtr = getCppValuePtrInPikaObj<Color>(color);
        if (!colorPtr) return;

        ui->SetColor(*colorPtr);
    }

    // ShouldCreatenewLEDLayer method
    void _MatrixOS_UI_UI_ShouldCreatenewLEDLayer(PikaObj *self, pika_bool create) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->ShouldCreatenewLEDLayer(create);
    }

    // SetSetupFunc method
    pika_bool _MatrixOS_UI_UI_SetSetupFunc(PikaObj *self, Arg* setupFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"setupFunc", setupFunc);

        ui->SetSetupFunc([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"setupFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetLoopFunc method
    pika_bool _MatrixOS_UI_UI_SetLoopFunc(PikaObj *self, Arg* loopFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"loopFunc", loopFunc);

        ui->SetLoopFunc([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"loopFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetEndFunc method
    pika_bool _MatrixOS_UI_UI_SetEndFunc(PikaObj *self, Arg* endFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"endFunc", endFunc);

        ui->SetEndFunc([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"endFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetPreRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPreRenderFunc(PikaObj *self, Arg* pre_renderFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"preRenderFunc", pre_renderFunc);

        ui->SetPreRenderFunc([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"preRenderFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetPostRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPostRenderFunc(PikaObj *self, Arg* post_renderFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"postRenderFunc", post_renderFunc);

        ui->SetPostRenderFunc([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"postRenderFunc");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // SetKeyEventHandler - legacy bridge, will be replaced later
    pika_bool _MatrixOS_UI_UI_SetKeyEventHandler(PikaObj *self, Arg* key_event_handler) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;

        SaveCallbackObjToPikaObj(self, (char*)"keyEventHandler", key_event_handler);

        ui->SetInputEventHandler([self](InputEvent* inputEvent) -> bool {
            if (!hasCppHandleInPikaObj(self)) return false;

            Arg* result = CallCallbackInPikaObj0(self, (char*)"keyEventHandler");

            bool retval = false;
            if (result) {
                if (arg_getType(result) == ARG_TYPE_BOOL) {
                    retval = arg_getBool(result);
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // AddUIComponent method
    void _MatrixOS_UI_UI_AddUIComponent(PikaObj *self, PikaObj* uiComponent, PikaObj* xy) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        // Retrieve the UIComponent base pointer stored during construction.
        // This avoids unsafe void*→UIComponent* casts for derived types.
        UIComponent* component = static_cast<UIComponent*>(obj_getPtr(uiComponent, (char*)"_component"));
        if (!component) return;

        // Verify the owning handle is still alive
        if (!hasCppHandleInPikaObj(uiComponent)) return;

        Point* point = getCppValuePtrInPikaObj<Point>(xy);
        if (!point) return;

        ui->AddUIComponent(component, *point);
    }

    // ClearUIComponents method
    void _MatrixOS_UI_UI_ClearUIComponents(PikaObj *self) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->ClearUIComponents();
    }

    // AllowExit method
    void _MatrixOS_UI_UI_AllowExit(PikaObj *self, pika_bool allow) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->AllowExit(allow);
    }

    // SetFPS method
    void _MatrixOS_UI_UI_SetFPS(PikaObj *self, int fps) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->SetFPS(fps);
    }
}