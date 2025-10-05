// Matrix Founder Edition Variant Configuration
#include "Device.h"

void LoadFounderEdition() {
  // Configure LED
  Device::LED::led_port = GPIOC;
  Device::LED::led_pin = GPIO_PIN_7;

  // Configure FN key
  Device::KeyPad::fn_port = GPIOA;
  Device::KeyPad::fn_pin = GPIO_PIN_8;

  // Configure keypad write pins (columns)
  Device::KeyPad::keypad_write_ports[0] = GPIOB; Device::KeyPad::keypad_write_pins[0] = GPIO_PIN_12; // Key1
  Device::KeyPad::keypad_write_ports[1] = GPIOB; Device::KeyPad::keypad_write_pins[1] = GPIO_PIN_13; // Key2
  Device::KeyPad::keypad_write_ports[2] = GPIOB; Device::KeyPad::keypad_write_pins[2] = GPIO_PIN_14; // Key3
  Device::KeyPad::keypad_write_ports[3] = GPIOB; Device::KeyPad::keypad_write_pins[3] = GPIO_PIN_15; // Key4
  Device::KeyPad::keypad_write_ports[4] = GPIOC; Device::KeyPad::keypad_write_pins[4] = GPIO_PIN_6;  // Key5
  Device::KeyPad::keypad_write_ports[5] = GPIOC; Device::KeyPad::keypad_write_pins[5] = GPIO_PIN_7;  // Key6
  Device::KeyPad::keypad_write_ports[6] = GPIOC; Device::KeyPad::keypad_write_pins[6] = GPIO_PIN_8;  // Key7
  Device::KeyPad::keypad_write_ports[7] = GPIOC; Device::KeyPad::keypad_write_pins[7] = GPIO_PIN_9;  // Key8

  // Configure keypad read pins (rows)
  Device::KeyPad::keypad_read_ports[0] = GPIOB; Device::KeyPad::keypad_read_pins[0] = GPIO_PIN_0;  // KeyRead1
  Device::KeyPad::keypad_read_ports[1] = GPIOB; Device::KeyPad::keypad_read_pins[1] = GPIO_PIN_1;  // KeyRead2
  Device::KeyPad::keypad_read_ports[2] = GPIOA; Device::KeyPad::keypad_read_pins[2] = GPIO_PIN_11; // KeyRead3
  Device::KeyPad::keypad_read_ports[3] = GPIOA; Device::KeyPad::keypad_read_pins[3] = GPIO_PIN_12; // KeyRead4
  Device::KeyPad::keypad_read_ports[4] = GPIOC; Device::KeyPad::keypad_read_pins[4] = GPIO_PIN_4;  // KeyRead5
  Device::KeyPad::keypad_read_ports[5] = GPIOA; Device::KeyPad::keypad_read_pins[5] = GPIO_PIN_15; // KeyRead6
  Device::KeyPad::keypad_read_ports[6] = GPIOC; Device::KeyPad::keypad_read_pins[6] = GPIO_PIN_5;  // KeyRead7
  Device::KeyPad::keypad_read_ports[7] = GPIOC; Device::KeyPad::keypad_read_pins[7] = GPIO_PIN_3;  // KeyRead8

  // Configure touchbar
  Device::KeyPad::touchData_Port = GPIOA;
  Device::KeyPad::touchData_Pin = GPIO_PIN_0;
  Device::KeyPad::touchClock_Port = GPIOA;
  Device::KeyPad::touchClock_Pin = GPIO_PIN_1;

  // Set device name and model
  Device::name = "MatrixBlock5";
  Device::model = "MB5F";
  Device::product_name = "MatrixBlock5";
}