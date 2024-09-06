#include "Device.h"

namespace Device
{
    namespace HWMidi
    {
        MidiPort* midiPort;
        TaskHandle_t portTaskHandle = NULL;
        uart_port_t uartChannel = UART_NUM_2;

        void portTask(void* param) {
            MidiPort port = MidiPort("Midi Port", MIDI_PORT_PHYISCAL);
            midiPort = &port;
            MidiPacket packet;
            while (true)
            {
                if (port.Get(&packet, portMAX_DELAY))
                { uart_write_bytes(uartChannel, packet.data, 3); }
            }
        }

        void Init()
        {
            uart_config_t uart_config = {
                .baud_rate = 31250,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .rx_flow_ctrl_thresh = 0,
                .source_clk = UART_SCLK_DEFAULT,
            };

            int rx_buffer_size = 2048; // TODO: Optimize this value
            if(rx_gpio == GPIO_NUM_NC)
            { 
                rx_buffer_size = 129; // Must be larger than 128, even though we don't use it
            }

            ESP_ERROR_CHECK(uart_driver_install(uartChannel, rx_buffer_size, 0, 0, NULL, 0));
            ESP_ERROR_CHECK(uart_param_config(uartChannel, &uart_config));
            ESP_ERROR_CHECK(uart_set_pin(uartChannel, tx_gpio, rx_gpio, GPIO_NUM_NC, GPIO_NUM_NC));
            xTaskCreate(portTask, "Hardware Midi Port", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2,&portTaskHandle);
        }
    }
}