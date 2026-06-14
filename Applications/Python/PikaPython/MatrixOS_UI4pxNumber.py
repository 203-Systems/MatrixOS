import _MatrixOS_UI4pxNumber


class UI4pxNumber:
    def __init__(self):
        self.native = _MatrixOS_UI4pxNumber.UI4pxNumber()

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

    def set_alternative_color(self, alternative_color) -> bool:
        if hasattr(alternative_color, "native"):
            alternative_color = alternative_color.native
        return self.native.SetAlternativeColor(alternative_color)

    def set_digits(self, digits: int) -> bool:
        return self.native.SetDigits(digits)

    def set_spacing(self, spacing: int) -> bool:
        return self.native.SetSpacing(spacing)

    def set_value_func(self, get_value_func) -> bool:
        return self.native.SetValueFunc(get_value_func)

    def set_color_func(self, color_func) -> bool:
        return self.native.SetColorFunc(color_func)

    def raw(self):
        return self.native
