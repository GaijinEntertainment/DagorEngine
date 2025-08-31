// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shOpcodeFormat.h"
#include <debug/dag_debug.h>

namespace shaderopcode
{
bool formatUnittest()
{
  int opcode, o, p1, p2, p3, _o, _p1, _p2, _p3;
  debug_flush(true);

  for (o = 0; o < 150; o++)
  {
    debug("testing op0...");
    opcode = shaderopcode::makeOp0(o);
    _o = shaderopcode::getOp(opcode);
    if (_o != o)
    {
      debug("! op(%d) -> %d [%p]", o, _o, opcode);
      return false;
    }

    debug("testing op1...");
    for (p1 = -1; p1 < 0xFFFF; p1++)
    {
      opcode = shaderopcode::makeOp1(o, p1);
      _o = shaderopcode::getOp(opcode);
      _p1 = shaderopcode::getOp1p1s(opcode);
      if (_o != o || _p1 != p1)
      {
        debug("! op(%d,%d) -> %d %d [%p]", o, p1, _o, _p1, opcode);
        return false;
      }
    }

    debug("testing op2...");
    for (p1 = -1; p1 < 0x3FF; p1++)
    {
      for (p2 = -1; p2 < 0x3FFF; p2++)
      {
        opcode = shaderopcode::makeOp2(o, p1, p2);
        _o = shaderopcode::getOp(opcode);
        _p1 = shaderopcode::getOp2p1s(opcode);
        _p2 = shaderopcode::getOp2p2s(opcode);
        if (_o != o || _p1 != p1 || _p2 != p2)
        {
          debug("! op(%d,%d,%d) -> %d %d %d [%p]", o, p1, p2, _o, _p1, _p2, opcode);
          return false;
        }
      }
    }

    debug("testing op3...");
    for (p1 = -1; p1 < 0xFF; p1++)
    {
      for (p2 = -1; p2 < 0xFF; p2++)
      {
        for (p3 = -1; p3 < 0xFF; p3++)
        {
          opcode = shaderopcode::makeOp3(o, p1, p2, p3);
          _o = shaderopcode::getOp(opcode);
          _p1 = shaderopcode::getOp3p1s(opcode);
          _p2 = shaderopcode::getOp3p2s(opcode);
          _p3 = shaderopcode::getOp3p3s(opcode);
          if (_o != o || _p1 != p1 || _p2 != p2 || _p3 != p3)
          {
            debug("! op(%d,%d,%d,%d) -> %d %d %d %d [%p]", o, p1, p2, p3, _o, _p1, _p2, _p3, opcode);
            return false;
          }
        }
      }
    }
  }

  debug("testing data2...");
  for (p1 = -1; p1 < 0xFFFF; p1++)
  {
    for (p2 = -1; p2 < 0xFFFF; p2++)
    {
      opcode = shaderopcode::makeData2(p1, p2);
      _p1 = shaderopcode::getData2p1s(opcode);
      _p2 = shaderopcode::getData2p2s(opcode);
      if (_p1 != p1 || _p2 != p2)
      {
        debug("! data(%d,%d) -> %d %d [%p]", p1, p2, _p1, _p2, opcode);
        return false;
      }
    }
  }

  debug_flush(false);
  return true;
}
} // namespace shaderopcode
