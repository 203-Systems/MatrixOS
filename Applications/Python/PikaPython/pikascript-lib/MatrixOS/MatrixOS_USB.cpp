#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // USB class implementation
    pika_bool _MatrixOS_USB_Connected(PikaObj *self) {
        return MatrixOS::USB::Connected();
    }
}