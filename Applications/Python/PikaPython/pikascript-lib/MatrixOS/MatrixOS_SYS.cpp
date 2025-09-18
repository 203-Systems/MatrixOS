#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    void _MatrixOS_SYS_Reboot(PikaObj *self) {
        MatrixOS::SYS::Reboot();
        }

    void _MatrixOS_SYS_Bootloader(PikaObj *self) {
        MatrixOS::SYS::Bootloader();
    }

    void _MatrixOS_SYS_DelayMs(PikaObj *self, int ms) {
        MatrixOS::SYS::DelayMs(ms);
    }

    int64_t _MatrixOS_SYS_Millis(PikaObj *self) {
        return MatrixOS::SYS::Millis();
    }

    int64_t _MatrixOS_SYS_Micros(PikaObj *self) {
        return MatrixOS::SYS::Micros();
    }

    void _MatrixOS_SYS_OpenSetting(PikaObj *self) {
        MatrixOS::SYS::OpenSetting();
    }

    void _MatrixOS_SYS_ExecuteAPP(PikaObj *self, char* author, char* app_name, PikaObj* args_list) {
        vector<string> args;

        // Convert PikaPython list to vector<string>
        if (args_list != nullptr) {
            size_t list_len = pikaList_getSize((PikaList*)args_list);
            for (int i = 0; i < list_len; i++) {
                char* item = pikaList_getStr((PikaList*)args_list, i);
                if (item != nullptr) {
                    args.push_back(string(item));
                }
            }
        }

        MatrixOS::SYS::ExecuteAPP(string(author), string(app_name), args);
    }

    void _MatrixOS_SYS_ExecuteAPPByID(PikaObj *self, int app_id, PikaObj* args_list) {
        vector<string> args;

        // Convert PikaPython list to vector<string>
        if (args_list != nullptr) {
            size_t list_len = pikaList_getSize((PikaList*)args_list);
            for (int i = 0; i < list_len; i++) {
                char* item = pikaList_getStr((PikaList*)args_list, i);
                if (item != nullptr) {
                    args.push_back(string(item));
                }
            }
        }

        MatrixOS::SYS::ExecuteAPP(app_id, args);
    }

}