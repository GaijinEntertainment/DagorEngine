// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_perfTimer.h>

enum
{
  SWRT_BUF_PAGE_SIZE = 65536,
  SWRT_BUF_PAGE_MASK = SWRT_BUF_PAGE_SIZE - 1
};

inline void ensure_buf_size_and_update(UniqueBufHolder &buf, const uint8_t *data, uint32_t size, const char *name)
{
  const uint32_t cSize = buf ? buf.getBuf()->getSize() : 0;
  const uint32_t nextSize = ((size + SWRT_BUF_PAGE_MASK) & ~SWRT_BUF_PAGE_MASK);
  if (cSize < size || cSize * 2 > nextSize)
  {
    buf.close();
    buf = dag::create_sbuffer(sizeof(uint32_t), nextSize / sizeof(uint32_t),
      SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, //|SBCF_BIND_UNORDERED
      0, name);
  }
  if (size)
  {
    const int64_t reft = profile_ref_ticks();
    buf.getBuf()->updateData(0, size, data, 0);
    debug("update %s in %dus", name, profile_time_usec(reft));
  }
}

inline void ensure_buf_size_and_update(UniqueBufHolder &buf, const Tab<uint8_t> &data, const char *name)
{
  ensure_buf_size_and_update(buf, data.begin(), data.size(), name);
}
