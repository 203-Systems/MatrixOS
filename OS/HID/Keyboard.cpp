#include "MatrixOS.h"
#include "USB.h"
#include "tusb.h"
#include "task.h"

#define NKRO_KEY_COUNT (8*13)

typedef union __attribute__((packed, aligned(1))) {
	// Low level key report: up to 6 keys and shift, ctrl etc at once
	uint8_t whole8[0];
	uint16_t whole16[0];
	uint32_t whole32[0];
	struct __attribute__((packed, aligned(1))) {
		uint8_t modifiers;
		uint8_t reserved;
		KeyboardKeycode keycodes[6];
	};
	uint8_t keys[8];
} HID_KeyboardReport_Data_t;

namespace MatrixOS::HID::Keyboard
{
    HID_KeyboardReport_Data_t _keyReport;
    static TaskHandle_t _sendTaskHandle = nullptr;
    static StaticTask_t _sendTaskBuffer;
    static StackType_t _sendTaskStack[configMINIMAL_STACK_SIZE];
    static uint64_t _lastSendMicros = 0;

    static void SendTask(void* param);
    static void EnsureSendTask();
    
    // Internal API
    bool Set(KeyboardKeycode k, bool s)
    {
        // It's a modifier key
        if(k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI)
        {
            // Convert key into bitfield (0 - 7)
            k = KeyboardKeycode(uint8_t(k) - uint8_t(KEY_LEFT_CTRL));
            if(s){
                _keyReport.modifiers |= (1 << k);
            }
            else{
                _keyReport.modifiers &= ~(1 << k);
            }
            return true;
        }
        // Its a normal key
        else{
            // get size of keycodes during compile time
            const uint8_t keycodesSize = sizeof(_keyReport.keycodes);

            // if we are adding an element to keycodes
            if (s){
                // iterate through the keycodes
                for (uint8_t i = 0; i < keycodesSize; i++)
                {
                    auto key = _keyReport.keycodes[i];
                    // if target key is found
                    if (key == uint8_t(k)) {
                        // do nothing and exit
                        return true;
                    }
                }
                // iterate through the keycodes again, this only happens if no existing
                // keycodes matches k
                for (uint8_t i = 0; i < keycodesSize; i++)
                {
                    auto key = _keyReport.keycodes[i];
                    // if first instance of empty slot is found
                    if (key == KEY_RESERVED) {
                        // change empty slot to k and exit
                        _keyReport.keycodes[i] = k;
                        return true;
                    }
                }
            } else { // we are removing k from keycodes
                // iterate through the keycodes
                for (uint8_t i = 0; i < keycodesSize; i++)
                {
                    auto key = _keyReport.keycodes[i];
                    // if target key is found
                    if (key == k) {
                        // remove target and exit
                        _keyReport.keycodes[i] = KEY_RESERVED;
                        return true;
                    }
                }
            }
        }

        // No empty/pressed key was found
        return false;
    }

    static void SendTask(void* param)
    {
        (void)param;

        while(true)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            if(!HID::Ready())
            {
                continue;
            }

            if (tud_suspended())
            {
                // Wake up host if we are in suspend mode
                // and REMOTE_WAKEUP feature is enabled by host
                tud_remote_wakeup();
            }

            uint64_t nowMicros = MatrixOS::SYS::Micros();
            if(_lastSendMicros != 0)
            {
                uint64_t elapsed = nowMicros - _lastSendMicros;
                if(elapsed < 1000) // keep at least 1ms gap
                {
                    MatrixOS::SYS::DelayMs(1);
                }
            }

            tud_hid_n_report(0, REPORT_ID_KEYBOARD, &_keyReport, sizeof(_keyReport));
            _lastSendMicros = MatrixOS::SYS::Micros();
        }
    }

    static void EnsureSendTask()
    {
        if(_sendTaskHandle == nullptr)
        {
            _sendTaskHandle = xTaskCreateStatic(SendTask, "kbd_send", configMINIMAL_STACK_SIZE, nullptr, 2, _sendTaskStack, &_sendTaskBuffer);
        }
    }

    void Send()
    {
        EnsureSendTask();
        if(_sendTaskHandle != nullptr)
        {
            xTaskNotifyGive(_sendTaskHandle);
        }
    }

    // User API
    bool Tap(KeyboardKeycode k, uint16_t length_ms)
    {
        bool ret = Press(k);
        if(ret)
        {
            SYS::DelayMs(length_ms);
            ret = Release(k);
        }
        return ret;
    }

    bool Press(KeyboardKeycode k)
    {
        bool ret = Set(k, true);
        if(ret){
            Send();
        }
        return ret;
    }

    bool Release(KeyboardKeycode k)
    {
        bool ret = Set(k, false);
        if(ret){
            Send();
        }
        return ret;
    }

    void ReleaseAll(void)
    {
        memset(&_keyReport, 0, sizeof(_keyReport));
        Send();
    }
}
