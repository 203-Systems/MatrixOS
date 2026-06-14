import _MatrixOS_NVS


def get_size(hash: int) -> int:
    return _MatrixOS_NVS.GetSize(hash)


def get(hash: int) -> bytes:
    return _MatrixOS_NVS.GetVariable(hash)


def set(hash: int, data: bytes) -> bool:
    return _MatrixOS_NVS.SetVariable(hash, data, len(data))


def delete(hash: int) -> bool:
    return _MatrixOS_NVS.DeleteVariable(hash)


def get_u8(hash: int, default: int = 0) -> int:
    data = get(hash)
    if data is None or len(data) < 1:
        return default
    return data[0]


def set_u8(hash: int, value: int) -> bool:
    return set(hash, bytes([value & 0xFF]))


def get_u16(hash: int, default: int = 0) -> int:
    data = get(hash)
    if data is None or len(data) < 2:
        return default
    return data[0] | (data[1] << 8)


def set_u16(hash: int, value: int) -> bool:
    return set(hash, bytes([value & 0xFF, (value >> 8) & 0xFF]))


def get_u32(hash: int, default: int = 0) -> int:
    data = get(hash)
    if data is None or len(data) < 4:
        return default
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)


def set_u32(hash: int, value: int) -> bool:
    return set(hash, bytes([
        value & 0xFF,
        (value >> 8) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 24) & 0xFF,
    ]))


def get_str(hash: int, default: str = "") -> str:
    data = get(hash)
    if data is None:
        return default
    return data.decode()


def set_str(hash: int, value: str) -> bool:
    return set(hash, value.encode())
