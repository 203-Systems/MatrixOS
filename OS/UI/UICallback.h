#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

// Lightweight type-erased callable that replaces std::function in the UI layer.
// Uses named template function-pointer trampolines instead of std::function's
// complex type-erasure vtable. This produces stable, predictable WASM indirect
// call table entries and avoids libc++ std::function issues on Emscripten.
//
// Same design pattern as the Application_Info factory/destructor fix.

template <typename Sig>
class UICallback;

template <typename R, typename... Args>
class UICallback<R(Args...)> {
  using InvokeFn = R (*)(void*, Args...);
  using DestroyFn = void (*)(void*);

  void* state_ = nullptr;
  InvokeFn invoke_ = nullptr;
  DestroyFn destroy_ = nullptr;

  // Named template trampolines — each instantiation has a stable address
  // in the WASM indirect call table at link time.
  template <typename Stored>
  static R Invoke(void* s, Args... args) {
    return (*static_cast<Stored*>(s))(static_cast<Args&&>(args)...);
  }

  template <typename Stored>
  static void Destroy(void* s) {
    delete static_cast<Stored*>(s);
  }

public:
  UICallback() = default;

  // Construct from any callable (lambda, functor, etc.)
  template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, UICallback>>>
  UICallback(F&& callable) {
    using Stored = std::decay_t<F>;
    state_ = new Stored(static_cast<F&&>(callable));
    invoke_ = &Invoke<Stored>;
    destroy_ = &Destroy<Stored>;
  }

  ~UICallback() { Reset(); }

  // Move-only
  UICallback(const UICallback&) = delete;
  UICallback& operator=(const UICallback&) = delete;

  UICallback(UICallback&& o) noexcept
      : state_(o.state_), invoke_(o.invoke_), destroy_(o.destroy_) {
    o.state_ = nullptr;
    o.invoke_ = nullptr;
    o.destroy_ = nullptr;
  }

  UICallback& operator=(UICallback&& o) noexcept {
    if (this != &o)
    {
      Reset();
      state_ = o.state_;
      invoke_ = o.invoke_;
      destroy_ = o.destroy_;
      o.state_ = nullptr;
      o.invoke_ = nullptr;
      o.destroy_ = nullptr;
    }
    return *this;
  }

  explicit operator bool() const { return invoke_ != nullptr; }

  R operator()(Args... args) const {
    return invoke_(state_, static_cast<Args&&>(args)...);
  }

  void Reset() {
    if (destroy_ && state_)
    {
      destroy_(state_);
    }
    state_ = nullptr;
    invoke_ = nullptr;
    destroy_ = nullptr;
  }
};
