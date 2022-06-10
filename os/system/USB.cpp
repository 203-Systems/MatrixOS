#include "MatrixOS.h"
#include "printf/printf.h"

namespace MatrixOS::USB
{
    void usb_device_task(void* param)
    {
        (void) param;
        // RTOS forever loop
        while (1)
        {
            // tinyusb device task
            tud_task();
        }
    }

    // Create a task for tinyusb device stack
    #define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE)
    StackType_t  usb_device_stack[USBD_STACK_SIZE];
    StaticTask_t usb_device_taskdef;
    void Init()
    {
        tusb_init();
        (void) xTaskCreateStatic( usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, usb_device_stack, &usb_device_taskdef);
    }

    bool Inited()
    {
        return tusb_inited();
    }

    bool Connected()
    {
        return tud_ready();
    }

    namespace CDC
    {

        bool Connected(void)
        {
            return tud_cdc_n_connected(0);
        }

        uint32_t Available(void)
        {
            return tud_cdc_n_available(0);
        }

        void Poll(void)
        {
            //TODO
        }

        void Print(string str)
        {
            for(uint16_t i = 0; i < str.length(); i++)
            {
                while(!tud_cdc_n_write_available(0)){}
                tud_cdc_n_write_char(0, str[i]);
            }
            // tud_cdc_n_write_str(0, str);
            Flush();
        }

        void Println(string str)
        {
            Print(str);
            Print("\n\r");
            Flush();
        }

        void WriteChar(char c, void* arg)
        {
            while(Connected() && !tud_cdc_n_write_available(0))
            {
                taskYIELD();
            }
            tud_cdc_n_write_char(0, c);
        }

        void Printf(string format, ...)
        {
            // Print(format);
            va_list valst;
            va_start(valst, format.c_str());
            VPrintf(format, valst);
        }

        void VPrintf(string format, va_list valst)
        {
            vfctprintf(&WriteChar, NULL, format.c_str(), valst);
            Flush();
        }


        void Flush(void)
        {
            tud_cdc_n_write_flush(0);
        }

        // void Read() //Prob won't work, implentation need work
        // {
        //     if (tud_cdc_n_available(0))
        //     {
        //         char buf[256];
        //         uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        //         if(handler)
        //         {
        //             handler(buf); 
        //         }
        //     }
        // }

        int8_t Read(void)
        {
            return tud_cdc_n_read_char(0);
        }

        uint32_t  ReadBytes(void* buffer, uint32_t length)
        {
            return tud_cdc_n_read(0, buffer, length);
        }

        string ReadString(void)
        {
            string str;
            uint8_t i = 0;
            // Print("Buffer remains ");
            // Println(std::to_string(Available()).c_str());
            while(Available())
            {
                int8_t c = Read();
                // Print("Char read ");
                // Println(std::to_string(c).c_str());
                if(c == -1) break;
                i++;
                str.push_back(c);
                if(c == 0) break;
            }
            // Print("String read with length ");
            // Println(std::to_string(i).c_str());
            return str;
        }
    }
}

void putchar_(char character)
{
    tud_cdc_n_write_char(0, character);
}