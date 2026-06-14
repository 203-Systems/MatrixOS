import _MatrixOS_UIButton


class UIButton:
    def __init__(self):
        self.native = _MatrixOS_UIButton.UIButton()

    def close(self) -> bool:
        return self.native.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.native.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.native.SetEnableFunc(enable_func)

    def set_name(self, name: str) -> bool:
        return self.native.SetName(name)

    def set_color(self, color) -> bool:
        if hasattr(color, "native"):
            color = color.native
        return self.native.SetColor(color)

    def set_color_func(self, color_func) -> bool:
        return self.native.SetColorFunc(color_func)

    def set_size(self, dimension) -> bool:
        if hasattr(dimension, "native"):
            dimension = dimension.native
        return self.native.SetSize(dimension)

    def on_press(self, press_callback) -> bool:
        return self.native.OnPress(press_callback)

    def on_hold(self, hold_callback) -> bool:
        return self.native.OnHold(hold_callback)

    def raw(self):
        return self.native
