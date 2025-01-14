// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frame_command_logger.h"

// Dependencies of commands
#include "device_queue.h"
#include "fsr_args.h"
#include "fsr2_wrapper.h"
#include "info_types.h"
#include "pipeline.h"
#include "query_manager.h"
#include "resource_memory_heap.h"
#include "swapchain.h"
#include "viewport_state.h"
#include "xess_wrapper.h"
#include <3d/dag_amdFsr.h>
#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_heap.h>
#include <osApiWrappers/dag_events.h>

#include <util/dag_string.h>
#include <value_range.h>


using namespace drv3d_dx12;

namespace
{

// return string literal
#define ENUM_TO_RETURN_CSTR_CASE(scope, value) \
  case scope::value: return #value;

//
#define APPEND_ENUM_DEFAULT(target, msg) \
  default:                               \
    target += msg;                       \
    G_ASSERT(false);                     \
    break;
//
#define APPEND_ENUM_AS_STRING(target, type, label) \
  case type::label: target += #label; break;

const char *host_device_shader_memory_region_source_to_cstr(const HostDeviceSharedMemoryRegion::Source source)
{
  switch (source)
  {
    ENUM_TO_RETURN_CSTR_CASE(HostDeviceSharedMemoryRegion::Source, PERSISTENT_BIDIRECTIONAL)
    ENUM_TO_RETURN_CSTR_CASE(HostDeviceSharedMemoryRegion::Source, PERSISTENT_UPLOAD)
    ENUM_TO_RETURN_CSTR_CASE(HostDeviceSharedMemoryRegion::Source, PERSISTENT_READ_BACK)
    ENUM_TO_RETURN_CSTR_CASE(HostDeviceSharedMemoryRegion::Source, PUSH_RING)
    ENUM_TO_RETURN_CSTR_CASE(HostDeviceSharedMemoryRegion::Source, TEMPORARY)
  }
  G_ASSERT(false);
  return "INVALID";
}

void append_arg(String &target, HostDeviceSharedMemoryRegion::Source const &arg, const char *)
{
  target += host_device_shader_memory_region_source_to_cstr(arg);
}

const char *image_view_state_type_to_cstr(const int type)
{
  switch (type)
  {
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, INVALID)
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, SRV)
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, UAV)
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, RTV)
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, DSV_RW)
    ENUM_TO_RETURN_CSTR_CASE(ImageViewState, DSV_CONST)
  }
  G_ASSERT(false);
  return "INVALID";
}

void append_arg(String &target, Query *const &arg, const char *) { target.aprintf(32, "%d:Query", arg->getId()); }

void append_arg(String &target, Image *const &arg, const char *)
{
  if (arg)
    target.aprintf(32, "image at 0x%p", arg);
  else
    target += "nullptr";
}

void append_arg(String &target, BufferResourceReferenceAndRange const &arg, const char *)
{
  target.aprintf(64, "{buffer: 0x%p, offset: %d, size: %d}", arg.buffer, arg.offset, arg.size);
}

void append_arg(String &target, HostDeviceSharedMemoryRegion const &arg, const char *)
{
  target.aprintf(64, "HostDeviceSharedMemoryRegion{buffer: 0x%p, source: %s, range:%d-%d}", arg.buffer,
    host_device_shader_memory_region_source_to_cstr(arg.source), arg.range.start, arg.range.stop);
}

void append_arg(String &target, ImageViewState const &arg, const char *)
{
  target.aprintf(64, "ImageViewState{value: %d , type: %s}", arg.wrapper.value, image_view_state_type_to_cstr(arg.type));
}

void append_arg(String &target, D3D12_CPU_DESCRIPTOR_HANDLE const &arg, const char *)
{
  target.aprintf(32, "D3D12_CPU_DESCRIPTOR_HANDLE{ 0x%p }", arg.ptr);
}

void append_arg(String &target, ImageViewInfo const &arg, const char *)
{
  target.aprintf(64, "ImageViewInfo{handle: D3D12_CPU_DESCRIPTOR_HANDLE{ 0x%p }, state: ", arg.handle.ptr);
  append_arg(target, arg.state, nullptr);
  target += "}";
}

void append_arg(String &target, E3DCOLOR const &arg, const char *)
{
  target.aprintf(32, "E3DCOLOR[r: %d, g: %d, b: %d, a: %d]", uint32_t(arg.r), uint32_t(arg.g), uint32_t(arg.b), uint32_t(arg.a));
}

