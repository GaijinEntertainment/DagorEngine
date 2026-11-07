// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "names.h"
#include <device.h>

#include <osApiWrappers/dag_atomic_types.h>


namespace drv3d_dx12::debug
{

#if DX12_NAME_OBJECTS

static dag::AtomicInteger<uint64_t> serial{0};
uint64_t next_pool_object_serial() { return serial.fetch_add(1); }

void name_resource(ID3D12Resource *resource, eastl::string_view name)
{
  if (resource && !name.empty())
    get_device().nameResource(resource, name);
}

void name_object(ID3D12Object *object, eastl::string_view name)
{
  if (object && !name.empty())
    get_device().nameObject(object, name);
}

#endif

#if DX12_HAVE_GET_OBJECT_NAME

ObjectName get_object_name([[maybe_unused]] ID3D12Object *obj)
{
#if !_TARGET_XBOXONE
  if (obj)
  {
    wchar_t wcbuf[MAX_OBJECT_NAME_LENGTH];
    UINT cnt = sizeof(wcbuf);
    if (SUCCEEDED(obj->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf)) && cnt >= sizeof(wchar_t))
    {
      ObjectName result;
      result.length = (cnt / sizeof(wchar_t)) - 1;
      eastl::copy(wcbuf, wcbuf + result.length, result.storage);
      result.storage[result.length] = '\0';
      return result;
    }
  }
#endif //  #if !_TARGET_XBOXONE
  return {.storage = "\0", .length = 0};
}

#endif

} // namespace drv3d_dx12::debug
