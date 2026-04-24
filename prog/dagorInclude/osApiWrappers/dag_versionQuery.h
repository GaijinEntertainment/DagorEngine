//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <compare>
#include <EASTL/bit.h>
#include <EASTL/fixed_string.h>
#include <EASTL/optional.h>
#include <EASTL/string_view.h>
#include <inttypes.h>

#include <supp/dag_define_KRNLIMP.h>

struct TwoComponentVersion
{
  uint32_t major;
  uint32_t minor;

  constexpr auto operator<=>(const TwoComponentVersion &) const = default;
};

struct DriverVersion
{
  uint16_t product; // WDDM version x 10 -> 27 driver for WDDM 2.7
  uint16_t major;   // Unknown, may be related to target windows Version / protocol?, all WDDM 2.7 drivers have this at 20 or 21
  uint16_t minor;   // Vendor specific major
  uint16_t build;   // Vendor specific minor

  constexpr auto operator<=>(const DriverVersion &) const = default;

  /*constexpr*/ uint64_t raw() const { return eastl::bit_cast<uint64_t>(*this); }

  constexpr static DriverVersion fromRegistryValue(uint64_t value)
  {
    return {
      .product = uint16_t((value & 0xFFFF000000000000) >> 48),
      .major = uint16_t((value & 0x0000FFFF00000000) >> 32),
      .minor = uint16_t((value & 0x00000000FFFF0000) >> 16),
      .build = uint16_t((value & 0x000000000000FFFF)),
    };
  }

  eastl::fixed_string<char, 24> toString() const
  {
    return {
      eastl::fixed_string<char, 24>::CtorSprintf{}, "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16, product, major, minor, build};
  }
};

struct LibraryVersion
{
  uint16_t major;
  uint16_t minor;
  uint16_t build;
  uint16_t revision;

  constexpr auto operator<=>(const LibraryVersion &) const = default;

  eastl::fixed_string<char, 24> toString() const
  {
    return {
      eastl::fixed_string<char, 24>::CtorSprintf{}, "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16, major, minor, build, revision};
  }
};

#ifndef _MINWINDEF_
struct HINSTANCE__;
using HINSTANCE = HINSTANCE__ *;
using HMODULE = HINSTANCE;
#endif

// On Windows will return the dynamic link library's version number.
KRNLIMP eastl::optional<LibraryVersion> get_library_version(eastl::string_view library);
// On Windows will return the dynamic link library's version number.
KRNLIMP eastl::optional<LibraryVersion> get_library_version(eastl::wstring_view library);
// On Windows will return the dynamic link library's version number.
KRNLIMP eastl::optional<LibraryVersion> get_library_version(HMODULE module);

extern KRNLIMP TwoComponentVersion (*to_nvidia_version)(DriverVersion version);

#include <supp/dag_undef_KRNLIMP.h>