void append_arg(String &target, DeviceQueueType const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_DEFAULT(target, "INVALID")
    APPEND_ENUM_AS_STRING(target, DeviceQueueType, GRAPHICS)
    APPEND_ENUM_AS_STRING(target, DeviceQueueType, COMPUTE)
    APPEND_ENUM_AS_STRING(target, DeviceQueueType, UPLOAD)
    APPEND_ENUM_AS_STRING(target, DeviceQueueType, READ_BACK)
  }
}

void append_arg(String &target, ViewportState const &arg, const char *)
{
  target.aprintf(128, "ViewportState[x: %d, y: %d, width: %d, height: %d, minZ: %f, maxZ: %f]", arg.x, arg.y, arg.width, arg.height,
    arg.minZ, arg.maxZ);
}


void append_arg(String &target, GpuPipeline const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_DEFAULT(target, "INVALID")
    APPEND_ENUM_AS_STRING(target, GpuPipeline, GRAPHICS)
    APPEND_ENUM_AS_STRING(target, GpuPipeline, ASYNC_COMPUTE)
  }
}

void append_arg(String &target, uint64_t const &arg, const char *) { target.aprintf(32, "%d", arg); }

void append_arg(String &target, uint32_t const &arg, const char *) { target.aprintf(32, "%d", arg); }

void append_arg(String &target, uint8_t const &arg, const char *) { target.aprintf(32, "%d", arg); }

void append_arg(String &target, int64_t const &arg, const char *) { target.aprintf(32, "%d", arg); }

void append_arg(String &target, int32_t const &arg, const char *) { target.aprintf(32, "%d", arg); }

void append_arg(String &target, bool const &arg, const char *) { target += arg ? "true" : "false"; }

void append_arg(String &target, float const &arg, const char *) { target.aprintf(32, "%f", arg); }

void append_arg(String &target, ProgramID const &arg, const char *)
{
  target.aprintf(
    128, "ProgramID{type: %s (%d), group: %d, index: %d}",
    [&] {
      if (arg.isGraphics())
        return "graphics";
      if (arg.isCompute())
        return "compute";
      return "INVALID";
    }(),
    arg.getType(), arg.getGroup(), arg.getIndex());
}

void append_arg(String &target, GraphicsProgramID const &arg, const char *)
{
  target.aprintf(32, "GraphicsProgramID{group: %d, index: %d}", arg.getGroup(), arg.getIndex());
}

void append_arg(String &target, ShaderID const &arg, const char *)
{
  target.aprintf(32, "ShaderID{group: %d, index: %d}", arg.getGroup(), arg.getIndex());
}

void append_arg(String &target, StageShaderModule *const &arg, const char *)
{
  char hash_str[64];
  arg->ident.shaderHash.convertToString(hash_str, sizeof(hash_str));
  target.aprintf(128, "StageShaderModule{debugName: %s, ident.shaderHash: %s, ident.shaderSize: %d}", arg->debugName, hash_str,
    arg->ident.shaderSize);
}

void append_arg(String &target, BufferGlobalId const &arg, const char *)
{
  target.aprintf(64, "BufferGlobalId{value: %d, index: %d, state: ", arg.get(), arg.index());

  if (arg.isUsedAsVertexOrConstBuffer())
    target.aprintf(32, "VertexOrConstBuffer, ");
  if (arg.isUsedAsIndexBuffer())
    target.aprintf(32, "IndexBuffer, ");
  if (arg.isUsedAsIndirectBuffer())
    target.aprintf(32, "IndirectBuffer, ");
  if (arg.isUsedAsNonPixelShaderResourceBuffer())
    target.aprintf(32, "NonPixelShaderResourceBuffer, ");
  if (arg.isUseAsPixelShaderResourceBuffer())
    target.aprintf(32, "PixelShaderResourceBuffer, ");
  if (arg.isUsedAsCopySourceBuffer())
    target.aprintf(32, "CopySourceBuffer, ");
  if (arg.isUsedAsUAVBuffer())
    target.aprintf(32, "UAVBuffer, ");

  target.aprintf(1, "}");
}

void append_arg(String &target, BufferReference const &arg, const char *)
{
  target.aprintf(128, "BufferReference{buffer: 0x%p, offset: %d, size: %d, resourceId: ", arg.buffer, arg.offset, arg.size);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(128, ", srv: 0x%p, uav: 0x%p, clearview: 0x%p, gpuPointer: 0x%p, cpuPointer: 0x%p}", arg.srv.ptr, arg.uav.ptr,
    arg.clearView.ptr, arg.gpuPointer, arg.cpuPointer);
}

