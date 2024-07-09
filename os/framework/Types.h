#pragma once
#include <cstring>
#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <stack>
#include <utility>
#include <string>
#include <tuple>

using std::array;
using std::deque;
using std::forward_list;
using std::list;
using std::map;
using std::multimap;
using std::multiset;
using std::priority_queue;
using std::queue;
using std::set;
using std::stack;
using std::unordered_map;
using std::unordered_multimap;
using std::unordered_multiset;
using std::unordered_set;
using std::vector;
using std::pair;
using std::string;
using std::tuple;


#ifdef __cplusplus
extern "C"
#endif

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