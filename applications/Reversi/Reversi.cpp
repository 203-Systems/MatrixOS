#include "Reversi.h"

void Reversi::Setup() {
  ResetGame(true);
}

void Reversi::Loop()
{
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }

  if(renderTimer.Tick(1000/Device::fps))
  {
    Render();
  }
}

uint8_t Reversi::Flip(Point pos, uint8_t currentPlayer, bool update)
{
  uint8_t opponentPlayer = currentPlayer == 1 ? 2 : 1;
  uint8_t flipped = 0;

    // Check all 8 directions
    for(int8_t dy = -1; dy <= 1; dy++)
    {
        for(int8_t dx = -1; dx <= 1; dx++)
        {
            if(dy == 0 && dx == 0)
            {
                continue;
            }

            int distance = 1;
            int checkY = pos.y + dy * distance;
            int checkX = pos.x + dx * distance;
            bool hasOpponentBetween = false;

            // Immediate neighbor check
            if(checkY >= 0 && checkY < 8 && checkX >= 0 && checkX < 8 && board[checkY][checkX].player == opponentPlayer)
            {
                hasOpponentBetween = true;

                while(true)
                {
                    distance++;
                    checkY = pos.y + dy * distance;
                    checkX = pos.x + dx * distance;

                    // Check for board boundaries
                    if(checkY < 0 || checkY >= 8 || checkX < 0 || checkX >= 8)
                    {
                        hasOpponentBetween = false;
                        break;
                    }

                    int playerAtPosition = board[checkY][checkX].player;

                    if(playerAtPosition == opponentPlayer)
                    {
                        continue;
                    }
                    else if(playerAtPosition == currentPlayer)
                    {
                        // Flip pieces between pos and (checkX, checkY)
                        int flipY = pos.y + dy;
                        int flipX = pos.x + dx;
                        while(flipY != checkY || flipX != checkX)
                        {
                            if(update) 
                            {
                              board[flipY][flipX].player = currentPlayer;
                            }
                            flipped++;
                            flipY += dy;
                            flipX += dx;
                        }
                        break;
                    }
                    else
                    {
                        // Empty cell, cannot flip in this direction
                        hasOpponentBetween = false;
                        break;
                    }
                }
            }
        }
    }
    return flipped;
}


void Reversi::Place(Point pos)
{
  invalidPlace = pos;
  invalidPlaceTime = MatrixOS::SYS::Millis();
  if(pos.x < 0 || pos.x >= 8 || pos.y < 0 || pos.y >= 8)
  {
    return;
  }

  if(gameState != Waiting)
  {
    return;
  }

  if(board[pos.y][pos.x].validMove == 0)
  {
    return;
  }

  invalidPlace = Point::Invalid();

  started = true;
  
  // Update board
  for(uint8_t y = 0; y < 8; y++)
  {
    for(uint8_t x = 0; x < 8; x++)
    {
      // Copy player to wasPlayer
      board[y][x].wasPlayer = board[y][x].player;
      board[y][x].newlyPlaced = false;
    }
  }

  board[pos.y][pos.x].newlyPlaced = true;
  board[pos.y][pos.x].player = currentPlayer;
  placedPos = pos;

  Flip(pos, currentPlayer, true);
  
  gameState = Moving;
  MLOGD("Reversi", "Moving");
  lastEventTime = MatrixOS::SYS::Millis();
}


