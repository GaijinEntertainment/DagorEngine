// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <driver.h>

#include <debug/dag_log.h>

#include <EASTL/span.h>
#include <EASTL/string_view.h>


namespace drv3d_dx12::debug::gpu_capture
{
class NoTool
{
public:
  void configure() { logdbg("DX12: ...no frame capturing tool is active..."); }
  constexpr void beginCapture(const wchar_t *) {}
  constexpr void endCapture() {}
  constexpr void onPresent() {}
  constexpr void captureFrames(const wchar_t *, int) {}
  constexpr void beginEvent(D3DGraphicsCommandList *, eastl::span<const char>) {}
  constexpr void endEvent(D3DGraphicsCommandList *) {}
  constexpr void marker(D3DGraphicsCommandList *, eastl::span<const char>) {}
  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; };
  constexpr void nameResource(ID3D12Resource *, eastl::string_view) {}
  constexpr void nameResource(ID3D12Resource *, eastl::wstring_view) {}
  constexpr void nameObject(ID3D12Object *, eastl::string_view) {}
  constexpr void nameObject(ID3D12Object *, eastl::wstring_view) {}
};
} // namespace drv3d_dx12::debug::gpu_capture
