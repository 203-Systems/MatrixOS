# MatrixOS Python Interface — Input
# Module-level functions for the new input-centric API.

from MatrixOS_InputId import InputId
from MatrixOS_InputEvent import InputEvent
from MatrixOS_InputSnapshot import InputSnapshot
from MatrixOS_InputCluster import InputCluster
from MatrixOS_Point import Point

# Poll for the next input event. Returns InputEvent or None on timeout.
def GetEvent(timeout_ms: int) -> any: ...

# Get the current snapshot state of an input. Returns InputSnapshot or None.
def GetState(input_id: InputId) -> any: ...

# Get the grid coordinate of an input. Returns Point or None.
def GetPosition(input_id: InputId) -> any: ...

# Find all inputs at a grid coordinate. Returns list of InputId.
def GetInputsAt(point: Point) -> list: ...

# Discard all queued input events.
def ClearQueue() -> None: ...

# Discard all input state (pressure, hold, etc.).
def ClearState() -> None: ...

# Return the InputId for the function key.
def FunctionKey() -> InputId: ...

# Return list of all input clusters.
def GetClusters() -> list: ...

# Return the primary grid cluster, or None.
def GetPrimaryGridCluster() -> any: ...
