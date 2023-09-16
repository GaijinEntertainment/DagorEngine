//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <soundSystem/events.h>
#include <soundSystem/streams.h>

namespace sndsys
{
template <bool Abandoner, typename Handle = sndsys::EventHandle>
struct UniqueHandleEx
{
  UniqueHandleEx() = default;
  explicit UniqueHandleEx(Handle handle) : handle(handle) {}
  UniqueHandleEx(const UniqueHandleEx &) = delete;
  UniqueHandleEx(UniqueHandleEx &&other)
  {
    reset(other.handle);
    other.handle.reset();
  }
  UniqueHandleEx &operator=(const UniqueHandleEx &) = delete;
  UniqueHandleEx &operator=(UniqueHandleEx &&other)
  {
    reset(other.handle);
    other.handle.reset();
    return *this;
  }
  ~UniqueHandleEx() { reset(); }

  inline void abandonHandle() { sndsys::abandon(handle); }
  inline void releaseHandle() { sndsys::release(handle); }
  inline Handle get() const { return handle; }
  bool operator==(const UniqueHandleEx &other) const { return handle == other.handle; }
  bool operator!=(const UniqueHandleEx &other) const { return handle != other.handle; }

  inline void reset(Handle value = {})
  {
    if (handle && handle != value)
    {
      if /*constexpr*/ (isAbandoner)
        abandonHandle();
      else
        releaseHandle();
    }
    handle = value;
  }

  inline Handle release()
  {
    Handle ret = handle;
    handle = {};
    return ret;
  }

private:
  static constexpr bool isAbandoner = Abandoner;
  Handle handle;
};

typedef UniqueHandleEx<false> UniqueHandle;
} // namespace sndsys
