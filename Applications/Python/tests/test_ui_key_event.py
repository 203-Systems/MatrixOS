import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeUI:
    def __init__(self):
        self.codes = []

    def PullInputCode(self):
        if not self.codes:
            return -1
        return self.codes.pop(0)


module = types.ModuleType("_MatrixOS_UI")
module.UI = NativeUI
sys.modules["_MatrixOS_UI"] = module

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

    assert ui.PullInput() is None


def test_pull_input_wraps_code_as_key_event():
    ui = MatrixOS_UI.UI()
    ui.codes.append(0x002A0201)

    event = ui.PullInput()

    assert event is not None
    assert event.ClusterId() == 1
    assert event.KeyState() == 2
    assert event.MemberId() == 42
    assert event.cluster_id() == 1
    assert event.key_state() == 2
    assert event.member_id() == 42
    assert event.is_pressed()
    assert not event.is_released()


def test_grid_key_event_decodes_xy_from_member_id():
    event = MatrixOS_UI.KeyEvent((26 << 16) | (2 << 8) | 1)

    assert event.ClusterId() == 1
    assert event.KeyState() == 2
    assert event.MemberId() == 26
    assert event.X() == 2
    assert event.Y() == 3
    assert event.x() == 2
    assert event.y() == 3


def test_non_grid_key_event_decodes_packed_xy_member_id():
    event = MatrixOS_UI.KeyEvent(0x03040202)

    assert event.ClusterId() == 2
    assert event.X() == 4
    assert event.Y() == 3


if __name__ == "__main__":
    test_pull_input_returns_none_for_empty_queue()
    test_pull_input_wraps_code_as_key_event()
    test_grid_key_event_decodes_xy_from_member_id()
    test_non_grid_key_event_decodes_packed_xy_member_id()
