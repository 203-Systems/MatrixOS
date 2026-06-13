import _MatrixOS_InputId

class InputId(_MatrixOS_InputId.InputId):
    def cluster_id(self) -> int:
        return self.ClusterId()

    def member_id(self) -> int:
        return self.MemberId()

    def input_id(self) -> int:
        return self.MemberId()

    def is_function_key(self) -> bool:
        return self.ClusterId() == 0

    def raw(self):
        return self

class InputIdView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputId.InputId()
        self.native = native

    def ClusterId(self) -> int:
        return self.native.ClusterId()

    def cluster_id(self) -> int:
        return self.ClusterId()

    def MemberId(self) -> int:
        return self.native.MemberId()

    def member_id(self) -> int:
        return self.MemberId()

    def input_id(self) -> int:
        return self.MemberId()

    def is_function_key(self) -> bool:
        return self.ClusterId() == 0

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

def input_id(cluster_id: int, member_id: int):
    return InputId(cluster_id, member_id)
