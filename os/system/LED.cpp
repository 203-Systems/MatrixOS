#include "MatrixOS.h"

namespace MatrixOS::LED
{
    // static timer
    StaticTimer_t led_tmdef;
    TimerHandle_t led_tm;
    
    vector<Color*> frameBuffers;
    bool needUpate = false;
    bool pauseAutoUpdate = false;

    void LEDTimerCallback( TimerHandle_t xTimer )
    {
        if(needUpate && pauseAutoUpdate == false)
        {
            Device::LED::Update(frameBuffers.back(), UserVar::brightness);
            needUpate = false;
        }
    }

    void Init()
    {   
        CreateLayer();
        led_tm = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::fps, true, NULL, LEDTimerCallback, &led_tmdef);
        xTimerStart(led_tm, 0);
    }

    void SetColor(Point xy, Color color, uint8_t layer)
    {
        if(layer == 255)
        {
            layer = CurrentLayer();
        }
        else if(layer > CurrentLayer())
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        // MatrixOS::Logging::LogVerbose("LED", "Set Color %d %d", xy.x, xy.y);
        xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
        if(layer > CurrentLayer())
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        uint16_t index = Device::LED::XY2Index(xy);
        if(index == UINT16_MAX) return;
        frameBuffers[layer][index] = color;
    }

    void SetColor(uint16_t ID, Color color, uint8_t layer)
    {
        if(layer == 255)
        {
            layer = CurrentLayer();
        }
        else if(layer > CurrentLayer())
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        uint16_t index = Device::LED::ID2Index(ID);
        if(index == UINT16_MAX) return;
        uint16_t bufferIndex = index + Device::numsOfLED * layer;
        frameBuffers[layer][bufferIndex] = color;

    }

    void Fill(Color color, uint8_t layer)
    {
        if(layer == 255)
        {
            layer = CurrentLayer();
        }
        else if(layer > CurrentLayer())
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        for(uint16_t index = 0; index < Device::numsOfLED; index++)
        {
            frameBuffers[layer][index] = color;
        }
    }

    void Update(uint8_t layer) 
    {
        if(layer == 255 || layer == CurrentLayer())
            needUpate = true;
    }

    int8_t CurrentLayer()
    {
        return frameBuffers.size() - 1;
    }

    int8_t CreateLayer()
    {
        if(CurrentLayer() >= MAX_LED_LAYERS - 1)
        {
            MatrixOS::SYS::ErrorHandler("Max LED Layer Exceded");
            return -1;
        }
        Color* frameBuffer = (Color*)pvPortMalloc(Device::numsOfLED * sizeof(Color));
        if(frameBuffer == nullptr)
        {
            MatrixOS::SYS::ErrorHandler("Failed to allocate new led buffer");
            return -1;
        }
        Fill(0);
        frameBuffers.push_back(frameBuffer);
        // needUpate = true; //Not gonna update till next drawing
        return CurrentLayer();
    }

    bool DestoryLayer()
    {
        if(CurrentLayer() > 0)
        {
            vPortFree(frameBuffers.back());
            frameBuffers.pop_back();
            needUpate = true;
            return true;
        }
        else
        {
            Fill(0);
            needUpate = true;
            return false;
        }
    }

    void ShiftCanvas(EDirection direction, int8_t distance, uint8_t layer)
    {
        // Color[] tempBuffer;
    }

    void RotateCanvas(EDirection direction, uint8_t layer)
    {
        //TODO
    }

    void PauseUpdate(bool pause)
    {
        pauseAutoUpdate = pause;
    }
}