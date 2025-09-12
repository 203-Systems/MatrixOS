#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // HID Ready function
    pika_bool _MatrixOS_HID_Ready(PikaObj *self) {
        return MatrixOS::HID::Ready();
    }

    // HID Keyboard functions
    pika_bool _MatrixOS_HID_Keyboard_Write(PikaObj *self, int keycode) {
        return MatrixOS::HID::Keyboard::Write((KeyboardKeycode)keycode);
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

    void _MatrixOS_HID_Gamepad_XAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::XAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_YAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::YAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_ZAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::ZAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_RXAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::RXAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_RYAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::RYAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_RZAxis(PikaObj *self, int value) {
        MatrixOS::HID::Gamepad::RZAxis((int16_t)value);
    }

    void _MatrixOS_HID_Gamepad_DPad(PikaObj *self, int direction) {
        MatrixOS::HID::Gamepad::DPad((GamepadDPadDirection)direction);
    }

    // HID RawHID functions
    Arg* _MatrixOS_HID_RawHID_Get(PikaObj *self, int timeout_ms) {
        uint8_t* report;
        size_t size = MatrixOS::HID::RawHID::Get(&report, (uint32_t)timeout_ms);
        
        if (size == 0 || report == nullptr) {
            return nullptr;
        }
        
        // TODO
    }

    pika_bool _MatrixOS_HID_RawHID_Send(PikaObj *self, char* data, int length) {
        std::vector<uint8_t> report;
        report.resize(length);
        memcpy(report.data(), data, length);
        return MatrixOS::HID::RawHID::Send(report) ? pika_true : pika_false;
    }

    // HID Consumer functions
   
    // HID System functions
}