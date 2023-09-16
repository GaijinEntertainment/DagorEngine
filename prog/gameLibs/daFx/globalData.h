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

  GlobalData(GlobalData &&) : size(0), cpuRes(), gpuBuf(), bindSpinLock() {}

  int size;
  CpuResourcePtr cpuRes;
  SbufferIDHolderWithVar gpuBuf;
  OSSpinlock bindSpinLock;
};

bool init_global_values(GlobalData &dst);
void update_global_data(Context &ctx);
void set_global_value(Context &ctx, const char *name, size_t name_len, const void *data, int size);
bool get_global_value(Context &ctx, const eastl::string &name, void *data, int size);
} // namespace dafx