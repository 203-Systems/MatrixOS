//Define Device Keypad Function
#include "Device.h"

#define FSR_KEYPAD_ADC_ATTEN ADC_ATTEN_DB_0
#define FSR_KEYPAD_ADC_WIDTH ADC_WIDTH_BIT_12

namespace Device::KeyPad
{
    esp_adc_cal_characteristics_t adc1_chars;

    void Init()
    {
        InitKeyPad();
        InitTouchBar();
    }

    void InitKeyPad()
    {        
        gpio_config_t io_conf;

        //Config FN
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pin_bit_mask = (1ULL<<fn_pin);
        #ifdef fn_pin_ACTIVE_HIGH //Active HIGH
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        #else //Active Low
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        #endif
        gpio_config(&io_conf);

        //Config Matrix Input

        if(!FSR) //Non FSR keypad
        {
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            for(uint8_t y = 0; y < y_size; y++)
            {
                io_conf.pin_bit_mask = (1ULL<<keypad_read_pins[y]);
                gpio_config(&io_conf);
                gpio_set_pull_mode(keypad_read_pins[y], GPIO_PULLDOWN_ONLY);
            }
        }
        else //FSR keypad
        {
            adc1_config_width(FSR_KEYPAD_ADC_WIDTH);
            esp_adc_cal_characterize(ADC_UNIT_1, FSR_KEYPAD_ADC_ATTEN,  FSR_KEYPAD_ADC_WIDTH, 0, &adc1_chars);
            for(uint8_t y = 0; y < y_size; y++)
            {
                adc1_config_channel_atten(keypad_read_adc_channel[y], FSR_KEYPAD_ADC_ATTEN);
            }
        }

        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        for(uint8_t x = 0; x < x_size; x++)
        {
            io_conf.pin_bit_mask = (1ULL<<keypad_write_pins[x]);
            gpio_set_direction(keypad_write_pins[x], GPIO_MODE_OUTPUT);
            gpio_set_level(keypad_write_pins[x], 0);
        }

    }

    uint16_t* Scan()
    {
        clearList();

        if(!isListFull()) FNScan(); //Prob not need to check if list is full but it makes the code looks nicer
        if(!isListFull()) KeyPadScan();
        if(!isListFull() && touchbar_enable) TouchBarScan();

        return changeList;
    }

    void Clear()
    {
        fnState.Clear();

        for(uint8_t x = 0; x < x_size; x++)
        {
            for(uint8_t y = 0; y < y_size; y++)
            {
                keypadState[x][y].Clear();
            }
        }

        for(uint8_t i = 0; i < touchbar_size; i++)
        {
            touchbarState[i].Clear();
        }
    }

    // KeyInfo GetKey(Point keyXY)
    // {
    //     uint16_t keyID = XY2ID(keyXY);
    //     return GetKey(keyID);
        
    // }

    KeyInfo* GetKey(uint16_t keyID)
    {
        uint8_t keyClass = keyID >> 12;
        switch(keyClass)
        {   
            case 0: //System
            {
                uint16_t index = keyID & (0b0000111111111111);
                switch(index)
                {
                    case 0:
                        return &fnState;
                }
                break;
            }
            case 1: //Main Grid
            {
                int16_t x = (keyID & (0b0000111111000000)) >> 6;
                int16_t y = keyID & (0b0000000000111111);
                if(x < x_size && y < y_size)  return &keypadState[x][y];
                break;
            }
            case 2: //Touch Bar
            {
                uint16_t index = keyID & (0b0000111111111111);
                // MatrixOS::Logging::LogDebug("Keypad", "Read Touch %d", index);
                if(index < touchbar_size) return &touchbarState[index];
                break;
            }
        }
        return nullptr; //Return an empty KeyInfo
    }

