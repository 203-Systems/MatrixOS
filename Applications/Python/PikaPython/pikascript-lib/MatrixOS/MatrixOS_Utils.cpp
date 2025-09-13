#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // StringHash function - generates a 32-bit hash from a string
    int _MatrixOS_Utils_StringHash(PikaObj *self, char* text) {
        if (!text) return 0;
        
        // Convert to std::string and compute hash
        std::string str(text);
        uint32_t hash = StringHash(str);
        
        // Return as signed int (Python doesn't have unsigned types)
        // The bit pattern is preserved, just interpreted differently
        return (int)hash;
    }
}