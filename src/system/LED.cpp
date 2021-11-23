#include "MatrixOS.h"

namespace MatrixOS::LED
{
    void Init()
    {
        uint8_t size = sizeof(Color);
        SysVar::fps_millis = 1000/UserVar::fps;
        Color* frameBuffer = (Color*)calloc(Device::numsOfLED, sizeof(Color));
        for(uint32_t i = 0; i < Device::numsOfLED; i++)
        {
            frameBuffer[i] = Color();
        }

        uint8_t currentLayer = 0;
        frameBuffers.push_back(frameBuffer);
        Update();
    }

    void SetColor(Point xy, Color color, uint8_t layer)
    {
        if(layer > currentLayer)
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
        if(layer > currentLayer)
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        uint16_t index = Device::LED::ID2Index(ID);
        // ESP_LOGI("SetColor", "%d", index);
        if(index == UINT16_MAX) return;
        uint16_t bufferIndex = index + Device::numsOfLED * layer;
        frameBuffers[layer][bufferIndex] = color;

    }

    void Fill(Color color, uint8_t layer)
    {
        if(layer > currentLayer)
        {
            MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
            return;
        }
        for(uint16_t index = 0; index < Device::numsOfLED; index++)
        {
            frameBuffers[layer][index] = color;
        }
    }

    void Update(int8_t layer)
    {
        Device::LED::Update(frameBuffers[layer], 64); //TODO: Get brightness
    }

    // void SwitchLayer(uint8_t layer)
    // {
    //     if(layer >= LED_LAYERS)
    //     {
    //         MatrixOS::SYS::ErrorHandler("LED Layer Unavailable");
    //         return;
    //     }
    //     currentLayer = layer;
    // }

    int8_t CreateLayer()
    {
        if(currentLayer >= MAX_LED_LAYERS - 1)
            return -1;
        Color* frameBuffer = (Color*)calloc(Device::numsOfLED, sizeof(Color));
        for(uint32_t i = 0; i < Device::numsOfLED; i++)
        {
            frameBuffer[i] = Color();
        }
        currentLayer++;
        frameBuffers.push_back(frameBuffer);
        return currentLayer;
    }

    void DestoryLayer()
    {
        if(currentLayer > 0)
        {
            free(frameBuffers.back());
            frameBuffers.pop_back();
        }
        else
        {
            Fill(0);
        }
    }

    void ShiftCanvas(EDirection direction, int8_t distance)
    {
        // Color[] tempBuffer;
    }

    void RotateCanvas(EDirection direction)
    {
        
    }
}