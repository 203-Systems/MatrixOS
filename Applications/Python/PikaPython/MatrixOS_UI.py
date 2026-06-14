import _MatrixOS_UI

from MatrixOS_UIComponent import UIComponent
from MatrixOS_UIButton import UIButton
from MatrixOS_UISelector import UISelector
from MatrixOS_UI4pxNumber import UI4pxNumber
from MatrixOS_InputId import InputId

Component = UIComponent
Button = UIButton
Selector = UISelector
Number = UI4pxNumber


def _native(value):
    if hasattr(value, "native"):
        return value.native
    return value


class KeyEvent:
    def __init__(self, code: int):
        self.code = code

    def cluster_id(self) -> int:
        return self.code & 0xFF

    def key_state(self) -> int:
        return (self.code >> 8) & 0xFF

    def member_id(self) -> int:
        return (self.code >> 16) & 0xFFFF

    def id(self):
        return InputId(self.cluster_id(), self.member_id())

    def as_input_id(self):
        return self.id()

    def x(self) -> int:
        if self.cluster_id() == 1:
            return self.member_id() % 8
        return self.member_id() & 0xFF

    def y(self) -> int:
        if self.cluster_id() == 1:
            return self.member_id() // 8
        return (self.member_id() >> 8) & 0xFF

    def is_pressed(self) -> bool:
        return self.key_state() == 2

    def is_hold(self) -> bool:
        return self.key_state() == 3

    def is_aftertouch(self) -> bool:
        return self.key_state() == 4

    def is_released(self) -> bool:
        return self.key_state() == 5

    def is_function_key(self) -> bool:
        return self.cluster_id() == 0

    def is_grid(self) -> bool:
        return self.cluster_id() == 1


class UI:
    Component = UIComponent
    Button = UIButton
    Selector = UISelector
    Number = UI4pxNumber

    def __init__(self, *args):
        self.native = _MatrixOS_UI.UI(*args)

    def pull_input(self) -> any:
        code = self.native.PullInputCode()
        if code < 0:
            return None
        return KeyEvent(code)

    def start(self) -> None:
        self.native.Start()

    def exit(self) -> None:
        self.native.Exit()

    def close(self) -> bool:
        return self.native.Close()

    def set_name(self, name: str) -> None:
        self.native.SetName(name)

    def set_color(self, color) -> None:
        self.native.SetColor(_native(color))

    def set_setup_func(self, setup_func) -> bool:
        return self.native.SetSetupFunc(setup_func)

    def set_loop_func(self, loop_func) -> bool:
        return self.native.SetLoopFunc(loop_func)

    def set_end_func(self, end_func) -> bool:
        return self.native.SetEndFunc(end_func)

    def set_pre_render_func(self, pre_render_func) -> bool:
        return self.native.SetPreRenderFunc(pre_render_func)

    def set_post_render_func(self, post_render_func) -> bool:
        return self.native.SetPostRenderFunc(post_render_func)

    def add(self, component, xy) -> None:
        self.native.AddUIComponent(_native(component), _native(xy))

    def add_component(self, component, xy) -> None:
        self.add(component, xy)

    def clear_components(self) -> None:
        self.native.ClearUIComponents()

    def allow_exit(self, allow: bool) -> None:
        self.native.AllowExit(allow)

    def set_fps(self, fps: int) -> None:
        self.native.SetFPS(fps)

    def raw(self):
        return self.native
