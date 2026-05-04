// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <driver.h>

#include <EASTL/span.h>
#include <EASTL/string_view.h>


namespace drv3d_dx12::debug::gpu_capture
{
class GenericNamingTool
{
public:
  void nameResource(ID3D12Resource *, eastl::string_view);
  void nameResource(ID3D12Resource *, eastl::wstring_view);
  void nameObject(ID3D12Object *, eastl::string_view);
  void nameObject(ID3D12Object *, eastl::wstring_view);
};

class GenericEventTool
{
public:
  void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>);
  void endEvent(D3DGraphicsCommandList *);
  void marker(D3DGraphicsCommandList *, eastl::span<const char>);
};
} // namespace drv3d_dx12::debug::gpu_capture
