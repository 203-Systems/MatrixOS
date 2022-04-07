//Define Device Keypad Function
#include "Device.h"

namespace Device::KeyPad
{
    #ifdef  FSR_KEYPAD
    esp_adc_cal_characteristics_t *adc1_chars;
    #endif 

    void Init()
    {
        #if Key2_Pin == GPIO_NUM_26 //OOPS, used SPICS1 as keypad pin
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_26);
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	    gpio_config(&io_conf);
        #endif
        
        #ifdef FN_PIN_ACTIVE_LOW //Active Low
        gpio_set_pull_mode(FN_Pin, GPIO_PULLUP_ONLY);
        #else //Active High
        gpio_set_pull_mode(FN_Pin, GPIO_PULLDOWN_ONLY);
        #endif

        for(uint8_t x = 0; x < x_size; x++)
        {
            gpio_set_direction(keypad_write_pins[x], GPIO_MODE_OUTPUT);
            gpio_set_level(keypad_write_pins[x], 0);
        }

        #ifndef  FSR_KEYPAD
        for(uint8_t y = 0; y < y_size; y++)
        {
            gpio_set_direction(keypad_read_pins[y], GPIO_MODE_INPUT);
            gpio_set_pull_mode(keypad_read_pins[y], GPIO_PULLDOWN_ONLY);
        }
        #else
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11,  ADC_WIDTH_BIT_12, 0, adc1_chars);
        #endif

    }

    uint16_t* Scan()
    {
        clearList();

        if(!isListFull()) FNScan(); //Prob not need to check if list is full but it makes the code looks nicer
        if(!isListFull()) KeyPadScan();
        // if(!isListFull()) TouchBarScan();

        return changeList;
    }

    KeyInfo GetKey(Point keyXY)
    {
        uint16_t keyID = XY2ID(keyXY);
        return GetKey(keyID);
        
    }

    KeyInfo GetKey(uint16_t keyID)
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
                        return fnState;
                }
                break;
            }
            case 1: //Main Grid
            {
                int16_t x = (keyID & (0b0000111111000000)) >> 6;
                int16_t y = keyID & (0b0000000000111111);
                if(x < x_size && y < y_size)  return keypadState[x][y];
                break;
            }
            case 2: //Touch Bar
            {
                uint16_t index = keyID & (0b0000111111111111);
                if(index < touchbar_size) return touchbarState[index];
                break;
            }
        }
        return KeyInfo(); //Return an empty KeyInfo
    }

    void FNScan()
    {   
        Fract16 read = gpio_get_level(FN_Pin) * UINT16_MAX;
        #ifdef FN_PIN_ACTIVE_LOW
        read = UINT16_MAX - (uint16_t)read;
        #endif
        if(fnState.update(read))
        {
            addToList(0);
        }
    }

    void KeyPadScan()
    {
        // int64_t time =  esp_timer_get_time();
        for(uint8_t x = 0; x < Device::x_size; x ++)
        {
            gpio_set_level(keypad_write_pins[x], 1);
            for(uint8_t y = 0; y < Device::y_size; y ++)
            {
                #ifndef  FSR_KEYPAD
                Fract16 read = gpio_get_level(keypad_read_pins[y]) * UINT16_MAX;
                #else
                Fract16 read = gpio_get_level(keypad_read_pins[y]);
                if(read)
                {
                    uint32_t raw_voltage = adc1_get_raw(keypad_read_adc_channel[y]);
                    // uint32_t voltage = esp_adc_cal_raw_to_voltage(raw_voltage, &adc1_chars);
                    // ESP_LOGI("Keypad", "Key %d:%d @ %d - %dmv", x, y, raw_voltage, voltage);
                    
                    read =  (raw_voltage > 4095) * ((raw_voltage << 3) + (raw_voltage >> 10));
                }
                // return;
                #endif
                bool updated = keypadState[x][y].update(read);
                if(updated)
                {
                    uint16_t keyID = (1 << 12) + (x << 6) + y;
                    if(addToList(keyID))
                    {
                        gpio_set_level(keypad_write_pins[x], 0); //Set pin back to low
                        return; //List is full
                    }
                }
            }
            gpio_set_level(keypad_write_pins[x], 0);
            // volatile int i; for(i=0; i<5; ++i) {} //Add small delay
        }
        // int64_t time_taken =  esp_timer_get_time() - time;
        // ESP_LOGI("Keypad", "%d μs passed, %.2f", (int32_t)time_taken, 1000000.0 / time_taken);
    }

    void TouchBarScan()
    {
        
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
            return (1 << 12) + (xy.x << 7) + xy.y;
        }
        // else if(xy.x >= 0 && xy.x < 8 && xy.y == 8) //Touch Bar
        // {
        //     return (2 << 12) + xy.x;
        // }
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
                if(index < Device::touchbar_size) return Point(Device::y_size, index);
                break;
            }
        }
        return Point(INT16_MIN, INT16_MIN);
    }
}