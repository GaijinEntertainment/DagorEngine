// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "generic_tools.h"
#include <d3d12_utils.h>

#if USE_PIX
#include <pix3.h> // not self-contained, has to be iuncluded after driver.h
#else
#include <pix.h> // From Windows SDK
#define WINPIX_EVENT_ANSI_VERSION DirectX::Detail::PIX_EVENT_ANSI_VERSION
#endif


namespace drv3d_dx12::debug::gpu_capture
{
void GenericEventTool::beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->BeginEvent(WINPIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void GenericEventTool::endEvent(D3DGraphicsCommandList *cmd) { cmd->EndEvent(); }

void GenericEventTool::marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
{
  cmd->SetMarker(WINPIX_EVENT_ANSI_VERSION, text.data(), text.size());
}

void GenericNamingTool::nameResource(ID3D12Resource *resource, eastl::string_view name) { set_object_name(resource, name); }
void GenericNamingTool::nameResource(ID3D12Resource *resource, eastl::wstring_view name) { set_object_name(resource, name); }
void GenericNamingTool::nameObject(ID3D12Object *object, eastl::string_view name) { set_object_name(object, name); }
void GenericNamingTool::nameObject(ID3D12Object *object, eastl::wstring_view name) { set_object_name(object, name); }
} // namespace drv3d_dx12::debug::gpu_capture
