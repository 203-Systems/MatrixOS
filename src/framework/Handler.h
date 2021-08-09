#pragma once

struct handler
{
  void (*value)();

  // accept any free function pointer type
  template <typename base, typename... types>
  handler(base (*const value)(types...)) : value{reinterpret_cast<void (*)()>(value)} {}

  template <typename base, typename target, typename... types>
  handler(base (*const value)(types...)) : value{reinterpret_cast<void (*)()>(value)} {}

  // implicit conversion (no need for custom `operator ()` and pointer operations
  typedef void (*value_t)();

  operator value_t&() { return this -> value; }
  operator value_t const&() const { return this -> value; }
};