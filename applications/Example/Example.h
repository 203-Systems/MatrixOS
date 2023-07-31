/*
This is the example application for Matrix OS
Remember to include this header file in the Applications.h in your device family folder (devices/<Device Family>/Applications.h)

What this application does:
Any pressed will lit up the key (in a user defined color)
Click the function key will open an menu UI
Top left button will open a number selector UI that saves teh value into the number variable
Top right button will open a color picker UI that saves the value into the color variable. This also changes the color of the button pressed
Click the function key in the menu will exit the UI
Hold the function key in the menu will exit the application
Midi signal recived will be echoed back to the host
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

class ExampleAPP : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;
  void End() override;



  // Wanna make your number and color saves between restarts? Comment out the define below. 
  // This macro change the code that will the color variable to a saved variable
  // And replace part of the code to support it

  // #define EXAMPLEAPP_SAVED_VAR

#ifndef EXAMPLEAPP_SAVED_VAR
  uint8_t number = 0;
  Color color = Color(0xFFFFFF);
#else
  CreateSavedVar("Example", number, uint8_t, 0);
  CreateSavedVar("Example", color, Color, Color(0xFFFFFF));
  
  // Namespace (This namespace only applies to this application. So even if two different applications have the same variable name, they won't conflict)， variable name (no ""), variable type, default value
  // And then just use the variable as a normal variable. The value will be saved & loaded automatically!
  // However, not all variable type and operator is supported. If that is the case, you have to get the variable via .Get() and .Set()
  // For more, see /os/framework/SavedVariable.h
  #endif

  void UIMenu();
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
  void MidiEventHandler(MidiPacket midiPacket);
};

// Meta data about this application
inline Application_Info ExampleAPP::info = {
    .name = "Example",
    .author = "203 Electronics",
    .color = Color(0xFFFFFF),
    .version = 1,
    .visibility = true,
};

// Register this Application to the OS (Use the class name of your application as the variable)
REGISTER_APPLICATION(ExampleAPP);