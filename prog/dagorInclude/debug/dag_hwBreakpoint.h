//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if DAGOR_DBGLEVEL == 0
#error "Debug only feature. Do not use in release build!"
#endif

#include <supp/dag_define_KRNLIMP.h>

#include <EASTL/memory.h>
#include <EASTL/functional.h>


namespace dag::hwbrk
{
enum class Size : uint8_t
{
  _1 = 0b00,
  _2 = 0b01,
  _4 = 0b11,
  _8 = 0b10,
};

enum class Slot : uint8_t
{
  _0 = 0b0001,
  _1 = 0b0010,
  _2 = 0b0100,
  _3 = 0b1000,
};

enum class When : uint8_t
{
  EXECUTE = 0b00,
  WRITE = 0b01,
  ACCESS = 0b11,
};

KRNLIMP void set(const void *address, Size size, Slot slot, When when);
KRNLIMP void remove(Slot slot);
KRNLIMP void on_break(eastl::function<void(void *, Slot)> action);

template <class T>
void set(const T &data, Slot slot, When when)
{
  constexpr size_t typeSize = sizeof(T);
  static_assert(typeSize == 1 || typeSize == 2 || typeSize == 4 || typeSize == 8, "Unsupported size for data breakpoint");
  constexpr Size size = typeSize == 1 ? Size::_1 : (typeSize == 2 ? Size::_2 : (typeSize == 4 ? Size::_4 : Size::_8));
  Set(eastl::addressof(data), size, slot, when);
}
} // namespace dag::hwbrk

#include <supp/dag_undef_KRNLIMP.h>
