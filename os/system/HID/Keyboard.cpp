#include "MatrixOS.h"
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
    
    // Internal API
    size_t Set(KeyboardKeycode k, bool s)
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
            return 1;
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
                        return 1;
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
                        return 1;
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
                        return 1;
                    }
                }
            }
        }

        // No empty/pressed key was found
        return 0;
    }

    size_t RemoveAll(void)
    {
        // Release all keys
        size_t ret = 0;
        for (uint8_t i = 0; i < sizeof(_keyReport.keys); i++)
        {
            // Is a key in the list or did we found an empty slot?
            if(_keyReport.keys[i]){
                ret++;
            }
            _keyReport.keys[i] = 0x00;
        }
        return ret;
    }

    void Send()
    {
        if(!HID::Ready()) {
            return;
        }

        if (tud_suspended())
        {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
        }

        tud_hid_n_report(0, REPORT_ID_KEYBOARD, &_keyReport, sizeof(_keyReport));
    }

    // User API
    size_t Write(KeyboardKeycode k)
    {
    auto ret = Press(k);
    if(ret){
        Release(k);
    }
    return ret;
    }

    size_t Press(KeyboardKeycode k)
    {
    auto ret = Set(k, true);
    if(ret){
        Send();
    }
    return ret;
    }

    size_t Release(KeyboardKeycode k)
    {
    auto ret = Set(k, false);
    if(ret){
        Send();
    }
    return ret;
    }

    size_t ReleaseAll(void)
    {
        // Release all keys
        auto ret = RemoveAll();
        if(ret){
            Send();
        }
        return ret;
    }
}