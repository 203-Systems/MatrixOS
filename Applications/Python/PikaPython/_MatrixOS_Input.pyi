# MatrixOS Python Interface — Input
# Module-level functions for the new input-centric API.
#
# Nullable convention: PikaPython has no Optional[T].
# Functions that may return None use `-> any` (maps to Arg* in C,
# which carries runtime type information including None).
# The real return type is documented in each function's comment.

from MatrixOS_InputId import InputId
from MatrixOS_InputEvent import InputEvent
from MatrixOS_InputSnapshot import InputSnapshot
from MatrixOS_InputCluster import InputCluster
from MatrixOS_Point import Point

# Poll for the next input event.
# Returns InputEvent on success, None on timeout.
def GetEvent(timeout_ms: int) -> any: ...

# Get the current live state of an input from device hardware.
# Returns InputSnapshot on success, None if input not found.
def GetState(input_id: InputId) -> any: ...

# Get the grid coordinate of an input.
# Returns Point on success, None if input has no position.
def GetPosition(input_id: InputId) -> any: ...

# Find all inputs at a grid coordinate.
# Returns list of InputId (empty list if none found).
def GetInputsAt(point: Point) -> list: ...

# Discard all pending input events from the OS input buffer.
def ClearInputBuffer() -> None: ...

# Return the InputId for the function key.
def FunctionKey() -> InputId: ...

# Return list of all InputCluster objects.
def GetClusters() -> list: ...

# Return the primary grid cluster.
# Returns InputCluster on success, None if no grid cluster exists.
def GetPrimaryGridCluster() -> any: ...