void Reversi::Render()
{
  uint32_t timeSinceEvent = MatrixOS::SYS::Millis() - lastEventTime;
  if(gameState == Waiting)
  {

    Color hint_move_color;
    if(hint)
    {
      uint8_t hint_move_ratio;
      if(timeSinceEvent <= 750)
      {
        hint_move_ratio = ColorEffects::Breath(1500, lastEventTime);
      }
      else
      {
        hint_move_ratio = ColorEffects::BreathLowBound(64, 1500, lastEventTime);
      }
      hint_move_color = GetPlayerColor(currentPlayer).Dim().Dim(hint_move_ratio);
      last_breating_brightness = hint_move_color.B; // Hack: Get the channel brightness val
    }
    else
    {
      last_breating_brightness = 0;
    }

    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        if(board[y][x].validMove && hint)
        {
          MatrixOS::LED::SetColor(Point(x, y), hint_move_color);
        }
        else
        {
          MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(board[y][x].player));
        }
      }
    }

    uint32_t timeSinceInvalidPlace = MatrixOS::SYS::Millis() - invalidPlaceTime;
    if(invalidPlace && timeSinceInvalidPlace < 800)
    {
      if(timeSinceInvalidPlace <= 500)
      {
        MatrixOS::LED::SetColor(invalidPlace, Color(0xFF0000));
      }
      else
      {
        MatrixOS::LED::SetColor(invalidPlace, Color::Crossfade(Color(0xFF0000), GetPlayerColor(board[invalidPlace.y][invalidPlace.x].player), 255 - ((timeSinceInvalidPlace - 500) * 255 / 300)));
      }
    }
    MatrixOS::LED::FillPartition("Underglow", ColorEffects::ColorBreath(GetPlayerColor(currentPlayer), 2000, lastEventTime - 500));
  }
  else if(gameState == Moving)
  {
    bool done = true;
    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        if(board[y][x].player != board[y][x].wasPlayer)
        {
          int8_t deltaX = x - placedPos.x;
          int16_t powerX = pow(deltaX, 2);
          int8_t deltaY = y - placedPos.y;
          int16_t powerY = pow(deltaY, 2);
          float distanceFromPlaced = sqrt(powerX + powerY);
          Fract16 ratio;

          uint32_t startTime = distanceFromPlaced * 100 + 200;

          if(board[y][x].newlyPlaced)
          {
            MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(board[y][x].player));//.Dim(new_piece_ratio));
            continue;
          }

          if(timeSinceEvent <= startTime)
          {
            ratio = 0;
            done = false;
          }
          else if(timeSinceEvent - 300 > startTime )
          {
            ratio = FRACT16_MAX;
          }
          else
          {
            ratio = (timeSinceEvent - startTime) * FRACT16_MAX / 300;
            done = false;
          }

          MatrixOS::LED::SetColor(Point(x, y), Color::Crossfade(GetPlayerColor(board[y][x].wasPlayer), GetPlayerColor(currentPlayer), ratio));
        }
        else if(board[y][x].validMove && hint)
        {
          uint8_t ratio;
          if(timeSinceEvent <= 200)
          {
            ratio = last_breating_brightness - (timeSinceEvent * last_breating_brightness / 200);
          }
          else
          {
            ratio = 0;
          }

          MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(currentPlayer).Dim(ratio));
        }
        else
        {
          MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(board[y][x].player));
        }
      }
    }

    MatrixOS::LED::FillPartition("Underglow", GetPlayerColor(currentPlayer));

    if(done && timeSinceEvent >= 250)
    {
      winner = 255;
      gameState = Intermission;
      MLOGD("Reversi", "Intermission");
      lastEventTime = MatrixOS::SYS::Millis();
    }
  }
  else if(gameState == NoValidMoves)
  {
    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        if(board[y][x].player == 0)
        {
          MatrixOS::LED::SetColor(Point(x, y), ColorEffects::ColorStrobe(Color(0xFF0000).Dim(), 500, lastEventTime));
        }
      }
    }

    MatrixOS::LED::FillPartition("Underglow", ColorEffects::ColorStrobe(GetPlayerColor(currentPlayer), 500, lastEventTime));

    if(timeSinceEvent >= 2000)
    {
      winner = 255;
      gameState = Intermission;
      MLOGD("Reversi", "Intermission");
      lastEventTime = MatrixOS::SYS::Millis();
    }
  }
  else if(gameState == Intermission)
  {
    if(winner == 255)
    {
      winner = CheckGameOver();
    }

    uint8_t nextPlayer = currentPlayer == 1 ? 2 : 1;
    Fract16 ratio;

    if (timeSinceEvent <= 500)
    { ratio = timeSinceEvent * FRACT16_MAX / 500;}
    else
    { ratio = FRACT16_MAX; }

    if(winner != 0 && winner != 254)
    {
      MatrixOS::LED::FillPartition("Underglow", Color::Crossfade(GetPlayerColor(currentPlayer), GetPlayerColor(winner), ratio));
    }
    else
    {
      MatrixOS::LED::FillPartition("Underglow", Color::Crossfade(GetPlayerColor(currentPlayer), GetPlayerColor(nextPlayer), ratio));
    }

    if(timeSinceEvent >= 500)
    {
      if(winner == 0)
      {
        gameState = Waiting;
        MLOGD("Reversi", "Waiting");
        currentPlayer = nextPlayer;
      }
      else if(winner == 254)
      {
        gameState = NoValidMoves;
        MLOGD("Reversi", "NoValidMoves");
        currentPlayer = nextPlayer;
      }
      else
      {
        gameState = WinnerUnveil;
        MLOGD("Reversi", "WinnerUnveil");
      }
      lastEventTime = MatrixOS::SYS::Millis();
    }
  }
  else if(gameState == WinnerUnveil)
  {
    bool done = true;
    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        uint32_t startTime = (y * 8 + x) * 50;

        if(timeSinceEvent >= (startTime + 1000)) // 1000 (Done)
        {
          uint8_t localWinner = winner;
          if(winner == 3)
          {
            localWinner = (x + y * 7) % 2 == 0 ? 1 : 2;
          }
          MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(localWinner));
        }
        else if(timeSinceEvent >= (startTime + 700)) // 700 - 1000
        {
          done = false;
          uint8_t localWinner = winner;
          if(winner == 3)
          {
            localWinner = (x + y * 7) % 2 == 0 ? 1 : 2;
          }
          Fract16 ratio = (timeSinceEvent - 700 - startTime) * FRACT16_MAX / 300;
          MatrixOS::LED::SetColor(Point(x, y), Color::Crossfade(Color(0xFFFFFF), GetPlayerColor(localWinner), ratio));
        }
        else if(timeSinceEvent >= (startTime + 300)) // 300 - 700
        {
          done = false;
          MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF));
        }
        else if(timeSinceEvent >= startTime) // 0 - 300
        {
          done = false;
          Fract16 ratio = (timeSinceEvent - startTime) * FRACT16_MAX / 300;
          MatrixOS::LED::SetColor(Point(x, y), Color::Crossfade(GetPlayerColor(board[y][x].player), Color(0xFFFFFF), ratio));
        }
        else // Before the start time
        {
          done = false;
          MatrixOS::LED::SetColor(Point(x, y), GetPlayerColor(board[y][x].player));
        }
      }
    }

    if(done)
    {
      gameState = Ended;
      MLOGD("Reversi", "Ended");
    }
  }
  else if(gameState == Ended)
  {
    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        uint8_t localWinner = winner;
        if(winner == 3)
        {
         localWinner = (x + y * 7) % 2 == 0 ? 1 : 2;
        }
        MatrixOS::LED::SetColor(Point(x,y), ColorEffects::ColorBreathLowBound(GetPlayerColor(localWinner), 64, 2000, lastEventTime + 1000));
      }
    }

  }

  MatrixOS::LED::Update();
}

