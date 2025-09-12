#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // USB class implementation
    pika_bool _MatrixOS_USB_Connected(PikaObj *self) {
        return MatrixOS::USB::Connected();
    }

    // USB CDC class implementation
    pika_bool _MatrixOS_USB_CDC_Connected(PikaObj *self) {
        return MatrixOS::USB::CDC::Connected();
    }

    int _MatrixOS_USB_CDC_Available(PikaObj *self) {
        return MatrixOS::USB::CDC::Available();
    }

    void _MatrixOS_USB_CDC_Poll(PikaObj *self) {
        MatrixOS::USB::CDC::Poll();
    }

    void _MatrixOS_USB_CDC_Print(PikaObj *self, char* text, char* end) {
        MatrixOS::USB::CDC::Print(string(text));
        MatrixOS::USB::CDC::Print(string(end));
    }

    void _MatrixOS_USB_CDC_Flush(PikaObj *self) {
        MatrixOS::USB::CDC::Flush();
    }

    int _MatrixOS_USB_CDC_Read(PikaObj *self) {
        return MatrixOS::USB::CDC::Read();
    }

    PikaObj* _MatrixOS_USB_CDC_ReadBytes(PikaObj *self, int length) {
        // // Create a buffer to read data
        // uint8_t* buffer = new uint8_t[length];
        // uint32_t bytes_read = MatrixOS::USB::CDC::ReadBytes(buffer, length);
        
        // // Create PikaObj to return bytes
        // PikaObj* bytes_obj = newNormalObj(New_PikaObj);
        
        // // Store the buffer and length for later retrieval
        // obj_setPtr(bytes_obj, (char*)"data", buffer);
        // obj_setInt(bytes_obj, (char*)"length", bytes_read);

        // return bytes_obj;

        //TODO not new
        
        return NULL;
    }

    PikaObj* _MatrixOS_USB_CDC_ReadString(PikaObj *self) {
        string result = MatrixOS::USB::CDC::ReadString();
        
        PikaObj* str_obj = newNormalObj(New_PikaObj);
        obj_setStr(str_obj, (char*)"value", (char*)result.c_str());
        
        return str_obj;
    }
}