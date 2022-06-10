# Matrix OS

An operation system for human interface devices that is designed to be cross-platform and moudlar.

Currently the OS only supports MIDI but more to come in later revisions.

## Description
Matrix OS is composited by 4 different layers:
#### Core Layer
Handles the basic of the chip. Located at core/
#### Device Layer
Drivers and configurations of target device. Located at devices/
#### System Layer
Handles the system and application runtime. Located at os/
#### Application Layer
Applications that runs on Matrix OS. All user interations are Matrix OS applications. Located at applications/ 

## Supported Devices
| Device Name            | Type | Chip     | Supported | Note              | Purchase                                       |
|------------------------|------|----------|-----------|-------------------|------------------------------------------------|
| Matrix Pro             | Grid | ESP32-S3 | Yes       |                   | [203.io](https://203.io/products/matrix-pro-pre-order)   |
| Matrix Founder Edition | Grid | STM32F1  | Partially | Not fully working | [203.io](https://203.io/products/matrix-founder-edition) |


## Getting Started
### User Interface
   TODO
### Compile and Upload
   Compile and Upload for Matrix Pro:
   Set Matrix Pro into bootloader mode (Hold fn key while power up or go though system setting) and the following
   ```
   make DEVICE=MatrixPro build uf2-upload
   ```
   TODO
### Make your own Matrix OS applications
   TODO
   
## Help

Any advise for common problems or issues, please post it in the github issues.

## License

This project is licensed under the MIT License - see the LICENSE.md file for details

## Acknowledgments
This project have used following projects:

[FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel) - RTOS kernal 

[TinyUSB](https://github.com/hathach/tinyusb) - USB Communlication

[Printf](https://github.com/eyalroz/printf/) - Logging Printf

Special thanks to [LA104](https://github.com/gabonator/LA104) for a lot of inspiration of this OS



