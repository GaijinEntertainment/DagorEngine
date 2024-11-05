// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include "buffers.h"
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_spinlock.h>

namespace dafx
{
struct Context;
struct GlobalData
{
  GlobalData() = default;
  ~GlobalData() = default;
  GlobalData(const GlobalData &) = delete;
  Context &operator=(const Context &) = delete;
  Context &operator=(Context &&) = delete;

  GlobalData(GlobalData &&) = default;

  int size;
  CpuResourcePtr cpuRes;
  SbufferIDHolderWithVar gpuBuf;
  static inline OSSpinlock bindSpinLock;
};

bool init_global_values(GlobalData &dst);
void update_global_data(Context &ctx);
void set_global_value(Context &ctx, const char *name, size_t name_len, uint32_t name_hash, const void *data, int size);
template <unsigned N>
inline void set_global_value(Context &ctx, const char (&name)[N], const void *data, int size)
{
  uint32_t hash = wyhash((const uint8_t *)&name, N - 1, 1); // DefaultOAHasher<false>().hash_data(&name[0], N - 1))
  set_global_value(ctx, &name[0], N - 1, hash, data, size);
}
bool get_global_value(Context &ctx, const eastl::string &name, void *data, int size);
} // namespace dafx