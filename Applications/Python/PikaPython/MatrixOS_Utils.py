# Utility functions wrapper for MatrixOS
# Binding of OS/Framework/Utils/Hash.h and other utilities

import _MatrixOS_Utils

def StringHash(text: str) -> int:
    return _MatrixOS_Utils.StringHash(text)