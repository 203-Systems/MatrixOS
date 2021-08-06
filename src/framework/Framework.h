#ifndef __FRAMEWORK_H
#define __FRAMEWORK_H

// #include "Types.h"
#include "Color.h"
#include "Point.h"

struct handler
{
  void (*value)();

  // accept any free function pointer type
  template <typename base, typename... types>
  handler(base (*const value)(types...)) : value{reinterpret_cast<void (*)()>(value)} {}

  // implicit conversion (no need for custom `operator ()` and pointer operations
  typedef void (*value_t)();

  operator value_t&() { return this -> value; }
  operator value_t const&() const { return this -> value; }
};

#endif