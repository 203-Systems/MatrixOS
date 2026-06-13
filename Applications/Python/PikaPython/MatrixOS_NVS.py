import _MatrixOS_NVS

def GetSize(hash: int) -> int:
    return _MatrixOS_NVS.GetSize(hash)

def get_size(hash: int) -> int:
    return GetSize(hash)

def GetVariable(hash: int) -> bytes:
    return _MatrixOS_NVS.GetVariable(hash)

def get(hash: int) -> bytes:
    return GetVariable(hash)

def SetVariable(hash: int, data: bytes) -> bool:
    return _MatrixOS_NVS.SetVariable(hash, data, len(data))

def set(hash: int, data: bytes) -> bool:
    return SetVariable(hash, data)

def DeleteVariable(hash: int) -> bool:
    return _MatrixOS_NVS.DeleteVariable(hash)

def delete(hash: int) -> bool:
    return DeleteVariable(hash)
