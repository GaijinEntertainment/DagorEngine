//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_KRNLIMP.h>

class String;

//! fills stack with current call stack addresses and returns number of stack frames;
//! no more than max_size addresses stored, the tail entry (if any space left) filled with 0xFFFFFFFF
//! upper skip_frames frames are skipped
KRNLIMP unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int skip_frames = 0);

//! fills stack with specified call stack start addresses and returns number of stack frames;
//! no more than max_size addresses stored, the tail entry (if any space left) filled with 0xFFFFFFFF
KRNLIMP unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size, const void *ctx_ptr);

//! returns pointer out_buf filled with decoded call stack
KRNLIMP const char *stackhlp_get_call_stack(char *out_buf, int out_buf_sz, const void *const *stack, unsigned max_size);

//! returns string filled with decoded call stack
KRNLIMP String stackhlp_get_call_stack_str(const void *const *stack, unsigned max_size);

KRNLIMP void *stackhlp_get_bp();

//! callstack of the specified thread.
void dump_thread_callstack(intptr_t thread_id);

//! callstacks of all the threads of the current process.
void dump_all_thread_callstacks();

namespace stackhelp
{
// Wraps inputs for call stack related functions to enable easier use
struct CallStackInfo
{
  void **stack = nullptr;
  unsigned stackSize = 0;

  CallStackInfo() = default;

  template <size_t N>
  CallStackInfo(void *(&s)[N]) : stack{s}, stackSize{N}
  {}
  CallStackInfo(void **s, unsigned l) : stack{s}, stackSize{l} {}
  CallStackInfo(const CallStackInfo &) = default;
};

inline unsigned fill_stack(CallStackInfo stack, int skip_frames = 0)
{
  return stackhlp_fill_stack(stack.stack, stack.stackSize, skip_frames);
}

inline unsigned fill_stack_exact(CallStackInfo stack, const void *ctx_ptr)
{
  return stackhlp_fill_stack_exact(stack.stack, stack.stackSize, ctx_ptr);
}

inline const char *get_call_stack(char *buf, unsigned max_buf, CallStackInfo stack)
{
  return stackhlp_get_call_stack(buf, max_buf, stack.stack, stack.stackSize);
}

// Simple wrapper for a array of void * with a size of N and a paired size.
// It has a type cast operator to make it easy to use in places where
// CallStackInfo is accepted.
// Also provides capture methods for easy capturing of the current call stack.
template <unsigned N>
struct CallStackCaptureStore
{
  void *store[N]{};
  unsigned storeSize = N;

  operator CallStackInfo() { return {store, storeSize}; }
  void capture(int skip_frames = 0) { storeSize = fill_stack(*this, skip_frames); }
  void capture(void *ctx_ptr) { storeSize = fill_stack_exact(*this, ctx_ptr); }
};

namespace ext
{
// Writes the given state of 'stack' as a text into 'buf'. Where 'max_buf' denotes the max
// length of that text. The returned value is the number of characters written to 'buf', excluding
// a mandatory null terminator.
using CallStackResolverCallback = unsigned (*)(char *buf, unsigned max_buf, CallStackInfo stack);

// A pair of a resolver callback pointer and written size. It is used as a return value by the
// 'CallStackCaptureCallback' function pointer. The resolver is supposed to be used afterwards
// to resolve the captured extended stack state into a text representation.
// The 'size' member represents the number of entries written by the 'CallStackCaptureCallback'
// function into the 'stack' parameter it was passed.
struct CallStackResolverCallbackAndSizePair
{
  CallStackResolverCallback resolver = nullptr;
  unsigned size = 0;
};

// Captures the current extended call stack state info 'stack'. 'context' is the paired execution context of the
// callback function.
// Must return a resolver callback pointer and the numbers of slots used in 'stack', as a CallStackResolverCallbackAndSizePair
// value.
using CallStackCaptureCallback = CallStackResolverCallbackAndSizePair (*)(CallStackInfo stack, void *context);

// Pairs a CallStackCaptureCallback with a context pointer. This is used by the extended call stack capture system.
struct CallStackContext
{
  using Context = void *;
  using Callback = CallStackCaptureCallback;

  Callback capture = nullptr;
  Context context = nullptr;

