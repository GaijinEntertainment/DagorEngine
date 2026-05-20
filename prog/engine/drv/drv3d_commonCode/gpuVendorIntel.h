// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>


namespace gpu
{

constexpr bool IsPreXe(uint16_t deviceId)
{
  // ---- Gen11 -- Ice Lake, Jasper Lake, Elkhart Lake ----
  // Ice Lake:        0x8A50..0x8A7F
  // Elkhart/Jasper:  0x4E51..0x4E71
  if ((deviceId & 0xFF80) == 0x8A00)
    return true;
  if ((deviceId & 0xFF80) == 0x4E00)
    return true;

  // ---- Gen9.5 -- Coffee Lake, Comet Lake, Whiskey Lake, Amber Lake ----
  // Coffee Lake:     0x3E90..0x3EA9, 0x3EA0..0x3EAF, 0x87C0, 0x87CA
  // Comet Lake:      0x9B21, 0x9B41, 0x9BA0..0x9BFF, 0x9BC0..0x9BCA
  if ((deviceId & 0xFF00) == 0x3E00)
    return true;
  if ((deviceId & 0xFF00) == 0x9B00)
    return true;
  if (deviceId == 0x87C0 || deviceId == 0x87CA)
    return true;

  // ---- Gen9 -- Skylake, Kaby Lake, Apollo Lake, Gemini Lake ----
  // Skylake:         0x1902..0x193B
  // Kaby Lake:       0x5902..0x593B
  // Apollo Lake:     0x5A84, 0x5A85, 0x0A84
  // Gemini Lake:     0x3184, 0x3185
  if ((deviceId & 0xFF00) == 0x1900)
    return true;
  if ((deviceId & 0xFF00) == 0x5900)
    return true;
  if (deviceId == 0x5A84 || deviceId == 0x5A85)
    return true;
  if (deviceId == 0x0A84)
    return true;
  if (deviceId == 0x3184 || deviceId == 0x3185)
    return true;

  // ---- Gen8 -- Broadwell, Cherry Trail ----
  // Broadwell:       0x1602..0x162E
  // Cherry Trail:    0x22B0..0x22B3
  if ((deviceId & 0xFF00) == 0x1600)
    return true;
  if ((deviceId & 0xFF00) == 0x2200)
    return true;

  // ---- Gen7.5 -- Haswell ----
  // Haswell uses 0x0402..0x0D2E sparsely across 0x04xx, 0x0A/C/Dxx
  if ((deviceId & 0xFF00) == 0x0400)
    return true;
  if ((deviceId & 0xFF00) == 0x0A00)
    return true;
  if ((deviceId & 0xFF00) == 0x0C00)
    return true;
  if ((deviceId & 0xFF00) == 0x0D00)
    return true;

  // ---- Gen7 -- Ivy Bridge, Bay Trail, Valley View ----
  // Ivy Bridge:      0x0152, 0x0156, 0x015A, 0x015E,
  //                  0x0162, 0x0166, 0x016A,
  //                  0x0172, 0x0176, 0x017A
  // Bay Trail:       0x0F30..0x0F33
  if ((deviceId & 0xFFF0) == 0x0150)
    return true;
  if ((deviceId & 0xFFF0) == 0x0160)
    return true;
  if ((deviceId & 0xFFF0) == 0x0170)
    return true;
  if ((deviceId & 0xFFF0) == 0x0F30)
    return true;

  // ---- Gen6 -- Sandy Bridge ----
  // 0x0102, 0x0106, 0x010A, 0x0112, 0x0116, 0x011A, 0x0122, 0x0126, 0x012A
  if ((deviceId & 0xFFF0) == 0x0100)
    return true;
  if ((deviceId & 0xFFF0) == 0x0110)
    return true;
  if ((deviceId & 0xFFF0) == 0x0120)
    return true;

  // ---- Gen5 -- Ironlake (Clarkdale, Arrandale) ----
  // 0x0042, 0x0046
  if ((deviceId & 0xFFF0) == 0x0040)
    return true;

  // ---- Gen4 -- GMA 4500 / X4500 / GMA X3000 series ----
  // G45/G43/Q45:     0x2E02, 0x2E12, 0x2E22, 0x2E32, 0x2E42, 0x2E92
  // GM45/GS45:       0x2A42
  // 965/X3000:       0x2A02, 0x2A12, 0x29A2, 0x2992, 0x2982, 0x2972
  if ((deviceId & 0xFF00) == 0x2A00)
    return true;
  if ((deviceId & 0xFF00) == 0x2E00)
    return true;
  if ((deviceId & 0xFF00) == 0x2900)
    return true;

  // Anything that falls through is either Xe-class (DG1 0x49xx,
  // Tiger Lake 0x9Axx, Rocket Lake 0x4C8x, Alder Lake 0x46xx,
  // Raptor Lake 0xA7xx, DG2/Arc 0x56xx, Meteor Lake 0x7Dxx,
  // Battlemage 0xE2xx, Lunar Lake 0x64xx) or unknown.
  return false;
}

} // namespace gpu
