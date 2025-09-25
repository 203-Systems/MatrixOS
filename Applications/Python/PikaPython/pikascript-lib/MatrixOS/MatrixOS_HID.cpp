#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // HID Ready function
    pika_bool _MatrixOS_HID_Ready(PikaObj *self) {
        return MatrixOS::HID::Ready();
    }

    // HID Keyboard functions
    pika_bool _MatrixOS_HID_Keyboard_Tap(PikaObj *self, int keycode, int length_ms) {
        return MatrixOS::HID::Keyboard::Tap((KeyboardKeycode)keycode, length_ms);
    }

    pika_bool _MatrixOS_HID_Keyboard_Press(PikaObj *self, int keycode) {
        return MatrixOS::HID::Keyboard::Press((KeyboardKeycode)keycode);
    }

    pika_bool _MatrixOS_HID_Keyboard_Release(PikaObj *self, int keycode) {
        return MatrixOS::HID::Keyboard::Release((KeyboardKeycode)keycode);
    }

    void _MatrixOS_HID_Keyboard_ReleaseAll(PikaObj *self) {
        MatrixOS::HID::Keyboard::ReleaseAll();
    }

    // HID Mouse functions
   
    // HID Touch functions
    
    // HID Gamepad functions

    void _MatrixOS_HID_Gamepad_Tap(PikaObj *self, int button_id, int length_ms) {
        MatrixOS::HID::Gamepad::Tap(button_id, length_ms);
    }

    void _MatrixOS_HID_Gamepad_Press(PikaObj *self, int button_id) {
        MatrixOS::HID::Gamepad::Press((uint8_t)button_id);
    }

    void _MatrixOS_HID_Gamepad_Release(PikaObj *self, int button_id) {
        MatrixOS::HID::Gamepad::Release((uint8_t)button_id);
    }   

    void _MatrixOS_HID_Gamepad_ReleaseAll(PikaObj *self) {
        MatrixOS::HID::Gamepad::ReleaseAll();
    }

    void _MatrixOS_HID_Gamepad_Button(PikaObj *self, int button_id, pika_bool state) {
        MatrixOS::HID::Gamepad::Button((uint8_t)button_id, state != 0);
    }

    void _MatrixOS_HID_Gamepad_Buttons(PikaObj *self, int button_mask) {
        MatrixOS::HID::Gamepad::Buttons((uint32_t)button_mask);
    }

    // Helper function to convert float (-1.0 to 1.0) to int16_t with clamping
    int16_t clampFloatToInt16(pika_float value) {
        // Clamp input to -1.0 to 1.0 range
        if (value > 1.0f) value = 1.0f;
        if (value < -1.0f) value = -1.0f;
        // Convert to int16_t range (-32767 to 32767)
        return (int16_t)(value * 32767.0f);
    }
    void _MatrixOS_HID_Gamepad_XAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::XAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_YAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::YAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_ZAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::ZAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_RXAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::RXAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_RYAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::RYAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_RZAxis(PikaObj *self, pika_float value) {
        MatrixOS::HID::Gamepad::RZAxis(clampFloatToInt16(value));
    }

    void _MatrixOS_HID_Gamepad_DPad(PikaObj *self, int direction) {
        MatrixOS::HID::Gamepad::DPad((GamepadDPadDirection)direction);
    }

    // HID RawHID functions
    Arg* _MatrixOS_HID_RawHID_Get(PikaObj *self, int timeout_ms) {
        uint8_t* report;
        size_t size = MatrixOS::HID::RawHID::Get(&report, (uint32_t)timeout_ms);
        
        if (size == 0 || report == nullptr) {
            Arg* result = arg_newBytes(report, 0);
            return result;
        }
        
        // Create bytes argument with the report data
        Arg* result = arg_newBytes(report, size);
        
        return result;
    }

    pika_bool _MatrixOS_HID_RawHID_Send(PikaObj *self, char* data, int length) {
        std::vector<uint8_t> report;
        report.resize(length);
        memcpy(report.data(), data, length);
        return MatrixOS::HID::RawHID::Send(report);
    }

    // HID Consumer functions
   
    // HID System functions
}