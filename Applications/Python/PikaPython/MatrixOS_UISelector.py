import _MatrixOS_UISelector

class UISelector(_MatrixOS_UISelector.UISelector):
    def close(self) -> bool:
        return self.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.SetEnableFunc(enable_func)

    def set_value_func(self, get_value_func) -> bool:
        return self.SetValueFunc(get_value_func)

    def set_color_func(self, color_func) -> bool:
        return self.SetColorFunc(color_func)

    def set_individual_color_func(self, individual_color_func) -> bool:
        return self.SetIndividualColorFunc(individual_color_func)

    def set_name_func(self, name_func) -> bool:
        return self.SetNameFunc(name_func)

    def set_lit_mode(self, lit_mode: int) -> bool:
        return self.SetLitMode(lit_mode)

    def set_dimension(self, dimension) -> bool:
        return self.SetDimension(dimension)

    def set_name(self, name: str) -> bool:
        return self.SetName(name)

    def set_count(self, count: int) -> bool:
        return self.SetCount(count)

    def set_direction(self, direction: int) -> bool:
        return self.SetDirection(direction)

    def set_color(self, color) -> bool:
        return self.SetColor(color)

    def on_change(self, change_callback) -> bool:
        return self.OnChange(change_callback)

class UISelectorDirection(_MatrixOS_UISelector.UISelectorDirection):
    pass

class UISelectorLitMode(_MatrixOS_UISelector.UISelectorLitMode):
    pass
