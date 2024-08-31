# Matrix OS

An operating system for human interface devices that is designed to be cross-platform and modular.


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
   [Matrix OS User Manual]([https://203.io/products/matrix-founder-edition](https://github.com/203-Systems/Matrix-OS-User-Manual))
### Compile and Upload
   Compile and Upload for Matrix Pro:
   1. Clone this repo to your local system and run the following while inside the repo directory to clone the library dependencies: `git submodule init; git submodule update --recursive`
   2. Install toolchain for ESP32-S3: [Windows](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/windows-setup.html) or [MacOS and Linux](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/linux-macos-setup.html)
   3. Ensure that `idf.py --version` returns something like `ESP-IDF v5.1.1` in the shell session you're going to use for the upcoming build and firmware upload step. If it does not, ensure you've followed `Step 4. Set up the environment variables` for [Windows](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/windows-setup.html#launching-esp-idf-environment) or [MacOS and Linux](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/linux-macos-setup.html#step-4-set-up-the-environment-variables)
   3. Set Matrix into bootloader mode (Hold the center key while powering up or going though system settings) and the following
   ```
   make DEVICE=Matrix build uf2-upload
   ```
   [Draft Full Setup Guide](https://docs.google.com/document/d/135LQrOv90ddeh9eeDWQ9uNN7ohSIBCHwusqJAIAWjfw/edit?usp=sharing)
   
### Make your own Matrix OS applications
   TODO
   
## Help

Any advice for common problems or issues, please post it in the GitHub issues or reach out via [Discord](https://discord.gg/92gXq6F2JH)

## License

This project is licensed under the MIT License - see the LICENSE.md file for details

## Acknowledgments
This project has been made possible with the following open-source projects:

[FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel) - Real Time OS Kernel 

[TinyUSB](https://github.com/hathach/tinyusb) - USB Host/Device stack

[Printf](https://github.com/eyalroz/printf/) - Formatted Printing Functions

[CB0R](https://github.com/quartzjer/cb0r) -  Zero-Footprint CBOR Decoder (Modified by @All-Your-Locks-Are-Belong-To-Us)

Special thanks to [LA104](https://github.com/gabonator/LA104) for a lot of inspiration for this OS
