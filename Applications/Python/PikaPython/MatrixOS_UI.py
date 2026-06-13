import _MatrixOS_UI

from MatrixOS_UIComponent import UIComponent
from MatrixOS_UIButton import UIButton
from MatrixOS_UISelector import UISelector
from MatrixOS_UI4pxNumber import UI4pxNumber

class KeyEvent:
    def __init__(self, code: int):
        self.code = code

    def ClusterId(self) -> int:
        return self.code & 0xFF

    def cluster_id(self) -> int:
        return self.ClusterId()

    def KeyState(self) -> int:
        return (self.code >> 8) & 0xFF

    def key_state(self) -> int:
        return self.KeyState()

    def MemberId(self) -> int:
        return (self.code >> 16) & 0xFFFF

    def member_id(self) -> int:
        return self.MemberId()

    def X(self) -> int:
        if self.ClusterId() == 1:
            return self.MemberId() % 8
        return self.MemberId() & 0xFF

    def x(self) -> int:
        return self.X()

    def Y(self) -> int:
        if self.ClusterId() == 1:
            return self.MemberId() // 8
        return (self.MemberId() >> 8) & 0xFF

    def y(self) -> int:
        return self.Y()

    def IsPressed(self) -> bool:
        return self.KeyState() == 2

    def is_pressed(self) -> bool:
        return self.IsPressed()

    def IsHold(self) -> bool:
        return self.KeyState() == 3

    def is_hold(self) -> bool:
        return self.IsHold()

    def IsAftertouch(self) -> bool:
        return self.KeyState() == 4

    def is_aftertouch(self) -> bool:
        return self.IsAftertouch()

    def IsReleased(self) -> bool:
        return self.KeyState() == 5

    def is_released(self) -> bool:
        return self.IsReleased()

    def IsFunctionKey(self) -> bool:
        return self.ClusterId() == 0

    def is_function_key(self) -> bool:
        return self.IsFunctionKey()

class UI(_MatrixOS_UI.UI):
    def PullInput(self) -> any:
        code = self.PullInputCode()
        if code < 0:
            return None
        return KeyEvent(code)

    def pull_input(self) -> any:
        return self.PullInput()

    def start(self) -> None:
        self.Start()

    def exit(self) -> None:
        self.Exit()

    def close(self) -> bool:
        return self.Close()

    def set_name(self, name: str) -> None:
        self.SetName(name)

    def set_color(self, color) -> None:
        self.SetColor(color)

    def set_setup_func(self, setup_func) -> bool:
        return self.SetSetupFunc(setup_func)

    def set_loop_func(self, loop_func) -> bool:
        return self.SetLoopFunc(loop_func)

    def set_end_func(self, end_func) -> bool:
        return self.SetEndFunc(end_func)

    def set_pre_render_func(self, pre_render_func) -> bool:
        return self.SetPreRenderFunc(pre_render_func)

    def set_post_render_func(self, post_render_func) -> bool:
        return self.SetPostRenderFunc(post_render_func)

    def add(self, component, xy) -> None:
        self.AddUIComponent(component, xy)

    def add_component(self, component, xy) -> None:
        self.AddUIComponent(component, xy)

    def clear_components(self) -> None:
        self.ClearUIComponents()

    def allow_exit(self, allow: bool) -> None:
        self.AllowExit(allow)

    def set_fps(self, fps: int) -> None:
        self.SetFPS(fps)
