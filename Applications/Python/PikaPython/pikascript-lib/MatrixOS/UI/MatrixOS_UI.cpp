#include "MatrixOS.h"
#include "UI/UI.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"
#include "../PikaCallbackUtils.h"

struct PythonUIInputQueue {
    static constexpr uint8_t SIZE = 16;
    uint32_t codes[SIZE] = {};
    uint8_t head = 0;
    uint8_t tail = 0;
    uint8_t count = 0;

    void Push(uint32_t code) {
        codes[tail] = code;
        tail = (tail + 1) % SIZE;
        if (count < SIZE) {
            count++;
        } else {
            head = (head + 1) % SIZE;
        }
    }

    bool Pop(uint32_t* code) {
        if (count == 0) return false;
        *code = codes[head];
        head = (head + 1) % SIZE;
        count--;
        return true;
    }

    void Clear() {
        head = 0;
        tail = 0;
        count = 0;
    }
};

static PythonUIInputQueue* GetInputQueue(PikaObj* self) {
    return (PythonUIInputQueue*)obj_getPtr(self, (char*)"_inputQueue");
}

static void InstallInputCapture(UI* ui, PythonUIInputQueue* queue) {
    if (!ui || !queue) return;

    ui->SetInputEventHandler([queue](InputEvent* inputEvent) -> bool {
        if (inputEvent->inputClass != InputClass::Keypad) return false;

        uint32_t code = (uint32_t)inputEvent->id.clusterId |
                        ((uint32_t)inputEvent->keypad.state << 8) |
                        ((uint32_t)inputEvent->id.memberId << 16);
        queue->Push(code);
        return false;
    });
}

extern "C" {
    int _MatrixOS_UI_UI_PullInputCode(PikaObj *self) {
        if (!self) return -1;

        PythonUIInputQueue* queue = GetInputQueue(self);
        if (!queue) return -1;

        uint32_t code = 0;
        if (!queue->Pop(&code)) return -1;
        return (int)code;
    }

    // UI constructor - supports optional parameters with progressive fill
    // Native UI object is heap-allocated via handle wrapper.
    void _MatrixOS_UI_UI___init__(PikaObj *self, PikaTuple* val) {
        int arg_count = val == nullptr ? 0 : pikaTuple_getSize(val);

        // Start with defaults
        string name = "";
        Color color = Color::White;
        bool newLEDLayer = true;

        // Progressive fill based on available arguments
        if (arg_count >= 1) {
            char* name_arg = pikaTuple_getStr(val, 0);
            if (name_arg) {
                name = string(name_arg);
            }
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
        InitCallbackContext(self);

        PythonUIInputQueue* queue = new PythonUIInputQueue();
        obj_setPtr(self, (char*)"_inputQueue", queue);

        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        InstallInputCapture(ui, queue);
    }

    // Close method - deterministic teardown.
    // Invalidates callbacks, destroys native UI, frees context.
    pika_bool _MatrixOS_UI_UI_Close(PikaObj *self) {
        InvalidateCallbackContext(self);

        destroyCppHandleInPikaObj<UI>(self);

        ClearCallbackInPikaObj(self, (char*)"setupFunc");
        ClearCallbackInPikaObj(self, (char*)"loopFunc");
        ClearCallbackInPikaObj(self, (char*)"endFunc");
        ClearCallbackInPikaObj(self, (char*)"preRenderFunc");
        ClearCallbackInPikaObj(self, (char*)"postRenderFunc");

        PythonUIInputQueue* queue = GetInputQueue(self);
        if (queue) {
            delete queue;
            obj_setPtr(self, (char*)"_inputQueue", nullptr);
        }

        DestroyCallbackContext(self);
        return true;
    }

    // Start method
    void _MatrixOS_UI_UI_Start(PikaObj *self) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->Start();
    }

    // Exit method
    void _MatrixOS_UI_UI_Exit(PikaObj *self) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return;

        ui->Exit();
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
        if (!setupFunc) return false;

        if (!SaveCallbackObjToPikaObj(self, (char*)"setupFunc", setupFunc)) return false;
        PythonCallbackContext* ctx = GetCallbackContext(self);

        ui->SetSetupFunc([ctx]() {
            Arg* result = SafeCallCallback0(ctx, (char*)"setupFunc");
            if (result) arg_deinit(result);
        });

        return true;
    }

    // SetLoopFunc method
    pika_bool _MatrixOS_UI_UI_SetLoopFunc(PikaObj *self, Arg* loopFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;
        if (!loopFunc) return false;

        if (!SaveCallbackObjToPikaObj(self, (char*)"loopFunc", loopFunc)) return false;
        PythonCallbackContext* ctx = GetCallbackContext(self);

        ui->SetLoopFunc([ctx]() {
            Arg* result = SafeCallCallback0(ctx, (char*)"loopFunc");
            if (result) arg_deinit(result);

            PikaObj* owner = ctx ? ctx->owner : nullptr;
            PythonUIInputQueue* queue = GetInputQueue(owner);
            if (queue) queue->Clear();
        });

        return true;
    }

    // SetEndFunc method
    pika_bool _MatrixOS_UI_UI_SetEndFunc(PikaObj *self, Arg* endFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;
        if (!endFunc) return false;

        if (!SaveCallbackObjToPikaObj(self, (char*)"endFunc", endFunc)) return false;
        PythonCallbackContext* ctx = GetCallbackContext(self);

        ui->SetEndFunc([ctx]() {
            Arg* result = SafeCallCallback0(ctx, (char*)"endFunc");
            if (result) arg_deinit(result);
        });

        return true;
    }

    // SetPreRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPreRenderFunc(PikaObj *self, Arg* pre_renderFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;
        if (!pre_renderFunc) return false;

        if (!SaveCallbackObjToPikaObj(self, (char*)"preRenderFunc", pre_renderFunc)) return false;
        PythonCallbackContext* ctx = GetCallbackContext(self);

        ui->SetPreRenderFunc([ctx]() {
            Arg* result = SafeCallCallback0(ctx, (char*)"preRenderFunc");
            if (result) arg_deinit(result);
        });

        return true;
    }

    // SetPostRenderFunc method
    pika_bool _MatrixOS_UI_UI_SetPostRenderFunc(PikaObj *self, Arg* post_renderFunc) {
        UI* ui = getCppHandlePtrInPikaObj<UI>(self);
        if (!ui) return false;
        if (!post_renderFunc) return false;

        if (!SaveCallbackObjToPikaObj(self, (char*)"postRenderFunc", post_renderFunc)) return false;
        PythonCallbackContext* ctx = GetCallbackContext(self);

        ui->SetPostRenderFunc([ctx]() {
            Arg* result = SafeCallCallback0(ctx, (char*)"postRenderFunc");
            if (result) arg_deinit(result);
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
