// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globalData.h"
#include "context.h"
#include <daFx/dafx_global_data_desc.hlsli>

namespace dafx
{
bool init_global_values(GlobalData &dst)
{
  int sz = DAFX_GLOBAL_DATA_SIZE;
  dst.size = sz;

  bool v = true;
  v &= create_cpu_res(dst.cpuRes, DAFX_ELEM_STRIDE * sizeof(int), sz / DAFX_ELEM_STRIDE, "dafx_global_data");
  v &= create_gpu_cb_res(dst.gpuBuf, DAFX_ELEM_STRIDE * sizeof(int), sz / DAFX_ELEM_STRIDE, "dafx_global_data");

  if (v)
    dst.gpuBuf.setVarId(::get_shader_variable_id("dafx_global_data"));

  memset(dst.cpuRes.get(), 0, sz);

  return v;
}

static bool get_value_bind(Context &ctx, const char *name, size_t name_len, uint32_t name_hash, int size, ValueBind &v)
{
#ifdef _DEBUG_TAB_
  G_ASSERT(name_hash == DefaultOAHasher<false>().hash_data(name, name_len));
#endif

  unsigned nameId = fxSysNameMap.getNameId(name, name_len, name_hash);
  if (nameId >= ctx.binds.globalValues.size())
    return false;

  v = ctx.binds.globalValues[nameId];
  if (v == ValueBind{})
    return false;

  if (v.size != size)
  {
    logerr("dafx: size for global value:%s doesn't match (%d/%d)", name, v.size, size);
    return false;
  }

  G_ASSERT_RETURN(v.offset + v.size <= ctx.globalData.size, false);
  return true;
}

void set_global_value(Context &ctx, const char *name, size_t name_len, uint32_t name_hash, const void *data, int size)
{
  G_ASSERT_RETURN(name_len && data && size > 0, );

  ValueBind v;
  {
    OSSpinlockScopedLock lock{GlobalData::bindSpinLock};
    if (!get_value_bind(ctx, name, name_len, name_hash, size, v))
      return;
  }

  unsigned char *ptr = ctx.globalData.cpuRes.get();
  memcpy(ptr + v.offset, data, size);
}

void set_global_value_ext(ContextId cid, const char *name, size_t name_len, uint32_t name_hash, const void *data, int size)
{
  GET_CTX();
  set_global_value(ctx, name, name_len, name_hash, data, size);
}

bool get_global_value(Context &ctx, const eastl::string &name, void *data, int size)
{
  ValueBind v;
  if (!get_value_bind(ctx, name.c_str(), name.length(), wyhash_const((const uint8_t *)name.c_str(), name.length(), 1), size, v))
    return false;

  unsigned char *ptr = ctx.globalData.cpuRes.get();
  memcpy(data, ptr + v.offset, size);
  return true;
}

bool get_global_value(ContextId cid, const eastl::string &name, void *data, int size)
{
  GET_CTX_RET(false);
  return get_global_value(ctx, name, data, size);
}

void update_global_data(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_update_global_data);
  update_gpu_cb_buffer(ctx.globalData.gpuBuf.getBuf(), ctx.globalData.cpuRes.get(), ctx.globalData.size);
}

void flush_global_values(ContextId cid)
{
  GET_CTX();
  update_global_data(ctx);
}
} // namespace dafx
