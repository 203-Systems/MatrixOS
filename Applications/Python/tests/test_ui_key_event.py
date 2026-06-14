import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeUI:
    def __init__(self):
        self.codes = []
        self.calls = []

    def PullInputCode(self):
        if not self.codes:
            return -1
        return self.codes.pop(0)

    def Start(self):
        self.calls.append(("start",))

    def Exit(self):
        self.calls.append(("exit",))

    def Close(self):
        self.calls.append(("close",))
        return True

    def SetName(self, name):
        self.calls.append(("name", name))

    def SetColor(self, color):
        self.calls.append(("color", color))

    def SetSetupFunc(self, setup_func):
        self.calls.append(("setup", setup_func))
        return True

    def SetLoopFunc(self, loop_func):
        self.calls.append(("loop", loop_func))
        return True

    def SetEndFunc(self, end_func):
        self.calls.append(("end", end_func))
        return True

    def SetPreRenderFunc(self, pre_render_func):
        self.calls.append(("pre", pre_render_func))
        return True

    def SetPostRenderFunc(self, post_render_func):
        self.calls.append(("post", post_render_func))
        return True

    def AddUIComponent(self, component, xy):
        self.calls.append(("add", component, xy))

    def ClearUIComponents(self):
        self.calls.append(("clear",))

    def AllowExit(self, allow):
        self.calls.append(("allow_exit", allow))

    def SetFPS(self, fps):
        self.calls.append(("fps", fps))


module = types.ModuleType("_MatrixOS_UI")
module.UI = NativeUI
sys.modules["_MatrixOS_UI"] = module

class NativeInputId:
    def __init__(self, cluster=0, member=0):
        self.cluster = cluster
        self.member = member

    def ClusterId(self):
        return self.cluster

    def MemberId(self):
        return self.member

input_id_module = types.ModuleType("_MatrixOS_InputId")
input_id_module.InputId = NativeInputId
sys.modules["_MatrixOS_InputId"] = input_id_module

for name in ("MatrixOS_UIComponent", "MatrixOS_UIButton", "MatrixOS_UISelector", "MatrixOS_UI4pxNumber"):
    stub = types.ModuleType(name)
    stub.UIComponent = object
    stub.UIButton = object
    stub.UISelector = object
    stub.UI4pxNumber = object
    sys.modules[name] = stub

import MatrixOS_UI


def test_pull_input_returns_none_for_empty_queue():
    ui = MatrixOS_UI.UI()

    assert ui.pull_input() is None


def test_pull_input_wraps_code_as_key_event():
    ui = MatrixOS_UI.UI()
    ui.native.codes.append(0x002A0201)

    event = ui.pull_input()

    assert event is not None
    assert event.cluster_id() == 1
    assert event.key_state() == 2
    assert event.member_id() == 42
    assert event.id().cluster_id() == 1
    assert event.id().member_id() == 42
    assert event.is_pressed()
    assert event.is_grid()
    assert not event.is_released()
    assert event.as_input_id().member_id() == 42
    assert not event.is_hold()
    assert not event.is_aftertouch()
    assert not event.is_function_key()


def test_grid_key_event_decodes_xy_from_member_id():
    event = MatrixOS_UI.KeyEvent((26 << 16) | (2 << 8) | 1)

    assert event.cluster_id() == 1
    assert event.key_state() == 2
    assert event.member_id() == 26
    assert event.x() == 2
    assert event.y() == 3


def test_non_grid_key_event_decodes_packed_xy_member_id():
    event = MatrixOS_UI.KeyEvent(0x03040202)

    assert event.cluster_id() == 2
    assert event.x() == 4
    assert event.y() == 3


def test_key_event_state_helpers():
    assert MatrixOS_UI.KeyEvent((0 << 16) | (3 << 8) | 0).is_hold()
    assert MatrixOS_UI.KeyEvent((0 << 16) | (4 << 8) | 0).is_aftertouch()
    assert MatrixOS_UI.KeyEvent((0 << 16) | (5 << 8) | 0).is_released()
    assert MatrixOS_UI.KeyEvent((0 << 16) | (2 << 8) | 0).is_function_key()


def test_ui_pythonic_aliases_call_native_methods():
    ui = MatrixOS_UI.UI()

    ui.start()
    ui.exit()
    assert ui.close()
    ui.set_name("Demo")
    ui.set_color("white")
    assert ui.set_setup_func(lambda: None)
    assert ui.set_loop_func(lambda: None)
    assert ui.set_end_func(lambda: None)
    assert ui.set_pre_render_func(lambda: None)
    assert ui.set_post_render_func(lambda: None)
    ui.add("button", (1, 2))
    ui.add_component("button2", (3, 4))
    ui.clear_components()
    ui.allow_exit(False)
    ui.set_fps(60)

    assert ui.native.calls[0] == ("start",)
    assert ui.native.calls[1] == ("exit",)
    assert ui.native.calls[10][0] == "add"
    assert ui.native.calls[-1] == ("fps", 60)


if __name__ == "__main__":
    test_pull_input_returns_none_for_empty_queue()
    test_pull_input_wraps_code_as_key_event()
    test_grid_key_event_decodes_xy_from_member_id()
    test_non_grid_key_event_decodes_packed_xy_member_id()
    test_key_event_state_helpers()
    test_ui_pythonic_aliases_call_native_methods()
