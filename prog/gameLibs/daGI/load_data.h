// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

inline void load_data(void *bins, Sbuffer *buf, uint sz)
{
  void *data;
  if (buf->lock(0, 0, (void **)&data, VBLOCK_READONLY))
  {
    memcpy(bins, data, sz);
    buf->unlock();
  }
}
