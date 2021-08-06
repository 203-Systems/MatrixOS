#include "MatrixOS.h"

namespace MatrixOS::LED
{
    uint8_t currentLayer = 0;
    Color* frameBuffer;
    void Init()
    {
        uint8_t size = sizeof(Color);
        // frameBuffer = (Color*)malloc(Device::numsOfLED * sizeof(Color) * LED_LAYERS );
        frameBuffer = (Color*)calloc(Device::numsOfLED * LED_LAYERS, sizeof(Color));
        for(uint32_t i = 0; i < Device::numsOfLED * LED_LAYERS; i++)
        {
            frameBuffer[i] = Color();
        }
        Update();
    }

    void SetColor(Point xy, Color color, uint8_t layer)
    {
        if(layer >= LED_LAYERS)
        {
            MatrixOS::SYS::ErrorHandler();
            return;
        }
        uint16_t index = Device::LED::XY2Index(0, xy); //TODO Add multi chunk support
        frameBuffer[index + Device::numsOfLED * layer] = color;

    }

    void SetColor(uint32_t ID, Color color, uint8_t layer)
    {
        if(layer >= LED_LAYERS)
        {
            MatrixOS::SYS::ErrorHandler();
            return;
        }
        uint16_t index = Device::LED::ID2Index(0, ID); //TODO Add multi chunk support
        uint16_t bufferIndex = index + Device::numsOfLED * layer;
        frameBuffer[bufferIndex] = color;

    }

    void Fill(Color color, uint8_t layer)
    {
        if(layer >= LED_LAYERS)
        {
            MatrixOS::SYS::ErrorHandler();
            return;
        }
        for(uint16_t index = 0; index < Device::numsOfLED; index++)
        {
            frameBuffer[index + Device::numsOfLED * layer] = color;
        }
    }

    void Update(int8_t layer)
    {
        Device::LED::Update(&frameBuffer[Device::numsOfLED * layer], 64); //TODO: Get brightness
    }

    void SwitchLayer(uint8_t layer)
    {
        if(layer >= LED_LAYERS)
        {
            MatrixOS::SYS::ErrorHandler();
            return;
        }
        currentLayer = layer;
    }
}