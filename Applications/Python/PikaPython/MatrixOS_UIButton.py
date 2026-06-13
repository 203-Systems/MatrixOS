import _MatrixOS_UIButton

class UIButton(_MatrixOS_UIButton.UIButton):
    def close(self) -> bool:
        return self.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.SetEnableFunc(enable_func)

    def set_name(self, name: str) -> bool:
        return self.SetName(name)

    def set_color(self, color) -> bool:
        return self.SetColor(color)

    def set_color_func(self, color_func) -> bool:
        return self.SetColorFunc(color_func)

    def set_size(self, dimension) -> bool:
        return self.SetSize(dimension)

    def on_press(self, press_callback) -> bool:
        return self.OnPress(press_callback)

    def on_hold(self, hold_callback) -> bool:
        return self.OnHold(hold_callback)
