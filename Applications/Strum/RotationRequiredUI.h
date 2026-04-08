#pragma once

#include <MatrixOS.h>

bool RotationRequiredUI(bool up, bool down, bool left, bool right) {
  bool rotated = false;

  Direction currentOrientation = MatrixOS::UserVar::rotation;

  if (!up && !down && !left && !right)
  {
    MLOGW("RotationRequiredUI", "No direction is set to true, nothing to do");
    return false;
  }

  if ((up && currentOrientation == UP) || (down && currentOrientation == DOWN) || (left && currentOrientation == LEFT) ||
      (right && currentOrientation == RIGHT))
  {
    MLOGD("RotationRequiredUI", "Already in correct orientation, nothing to do");
    return true;
  }

  UI rotationRequiredUI("Rotation Required", Color(0x00FF00), true);

  uint32_t startTime = MatrixOS::SYS::Millis();

  UIButton brightnessBtn;
  ;
  brightnessBtn.SetColor(Color::White);
  brightnessBtn.SetSize(Dimension(2, 2));
  rotationRequiredUI.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton rotateUpBtn;
  rotateUpBtn.SetName("Rotate Up");
  rotateUpBtn.SetSize(Dimension(2, 1));
  Direction actualDirection = currentOrientation;
  if ((actualDirection == UP && up) || (actualDirection == DOWN && down) || (actualDirection == LEFT && left) ||
      (actualDirection == RIGHT && right))
  {
    rotateUpBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, startTime); });
    rotateUpBtn.OnPress([&]() -> void {
      MatrixOS::SYS::Rotate(UP);
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateUpBtn.SetColor(Color(0x00FF00).Dim(32));
  }
  rotationRequiredUI.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate Right");
  rotateRightBtn.SetSize(Dimension(1, 2));
  actualDirection = (Direction)((currentOrientation + 90) % 360);
  if ((actualDirection == UP && up) || (actualDirection == DOWN && down) || (actualDirection == LEFT && left) ||
      (actualDirection == RIGHT && right))
  {
    rotateRightBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, startTime); });
    rotateRightBtn.OnPress([&]() -> void {
      MatrixOS::SYS::Rotate(RIGHT);
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateRightBtn.SetColor(Color(0x00FF00).Dim(32));
  }
  rotationRequiredUI.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate Down");
  rotateDownBtn.SetSize(Dimension(2, 1));
  actualDirection = (Direction)((currentOrientation + 180) % 360);
  if ((actualDirection == UP && up) || (actualDirection == DOWN && down) || (actualDirection == LEFT && left) ||
      (actualDirection == RIGHT && right))
  {
    rotateDownBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, startTime); });
    rotateDownBtn.OnPress([&]() -> void {
      MatrixOS::SYS::Rotate(DOWN);
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateDownBtn.SetColor(Color(0x00FF00).Dim(32));
  }
  rotationRequiredUI.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate Left");
  rotateLeftBtn.SetSize(Dimension(1, 2));
  actualDirection = (Direction)((currentOrientation + 270) % 360);
  if ((actualDirection == UP && up) || (actualDirection == DOWN && down) || (actualDirection == LEFT && left) ||
      (actualDirection == RIGHT && right))
  {
    rotateLeftBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, startTime); });
    rotateLeftBtn.OnPress([&]() -> void {
      MatrixOS::SYS::Rotate(LEFT);
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateLeftBtn.SetColor(Color(0x00FF00).Dim(32));
  }
  rotationRequiredUI.AddUIComponent(rotateLeftBtn, Point(2, 3));

  // Second, set the key event handler to match the intended behavior
  rotationRequiredUI.SetKeyEventHandler([&](InputEvent* inputEvent) -> bool {
    // If function key is hold down. Exit the application
    if (inputEvent->id.IsFunctionKey())
    {
      if (inputEvent->keypad.state == KeypadState::Released)
      {
        rotationRequiredUI.Exit(); // Exit the UI
      }

      return true; // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false; // Nothing happened. Let the UI handle the key event
  });

  //   // The UI object is now fully set up. Let the UI runtime to start and take over.
  rotationRequiredUI.Start();

  return rotated;
}