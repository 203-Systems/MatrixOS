#include "MatrixOS.h"

namespace MatrixOS::LED
{
    // static timer
    StaticTimer_t led_tmdef;
    TimerHandle_t led_tm;
    
    vector<Color*> frameBuffers;
    bool needUpdate = false;

    void LEDTimerCallback( TimerHandle_t xTimer )
    {
        if(needUpdate)
        {
            // MatrixOS::Logging::LogDebug("LED", "Update layer #%d", CurrentLayer());
            Device::LED::Update(frameBuffers.back(), UserVar::brightness);
            needUpdate = false;
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
            needUpdate = true;
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
        frameBuffers.push_back(frameBuffer);
        Fill(0);
        // needUpdate = true; //Not gonna update till next drawing
        MatrixOS::Logging::LogDebug("LED Layer", "Layer Created - %d", CurrentLayer());
        return CurrentLayer();
    }

    bool DestoryLayer()
    {
        if(CurrentLayer() > 0)
        {
            // PauseUpdate(true);
            vPortFree(frameBuffers.back());
            frameBuffers.pop_back();
            MatrixOS::Logging::LogDebug("LED Layer", "Layer Destoried - %d", CurrentLayer());
            // PauseUpdate(false);
            needUpdate = true;
            return true;
        }
        else
        {
            Fill(0);
            needUpdate = true;
            MatrixOS::Logging::LogDebug("LED Layer", "Already at layer 0, can not delete layer");
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
        if(pause)
        {
            xTimerStop(led_tm, 0);
        }
        else
        {
            xTimerStart(led_tm, 0);
        }
    }
}