// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

namespace shaderopcode
{
// make 32-bit opcode
__forceinline uint32_t makeOp0(uint8_t op) { return op; }

__forceinline uint32_t makeOp1(uint8_t op, int p1)
{
  G_ASSERTF(p1 >= -1 && p1 < 0xFFFF, "p1 = %d", p1);

  p1 &= 0xFFFF;
  return op | (((uint32_t)p1) << 16);
}

__forceinline uint32_t makeOpStageSlot(uint8_t op, uint32_t stage, uint32_t slot, uint32_t ofs)
{
  G_ASSERTF(stage <= 0x7, "stage = %d", stage);
  G_ASSERTF(slot <= 0x7F, "slot = %d", slot);
  G_ASSERTF(ofs <= 0x3FFF, "ofs = %d", ofs);

  stage &= 0x7;
  slot &= 0x7F;
  ofs &= 0x3FFF;
  return op | (stage << 8) | (slot << 11) | (ofs << 18);
}

__forceinline uint32_t makeOp2(uint8_t op, int p1, int p2)
{
  G_ASSERTF(p1 >= -1 && p1 < 0x3FF, "p1 = %d", p1);
  G_ASSERTF(p2 >= -1 && p2 < 0x3FFF, "p2 = %d", p2);

  p1 &= 0x3FF;
  p2 &= 0x3FFF;
  return op | (((uint32_t)p1) << 8) | (((uint32_t)p2) << 18);
}

__forceinline uint32_t makeOp2_8_16(uint8_t op, int p1, int p2)
{
  G_ASSERTF(p1 >= -1 && p1 < 0xFF, "p1 = %d", p1);
  G_ASSERTF(p2 >= -1 && p2 < 0xFFFF, "p2 = %d", p2);

  p1 &= 0xFF;
  p2 &= 0xFFFF;
  return op | (((uint32_t)p1) << 8) | (((uint32_t)p2) << 16);
}

__forceinline uint32_t makeOp3(uint8_t op, int p1, int p2, int p3)
{
  G_ASSERTF(p1 >= -1 && p1 < 0xFF, "p1 = %d", p1);
  G_ASSERTF(p2 >= -1 && p2 < 0xFF, "p2 = %d", p2);
  G_ASSERTF(p3 >= -1 && p3 < 0xFF, "p3 = %d", p3);

  p1 &= 0xFF;
  p2 &= 0xFF;
  p3 &= 0xFF;
  return op | (((uint32_t)p1) << 8) | (((uint32_t)p2) << 16) | (((uint32_t)p3) << 24);
}


// replace (patch) 32-bit opcode parameters
__forceinline uint32_t patchOp2p2(uint32_t opcode, int new_p2)
{
  G_ASSERTF(new_p2 >= -1 && new_p2 < 0x3FFF, "new_p2 = %d", new_p2);

  new_p2 &= 0x3FFF;
  return (opcode & 0x0003FFFF) | (((uint32_t)new_p2) << 18);
}

__forceinline uint32_t patchOp2p1_8(uint32_t opcode, int new_p1)
{
  G_ASSERTF(new_p1 >= -1 && new_p1 < 0xFF, "new_p1 = %d", new_p1);

  new_p1 &= 0xFF;
  return (opcode & 0xFFFF00FF) | (((uint32_t)new_p1) << 8);
}

// make 32-bit data
__forceinline uint32_t makeData2(int p1, int p2)
{
  G_ASSERTF(p1 >= -1 && p1 < 0xFFFF, "p1 = %d", p1);
  G_ASSERTF(p2 >= -1 && p2 < 0xFFFF, "p2 = %d", p2);

  p1 &= 0xFFFF;
  p2 &= 0xFFFF;
  return p1 | (((uint32_t)p2) << 16);
}

// make 32-bit data
__forceinline uint32_t makeData4(int p1, int p2, int p3, int p4)
{
  G_ASSERTF(p1 >= -1 && p1 < 0xFF, "p1 = %d", p1);
  G_ASSERTF(p2 >= -1 && p2 < 0xFF, "p2 = %d", p2);
  G_ASSERTF(p3 >= -1 && p3 < 0xFF, "p3 = %d", p3);
  G_ASSERTF(p4 >= -1 && p4 < 0xFF, "p4 = %d", p4);

  p1 &= 0xFF;
  p2 &= 0xFF;
  p3 &= 0xFF;
  p4 &= 0xFF;
  return p1 | (((uint32_t)p2) << 8) | (((uint32_t)p3) << 16) | (((uint32_t)p4) << 24);
}

// get components from 32-bit opcode
__forceinline uint32_t getOp(uint32_t op) { return op & 0xFF; }

__forceinline uint32_t getOpStageSlot_Stage(uint32_t op) { return (op >> 8) & 0x7; }
__forceinline uint32_t getOpStageSlot_Slot(uint32_t op) { return (op >> 11) & 0x7F; }
__forceinline uint32_t getOpStageSlot_Reg(uint32_t op) { return (op >> 18) & 0x3FFF; }
__forceinline uint32_t getOp1p1(uint32_t op) { return (op >> 16) & 0xFFFF; }
__forceinline uint32_t getOp2p1(uint32_t op) { return (op >> 8) & 0x3FF; }
__forceinline uint32_t getOp2p2(uint32_t op) { return (op >> 18) & 0x3FFF; }
__forceinline uint32_t getOp3p1(uint32_t op) { return (op >> 8) & 0xFF; }
__forceinline uint32_t getOp3p2(uint32_t op) { return (op >> 16) & 0xFF; }
__forceinline uint32_t getOp3p3(uint32_t op) { return (op >> 24) & 0xFF; }
__forceinline uint32_t getOp2p1_8(uint32_t op) { return (op >> 8) & 0xFF; }
__forceinline uint32_t getOp2p2_16(uint32_t op) { return (op >> 16) & 0xFFFF; }

struct TwoWords
{
  uint16_t lo = 0;
  uint16_t hi = 0;
};
struct FourBytes
{
  uint8_t b1 = 0;
  uint8_t b2 = 0;
  uint8_t b3 = 0;
  uint8_t b4 = 0;
};

__forceinline TwoWords unpackData2(uint32_t op) { return {uint16_t(op & 0xFFFF), uint16_t(op >> 16)}; }
__forceinline FourBytes unpackData4(uint32_t op)
{
  return {uint8_t(op & 0xFF), uint8_t((op >> 8) & 0xFF), uint8_t((op >> 16) & 0xFF), uint8_t((op >> 24) & 0xFF)};
}

__forceinline int getOp1p1s(uint32_t op)
{
  uint32_t p = getOp1p1(op);
  return p == 0xFFFF ? -1 : int(p);
}
__forceinline int getOp2p1s(uint32_t op)
{
  uint32_t p = getOp2p1(op);
  return p == 0x3FF ? -1 : int(p);
}
__forceinline int getOp2p2s(uint32_t op)
{
  uint32_t p = getOp2p2(op);
  return p == 0x3FFF ? -1 : int(p);
}
__forceinline int getOp3p1s(uint32_t op)
{
  uint32_t p = getOp3p1(op);
  return p == 0xFF ? -1 : int(p);
}
__forceinline int getOp3p2s(uint32_t op)
{
  uint32_t p = getOp3p2(op);
  return p == 0xFF ? -1 : int(p);
}
__forceinline int getOp3p3s(uint32_t op)
{
  uint32_t p = getOp3p3(op);
  return p == 0xFF ? -1 : int(p);
}

// get components from 32-bit data
__forceinline uint32_t getData2p1(uint32_t d) { return d & 0xFFFF; }
__forceinline uint32_t getData2p2(uint32_t d) { return (d >> 16) & 0xFFFF; }
__forceinline uint32_t getData4p1(uint32_t d) { return d & 0xFF; }
__forceinline uint32_t getData4p2(uint32_t d) { return (d >> 8) & 0xFF; }
__forceinline uint32_t getData4p3(uint32_t d) { return (d >> 16) & 0xFF; }
__forceinline uint32_t getData4p4(uint32_t d) { return (d >> 24) & 0xFF; }
__forceinline int getData2p1s(uint32_t d)
{
  int p = getData2p1(d);
  return p == 0xFFFF ? -1 : int(p);
}
__forceinline int getData2p2s(uint32_t d)
{
  int p = getData2p2(d);
  return p == 0xFFFF ? -1 : int(p);
}
} // namespace shaderopcode
