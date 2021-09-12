#include "MatrixOS.h"

namespace MatrixOS::USB
{
    void Init()
    {
        tusb_init();
    }

    bool Inited()
    {
        return tusb_inited();
    }

    bool Connected()
    {
        return tud_ready();
    }

    void Poll()
    {
        tud_task();
    }

    namespace CDC
    {
        uint32_t Available()
        {
            return tud_cdc_n_write_available(0);
        }

        void Poll(void)
        {
            //TODO
        }

        void Print(char const* str)
        {
            tud_cdc_n_write_str(0, str);
            tud_cdc_n_write_flush(0);
        }

        void Println(char const* str)
        {
            tud_cdc_n_write_str(0, str);
            tud_cdc_n_write_str(0, "\r\n");
            tud_cdc_n_write_flush(0);
        }

        void (*handler)(char const*) = nullptr;

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

        void SetHandler(void (*handler)(char const*))
        {
            CDC::handler = handler;
        }
    }
}