uint8_t Reversi::CheckGameOver()
{
  uint8_t opponentPlayer = currentPlayer == 1 ? 2 : 1;
  bool forceWinner = false;

  // Check if there are any valid moves
  bool validMove = false;

  for(uint8_t y = 0; y < 8; y++)
  {
    for(uint8_t x = 0; x < 8; x++)
    {
      if(board[y][x].player == 0)
      {
        if(Flip(Point(x, y), opponentPlayer, false))
        {
          board[y][x].validMove = 1;
          validMove = true;
        }
        else
        {
          board[y][x].validMove = 0;
        }
      }
      else
      {
        board[y][x].validMove = 0;
      }
    }
  }

  if(!validMove) // Check the current player has valid move
  {
    MLOGD("Reversi", "No valid moves for player %d", currentPlayer);
    validMove = false;
    for(uint8_t y = 0; y < 8; y++)
    {
      for(uint8_t x = 0; x < 8; x++)
      {
        if(board[y][x].player == 0)
        {
          if(Flip(Point(x, y), currentPlayer, false))
          {
            validMove = true;
          }
        }
      }
    }

    if(!validMove)
    {
      MLOGD("Reversi", "No valid moves for both players, forcing a winner");
      forceWinner = true;
    }
    else
    {
      MLOGD("Reversi", "Valid moves for next player");
      return 254; // No valid moves
    }
  }
  else
  {
    MLOGD("Reversi", "Valid moves for current player");
    return 0;
  }


  uint8_t player1Count = 0;
  uint8_t player2Count = 0;
  for(uint8_t y = 0; y < 8; y++)
  {
    for(uint8_t x = 0; x < 8; x++)
    {
      if(board[y][x].player == 1)
      {
        player1Count++;
      }
      else if(board[y][x].player == 2)
      {
        player2Count++;
      }
    }
  }

  if(player1Count > player2Count)
  {
    return 1;
  }
  else if(player2Count > player1Count)
  {
    return 2;
  }
  else // Draw
  {
    return 3;
  }
}

