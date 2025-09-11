#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    void _MatrixOS_Reboot(PikaObj *self) {
        MatrixOS::SYS::Reboot();
    }

    void _MatrixOS_Bootloader(PikaObj *self) {
        MatrixOS::SYS::Bootloader();
    }

    void _MatrixOS_DelayMs(PikaObj *self, int ms) {
        MatrixOS::SYS::DelayMs(ms);
    }

    int _MatrixOS_Millis(PikaObj *self) {
        return MatrixOS::SYS::Millis();
    }

    void _MatrixOS_OpenSetting(PikaObj *self) {
        MatrixOS::SYS::OpenSetting();
    }

    void _MatrixOS_ExecuteAPP(PikaObj *self, char* author, char* app_name) {
        MatrixOS::SYS::ExecuteAPP(string(author), string(app_name));
    }

    void _MatrixOS_ExecuteAPPByID(PikaObj *self, int app_id) {
        MatrixOS::SYS::ExecuteAPP(app_id);
    }

    int _MatrixOS_Test(PikaObj *self, int input)
    {
        return input;
    }
}