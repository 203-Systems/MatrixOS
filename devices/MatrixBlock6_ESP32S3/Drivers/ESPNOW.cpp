// #include "Device.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/queue.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"

// #include "esp_system.h"
// // #include "esp_log.h"
// #include "esp_wifi.h"
// #include "esp_now.h"
// #include "esp_mac.h"
// #include "esp_netif.h"

// #include <queue>

// using std::queue;

// #define ESP_NOW_BUFFER_SIZE 192  // Can not go above 256? Multiple of 3 perfered for Midi
// #define ESP_NOW_RATE WIFI_PHY_RATE_36M

// #define ESP_NOW_MIDI_PORT_ID 3

// static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// uint8_t target_mac[ESP_NOW_ETH_ALEN] = {0x7C, 0xDF, 0xA1, 0x3E,
//                                         0xD2, 0xA2};  // Current this is hard coded to my own reciver. A pairing system
//                                                       // is required for future

// namespace Device
// {
//   namespace ESPNOW
//   {
//     bool started = false;
//     void ESPNOW_Recv_CB(const uint8_t* mac_addr, const uint8_t* data, int len);

//     nvs_handle_t esp_now_nvs;

//     // Static timer
//     StaticTimer_t espnow_tmdef;
//     TimerHandle_t espnow_tm;

//     void Init() {
//       if (esp_now_init() != 0)
//       { return; }

//       nvs_open("esp_now", NVS_READWRITE, &esp_now_nvs);

//       size_t mac_size = 6;  // Why does nvs needs a pointer to size??????
//       if (nvs_get_blob(esp_now_nvs, "target_mac", &target_mac, &mac_size) != ESP_OK)
//       {
//         ESP_LOGI("espnow_init", "Target Mac wrote: %#02X:%#02X:%#02X:%#02X:%#02X:%#02X", target_mac[0], target_mac[1],
//                  target_mac[2], target_mac[3], target_mac[4], target_mac[5]);
//         nvs_set_blob(esp_now_nvs, "target_mac", &target_mac, mac_size);
//         nvs_commit(esp_now_nvs);
//       }
//       else
//       {
//         ESP_LOGI("espnow_init", "Target Mac Load: %#02X:%#02X:%#02X:%#02X:%#02X:%#02X", target_mac[0], target_mac[1],
//                  target_mac[2], target_mac[3], target_mac[4], target_mac[5]);
//       }

//       esp_now_peer_info_t broadcast_info;
//       broadcast_info.channel = 0;
//       memcpy(broadcast_info.peer_addr, broadcast_mac, 6);
//       broadcast_info.ifidx = WIFI_IF_STA;
//       broadcast_info.encrypt = false;
//       esp_err_t status = esp_now_add_peer(&broadcast_info);

//       if (ESP_OK != status)
//       { ESP_LOGE("espnow_init", "Could not add boradcast peer"); }

//       esp_now_peer_info_t peer_info;
//       peer_info.channel = 0;
//       memcpy(peer_info.peer_addr, target_mac, 6);
//       peer_info.ifidx = WIFI_IF_STA;
//       peer_info.encrypt = false;
//       status = esp_now_add_peer(&peer_info);

//       if (ESP_OK != status)
//       { ESP_LOGE("espnow_init", "Could not add peer"); }

//       esp_wifi_config_espnow_rate(WIFI_IF_STA, ESP_NOW_RATE);

//       status = esp_now_register_recv_cb(ESPNOW_Recv_CB);
//       if (ESP_OK != status)
//       { ESP_LOGE("espnow_init", "Could not add callback"); }

//       espnow_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(5), true, NULL, Flush, &espnow_tmdef);
//       xTimerStart(espnow_tm, 0);
//     }

//     uint8_t esp_now_buffer[ESP_NOW_BUFFER_SIZE];
//     uint16_t esp_now_buffer_usage = 0;

//     void Flush(void* param) {
//       (void)param;
//       if (esp_now_buffer_usage == 0)
//         return;
//       uint8_t esp_now_send_buffer[esp_now_buffer_usage];
//       memcpy(&esp_now_send_buffer, &esp_now_buffer, esp_now_buffer_usage);
//       esp_err_t status = esp_now_send(target_mac, esp_now_send_buffer, esp_now_buffer_usage);
//       if (ESP_OK != status)
//       { ESP_LOGE("ESPNOW Flush", "Error sending message"); }
//       esp_now_buffer_usage = 0;
//     }

