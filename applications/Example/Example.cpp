#include "Example.h"
#include "ui/UI.h" // Include the UI Framework

// Run once
void ExampleAPP::Setup() {
  MLOGI("Example", "Example Started");
}

// Run in a loop after Setup()
void ExampleAPP::Loop() {
  // Set up key event handler
  struct KeyEvent keyEvent; // Variable for the latest key event to be stored at
  while (MatrixOS::KEYPAD::Get(&keyEvent)) // While there is still keyEvent in the queue
  { KeyEventHandler(keyEvent.id, &keyEvent.info); } // Handle them

struct MidiPacket midiPacket; // Variable for the latest midi packet to be stored at
  while (MatrixOS::MIDI::Get(&midiPacket)) // While there is still midi packet in the queue
  { MidiEventHandler(midiPacket); } // Handle them
}

// Handle the key event from the OS
void ExampleAPP::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  Point xy = MatrixOS::KEYPAD::ID2XY(keyID);  // Trying to get the XY coordination of the KeyID
  if (xy)                                     // IF XY is valid, means it is a key on the grid
  {
    MLOGD("Example", "Key %d %d %d", xy.x, xy.y, keyInfo->state); // Print the key event to the debug log
    if (keyInfo->state == PRESSED)            // Key is pressed
    {
      MatrixOS::LED::SetColor(xy, color, 0);      // Set the LED color to a color. Last 0 means writes to the active layer (255 writes to the active layer as well but do not trigger auto update.)
    }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::LED::SetColor(xy, 0x000000, 0);  // Set the LED to off
    }
  }
  else                          // XY Not valid,
  {
    if (keyID == FUNCTION_KEY)  // FUNCTION_KEY is pre defined by the device, as the keyID for the system function key
    {
      UIMenu();                 // Open UI Menu
    }
  }
}

void ExampleAPP::MidiEventHandler(MidiPacket midiPacket) {
  // Echo back the midi packet to the source
  MatrixOS::MIDI::Send(midiPacket);

  //Midi Packet has port, status, and data
  // Port shows where this midi signal is from (USB, Bluetooth, RTPMIDI, HWPort, etc)
  // When sending midi packets. This is also where the midi signal will be sent to
  // See EMidiStatus enum in /os/framework/midiPacket.h for all the midi status
  // 0x0 sends to all first of available ports
  // Status is the midi status (NoteOn, NoteOff, ControlChange, etc)
  // See EMidiStatus enum in /os/framework/midiPacket.h for all the midi status

  // Wanna do more with the packet? Here's a example parser

  /*
  switch (midiPacket.status)
  {
    case NoteOn:
    case ControlChange:
      NoteHandler(midiPacket.channel(), midiPacket.note(), midiPacket.velocity());
      break;
    case NoteOff:
      NoteHandler(midiPacket.channel(), midiPacket.note(), 0);
      break;
    case SysExData:
    case SysExEnd:
      SysExHandler(midiPacket);
      break;
    default:
      break;
  }
  */
}

void ExampleAPP::UIMenu() {
  // Matrix OS Debug Log, sent to hardware UART and USB CDC
  MLOGI("Example", "Enter UI Menu");

  // Create a UI Object
  // UI Name, Color (as the text scroll color). and new led layer (Set as true, the UI will render on a new led layer. Persevere what was rendered before after UI exits)
  UI menu("UI Menu", Color(0x00FFFF), true);

  // Create an UI element
  UIButton numberSelector("Number Selector",  // Name of this UI element
                          Color(0xFF0000),    // Color of the UI element
                          [&]() -> void {     // Callback function when the button is pressed
                            number =          // Set number variable as the return value of the NumberSelector8x8 function
                                MatrixOS::UIInterface::NumberSelector8x8(number, 0xFF0000, "Number", 0, 100);  // Current Number, color, low range, high range
                            // EXAMPLEAPP_SAVED_VAR does not affect this code
                            // For most value type. the saved variable wrapper library requires no changes to code!
                          });
  // Add the UI element to the UI object to top left conner
  menu.AddUIComponent(numberSelector, Point(0, 0));

  // Create an dynamic colored button: UIButtonWithColorFunc - Use the a function as the color of the button
  UIButtonWithColorFunc colorSelector(
      "Color Selector",                                                     // Name of this UI element
      [&]() -> Color { return color; },                                     // Use the color variable as the color of this UI element
      [&]() -> void {           // Callback function when the button is pressed
        #ifndef EXAMPLEAPP_SAVED_VAR                                            
        MatrixOS::UIInterface::ColorPicker(color);    // Refences to the color variable. The color variable will be updated by the ColorPicker function. Return true if color is changed, false if not.
        #else
        MatrixOS::UIInterface::ColorPicker(color.value);  // Get the actual value from the saved variable wrapper library
        color.Set(color.value);                           // Save the new variable
        // The saved variable wrapper doesn't implicitly converts to the refences type. 
        // This way you know you have to get the refences manually and set the value back to the saved variable manually.
        #endif
      },
      [&]() -> void {                                                       // Optional Callback function for hold down. Reset color to default white.
        color = 0xFFFFFF;
      });

  // Add the UI element to the UI object to top right conner
  menu.AddUIComponent(colorSelector, Point(Device::x_size - 1, 0));

  // A large button that cycles though the brightness of the device
  UIButtonLarge brightnessBtn(
      "Brightness", //Name
      Color(0xFFFFFF), // Color
      Dimension(2, 2), // Size of the button
      [&]() -> void { MatrixOS::SYS::NextBrightness(); } // Function to call when the button is pressed
      );
  // Place this button in the center of the device
  menu.AddUIComponent(brightnessBtn, Point((Device::x_size - 1) / 2, (Device::y_size - 1) / 2)); 

  // Set a key event handler for the UI object
  // By default, the UI exits after the function key is PRESSED.
  // Since this is the main UI for this application. 
  // We want to exit the application when the function key is hold down,
  // and exit the UI is released (but before the hold down time threshold)

  // First, disable the default exit behavior 
  menu.AllowExit(false);

  // Second, set the key event handler to match the intended behavior
  menu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    // If function key is hold down. Exit the application
    if (keyEvent->id == FUNCTION_KEY)
    {
        if(keyEvent->info.state == HOLD)
        {
            Exit();  // Exit the application.

            return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI. This is not really needed as the application exits after
                            // Exit();
        }
        else if(keyEvent->info.state == RELEASED)
        {
            menu.Exit(); // Exit the UI
            return true; // Block UI from to do anything with FN, basically this function control the life cycle of the UI
        }
    }
    return false; // Nothing happened. Let the UI handle the key event
  });

  // The UI object is now fully set up. Let the UI runtime to start and take over.
  menu.Start();
  // Once the UI is exited (Not the application exit!), the code will continue here.
  // If Exit() is called in UI. The code will start in the End() of this application and then exit.

 // See /os/framework/UI/UI.h for more UI Framework API
 // See /os/framework/UI/UIComponents.h for more UI Components
 // See /os/framework/UI/UIInterface.h for more UI built in UI Interface

 // You can also create your own UI Components and UI Interfaces for your own application.
 // You can see the Note application for an example of how to do that. (Note Pad. Octave Shifter. Scales, ScaleVisualizer...)


  MLOGI("Example", "Exited UI Menu");
}

void ExampleAPP::End() {
  MLOGI("Example", "Example Exited");
}