void append_arg(String &target, BufferResourceReference const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReference{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(1, "}");
}

void append_arg(String &target, BufferResourceReferenceAndAddress const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndAddress{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(32, ", gpuPointer: 0x%p}", arg.gpuPointer);
}

void append_arg(String &target, BufferResourceReferenceAndAddressRange const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndAddressRange{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(32, ", gpuPointer: 0x%p, size: %d}", arg.gpuPointer, arg.size);
}

void append_arg(String &target, BufferResourceReferenceAndOffset const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndOffset{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(32, ", offset: %d}", arg.offset);
}

void append_arg(String &target, BufferResourceReferenceAndShaderResourceView const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndShaderResourceView{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(64, ", srv: D3D12_CPU_DESCRIPTOR_HANDLE{ 0x%p }}", arg.srv.ptr);
}

void append_arg(String &target, BufferResourceReferenceAndUnorderedResourceView const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndUnorderedResourceView{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(64, ", uav: D3D12_CPU_DESCRIPTOR_HANDLE{ 0x%p }}", arg.uav.ptr);
}

void append_arg(String &target, BufferResourceReferenceAndClearView const &arg, const char *)
{
  target.aprintf(64, "BufferResourceReferenceAndClearView{buffer: 0x%p, resourceId: ", arg.buffer);
  append_arg(target, arg.resourceId, nullptr);
  target.aprintf(32, ", clearView: D3D12_CPU_DESCRIPTOR_HANDLE{ 0x%p }}", arg.clearView.ptr);
}

void append_arg(String &target, SubresourceRange const &arg, const char *)
{
  target.aprintf(64, "SubresourceRange{begin: %d, end: %d, size: %d}", arg.begin().at, arg.end().at, arg.size());
}

void append_arg(String &target, MipMapRange const &arg, const char *)
{
  target.aprintf(64, "MipMapRange{begin: %d, end: %d, size: %d}", arg.begin().at, arg.end().at, arg.size());
}

void append_arg(String &target, ResourceActivationAction const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, REWRITE_AS_COPY_DESTINATION)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, REWRITE_AS_UAV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, REWRITE_AS_RTV_DSV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, CLEAR_F_AS_UAV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, CLEAR_I_AS_UAV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, CLEAR_AS_RTV_DSV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, DISCARD_AS_UAV)
    APPEND_ENUM_AS_STRING(target, ResourceActivationAction, DISCARD_AS_RTV_DSV)
    APPEND_ENUM_DEFAULT(target, "INVALID")
  }
}

void append_arg(String &target, ResourceClearValue const &arg, const char *)
{
  target.aprintf(128,
    "ResourceClearValue asFloat[%.4f, %.4f, %.4f, %.4f]; asDepthStencil{%.4f, %u}; asUint[%u, %u, %u, %u]; asInt[%d, %d, %d, %d]",
    arg.asFloat[0], arg.asFloat[1], arg.asFloat[2], arg.asFloat[3], arg.asDepth, arg.asStencil, arg.asUint[0], arg.asUint[1],
    arg.asUint[2], arg.asUint[3], arg.asInt[0], arg.asInt[1], arg.asInt[2], arg.asInt[3]);
}

void append_arg(String &target, ClearColorValue const &arg, const char *)
{
  target.aprintf(128, "ClearColorValue{ float32[%.4f, %.4f, %.4f, %.4f]", arg.float32[0], arg.float32[1], arg.float32[2],
    arg.float32[3]);
}

void append_arg(String &target, ClearDepthStencilValue const &arg, const char *)
{
  target.aprintf(128, "ClearDepthStencilValue{depth: %.4f, stencil: %u}", arg.depth, arg.stencil);
}

void append_arg(String &target, Offset3D const &arg, const char *)
{
  target.aprintf(64, "Offset3D{ x=%d, y=%d, z=%d }", arg.x, arg.y, arg.z);
}

void append_arg(String &target, D3D12_BOX const &arg, const char *)
{
  target.aprintf(64, "D3D12_BOX{ left=%d, top=%d, front=%d, right=%d, bottom=%d, back=%d }", arg.left, arg.top, arg.front, arg.right,
    arg.bottom, arg.back);
}

void append_arg(String &target, D3D12_RECT const &arg, const char *)
{
  target.aprintf(64, "D3D12_RECT{ left=%u, top=%u, right=%u, bottom=%u }", arg.left, arg.top, arg.right, arg.bottom);
}

void append_arg(String &target, ImageCopy const &arg, const char *)
{
  target.aprintf(64, "ImageCopy{srcSubresourceId=%d, dstSubresourceId=%d, dstOffset=", arg.srcSubresource.index(),
    arg.dstSubresource.index());
  append_arg(target, arg.dstOffset, nullptr);
  target += ", srcBox=";
  append_arg(target, arg.srcBox, nullptr);
  target += "}";
}

void append_arg(String &target, PredicateInfo const &arg, const char *)
{
  target.aprintf(64, "PredicateInfo{heap: 0x%p, buffer: ", arg.heap);
  append_arg(target, arg.buffer, nullptr);
  target.aprintf(16, ", index: %d}", arg.index);
}

void append_arg(String &target, StaticRenderStateID const &arg, const char *)
{
  target.aprintf(32, "StaticRenderStateID{ %d }", arg.get());
}

void append_arg(String &target, InternalInputLayoutID const &arg, const char *)
{
  target.aprintf(32, "InternalInputLayoutID{ %d }", arg.get());
}

void append_arg(String &target, MipMapIndex const &arg, const char *) { target.aprintf(32, "MipMapIndex{ %d }", arg.index()); }

void append_arg(String &target, ArrayLayerIndex const &arg, const char *) { target.aprintf(32, "ArrayLayerIndex{ %d }", arg.index()); }

void append_arg(String &target, Drv3dTimings *const &arg, const char *)
{
  target.aprintf(256,
    "Drv3dTimings{ frontendUpdateScreenInterval: %lld, frontendToBackendUpdateScreenLatency: %lld, frontendBackendWaitDuration: %lld, "
    "backendFrontendWaitDuration: %lld, gpuWaitDuration: %lld, presentDuration: %lld, backbufferAcquireDuration: %lld, "
    "frontendWaitForSwapchainDuration: %lld}",
    arg->frontendUpdateScreenInterval, arg->frontendToBackendUpdateScreenLatency, arg->frontendBackendWaitDuration,
    arg->backendFrontendWaitDuration, arg->gpuWaitDuration, arg->presentDuration, arg->backbufferAcquireDuration,
    arg->frontendWaitForSwapchainDuration);
}

void append_arg(String &target, ValueRange<ExtendedImageGlobalSubresourceId> const &arg, const char *)
{
  target.aprintf(64, "ValueRange<ExtendedImageGlobalSubresourceId>{begin: %d, end: %d, size: %d}", arg.begin().at, arg.end().at,
    arg.size());
}

#if !_TARGET_XBOXONE
void append_arg(String &target, D3D12_SHADING_RATE const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_1X1)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_1X2)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_2X1)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_2X2)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_2X4)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_4X2)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_4X4)
    APPEND_ENUM_DEFAULT(target, "INVALID")
  }
}
#endif

