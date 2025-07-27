# cb0r - Minimal Zero-Footprint [CBOR](http://cbor.io) Decoder in C

A one-pass minimal malloc-free walk of the raw bytes using a fast jump table to determine state transitions and decode a result.

* public domain, single [cb0r.c](src/cb0r.c) file with one ~200 line function
* easy to use, just returns a given type, value, and offset locations in the bytes
* no dependencies, designed for embedded use
* requires no memory/malloc or copying, uses only what is passed in
* safely stops on anything invalid

For other CBOR implementations check [http://cbor.io/impls.html](http://cbor.io/impls.html).

## Usage

`uint8_t *cb0r(uint8_t *start, uint8_t *stop, uint32_t skip, cb0r_t result)`

* processes one item by default into result (if provided)
* returns end pointer to byte following the item value (may be == stop for complete, > stop for item value length overflow)
* can very efficiently skip any number of leading items of any type
* see [header](src/cb0r.h) for result structure and types
* fully supports maps, arrays, and indefinite length types
