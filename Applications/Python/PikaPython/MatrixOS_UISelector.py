import _MatrixOS_UISelector


class UISelector:
    def __init__(self):
        self.native = _MatrixOS_UISelector.UISelector()

    def close(self) -> bool:
        return self.native.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.native.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.native.SetEnableFunc(enable_func)

    def set_value_func(self, get_value_func) -> bool:
        return self.native.SetValueFunc(get_value_func)

    def set_color_func(self, color_func) -> bool:
        return self.native.SetColorFunc(color_func)

    def set_individual_color_func(self, individual_color_func) -> bool:
        return self.native.SetIndividualColorFunc(individual_color_func)

    def set_name_func(self, name_func) -> bool:
        return self.native.SetNameFunc(name_func)

    def set_lit_mode(self, lit_mode: int) -> bool:
        return self.native.SetLitMode(lit_mode)

    def set_dimension(self, dimension) -> bool:
        if hasattr(dimension, "native"):
            dimension = dimension.native
        return self.native.SetDimension(dimension)

    def set_name(self, name: str) -> bool:
        return self.native.SetName(name)

    def set_count(self, count: int) -> bool:
        return self.native.SetCount(count)

    def set_direction(self, direction: int) -> bool:
        return self.native.SetDirection(direction)

    def set_color(self, color) -> bool:
        if hasattr(color, "native"):
            color = color.native
        return self.native.SetColor(color)

    def on_change(self, change_callback) -> bool:
        return self.native.OnChange(change_callback)

    def raw(self):
        return self.native


class UISelectorDirection:
    HORIZONTAL = 0
    VERTICAL = 1


class UISelectorLitMode:
    SELECTED = 0
    RANGE = 1
