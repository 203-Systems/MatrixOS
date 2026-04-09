# MatrixOS Python Interface - UI
# Core UI class for creating user interfaces.
#
# Input handler example (input-centric, not keypad-assumed):
#
#   def on_input(event):
#       if event.InputClass() != InputClass.KEYPAD:
#           return False
#       info = event.Keypad()
#       if info is None:
#           return False
#       # handle keypad event using info.State(), info.Force(), etc.
#       return True
#
#   ui = MatrixOS.UI.UI("Demo", Color(0xFFFFFF))
#   ui.SetInputHandler(on_input)
#   ui.Start()

from MatrixOS_UIComponent import UIComponent
from MatrixOS_Point import Point
from MatrixOS_InputEvent import InputEvent
from MatrixOS_Color import Color

class UI:
    def __init__(self, *val) -> None: ...

    # Lifecycle
    def Start(self) -> None: ...
    def Close(self) -> bool: ...

    # Configuration
    def SetName(self, name: str) -> None: ...
    def SetColor(self, color: Color) -> None: ...
    def ShouldCreatenewLEDLayer(self, create: bool) -> None: ...

    # Callback setters (return bool indicating success).
    def SetSetupFunc(self, setupFunc: any) -> bool: ...
    def SetLoopFunc(self, loopFunc: any) -> bool: ...
    def SetEndFunc(self, endFunc: any) -> bool: ...
    def SetPreRenderFunc(self, pre_renderFunc: any) -> bool: ...
    def SetPostRenderFunc(self, post_renderFunc: any) -> bool: ...

    # Set the input handler callback.
    # The handler receives an InputEvent and must return bool
    # (True = consumed, False = propagate).
    # Check event.InputClass() before accessing keypad-specific payload.
    def SetInputHandler(self, input_handler: any) -> bool: ...

    # UI Component management
    def AddUIComponent(self, uiComponent: UIComponent, xy: Point) -> None: ...
    def ClearUIComponents(self) -> None: ...

    # UI control
    def AllowExit(self, allow: bool) -> None: ...
    def SetFPS(self, fps: int) -> None: ...