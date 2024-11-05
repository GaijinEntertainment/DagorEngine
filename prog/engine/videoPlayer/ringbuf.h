// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_debug.h>
#include <util/dag_globDef.h>

template <class BufType, int MAX_DEPTH>
struct GenRingBuffer
{
  BufType buf[MAX_DEPTH];
  int rdPos, wrPos, used, depth;

  inline void clearPos()
  {
    rdPos = 0;
    wrPos = 0;
    used = 0;
  }
  inline void clear(int d)
  {
    clearPos();
    depth = d;
    memset(buf, 0, sizeof(buf));
  }

  inline BufType &getWr() { return buf[wrPos]; }
  inline BufType &getRd() { return buf[rdPos]; }

  inline BufType &getRd(int ofs)
  {
    G_ASSERTF(ofs >= 0 && ofs < used, "ofs=%d, used=%d", ofs, used);

    int pos = rdPos + ofs;
    if (pos >= depth)
      pos -= depth;
    return buf[pos];
  }

  inline bool incWr()
  {
    wrPos++;
    if (wrPos >= depth)
      wrPos = 0;
    if (wrPos == rdPos)
    {
      DEBUG_CTX("overrun: rdPos=%d wrPos=%d used=%d depth=%d", rdPos, wrPos, used, depth);
      return false;
    }
    used++;
    return true;
  }
  inline bool incRd()
  {
    if (!used)
    {
      DEBUG_CTX("underrun: rdPos=%d wrPos=%d used=%d depth=%d", rdPos, wrPos, used, depth);
      return false;
    }
    rdPos++;
    if (rdPos == depth)
      rdPos = 0;
    used--;
    return true;
  }

  inline int bufLeft() { return depth - used; }
  inline int getDepth() { return depth; }
};
