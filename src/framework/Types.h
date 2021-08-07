#pragma once

#include <assert.h>
#ifdef __cplusplus
#include <algorithm>
#endif
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" 
#endif
void _HandleAssertion(const char*, int, const char*);

#define _STR(x) #x
#ifndef _ASSERT
#define  _ASSERT(e) {if(!(e)) _HandleAssertion(__FILE__, __LINE__, _STR(e)); }
#endif
#define ToWord(a, b) (ui16)(((a)<<8)|(b))
#define ToDword(a, b, c, d) (ui32)((ToWord(d, c)<<16)|ToWord(b,a))
//#define NULL nullptr
#ifdef __APPLE__
#define min(a,b) std::min(a,b)
#define max(a,b) std::max(a,b)
#else
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#define COUNT(arr) (int)(sizeof(arr)/sizeof(arr[0]))
#ifdef __APPLE__
#define NATIVEENUM uint32_t
#elif WIN32
#define NATIVEENUM uint32_t
#else
#define NATIVEENUM uint8_t
#endif
#define NATIVEPTR uintptr_t

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef const uint8_t uc8;
typedef const uint16_t uc16;
typedef const uint32_t uc32;
typedef const uint64_t uc64;

typedef volatile uint8_t vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

typedef volatile const uint8_t vuc8;
typedef volatile const uint16_t vuc16;
typedef volatile const uint32_t vuc32;
typedef volatile const uint64_t vuc64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef const int8_t sc8;
typedef const int16_t sc16;
typedef const int32_t sc32;
typedef const int64_t sc64;

typedef volatile int8_t vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;
typedef volatile int64_t vs64;

typedef volatile const int8_t vsc8;
typedef volatile const int16_t vsc16;
typedef volatile const int32_t vsc32;
typedef volatile const int64_t vsc64;
typedef void* PVOID;

#ifdef WIN32
typedef int BOOL;
#else
typedef bool BOOL;
#endif
typedef float FLOAT;
typedef int INT;
typedef const char * PCSTR;
typedef char * PSTR;
//typedef char CHAR;
#ifndef WIN32
typedef uint32_t UINT;
#endif

#define EVERY(ms) static long dwTick##__LINE__ = 0; bool bDo##__LINE__ = MatrixOS::SYS::GetTick() - dwTick##__LINE__ > ms; if (bDo##__LINE__) dwTick##__LINE__ = MatrixOS::SYS::GetTick(); if (bDo##__LINE__)

class fract16
{
    public:
    uint16_t value;
    fract16(uint32_t value)
    {
        this->value = (uint16_t)value;
    }

    fract16(uint16_t value, uint8_t bits)
    {
        this->value = value << (16 - bits);
    }

    // uint8_t to14bit(){return value >> 2;}
    // uint8_t to12bit(){return value >> 4;}
    // uint8_t to10bit(){return value >> 6;}
    uint8_t to8bit(){return value >> 8;}
    uint8_t to7bit(){return value >> 9;}
    
    operator bool() {return value > 0;}
    operator uint8_t() {return to8bit();}
    operator uint16_t() {return value;}
    operator uint32_t() {return value;}

    bool operator <(int value) {return this->value < value;}
    bool operator >(int value) {return this->value > value;}

    bool operator ==(int value) {return this->value == value;}
};