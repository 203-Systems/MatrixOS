namespace Device
{
    void Hardware_Init();
    void Reboot();
    void Bootloader();
    void DelayMs(int intervalMs);
    // uint32_t GetTick();
    uint32_t Millis();

    namespace LED
    {
        void Init();
        uint8_t Update(Color *array, uint8_t brightness = 255);
    }

    namespace KeyPad
    {
        void Init();
        void RegisterCallback();
        bool Scan();
    }

    namespace USB
    {
        void Init();
    }
}