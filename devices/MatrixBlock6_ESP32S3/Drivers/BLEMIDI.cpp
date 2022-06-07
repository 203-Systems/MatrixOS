#include "Device.h"
#include "blemidi/blemidi.h"

#include <queue>
using std::queue;

#define TAG "BLE-MIDI"

#define BLE_MIDI_PORT_ID 2

namespace Device
{
    namespace BLEMIDI
    {
        bool started = false;
        string name;
        TaskHandle_t taskHandle = NULL;

        static void Task(void *pvParameters)
        {
            portTickType xLastExecutionTime;

            // Initialise the xLastExecutionTime variable on task entry
            xLastExecutionTime = xTaskGetTickCount();

            while(true) 
            {
                vTaskDelayUntil(&xLastExecutionTime, 500 / portTICK_RATE_MS);
                blemidi_tick(); // for timestamp and output buffer handling
            }
        }

        // void Callback(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos)
        // {
        //     MatrixOS::Logging::LogInfo(TAG, "CALLBACK blemidi_port=%d, timestamp=%d, midi_status=0x%02x, len=%d, continued_sysex_pos=%d, remaining_message:", blemidi_port, timestamp, midi_status, len, continued_sysex_pos);
        //     // esp_log_buffer_hex(TAG, remaining_message, len);

        //     // loopback received message
        //     {
        //         // TODO: more comfortable packet creation via special APIs

        //         // Note: by intention we create new packets for each incoming message
        //         // this shows that running status is maintained, and that SysEx streams work as well
                
        //         if( midi_status == 0xf0 && continued_sysex_pos > 0 ) 
        //         {
        //         blemidi_send_message(0, remaining_message, len); // just forward
        //         } 
        //         else 
        //         {
        //             size_t loopback_message_len = 1 + len; // includes MIDI status and remaining bytes
        //             uint8_t *loopback_message = (uint8_t *)malloc(loopback_message_len * sizeof(uint8_t));
        //             if( loopback_message == NULL ) {
        //                 // no memory...
        //             } else {
        //                 loopback_message[0] = midi_status;
        //                 memcpy(&loopback_message[1], remaining_message, len);

        //                 blemidi_send_message(0, loopback_message, loopback_message_len);

        //                 free(loopback_message);
        //             }
        //         }
        //     }
        // }
        
        queue<MidiPacket> midi_buffer;
        void Callback(uint8_t blemidi_port, uint16_t timestamp, uint8_t midi_status, uint8_t *remaining_message, size_t len, size_t continued_sysex_pos)
        {
            // MatrixOS::Logging::LogDebug(TAG, "CALLBACK blemidi_port=%d, timestamp=%d, midi_status=0x%02x, len=%d, continued_sysex_pos=%d, remaining_message:", blemidi_port, timestamp, midi_status, len, continued_sysex_pos);
            // MatrixOS::Logging::LogDebug(TAG, "Recived 0x%02x 0x%02x 0x%02x", midi_status, remaining_message[0], remaining_message[1]);
            if(len == 2)
            {
                MidiPacket packet = MidiPacket(BLE_MIDI_PORT_ID, (EMidiStatus)(midi_status & 0xF0), midi_status, remaining_message[0], remaining_message[1]);
                midi_buffer.push(packet);
            }
        }

        void Toggle()
        {
            if(started == false)
            {
                Start();
            }
            else
            {
                Stop();
            }
        }

        void Init(string name)
        {
            BLEMIDI::name = name;
        }

        void Start()
        {
            int status = blemidi_init((void*)Callback, name.c_str());
            if( status < 0 ) {
                ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
            } else {
                ESP_LOGI(TAG, "BLE MIDI Driver initialized successfully");
                started = true;
                xTaskCreate(Task, "task_midi", 4096, NULL, 8, &taskHandle);
            }
        }

        void Stop()
        {
            int status = blemidi_deinit();
            if( status < 0 ) {
                ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
            } else {
                ESP_LOGI(TAG, "BLE MIDI Driver deinitialized successfully");
                started = false;
                if( taskHandle != NULL )
                {
                    vTaskDelete( taskHandle );
                }
            }
        }

        uint32_t MidiAvailable()
        {
            // ESP_LOGI(TAG, "Available: %d", midi_buffer.size());
            return midi_buffer.size();
        }

        MidiPacket GetMidi()
        {
            if(midi_buffer.size())
            {   
                MidiPacket packet = midi_buffer.front();
                midi_buffer.pop();
                return packet;
            }
            return MidiPacket(BLE_MIDI_PORT_ID, None);
        }

        bool SendMidi(uint8_t* packet)
        {
            // MatrixOS::Logging::LogDebug(TAG, "Sent 0x%02x 0x%02x 0x%02x", packet[0], packet[1], packet[2]);
            blemidi_send_message(0, packet, 3);
            return true;
        }
    }
}