bool Reversi::ConfirmMenu()
{
  bool confirmed = false;
  UI confirmUI("Reset Game?", Color(0xFF0000), false);

  UIButton cancelResetBtn;
  cancelResetBtn.SetName("Cancel");
  cancelResetBtn.SetColor(Color(0xFF0000));
  cancelResetBtn.SetSize(Dimension(2, 2));
  cancelResetBtn.OnPress([&]() -> void {
    confirmed = false;
    confirmUI.Exit();
  });
  confirmUI.AddUIComponent(cancelResetBtn, Point(1, 3));

  UIButton confirmResetBtn;
  confirmResetBtn.SetName("Confirm");
  confirmResetBtn.SetColor(Color(0x00FF00));
  confirmResetBtn.SetSize(Dimension(2, 2));
  confirmResetBtn.OnPress([&]() -> void {
    confirmed = true;
    confirmUI.Exit();
  });
  confirmUI.AddUIComponent(confirmResetBtn, Point(5, 3));

  confirmUI.Start();

  return confirmed;
}

bool Reversi::ResetGame(bool confirmed)
{
  if(!(gameState == Ended || !started) && (!confirmed && !ConfirmMenu()))
  {
      return false;
  }
  
  for(uint8_t y = 0; y < 8; y++)
  {
    for(uint8_t x = 0; x < 8; x++)
    {
      board[y][x].newlyPlaced = 0;
      board[y][x].player = 0;
      board[y][x].wasPlayer = 0;
      board[y][x].validMove = 0;
    }
  }

  firstPlayer.Load(); // For some reason, the firstPlayer is not auto loaded correctly

  uint8_t secondPlayer = firstPlayer == 1 ? 2 : 1;

  board[3][3].player = firstPlayer;
  board[3][3].wasPlayer = firstPlayer;
  board[4][4].player = firstPlayer;
  board[4][4].wasPlayer = firstPlayer;

  board[3][4].player = secondPlayer;
  board[3][4].wasPlayer = secondPlayer;
  board[4][3].player = secondPlayer;
  board[4][3].wasPlayer = secondPlayer;

  board[2][4].validMove = 1;
  board[3][5].validMove = 1;
  board[4][2].validMove = 1;
  board[5][3].validMove = 1;

  currentPlayer = firstPlayer;
  gameState = Waiting;
  MLOGD("Reversi", "Waiting");
  winner = 0;
  started = false;

  lastEventTime = MatrixOS::SYS::Millis();

  return true;
}

Color Reversi::GetPlayerColor(uint8_t player)
{
  if(player == 1)
  {
    return player1Color;
  }
  else if(player == 2)
  {
    return player2Color;
  }
  else if(player == 3) // Draw
  {
    return Color(0xFFFFFF);
  }
  else
  {
    return Color(0x000000);
  }
}

void Reversi::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  if (keyID == FUNCTION_KEY)
  {
    if (keyInfo->state == PRESSED)
    {
      Settings();
    }
    return;
  }

  if(gameState == Waiting)
  {
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    
    if (xy && keyInfo->state == RELEASED)  // IF XY is valid, means it's on the main grid
    {
      Place(xy);
    }
  }
}