//     bool SendMidi(uint8_t* packet) {
//       memcpy(&esp_now_buffer[esp_now_buffer_usage], &packet[1], 3);
//       esp_now_buffer_usage += 3;
//       // printf("%d\n", esp_now_buffer_usage);
//       if (esp_now_buffer_usage == ESP_NOW_BUFFER_SIZE)
//       {
//         ESP_LOGW("ESPNOW Send Midi", "Buffer Overflow");
//         Flush(NULL);
//       }
//       return true;
//     }

//     queue<MidiPacket> midi_buffer;
//     void ESPNOW_Recv_CB(const uint8_t* mac_addr, const uint8_t* data, int len) {
//       // ESP_LOGI("ESPNOW", "Message recivied from %#02X:%#02X:%#02X:%#02X:%#02X:%#02X", mac_addr[0], mac_addr[1],
//       // mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

//       // bool broadcast = true;
//       // for(uint8_t i = 0; i < 6; i++)
//       // {
//       //     if(mac_addr[i] != 0xFF)
//       //     {
//       //         broadcast = false;
//       //         break;
//       //     }
//       // }
//       // ESP_LOGI("ESPNOW", "Broadcast = %d", broadcast);

//       if (len == 7 && data[0] == 0xF4)
//       { UpdatePeer(&data[1]); }
//       else
//       {
//         for (int16_t i = 0; i < len; i += 3)
//         {
//           // Switch to 4byte system later
//           //  ESP_LOGI("Got Midi", "%#02X %#02X %#02X", data[i], data[i + 1], data[i + 2]);
//           if ((data[i] & 0xF0) != 0x80 && (data[i] & 0xF0) != 0x90)
//           {
//             ESP_LOGW("Got Midi", "Midi Incorrect %#02X %#02X %#02X", data[i], data[i + 1], data[i + 2]);
//             i -= 2;
//             while ((data[i + 3] & 0xF0) != 0x80 && (data[i + 3] & 0xF0) != 0x90 && i + 3 < len)
//             { i++; }
//             continue;
//           }
//           // MidiPacket packet = MidiPacket(ESP_NOW_MIDI_PORT_ID, (EMidiStatus)(data[i] & 0xF0), data[i], data[i + 1],
//           // data[i + 2]); ESP_LOGI("Got Midi 2", "%#02X %#02X %#02X", packet.data[0], packet.data[1], packet.data[2]);
//           midi_buffer.emplace(ESP_NOW_MIDI_PORT_ID, (EMidiStatus)(data[i] & 0xF0), data[i], data[i + 1], data[i + 2]);
//         }
//       }
//     }

//     uint32_t MidiAvailable() {
//       return midi_buffer.size();
//     }

//     MidiPacket GetMidi() {
//       if (midi_buffer.size())
//       {
//         MidiPacket packet = midi_buffer.front();
//         midi_buffer.pop();
//         // ESP_LOGI("GetMidi", "%#02X %#02X %#02X", packet.data[0], packet.data[1], packet.data[2]);
//         return packet;
//       }
//       return MidiPacket(ESP_NOW_MIDI_PORT_ID, None);
//     }

//     void BroadcastMac() {
//       uint8_t mac_packet[7] = {0xF4};
//       esp_read_mac(&mac_packet[1], ESP_MAC_WIFI_STA);
//       esp_err_t status = esp_now_send(broadcast_mac, mac_packet, 7);
//       (void)status;  // So status no longer unused
//     }

//     void UpdatePeer(const uint8_t* new_mac) {
//       ESP_LOGI("ESPNOW Update Peer", "Target Mac updated: %#02X:%#02X:%#02X:%#02X:%#02X:%#02X", new_mac[0], new_mac[1],
//                new_mac[2], new_mac[3], new_mac[4], new_mac[5]);

//       esp_now_peer_info_t peer_info;
//       esp_now_get_peer(target_mac, &peer_info);

//       esp_now_del_peer(target_mac);

//       memcpy(target_mac, new_mac, 6);
//       memcpy(peer_info.peer_addr, new_mac, 6);

//       esp_err_t status = esp_now_add_peer(&peer_info);

//       if (ESP_OK != status)
//       { ESP_LOGE("espnow_init", "Could not update peer"); }

//       nvs_set_blob(esp_now_nvs, "target_mac", &new_mac, 6);
//       nvs_commit(esp_now_nvs);
//     }
//   }
// }