void append_arg(String &target, D3D12_SHADING_RATE_COMBINER const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_COMBINER_PASSTHROUGH)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_COMBINER_OVERRIDE)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_COMBINER_MIN)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_COMBINER_MAX)
    APPEND_ENUM_AS_STRING(target, , D3D12_SHADING_RATE_COMBINER_SUM)
    APPEND_ENUM_DEFAULT(target, "INVALID")
  }
}

void append_arg(String &target, D3D12_PRIMITIVE_TOPOLOGY const &arg, const char *)
{
  // D3D12_PRIMITIVE_TOPOLOGY variable in dx12 command stream contains custom dagor d3d values
  // see: dag_3dConst_base.h:~110 and translate_primitive_topology_to_dx12 in dx12.cpp:~1272
  target.aprintf(32, "raw value: %d; dagor enum: ", arg);
  switch (int(arg))
  {
    APPEND_ENUM_AS_STRING(target, , PRIM_POINTLIST)
    APPEND_ENUM_AS_STRING(target, , PRIM_LINELIST)
    APPEND_ENUM_AS_STRING(target, , PRIM_LINESTRIP)
    APPEND_ENUM_AS_STRING(target, , PRIM_TRILIST)
    APPEND_ENUM_AS_STRING(target, , PRIM_TRISTRIP)
    APPEND_ENUM_AS_STRING(target, , PRIM_TRIFAN)
#if _TARGET_XBOX
    case PRIM_QUADLIST: target.aprintf(64, "PRIM_QUADLIST (D3D_PRIMITIVE_TOPOLOGY_QUADLIST)");
    case D3D_PRIMITIVE_TOPOLOGY_QUADLIST: target.aprintf(64, "D3D_PRIMITIVE_TOPOLOGY_QUADLIST (PRIM_QUADLIST)");
#endif
      APPEND_ENUM_AS_STRING(target, , PRIM_4_CONTROL_POINTS)
    default: target.aprintf(32, "PRIM_<unknown>");
  }
}

