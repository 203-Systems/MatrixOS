#pragma once

#include <MatrixOS.h>

bool RotationRequiredUI(bool up, bool down, bool left, bool right) {
  bool rotated = false;

  Direction current_orientation = MatrixOS::UserVar::rotation;

  if(!up && !down && !left && !right)
  {
    MLOGW("RotationRequiredUI", "No direction is set to true, nothing to do");
    return false;
  }

  if((up && current_orientation == UP) || (down && current_orientation == DOWN) || (left && current_orientation == LEFT) || (right && current_orientation == RIGHT))
  {
    MLOGD("RotationRequiredUI", "Already in correct orientation, nothing to do");
    return true;
  }

  UI rotationRequiredUI("Rotation Required", Color(0x00FF00), true);

  uint32_t start_time = MatrixOS::SYS::Millis();

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  rotationRequiredUI.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton rotateUpBtn;
  rotateUpBtn.SetName("Rotate Up");
  rotateUpBtn.SetSize(Dimension(2, 1)); 
  Direction actual_direction = current_orientation;
  if((actual_direction == UP && up) || (actual_direction == DOWN && down) || (actual_direction == LEFT && left) || (actual_direction == RIGHT && right))
  {
    rotateUpBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, start_time); });
    rotateUpBtn.OnPress([&]() -> void { 
      MatrixOS::SYS::Rotate(UP); 
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateUpBtn.SetColor(Color(0x00FF00).Dim());
  }
  rotationRequiredUI.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate Right");
  rotateRightBtn.SetSize(Dimension(1, 2));
  actual_direction = (Direction)((current_orientation + 90) % 360);
  if((actual_direction == UP && up) || (actual_direction == DOWN && down) || (actual_direction == LEFT && left) || (actual_direction == RIGHT && right))
  {
    rotateRightBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, start_time); });
    rotateRightBtn.OnPress([&]() -> void { 
      MatrixOS::SYS::Rotate(RIGHT); 
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateRightBtn.SetColor(Color(0x00FF00).Dim());
  }
  rotationRequiredUI.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate Down");
  rotateDownBtn.SetSize(Dimension(2, 1));
  actual_direction = (Direction)((current_orientation + 180) % 360);
  if((actual_direction == UP && up) || (actual_direction == DOWN && down) || (actual_direction == LEFT && left) || (actual_direction == RIGHT && right))
  {
    rotateDownBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, start_time); });
    rotateDownBtn.OnPress([&]() -> void { 
      MatrixOS::SYS::Rotate(DOWN); 
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateDownBtn.SetColor(Color(0x00FF00).Dim());
  }
  rotationRequiredUI.AddUIComponent(rotateDownBtn, Point(3, 5));
    
  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate Left");
  rotateLeftBtn.SetSize(Dimension(1, 2));
  actual_direction = (Direction)((current_orientation + 270) % 360);
  if((actual_direction == UP && up) || (actual_direction == DOWN && down) || (actual_direction == LEFT && left) || (actual_direction == RIGHT && right))
  {
    rotateLeftBtn.SetColorFunc([&]() -> Color { return ColorEffects::ColorBreathLowBound(Color(0x00FF00), 64, 2000, start_time); });
    rotateLeftBtn.OnPress([&]() -> void { 
      MatrixOS::SYS::Rotate(LEFT); 
      rotated = true;
      rotationRequiredUI.Exit();
    });
  }
  else
  {
    rotateLeftBtn.SetColor(Color(0x00FF00).Dim());
  }
  rotationRequiredUI.AddUIComponent(rotateLeftBtn, Point(2, 3));


  // Second, set the key event handler to match the intended behavior
  rotationRequiredUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    // If function key is hold down. Exit the application
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == RELEASED)
      {
        rotationRequiredUI.Exit();  // Exit the UI
      }

      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;  // Nothing happened. Let the UI handle the key event
  });

  //   // The UI object is now fully set up. Let the UI runtime to start and take over.
  rotationRequiredUI.Start();

  return rotated;
}