import _MatrixOS_NVS

def GetSize(hash: int) -> int:
    return _MatrixOS_NVS.GetSize(hash)

def GetVariable(hash: int, length: int) -> bytes:
    return _MatrixOS_NVS.GetVariable(hash, length)

def SetVariable(hash: int, data: bytes, length: int) -> bool:
    return _MatrixOS_NVS.SetVariable(hash, data, length)

def DeleteVariable(hash: int) -> bool:
    return _MatrixOS_NVS.DeleteVariable(hash)