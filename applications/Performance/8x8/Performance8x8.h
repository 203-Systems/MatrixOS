#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "ui/UI.h"

#include "UINotePad.h"

#include "applications/BrightnessControl/BrightnessControl.h"

#define NUMS_OF_KEYMAP 2
#define STFU_DEFAULT 2

class Performance : public Application {
 public:
  static Application_Info info;

  uint8_t currentKeymap = 0;

  uint8_t canvasLedLayer;
  bool uiOpened = false;

  // Saved Variables
  CreateSavedVar("Performance", velocitySensitive, bool, true);
  CreateSavedVar("Performance", compatibilityMode, bool, false);
  CreateSavedVar("Performance", menuLock, bool, false);
  CreateSavedVar("Performance", stfu, uint8_t, 0);

  void Setup() override;
  void Loop() override;

  Point NoteToXY(uint8_t note);
  int8_t XYToNote(Point xy);

  void MidiEventHandler(MidiPacket midiPacket);
  void NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity);
  void SysExHandler(MidiPacket midiPacket);

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);

  void GridKeyEvent(Point xy, KeyInfo* KeyInfo);
  void IDKeyEvent(uint16_t keyID, KeyInfo* KeyInfo);

  void ActionMenu();

  void stfuScan();

  const Color keymap_color[NUMS_OF_KEYMAP] = {Color(0xFF00FF), Color(0xFF5400)};

  const uint8_t keymap_channel[NUMS_OF_KEYMAP] = {0, 0};

  const uint8_t keymap[NUMS_OF_KEYMAP][8][8] = {{{64, 65, 66, 67, 96, 97, 98, 99},  // Drum Rack
                                                 {60, 61, 62, 63, 92, 93, 94, 95},
                                                 {56, 57, 58, 59, 88, 89, 90, 91},
                                                 {52, 53, 54, 55, 84, 85, 86, 87},
                                                 {48, 49, 50, 51, 80, 81, 82, 83},
                                                 {44, 45, 46, 47, 76, 77, 78, 79},
                                                 {40, 41, 42, 43, 72, 73, 74, 75},
                                                 {36, 37, 38, 39, 68, 69, 70, 71}},
                                                {{81, 82, 83, 84, 85, 86, 87, 88},  // Unmapped XY
                                                 {71, 72, 73, 74, 75, 76, 77, 78},
                                                 {61, 62, 63, 64, 65, 66, 67, 68},
                                                 {51, 52, 53, 54, 55, 56, 57, 58},
                                                 {41, 42, 43, 44, 45, 46, 47, 48},
                                                 {31, 32, 33, 34, 35, 36, 37, 38},
                                                 {21, 22, 23, 24, 25, 26, 27, 28},
                                                 {11, 12, 13, 14, 15, 16, 17, 18}}};

  const uint8_t touch_keymap[NUMS_OF_KEYMAP][4][8]  // Touchbar map, top mirors left and right (For Matrix rotation)
      {{{100, 101, 102, 103, 104, 105, 106, 107},   // Drum Rack
        {100, 101, 102, 103, 104, 105, 106, 107},
        {108, 109, 110, 111, 112, 113, 114, 115},
        {108, 109, 110, 111, 112, 113, 114, 115}},
       {{89, 79, 69, 59, 49, 39, 29, 19},  // Unmapped XY
        {89, 79, 69, 59, 49, 39, 29, 19},
        {81, 71, 61, 51, 41, 31, 21, 11},
        {81, 71, 61, 51, 41, 31, 21, 11}}};

  // {28, 29, 30, 31, 32, 33, 34, 35}, //Drum Rack (four side unique)
  // {100,101,102,103,104,105,106,107},
  // {116,117,118,119,120,121,122,123},
  // {108,109,110,111,112,113,114,115}

  const uint8_t note_pad_map[NUMS_OF_KEYMAP][2][8]{{{100, 101, 102, 103, 104, 105, 106, 107},  // Drum Rack
                                                    {108, 109, 110, 111, 112, 113, 114, 115}},
                                                   {{89, 79, 69, 59, 49, 39, 29, 19},  // Unmapped XY
                                                    {81, 71, 61, 51, 41, 31, 21, 11}}};

  const uint8_t user1_keymap_optimized[64] = {
      0x07, 0x17, 0x27, 0x37, 0x06, 0x16, 0x26, 0x36, 0x05, 0x15, 0x25, 0x35, 0x04, 0x14, 0x24, 0x34,
      0x03, 0x13, 0x23, 0x33, 0x02, 0x12, 0x22, 0x32, 0x01, 0x11, 0x21, 0x31, 0x00, 0x10, 0x20, 0x30,
      0x47, 0x57, 0x67, 0x77, 0x46, 0x56, 0x66, 0x76, 0x45, 0x55, 0x65, 0x75, 0x44, 0x54, 0x64, 0x74,
      0x43, 0x53, 0x63, 0x73, 0x42, 0x52, 0x62, 0x72, 0x41, 0x51, 0x61, 0x71, 0x40, 0x50, 0x60, 0x70};

  const Color palette[2][128] =  // color Palette
      {{
           // MatrixcolorPalette (Mat1s' Palette for now)
           0x00000000,  // 0
           0x003C0000,  // 1
           0x007D0000,  // 2
           0x00BE0000,  // 3
           0x00FF7D7D,  // 4
           0x00FF0000,  // 5
           0x003C0C00,  // 6
           0x007D1C00,  // 7
           0x00BE2C00,  // 8
           0x00FF9D7D,  // 9
           0x00FF3C00,  // 10
           0x003C1C00,  // 11
           0x007D3C00,  // 12
           0x00BE5D00,  // 13
           0x00FFBE7D,  // 14
           0x00FF7D00,  // 15
           0x003C2C00,  // 16
           0x007D5D00,  // 17
           0x00BE8D00,  // 18
           0x00FFDE7D,  // 19
           0x00FFBE00,  // 20
           0x003C3C00,  // 21
           0x007D7D00,  // 22
           0x00BEBE00,  // 23
           0x00FFFF7D,  // 24
           0x00FFFF00,  // 25
           0x002C3C00,  // 26
           0x005D7D00,  // 27
           0x008DBE00,  // 28
           0x00DEFF7D,  // 29
           0x00BEFF00,  // 30
           0x001C3C00,  // 31
           0x003C7D00,  // 32
           0x005DBE00,  // 33
           0x00BEFF7D,  // 34
           0x007DFF00,  // 35
           0x000C3C00,  // 36
           0x001C7D00,  // 37
           0x002CBE00,  // 38
           0x009DFF7D,  // 39
           0x003CFF00,  // 40
           0x00003C00,  // 41
           0x00007D00,  // 42
           0x0000BE00,  // 43
           0x007DFF7D,  // 44
           0x0000FF00,  // 45
           0x00003C0C,  // 46
           0x00007D1C,  // 47
           0x0000BE2C,  // 48
           0x007DFF9D,  // 49
           0x0000FF3C,  // 50
           0x00003C1C,  // 51
           0x00007D3C,  // 52
           0x0000BE5D,  // 53
           0x007DFFBE,  // 54
           0x0000FF7D,  // 55
           0x00003C2C,  // 56
           0x00007D5D,  // 57
           0x0000BE8D,  // 58
           0x007DFFDE,  // 59
           0x0000FFBE,  // 60
           0x00003C3C,  // 61
           0x00007D7D,  // 62
           0x0000BEBE,  // 63
           0x007DFFFF,  // 64
           0x0000FFFF,  // 65
           0x00002C3C,  // 66
           0x00005D7D,  // 67
           0x00008DBE,  // 68
           0x007DDEFF,  // 69
           0x0000BEFF,  // 70
           0x00001C3C,  // 71
           0x00003C7D,  // 72
           0x00005DBE,  // 73
           0x007DBEFF,  // 74
           0x00007DFF,  // 75
           0x00000C3C,  // 76
           0x00001C7D,  // 77
           0x00002CBE,  // 78
           0x007D9DFF,  // 79
           0x00003CFF,  // 80
           0x0000003C,  // 81
           0x0000007D,  // 82
           0x000000BE,  // 83
           0x007D7DFF,  // 84
           0x000000FF,  // 85
           0x000C003C,  // 86
           0x001C007D,  // 87
           0x002C00BE,  // 88
           0x009D7DFF,  // 89
           0x003C00FF,  // 90
           0x001C003C,  // 91
           0x003C007D,  // 92
           0x005D00BE,  // 93
           0x00BE7DFF,  // 94
           0x007D00FF,  // 95
           0x002C003C,  // 96
           0x005D007D,  // 97
           0x008D00BE,  // 98
           0x00DE7DFF,  // 99
           0x00BE00FF,  // 100
           0x003C003C,  // 101
           0x007D007D,  // 102
           0x00BE00BE,  // 103
           0x00FF7DFF,  // 104
           0x00FF00FF,  // 105
           0x003C002C,  // 106
           0x007D005D,  // 107
           0x00BE008D,  // 108
           0x00FF7DDE,  // 109
           0x00FF00BE,  // 110
           0x003C001C,  // 111
           0x007D003C,  // 112
           0x00BE005D,  // 113
           0x00FF7DBE,  // 114
           0x00FF007D,  // 115
           0x003C000C,  // 116
           0x007D001C,  // 117
           0x00BE002C,  // 118
           0x00FF7D9D,  // 119
           0x00FF003C,  // 120
           0x00242424,  // 121
           0x00484848,  // 122
           0x006D6D6D,  // 123
           0x00919191,  // 124
           0x00B6B6B6,  // 125
           0x00DADADA,  // 126
           0x00FFFFFF,  // 127
       },
       {
           // LaunchpadXcolorPalette (Legacy Palette)
           0x00000000,  // 0
           0x003F3F3F,  // 1
           0x007F7F7F,  // 2
           0x00FFFFFF,  // 3
           0x00FF3F3F,  // 4
           0x00FF0000,  // 5
           0x007F0000,  // 6
           0x003F0000,  // 7
           0x00FFBF6F,  // 8
           0x00FF3F00,  // 9
           0x007F1F00,  // 10
           0x003F0F00,  // 11
           0x00FFAF2F,  // 12
           0x00FFFF00,  // 13
           0x007F7F00,  // 14
           0x003F3F00,  // 15
           0x007FFF2F,  // 16
           0x004FFF00,  // 17
           0x002F7F00,  // 18
           0x00173F00,  // 19
           0x004FFF3F,  // 20
           0x0000FF00,  // 21
           0x00007F00,  // 22
           0x00003F00,  // 23
           0x004FFF4F,  // 24
           0x0000FF1F,  // 25
           0x00007F0F,  // 26
           0x00003F07,  // 27
           0x004FFF5F,  // 28
           0x0000FF5F,  // 29
           0x00007F2F,  // 30
           0x00003F17,  // 31
           0x004FFFBF,  // 32
           0x0000FF9F,  // 33
           0x00007F4F,  // 34
           0x00003F27,  // 35
           0x004FBFFF,  // 36
           0x0000AFFF,  // 37
           0x0000577F,  // 38
           0x00002F3F,  // 39
           0x004F7FFF,  // 40
           0x000057FF,  // 41
           0x00002F7F,  // 42
           0x0000173F,  // 43
           0x002F1FFF,  // 44
           0x000000FF,  // 45
           0x0000007F,  // 46
           0x0000003F,  // 47
           0x005F3FFF,  // 48
           0x002F00FF,  // 49
           0x0017007F,  // 50
           0x000F003F,  // 51
           0x00FF3FFF,  // 52
           0x00FF00FF,  // 53
           0x007F007F,  // 54
           0x003F003F,  // 55
           0x00FF3F6F,  // 56
           0x00FF004F,  // 57
           0x007F002F,  // 58
           0x003F001F,  // 59
           0x00FF0F00,  // 60
           0x009F3F00,  // 61
           0x007F4F00,  // 62
           0x002F2F00,  // 63
           0x00003F00,  // 64
           0x00003F1F,  // 65
           0x00001F6F,  // 66
           0x000000FF,  // 67
           0x00003F3F,  // 68
           0x001F00BF,  // 69
           0x005F3F4F,  // 70
           0x001F0F17,  // 71
           0x00FF0000,  // 72
           0x00BFFF2F,  // 73
           0x00AFEF00,  // 74
           0x005FFF00,  // 75
           0x000F7F00,  // 76
           0x0000FF5F,  // 77
           0x00009FFF,  // 78
           0x00002FFF,  // 79
           0x001F00FF,  // 80
           0x005F00EF,  // 81
           0x00AF1F7F,  // 82
           0x002F0F00,  // 83
           0x00FF2F00,  // 84
           0x007FDF00,  // 85
           0x006FFF1F,  // 86
           0x0000FF00,  // 87
           0x003FFF2F,  // 88
           0x005FEF6F,  // 89
           0x003FFFCF,  // 90
           0x005F8FFF,  // 91
           0x002F4FCF,  // 92
           0x006F4FDF,  // 93
           0x00DF1FFF,  // 94
           0x00FF005F,  // 95
           0x00FF4F00,  // 96
           0x00BFAF00,  // 97
           0x008FFF00,  // 98
           0x007F5F00,  // 99
           0x003F2F00,  // 100
           0x0000470F,  // 101
           0x000F4F1F,  // 102
           0x0017172F,  // 103
           0x00171F5F,  // 104
           0x005F3717,  // 105
           0x007F0000,  // 106
           0x00DF3F2F,  // 107
           0x00DF470F,  // 108
           0x00FFBF1F,  // 109
           0x009FDF2F,  // 110
           0x006FAF0F,  // 111
           0x0017172F,  // 112
           0x00DFDF6F,  // 113
           0x007FEF8F,  // 114
           0x009F9FFF,  // 115
           0x008F6FFF,  // 116
           0x003F3F3F,  // 117
           0x006F6F6F,  // 118
           0x00DFFFFF,  // 119
           0x009F0000,  // 120
           0x00370000,  // 121
           0x0017CF00,  // 122
           0x00003F00,  // 123
           0x00BFAF00,  // 124
           0x003F2F00,  // 125
           0x00AF4F00,  // 126
           0x004F0F00   // 127
       }};

 private:
  int8_t stfuMap[128];
  Timer stfuTimer;
};

inline Application_Info Performance::info = {
    .name = "Performance",
    .author = "203 Electronics",
    .color =  Color(0xFF0000),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Performance);