    void FNScan()
    {   
        Fract16 read = gpio_get_level(fn_pin) * UINT16_MAX;
        // ESP_LOGI("FN", "%d", gpio_get_level(fn_pin));
        if(fn_active_low)
        {
            read = UINT16_MAX - (uint16_t)read;
        }
        if(fnState.update(read, false))
        {
            addToList(0);
        }
    }

    bool key1_read = false;
    void KeyPadScan()
    {
        // int64_t time =  esp_timer_get_time();
        Fract16 read = 0;
        for(uint8_t y = 0; y < Device::y_size; y ++)
        {
            for(uint8_t x = 0; x < Device::x_size; x ++)
            {
                gpio_set_level(keypad_write_pins[x], 1); //Just more stable to turn off the pin and turn back on for each read
                if(!FSR) //Non FSR
                {
                    read = gpio_get_level(keypad_read_pins[y]) * UINT16_MAX;
                }
                else //FSR
                {
                    uint32_t raw_voltage = adc1_get_raw(keypad_read_adc_channel[y]);
                    read =  (raw_voltage << 4) + (raw_voltage >> 8); //Raw Voltage mapped. Will add calibration curve later.
                }
                gpio_set_level(keypad_write_pins[x], 0); //Set pin back to low
                bool updated = keypadState[x][y].update(read, true);
                if(updated)
                {   
                    uint16_t keyID = (1 << 12) + (x << 6) + y;
                    if(addToList(keyID))
                    {
                        return; //List is full
                    }
                }
            }
            // volatile int i; for(i=0; i<5; ++i) {} //Add small delay
        }
        // int64_t time_taken =  esp_timer_get_time() - time;
        // ESP_LOGI("Keypad", "%d μs passed, %.2f", (int32_t)time_taken, 1000000.0 / time_taken);
    }

    bool addToList(uint16_t keyID)
    {   
        if(isListFull()) return true; //Prevent overwrite

        changeList[0]++;
        changeList[changeList[0]] = keyID;
        return isListFull();
    }

    void clearList()
    {
        changeList[0] = 0;
    }

    bool isListFull()
    {
        return changeList[0] == MULTIPRESS;
    }

    uint16_t XY2ID(Point xy)
    {
        if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) //Main grid
        {
            return (1 << 12) + (xy.x << 6) + xy.y;
        }
        else if((xy.x == -1 || xy.x == 8) && (xy.y >= 0 && xy.y < 8)) //Touch Bar
        {
             return (2 << 12) + xy.y + (xy.x == 8) * 8;
        }
        // MatrixOS::Logging::LogError("Keypad", "Failed XY2ID %d %d", xy.x, xy.y);
        return UINT16_MAX;
    }

    // Matrix use the following ID Struct
    // CCCC IIIIIIIIIIII
    // C as class (4 bits), I as index (12 bits). I could be spilted by the class defination, for example, class 0 (grid), it's spilted to XXXXXXX YYYYYYY.
    // Class List:
    // Class 0 - System - IIIIIIIIIIII
    // Class 1 - Grid - XXXXXX YYYYYY
    // Class 2 - TouchBar - IIIIIIIIIIII
    // Class 3 - Underglow - IIIIIIIIIIII

    Point ID2XY(uint16_t keyID)
    {
        uint8_t keyClass = keyID >> 12;
        switch(keyClass)
        {
            case 1: //Main Grid
            {
                int16_t x = (keyID & 0b0000111111000000) >> 6;
                int16_t y = keyID & (0b0000000000111111);
                if(x < Device::x_size && y < Device::y_size)  return Point(x, y);
                break;
            }
            case 2: //TouchBar
            {
                uint16_t index = keyID & (0b0000111111111111);
                if(index < Device::touchbar_size)
                {
                    if(index / 8) //Right
                    {
                        return Point(Device::x_size, index % 8);
                    }
                    else //Left
                    {
                        return Point(-1, index % 8);
                    }
                }
                break;
            }
        }
        return Point(INT16_MIN, INT16_MIN);
    }
}