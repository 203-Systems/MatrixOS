import _MatrixOS_UI4pxNumber

class UI4pxNumber(_MatrixOS_UI4pxNumber.UI4pxNumber):
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

    def set_alternative_color(self, alternative_color) -> bool:
        return self.SetAlternativeColor(alternative_color)

    def set_digits(self, digits: int) -> bool:
        return self.SetDigits(digits)

    def set_spacing(self, spacing: int) -> bool:
        return self.SetSpacing(spacing)

    def set_value_func(self, get_value_func) -> bool:
        return self.SetValueFunc(get_value_func)

    def set_color_func(self, color_func) -> bool:
        return self.SetColorFunc(color_func)