void Reversi::Settings() {
  UI settingsUI("Settings", Color(0x00FFFF), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  settingsUI.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton rotateUpBtn;
  rotateUpBtn.SetName("Rotate Up");
  rotateUpBtn.SetColor(Color(0x00FF00));
  rotateUpBtn.SetSize(Dimension(2, 1));
  rotateUpBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(UP); });
  settingsUI.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate Right");
  rotateRightBtn.SetColor(Color(0x00FF00));
  rotateRightBtn.SetSize(Dimension(1, 2));
  rotateRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  settingsUI.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate Down");
  rotateDownBtn.SetColor(Color(0x00FF00));
  rotateDownBtn.SetSize(Dimension(2, 1));
  rotateDownBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  settingsUI.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate Left");
  rotateLeftBtn.SetColor(Color(0x00FF00));
  rotateLeftBtn.SetSize(Dimension(1, 2));
  rotateLeftBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  settingsUI.AddUIComponent(rotateLeftBtn, Point(2, 3));


    UIButton player1ColorSelector;
    player1ColorSelector.SetName("Player 1 Color");
    player1ColorSelector.SetColorFunc([&]() -> Color { return player1Color; });
    player1ColorSelector.SetSize(Dimension(8,1));
    player1ColorSelector.OnPress([&]() -> void { if(MatrixOS::UIUtility::ColorPicker(player1Color.Get())) { player1Color.Save(); } });
    settingsUI.AddUIComponent(player1ColorSelector, Point(0, 7));

    UIButton player2ColorSelector;
    player2ColorSelector.SetName("Player 2 Color");
    player2ColorSelector.SetColorFunc([&]() -> Color { return player2Color; });
    player2ColorSelector.SetSize(Dimension(8,1));
    player2ColorSelector.OnPress([&]() -> void { if(MatrixOS::UIUtility::ColorPicker(player2Color.Get())) { player2Color.Save(); } });
    settingsUI.AddUIComponent(player2ColorSelector, Point(0, 0));

    UIButton player1FirstHand;
    player1FirstHand.SetName("Player 1 First Hand");
    player1FirstHand.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(firstPlayer == 1); });
    player1FirstHand.OnPress([&]() -> void { if(firstPlayer == 1) {return;} if (gameState == Ended || !started || ConfirmMenu()) { firstPlayer = 1; ResetGame(true);} });
    settingsUI.AddUIComponent(player1FirstHand, Point(7, 6));

    UIButton player2FirstHand;
    player2FirstHand.SetName("Player 2 First Hand");
    player2FirstHand.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(firstPlayer == 2); });
    player2FirstHand.OnPress([&]() -> void { if(firstPlayer == 2) {return;} if (gameState == Ended || !started || ConfirmMenu()) { firstPlayer = 2; ResetGame(true);} });
    settingsUI.AddUIComponent(player2FirstHand, Point(0, 1));

    UIToggle hintToggle;
    hintToggle.SetName("Placement Hint");
    hintToggle.SetColor(Color(0x00FF00));
    hintToggle.SetSize(Dimension(1, 2));
    hintToggle.SetValue(&hint);
    hintToggle.OnPress([&]() -> void {hint.Save();});
    settingsUI.AddUIComponent(hintToggle, Point(0, 3));

    UIButton resetGameBtn;
    resetGameBtn.SetName("Reset Game");
    resetGameBtn.SetColorFunc([&]() -> Color { return Color(0xFF0000); });
    resetGameBtn.OnPress([&]() -> void {
      if(ResetGame(false)) { settingsUI.Exit(); }
    });
    resetGameBtn.SetSize(Dimension(1, 2));
    settingsUI.AddUIComponent(resetGameBtn, Point(7, 3));


    // UIButton testBtn;
    // testBtn.SetName("Test");
    // testBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00); });
    // testBtn.OnPress([&]() -> void {
    //   uint64_t seed = MatrixOS::SYS::Millis() * (uint32_t)this;
    //   seed ^= seed << 13;
    //   seed ^= seed >> 7;
    //   seed ^= seed << 17;
    //     for(uint8_t x = 0; x < 8; x++)
    //     {
    //       for (uint8_t y = 0; y < 8; y++)
    //       {
    //         uint8_t id = y * 8 + x;
    //         board[y][x].player = (seed >> id & 1) + 1;
    //       }
    //     }
    //     board[3][3].player = 0;
    // });
    // settingsUI.AddUIComponent(testBtn, Point(1, 3));

  
  // Second, set the key event handler to match the intended behavior
  settingsUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    // If function key is hold down. Exit the application
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      {
        Exit();  // Exit the application.
      }
      else if (keyEvent->info.state == RELEASED)
      {
        settingsUI.Exit();  // Exit the UI
      }

      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;  // Nothing happened. Let the UI handle the key event
  });
  
  // The UI object is now fully set up. Let the UI runtime to start and take over.
  settingsUI.Start();
}