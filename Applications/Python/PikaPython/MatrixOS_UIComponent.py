import _MatrixOS_UIComponent


class UIComponent:
    def __init__(self):
        self.native = _MatrixOS_UIComponent.UIComponent()

    def close(self) -> bool:
        return self.native.Close()

    def set_enabled(self, enabled: bool) -> bool:
        return self.native.SetEnabled(enabled)

    def set_enable_func(self, enable_func) -> bool:
        return self.native.SetEnableFunc(enable_func)

    def raw(self):
        return self.native
