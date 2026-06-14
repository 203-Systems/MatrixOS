import _MatrixOS_InputId


class InputId:
    def __init__(self, cluster_id: int = 0, member_id: int = 0):
        self.native = _MatrixOS_InputId.InputId(cluster_id, member_id)

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def member_id(self) -> int:
        return self.native.MemberId()

    def cluster_name(self) -> str:
        import MatrixOS_Input as Input
        return Input.get_cluster_name(self.native.ClusterId())

    def is_function_key(self) -> bool:
        return self.native.ClusterId() == 0

    def raw(self):
        return self.native

    def __eq__(self, other) -> bool:
        if hasattr(other, "native"):
            other = other.native
        return self.native == other

    def __ne__(self, other) -> bool:
        return not self.__eq__(other)

    def __bool__(self) -> bool:
        return bool(self.native)


class InputIdView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputId.InputId()
        self.native = native

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def member_id(self) -> int:
        return self.native.MemberId()

    def cluster_name(self) -> str:
        import MatrixOS_Input as Input
        return Input.get_cluster_name(self.native.ClusterId())

    def is_function_key(self) -> bool:
        return self.native.ClusterId() == 0

    def raw(self):
        return self.native

    def __eq__(self, other) -> bool:
        if hasattr(other, "native"):
            other = other.native
        return self.native == other

    def __ne__(self, other) -> bool:
        return not self.__eq__(other)

    def __bool__(self) -> bool:
        return bool(self.native)
