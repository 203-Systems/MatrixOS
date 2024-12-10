#include "MatrixOS.h"

typedef struct {
	// 32 Buttons, 6 Axis, 2 D-Pads
    int16_t	xAxis;
    int16_t	yAxis;
    int16_t	zAxis;
    int16_t	rzAxis;
    int16_t	rxAxis;
    int16_t	ryAxis;

    uint8_t	dPad;

    uint32_t buttons;
} HID_GamepadReport_Data_t;

namespace MatrixOS::HID::Gamepad
{   
    HID_GamepadReport_Data_t _report;

    void Send(void){ 
        // tud_hid_n_report(0, REPORT_ID_GAMEPAD, &_report, sizeof(_report));
        // MLOGD("Gamepad", "%d %d %d %d %d %d %d %d", _report.xAxis, _report.yAxis, _report.zAxis, _report.rzAxis, _report.rxAxis, _report.ryAxis, _report.dPad, _report.buttons);

        tud_hid_n_report(0, REPORT_ID_GAMEPAD, &_report, 17); // sizeof(_report));
    }

    void Press(uint8_t b)
    {
        _report.buttons |= (uint32_t)1 << b; 
        Send();
    }

    void Release(uint8_t b)
    {
        _report.buttons &= ~((uint32_t)1 << b); 
        Send();
    }

    void ReleaseAll(void)
    {
        _report.buttons = 0;
        Send();
    }

    void Button(uint8_t b, bool state)
    {
        if (state)
        {
            Press(b);
        }
        else
        {
            Release(b);
        }
    }

    void Buttons(uint32_t b)
    {
        _report.buttons = b; 
        Send();
    }

    void XAxis(int16_t value){ 
        _report.xAxis = value; 
        Send();
    }


    void YAxis(int16_t value){ 
        _report.yAxis = value; 
        Send();
    }


    void ZAxis(int16_t value){ 
        _report.zAxis = value; 
        Send();
    }


    void RXAxis(int16_t value){ 
        _report.rxAxis = value; 
        Send();
    }


    void RYAxis(int16_t value){ 
        _report.ryAxis = value; 
        Send();
    }


    void RZAxis(int16_t value){ 
        _report.rzAxis = value; 
        Send();
    }


    void DPad(GamepadDPadDirection d){ 
        _report.dPad = d; 
        Send();
    }
}