// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/algorithm.h>
#include <util/dag_globDef.h>

#include "driver.h"


namespace drv3d_dx12
{

inline wchar_t *lazyToWchar(const char *str, wchar_t *buf, size_t max_len)
{
  auto ed = str + max_len - 1;
  auto at = buf;
  for (; *str && str != ed; ++str, ++at)
    *at = *str;
  *at = L'\0';
  return buf;
}

// NOTE: This is intended for debug only, this is possibly slow, so use with care!
template <size_t N>
inline char *get_resource_name(ID3D12Object *res, char (&cbuf)[N])
{
#if !_TARGET_XBOXONE
  wchar_t wcbuf[N];
  UINT cnt = sizeof(wcbuf);
  res->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
  eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
  cbuf[min<uint32_t>(cnt, N - 1)] = '\0';
#else
  G_UNUSED(res);
  cbuf[0] = 0;
#endif
  return cbuf;
}

#if DX12_DOES_SET_DEBUG_NAMES
#define DX12_SET_DEBUG_OBJ_NAME(obj, name) obj->SetName(name)
#else
#define DX12_SET_DEBUG_OBJ_NAME(obj, name)
#endif

} // namespace drv3d_dx12
