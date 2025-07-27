// by jeremie miller - 2015-2017
// public domain UNLICENSE, contributions/improvements welcome via github at https://github.com/quartzjer/cb0r

/*
 * Copyright (c) 2022 Felix Gohla, Konrad Hanff, Tobias Kantusch,
 *                    Quentin Kuth, Felix Roth. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include "cb0r.h"

#if defined(__AVR__)
  // AVR is little endian.
  #define htobe16(x) __builtin_bswap16(x)
  #define htobe32(x) __builtin_bswap32(x)
  #define htobe64(x) __builtin_bswap64(x)
#elif defined(__ZEPHYR__)
    #include <zephyr/sys/byteorder.h>
    #define htobe16(x) sys_cpu_to_be16(x)
    #define htobe32(x) sys_cpu_to_be32(x)
    #define htobe64(x) sys_cpu_to_be64(x)
#elif defined(__APPLE__)
  // copied from https://gist.github.com/yinyin/2027912
  #include <libkern/OSByteOrder.h>
  #define htobe16(x) OSSwapHostToBigInt16(x)
  #define htole16(x) OSSwapHostToLittleInt16(x)
  #define be16toh(x) OSSwapBigToHostInt16(x)
  #define le16toh(x) OSSwapLittleToHostInt16(x)
  #define htobe32(x) OSSwapHostToBigInt32(x)
  #define htole32(x) OSSwapHostToLittleInt32(x)
  #define be32toh(x) OSSwapBigToHostInt32(x)
  #define le32toh(x) OSSwapLittleToHostInt32(x)
  #define htobe64(x) OSSwapHostToBigInt64(x)
  #define htole64(x) OSSwapHostToLittleInt64(x)
  #define be64toh(x) OSSwapBigToHostInt64(x)
  #define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(__has_include) && __has_include(<endian.h>) // Linux
    #include <endian.h>
#else
  #include "stm32f1xx_hal.h" // Include your specific STM32 HAL header here

  // Define endianness conversions for STM32
  #define htobe16(x) __builtin_bswap16(x)
  #define htobe32(x) __builtin_bswap32(x)
  #define htobe64(x) __builtin_bswap64(x)

  #define be16toh(x) __builtin_bswap16(x)
  #define be32toh(x) __builtin_bswap32(x)
  #define be64toh(x) __builtin_bswap64(x)
// #else
//     #error Unsupported architecture
#endif

// unhelpful legacy GCC warning noise for syntax used in cb0r()
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winitializer-overrides"
#pragma GCC diagnostic ignored "-Woverride-init"

// start at bin, returns end pointer (== stop if complete), either skips items or extracts result of current item
uint8_t *cb0r(uint8_t *start, uint8_t *stop, uint32_t skip, cb0r_t result)
{
  // type byte is fully unrolled for structure only
  static const void *go[] RODATA_SEGMENT_CONSTANT = 
  {
    [0x00 ... 0xff] = &&l_ebad,

    // first 5 bits
    [0x00 ... 0x17] = &&l_int,

    // Major type 1 CB0R_INT
    [0x18] = &&l_int1, [0x19] = &&l_int2,[0x1a] = &&l_int4, [0x1b] = &&l_int8,
    [0x20 ... 0x37] = &&l_int,

    // Major type 2 CB0R_NEG
    [0x38] = &&l_int1, [0x39] = &&l_int2,[0x3a] = &&l_int4, [0x3b] = &&l_int8,
    [0x40 ... 0x57] = &&l_byte,

    // Major type 3 CB0R_BYTE
    [0x58] = &&l_byte1, [0x59] = &&l_byte2,[0x5a] = &&l_byte4, [0x5b] = &&l_ebig,
    [0x5f] = &&l_until,

    // Major type 4 CB0R_UTF8
    [0x60 ... 0x77] = &&l_byte,
    [0x78] = &&l_byte1, [0x79] = &&l_byte2,[0x7a] = &&l_byte4, [0x7b] = &&l_ebig,
    [0x7f] = &&l_until,

    // Major type 5 CB0R_ARRAY
    [0x80 ... 0x97] = &&l_array,
    [0x98] = &&l_array1, [0x99] = &&l_array2,[0x9a] = &&l_array4, [0x9b] = &&l_ebig,
    [0x9f] = &&l_until,

    // Major type 6 CB0R_MAP
    [0xa0 ... 0xb7] = &&l_array,
    [0xb8] = &&l_array1, [0xb9] = &&l_array2,[0xba] = &&l_array4, [0xbb] = &&l_ebig,
    [0xbf] = &&l_until,

    // Major type 7 CB0R_TAG
    [0xc0 ... 0xd7] = &&l_tag,
    [0xd8] = &&l_tag1, [0xd9] = &&l_tag2,[0xda] = &&l_tag4, [0xdb] = &&l_tag8,

    // Major type 8 CB0R_SIMPLE / CB0R_FLOAT
    [0xe0 ... 0xf7] = &&l_int,
    [0xf8] = &&l_int1, [0xf9] = &&l_int2,[0xfa] = &&l_int4, [0xfb] = &&l_int8,
    [0xff] = &&l_break
  };

  uint8_t *end = start + 1;
  if(end > stop) {
    if(result) result->type = CB0R_ERR;
    return stop;
  }

  cb0r_e type = CB0R_ERR;
  uint8_t size = 0;
  uint32_t count = 0;

  goto *go[*start];

  // all types using integer structure
  l_int8:
    end += 4;
  l_int4:
    end += 2;
  l_int2:
    end += 1;
  l_int1:
    end += 1;
  l_int: 
    goto l_finish;

  // bytes and string structures
  l_byte4:
    size = 2;
    end += (uint32_t)(start[1]) << 24;
    end += (uint32_t)(start[2]) << 16;
  l_byte2:
    size += 1;
    end += (uint32_t)(start[size]) << 8;
  l_byte1:
    size += 1;
    end += start[size] + size;
    goto l_finish;
  l_byte: 
    end += (start[0] & 0x1f);
    goto l_finish;

  // array and map structures
  l_array4:
    size = 2;
    count += (uint32_t)(start[1]) << 24;
    count += (uint32_t)(start[2]) << 16;
  l_array2:
    size += 1;
    count += (uint32_t)(start[size]) << 8;
  l_array1:
    size += 1;
    count += start[size];
    goto l_skip;
  l_array: 
    count = (start[0] & 0x1f);
    goto l_skip;

  // skip fixed count of items in an array/map 
  l_skip:
    if(count) {
      // double map for actual count
      if(start[0] & 0x20) count <<= 1;
      end = cb0r(start+size+1,stop,count-1,NULL);
    }else{
      end += size;
    }
    goto l_finish;

  // cross between l_int and l_array
  l_tag8:
    size = 4;
  l_tag4:
    size += 2;
  l_tag2:
    size += 1;
  l_tag1:
    size += 1;
  l_tag: 
    // tag is like an array of 1, just grabs next item
    end = cb0r(start+size+1,stop,0,NULL);
    goto l_finish;

  // indefinite length wrapper
  l_until:
    count = CB0R_STREAM;
    end = cb0r(start+1,stop,count,NULL);
    goto l_finish;

  l_break: {
    if(skip == CB0R_STREAM) return end;
    goto l_eparse;
  }

  l_ebad:
    type = CB0R_EBAD;
    goto l_fail;

  l_eparse:
    type = CB0R_EPARSE;
    goto l_fail;

  l_ebig:
    type = CB0R_EBIG;
    goto l_fail;

  l_fail: // all errors
    skip = 0;

  l_finish: // only first 7 types
    type = (start[0] >> 5);

  // done done, extract value if result requested
  if(!skip)
  {
    if(!result) return end;
    result->start = start;
    result->end = end;
    result->type = type;
    result->value = 0;
    switch(type)
    {
      case CB0R_INT:
      case CB0R_NEG: 
        size = end - (start + 1);
      case CB0R_TAG: {
        switch(size)
        {
          case 8:
            result->value |= (uint64_t)(start[size - 7]) << 56;
            result->value |= (uint64_t)(start[size - 6]) << 48;
            result->value |= (uint64_t)(start[size - 5]) << 40;
            result->value |= (uint64_t)(start[size - 4]) << 32;
          case 4:
            result->value |= (uint32_t)(start[size - 3]) << 24;
            result->value |= (uint32_t)(start[size - 2]) << 16;
          case 2:
            result->value |= (uint32_t)(start[size - 1]) << 8;
          case 1:
            result->value |= start[size];
            break;
          case 0:
            result->value = start[0] & 0x1f;
            if(type == CB0R_TAG) switch(result->value)
            {
              case 0: result->type = CB0R_DATETIME; break;
              case 1: result->type = CB0R_EPOCH; break;
              case 2: result->type = CB0R_BIGNUM; break;
              case 3: result->type = CB0R_BIGNEG; break;
              case 4: result->type = CB0R_FRACTION; break;
              case 5: result->type = CB0R_BIGFLOAT; break;
              case 21: result->type = CB0R_BASE64URL; break;
              case 22: result->type = CB0R_BASE64; break;
              case 23: result->type = CB0R_HEX; break;
              case 24: result->type = CB0R_DATA; break;
            }
        }
      } break;

      case CB0R_BYTE:
      case CB0R_UTF8: {
        if(count == CB0R_STREAM) result->count = count;
        else result->length = end - (start + 1);
      } break;

      case CB0R_ARRAY:
      case CB0R_MAP: {
        result->count = count;
      } break;

      case CB0R_SIMPLE: {
        result->value = (start[0] & 0x1f);
        switch(result->value)
        {
          case 20: result->type = CB0R_FALSE; break;
          case 21: result->type = CB0R_TRUE; break;
          case 22: result->type = CB0R_NULL; break;
          case 23: result->type = CB0R_UNDEF; break;
          case 24:
            if(start[1] >= 32) result->value = start[1];
            else result->type = CB0R_EBAD;
            break;
          case 25:
            result->type = CB0R_FLOAT;
            result->length = 2;
            break;
          case 26:
            result->type = CB0R_FLOAT;
            result->length = 4;
            break;
          case 27:
            result->type = CB0R_FLOAT;
            result->length = 8;
            break;
        }
      } break;

      default: {
        if(result->type < CB0R_ERR) result->type = CB0R_ERR;
      }
    }
    result->header = size + 1;
    return end;
  }

  // max means indefinite mode skip
  if(skip != CB0R_STREAM) skip--;
  else if(result) result->count++;

  // tail recurse while skipping to not stack bloat
  return cb0r(end, stop, skip, result);
}

// safer high-level wrapper to read raw CBOR
bool cb0r_read(uint8_t *in, uint32_t len, cb0r_t result)
{
  if(!in || !len || !result) return false;
  cb0r(in, in+len, 0, result);
  if(result->type >= CB0R_ERR) return false;
  return true;
}

// fetch a given item from an array (or map), 0 index
bool cb0r_get(cb0r_t array, uint32_t index, cb0r_t result)
{
  if(!array || !result) return false;
  if(array->type != CB0R_ARRAY && array->type != CB0R_MAP) return false;
  cb0r(array->start+array->header, array->end, index, result);
  if(result->type >= CB0R_ERR) return false;
  return true;
}

// get the value of a given key from a map, number/bytes only used for some types
bool cb0r_find(cb0r_t map, cb0r_e type, uint64_t number, uint8_t *bytes, cb0r_t result)
{
  if(!map || !result) return false;
  if(map->type != CB0R_MAP) return false;

  for(uint32_t i = 0; i < map->length * 2; i += 2) {
    if(!cb0r_get(map, i, result)) return false;
    if(result->type != type) continue;
    // either number compare or number+bytes compare
    switch(type) {
      case CB0R_INT:
      case CB0R_NEG:
      case CB0R_SIMPLE:
      case CB0R_DATETIME:
      case CB0R_EPOCH:
      case CB0R_BIGNUM:
      case CB0R_BIGNEG:
      case CB0R_FRACTION:
      case CB0R_BIGFLOAT:
      case CB0R_BASE64URL:
      case CB0R_BASE64:
      case CB0R_HEX:
      case CB0R_DATA:
      case CB0R_FALSE:
      case CB0R_TRUE:
      case CB0R_NULL:
      case CB0R_UNDEF:
        if(number == result->value) break;
        continue;
      case CB0R_BYTE:
      case CB0R_UTF8:
      case CB0R_FLOAT:
        // compare value by given length
        if(number == result->length && memcmp(bytes, result->start + result->header, number) == 0) break;
        continue;
      case CB0R_MAP:
      case CB0R_ARRAY:
      case CB0R_TAG:
        // compare value by parsed byte length
        if(number == (uint64_t)(result->end - (result->start + result->header)) && memcmp(bytes, result->start + result->header, number) == 0) break;
        continue;
      default:
        continue;
    }

    // key matched
    if(!cb0r_get(map, i+1, result)) return false;
    return true;
  }

  return false;
}

// simple wrappers to return start and length
uint8_t *cb0r_value(cb0r_t data)
{
  if(!data) return NULL;
  return data->start + data->header;
}

uint32_t cb0r_vlen(cb0r_t data)
{
  if(!data) return 0;
  return data->end - cb0r_value(data);
}

uint8_t cb0r_write(uint8_t *out, cb0r_e type, uint64_t number)
{
  if(type >= CB0R_ERR) return 0;

  // built-in types
  switch(type) {
    case CB0R_DATETIME: number = 0; break;
    case CB0R_EPOCH: number = 1; break;
    case CB0R_BIGNUM: number = 2; break;
    case CB0R_BIGNEG: number = 3; break;
    case CB0R_FRACTION: number = 4; break;
    case CB0R_BIGFLOAT: number = 5; break;
    case CB0R_BASE64URL: number = 21; break;
    case CB0R_BASE64: number = 22; break;
    case CB0R_HEX: number = 23; break;
    case CB0R_DATA: number = 24; break;
    case CB0R_FALSE: type = CB0R_SIMPLE; number = 20; break;
    case CB0R_TRUE: type = CB0R_SIMPLE; number = 21; break;
    case CB0R_NULL: type = CB0R_SIMPLE; number = 22; break;
    case CB0R_UNDEF: type = CB0R_SIMPLE; number = 23; break;
    case CB0R_FLOAT: { // incoming number is size of float
      if(number == 2) number = 25;
      else if(number == 4) number = 26;
      else if(number == 8) number = 27;
      else return 0;
    }
    default:;
  }

  out[0] = type << 5;
  if(number <= 23) {
    out[0] |= number;
    return 1;
  }
  if(number >= UINT32_MAX) {
    out[0] |= 27;
    number = htobe64(number);
    memcpy(out + 1, &number, 8);
    return 9;
  }
  if(number > UINT16_MAX) {
    out[0] |= 26;
    number = htobe32(number);
    memcpy(out + 1, &number, 4);
    return 5;
  }
  if(number >= UINT8_MAX) {
    out[0] |= 25;
    number = htobe16(number);
    memcpy(out + 1, &number, 2);
    return 3;
  }
  out[0] |= 24;
  out[1] = number;
  return 2;
}