void append_arg(String &target, DXGI_FORMAT const &arg, const char *)
{
  switch (arg)
  {
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_UNKNOWN)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32A32_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32A32_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32A32_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32A32_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32B32_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16B16A16_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G32_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32G8X24_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R10G10B10A2_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R10G10B10A2_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R10G10B10A2_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R11G11B10_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8B8A8_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16G16_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_D32_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R32_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R24G8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_D24_UNORM_S8_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_X24_TYPELESS_G8_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_FLOAT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_D16_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R16_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8_UINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8_SINT)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_A8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R1_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R8G8_B8G8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_G8R8_G8B8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC1_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC1_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC1_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC2_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC2_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC2_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC3_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC3_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC3_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC4_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC4_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC4_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC5_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC5_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC5_SNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B5G6R5_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B5G5R5A1_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8A8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8X8_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8A8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8X8_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC6H_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC6H_UF16)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC6H_SF16)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC7_TYPELESS)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC7_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_BC7_UNORM_SRGB)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_AYUV)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_Y410)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_Y416)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_NV12)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_P010)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_P016)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_420_OPAQUE)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_YUY2)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_Y210)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_Y216)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_NV11)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_AI44)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_IA44)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_P8)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_A8P8)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_B4G4R4A4_UNORM)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_P208)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_V208)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_V408)
#if !_TARGET_XBOXONE
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE)
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE)
#endif
    APPEND_ENUM_AS_STRING(target, , DXGI_FORMAT_FORCE_UINT)
    APPEND_ENUM_DEFAULT(target, "INVALID")
  }
}

template <typename T>
void append_arg(String &target, T const &, const char *type)
{
  target += type;
}

#define DX12_STORE_CONTEXT_COMMAND_NAME_AND_PRIMACY(Name) \
  result = "Cmd" #Name;                                   \
  result.aprintf(128, " (%s)", cmd.is_primary() ? "primary" : "secondary");

#define DX12_BEGIN_CONTEXT_COMMAND(IsPrimary, Name)                                                            \
  const char *cmdToStr(String &result, debug::call_stack::Reporter &call_stack_reporter, const Cmd##Name &cmd) \
  {                                                                                                            \
    G_UNUSED(cmd);                                                                                             \
    DX12_STORE_CONTEXT_COMMAND_NAME_AND_PRIMACY(Name)

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(IsPrimary, Name, Param0Type, Param0Name)        \
  const char *cmdToStr(String &result, debug::call_stack::Reporter &call_stack_reporter, \
    const ExtendedVariant<Cmd##Name, Param0Type> &param)                                 \
  {                                                                                      \
    auto &cmd = param.cmd;                                                               \
    G_UNUSED(cmd);                                                                       \
    DX12_STORE_CONTEXT_COMMAND_NAME_AND_PRIMACY(Name)

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(IsPrimary, Name, Param0Type, Param0Name, Param1Type, Param1Name) \
  const char *cmdToStr(String &result, debug::call_stack::Reporter &call_stack_reporter,                  \
    const ExtendedVariant2<Cmd##Name, Param0Type, Param1Type> &param)                                     \
  {                                                                                                       \
    auto &cmd = param.cmd;                                                                                \
    G_UNUSED(cmd);                                                                                        \
    DX12_STORE_CONTEXT_COMMAND_NAME_AND_PRIMACY(Name)

#define DX12_CONTEXT_COMMAND_PARAM(type, name) \
  {                                            \
    result += "\n> " #name "=";                \
    append_arg(result, cmd.name, #type);       \
  }

#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) \
  result += "\n> " #name "=[";                             \
  for (int32_t i = 0; i < size; i++)                       \
  {                                                        \
    if (i != 0)                                            \
      result += ", ";                                      \
    append_arg(result, cmd.name[i], #type);                \
  }                                                        \
  result += "]";

#define DX12_END_CONTEXT_COMMAND                 \
  call_stack_reporter.append(result, "\n", cmd); \
  return result;                                 \
  }

#include <device_context_cmd.inc.h>

#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
#undef DX12_CONTEXT_COMMAND_IMPLEMENTATION
#undef HANDLE_RETURN_ADDRESS
#undef DX12_STORE_CONTEXT_COMMAND_NAME_AND_PRIMACY

}

namespace drv3d_dx12::debug::core
{
void FrameCommandLogger::dumpFrameCommandLog(FrameCommandLog &frame_log, call_stack::Reporter &call_stack_reporter)
{
  String buffer;
  logdbg("Frame %d log begin", frame_log.frameId);
  logdbg("--------------------");
  frame_log.log.visitAll([&buffer, &call_stack_reporter](const auto &value) { logdbg(cmdToStr(buffer, call_stack_reporter, value)); });
  logdbg("Frame %d log end", frame_log.frameId);
}
}