  CallStackResolverCallbackAndSizePair operator()(CallStackInfo stack) const
  {
    if (capture)
    {
      return capture(stack, context);
    }
    return {};
  }
};

// Sets thread local extended call stack context call back and returns previous one.
KRNLIMP CallStackContext set_extended_call_stack_capture_context(CallStackContext new_context);
// Invokes current set thread local extended call stack context call back.
KRNLIMP CallStackResolverCallbackAndSizePair capture_extended_call_stack(CallStackInfo stack);

// Wraps inputs for extended call stack related functions to enable easier use
struct CallStackInfo
{
  CallStackResolverCallback resolver = nullptr;
  void **stack = nullptr;
  unsigned stackSize = 0;

  CallStackInfo() = default;

  template <size_t N>
  CallStackInfo(CallStackResolverCallback r, void *(&s)[N]) : resolver{r}, stack{s}, stackSize{N}
  {}
  CallStackInfo(CallStackResolverCallback r, void **s, unsigned l) : resolver{r}, stack{s}, stackSize{l} {}
  CallStackInfo(CallStackResolverCallback r, CallStackInfo b) : resolver{r}, stack{b.stack}, stackSize{b.stackSize} {}
  CallStackInfo(CallStackResolverCallbackAndSizePair r, void **s) : resolver{r.resolver}, stack{s}, stackSize{r.size} {}
  CallStackInfo(CallStackResolverCallbackAndSizePair r, CallStackInfo b) : resolver{r.resolver}, stack{b.stack}, stackSize{r.size} {}
  CallStackInfo(const CallStackInfo &) = default;

  unsigned operator()(char *buf, unsigned max_buf) const
  {
    if (resolver)
    {
      return resolver(buf, max_buf, {stack, stackSize});
    }
    return 0;
  }
};

// Simple wrapper for a array of void * with a size of N, a paired size and resolver callback.
// It has a type cast operator to make it easy to use in places where ext::CallStackInfo is accepted.
// Also provides capture methods for easy capturing of the current call stack.
struct CallStackCaptureStore
{
  static constexpr size_t call_stack_max_size = 32;
  CallStackResolverCallback resolver = nullptr;
  void *store[call_stack_max_size]{};
  unsigned storeSize = call_stack_max_size;

  CallStackCaptureStore() = default;
  CallStackCaptureStore(const CallStackCaptureStore &) = default;
  operator CallStackInfo() { return {resolver, store, storeSize}; }
  CallStackCaptureStore &operator=(const CallStackResolverCallbackAndSizePair &p)
  {
    resolver = p.resolver;
    storeSize = p.size;
    return *this;
  }
  CallStackCaptureStore &operator=(const CallStackCaptureStore &) = default;
  void capture() { *this = capture_extended_call_stack({store}); }
};

// Simple scoped helper to set the given callback context as the current one and preserves the previous set values.
// On destruction it will restore previous values.
// Allows easy chaining with 'invokePrev' method.
class ScopedCallStackContext
{
  CallStackContext prev;

public:
  ScopedCallStackContext(CallStackContext ctx) : prev{set_extended_call_stack_capture_context(ctx)} {}
  ScopedCallStackContext(CallStackContext::Callback cap, CallStackContext::Context ctx) :
    prev{set_extended_call_stack_capture_context({cap, ctx})}
  {}
  ~ScopedCallStackContext() { set_extended_call_stack_capture_context(prev); }
  CallStackResolverCallbackAndSizePair invokePrev(stackhelp::CallStackInfo stack) const { return prev(stack); }
};

// Generates a text representation of both the call stack state 'stack' and the extended call stack state 'ext_stack'.
// This representation is written to 'buf', does not exceed 'max_buf' and is null terminated.
// The return value is the start of the generated text.
KRNLIMP const char *get_call_stack(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack,
  stackhelp::ext::CallStackInfo ext_stack);
// Similar to 'get_call_stack' this function generates a text representation of both the call stack state 'stack' and the extended call
// stack state 'ext_stack'.
// The generated text is returned as a 'String' value.
KRNLIMP String get_call_stack_str(stackhelp::CallStackInfo stack, stackhelp::ext::CallStackInfo ext_stack);
} // namespace ext
} // namespace stackhelp

#include <supp/dag_undef_KRNLIMP.h>
