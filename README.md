# Matrix OS

An operating system for human interface devices that is designed to be cross-platform and modular.

Currently, the OS only supports MIDI but more to come in later revisions.

## Description
Matrix OS is composited by 4 different layers:
#### Core Layer
Handles the basics of the chip. Located at core/
#### Device Layer
Drivers and configurations of the target device. Located at devices/
#### System Layer
Handles the system and application runtime. Located at os/
#### Application Layer
Applications that run on Matrix OS. All user interactions are Matrix OS applications. Located at applications/ 

## Supported Devices
| Device Name            | Type | Chip     | Supported | Note              | Purchase                                       |
|------------------------|------|----------|-----------|-------------------|------------------------------------------------|
| Matrix Pro             | Grid | ESP32-S3 | Yes       |                   | [203.io](https://203.io/products/matrix-pro-pre-order)   |
| Matrix Founder Edition | Grid | STM32F1  | Partially | Not fully stable | [203.io](https://203.io/products/matrix-founder-edition) |


## Getting Started
### User Interface
   TODO
### Compile and Upload
   Compile and Upload for Matrix Pro:
   Set Matrix Pro into bootloader mode (Hold the FN key while powering up or going though system settings) and the following
   ```
   make DEVICE=Matrix build uf2-upload
   ```
   TODO
### Make your own Matrix OS applications
   TODO
   
## Help

Any advice for common problems or issues, please post it in the GitHub issues.

## License

This project is licensed under the MIT License - see the LICENSE.md file for details

## Acknowledgments
This project has made possible with the following open source projects:

[FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel) - Real Time OS Kernel 

[TinyUSB](https://github.com/hathach/tinyusb) - USB Host/Device stack

[Printf](https://github.com/eyalroz/printf/) - Formatted Printing Functions

[CB0R](https://github.com/quartzjer/cb0r) -  Zero-Footprint CBOR Decoder (Modified by @All-Your-Locks-Are-Belong-To-Us)

Special thanks to [LA104](https://github.com/gabonator/LA104) for a lot of inspiration for this OS