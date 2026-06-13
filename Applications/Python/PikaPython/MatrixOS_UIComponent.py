import _MatrixOS_UIComponent

class UIComponent(_MatrixOS_UIComponent.UIComponent):
    def close(self) -> bool:
        return self.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.SetEnableFunc(enable_func)
