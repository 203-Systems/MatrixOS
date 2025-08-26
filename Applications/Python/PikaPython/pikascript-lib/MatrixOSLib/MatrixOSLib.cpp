#include "MatrixOS.h"
#include "pikaScript.h"

extern "C" {
    void MatrixOSLib_Reboot(PikaObj *self) {
        MatrixOS::SYS::Reboot();
    }

    void MatrixOSLib_Bootloader(PikaObj *self) {
        MatrixOS::SYS::Bootloader();
    }

    void MatrixOSLib_DelayMs(PikaObj *self, int ms) {
        MatrixOS::SYS::DelayMs(ms);
    }

    int MatrixOSLib_Millis(PikaObj *self) {
        return MatrixOS::SYS::Millis();
    }

    void MatrixOSLib_OpenSetting(PikaObj *self) {
        MatrixOS::SYS::OpenSetting();
    }

    void MatrixOSLib_ExecuteAPP(PikaObj *self, char* author, char* app_name) {
        MatrixOS::SYS::ExecuteAPP(string(author), string(app_name));
    }

    void MatrixOSLib_ExecuteAPPByID(PikaObj *self, int app_id) {
        MatrixOS::SYS::ExecuteAPP(app_id);
    }
}