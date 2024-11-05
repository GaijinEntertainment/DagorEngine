// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <util/le2be.h>

static void check_uint32_t_size(void) { G_STATIC_ASSERT(sizeof(uint32_t) == 4); }

static inline uint32_t be2le32(uint32_t v) { return le2be32(v); }

static inline uint32_t local_ntohl(uint32_t v)
{
#if _TARGET_CPU_BE
  return v;
#else
  return le2be32(v);
#endif
}

static inline uint32_t local_htonl(uint32_t v) { return local_ntohl(v); }
