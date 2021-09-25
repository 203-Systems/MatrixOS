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

        void Print(char const* str)
        {
            for(uint16_t i = 0; i < strlen(str); i++)
            {
                tud_cdc_n_write_char(0, str[i]);
            }
            // tud_cdc_n_write_str(0, str);
            tud_cdc_n_write_flush(0);
        }

        void Println(char const* str)
        {
            for(uint16_t i = 0; i < strlen(str); i++)
            {
                tud_cdc_n_write_char(0, str[i]);
            }
            tud_cdc_n_write_char(0, '\r');
            tud_cdc_n_write_char(0, '\n');
            tud_cdc_n_write_flush(0);
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

        std::string ReadString(void)
        {
            std::string str;
            while(Available())
            {
                int8_t c = Read();
                if(c == -1) break;
                str.push_back(c);
                if(c == 0) break;
            }
            return str;
        }
    }
}