// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <d3d12_error_handling.h>
#include <driver.h>

#include <dag/dag_vector.h>
#include <debug/dag_log.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <EASTL/initializer_list.h>
#include <EASTL/optional.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>
#include <EASTL/variant.h>

#if _TARGET_XBOX
#include <command_list_xbox.h>
#endif


template <class T>
struct IsSpan : eastl::false_type
{};

template <class T>
struct IsSpan<eastl::span<T>> : eastl::true_type
{
  using ElementType = T;
};

namespace drv3d_dx12::debug
{
enum class DX12_COMMAND_LOG_ITEM_TYPE : uint32_t
{
#define DX12_HARDWARE_COMMAND(TYPE, COMMAND_NAME, ...) TYPE,
#include <command_list_cmd.inc.h>
#undef DX12_HARDWARE_COMMAND
};

namespace core
{
template <DX12_COMMAND_LOG_ITEM_TYPE command_type, class... ARGS>
uint32_t full_log_item_size(ARGS... args);

template <DX12_COMMAND_LOG_ITEM_TYPE command_type, class... ARGS>
class CommandLogItem
{
public:
  CommandLogItem(ARGS... args) : args{args...} {}

  eastl::string toString() const
  {
    return eastl::apply([](ARGS... args) { return make_command_string({to_string(args)...}); }, args);
  }

  constexpr static uint32_t static_command_size() { return (static_argument_size<ARGS>() + ... + sizeof(DX12_COMMAND_LOG_ITEM_TYPE)); }

  uint32_t command_size() const
  {
    return eastl::apply([](ARGS... args) { return full_log_item_size<command_type>(args...); }, args);
  }

private:
  template <class ARG_TYPE>
  constexpr static uint32_t static_argument_size()
  {
    return IsSpan<ARG_TYPE>::value ? 0 : sizeof(ARG_TYPE);
  }

  static eastl::string make_command_string(std::initializer_list<eastl::string_view> args)
  {
    eastl::string result{to_string(command_type)};
    result += "(";
    for (auto it = args.begin(); it != args.end(); it++)
    {
      if (it != args.begin())
        result += ", ";
      result.append(it->data(), it->size());
    }
    result += ")";
    return result;
  }

  template <class T>
  static eastl::string to_string(T arg)
  {
    return eastl::to_string(arg);
  }

  template <class T>
  static eastl::string to_string(const eastl::optional<T> &optional_arg)
  {
    if (!optional_arg)
      return "nullptr";
    return to_string(*optional_arg);
  }

  template <class T>
  static eastl::string to_string(const eastl::span<T> &args)
  {
    eastl::string result{"{"};
    for (auto it = args.cbegin(); it != args.cend(); ++it)
    {
      if (it != args.cbegin())
        result += ", ";
      result += to_string(*it);
    }
    result += "}";
    return result;
  }

  static eastl::string to_string(const eastl::string &arg) { return arg; }

  static eastl::string to_string(D3D12_CPU_DESCRIPTOR_HANDLE arg)
  {
    eastl::string result;
    result.sprintf("0x%x", arg.ptr);
    return result;
  }

  static eastl::string to_string(D3D12_GPU_DESCRIPTOR_HANDLE arg)
  {
    eastl::string result;
    result.sprintf("0x%x", arg.ptr);
    return result;
  }

  static eastl::string to_string(const void *arg)
  {
    eastl::string result;
    result.sprintf("0x%p", arg);
    return result;
  }

  static const char *to_string(DX12_COMMAND_LOG_ITEM_TYPE cmd_type)
  {
    switch (cmd_type)
    {
#define DX12_HARDWARE_COMMAND(TYPE, COMMAND_NAME, ...) \
  case DX12_COMMAND_LOG_ITEM_TYPE::TYPE:               \
    return COMMAND_NAME;
#include <command_list_cmd.inc.h>
#undef DX12_HARDWARE_COMMAND
      default: D3D_ERROR("Unknown command type %d", (int)cmd_type); return "Unknown";
    }
  }

  static eastl::string to_string(ID3D12Resource *arg) { return to_string(static_cast<const void *>(arg)); }
  static eastl::string to_string(ID3D12RootSignature *arg) { return to_string(static_cast<const void *>(arg)); }
  static eastl::string to_string(ID3D12PipelineState *arg) { return to_string(static_cast<const void *>(arg)); }
  static eastl::string to_string(ID3D12CommandSignature *arg) { return to_string(static_cast<const void *>(arg)); }
  static eastl::string to_string(ID3D12QueryHeap *arg) { return to_string(static_cast<const void *>(arg)); }
  static eastl::string to_string(ID3D12DescriptorHeap *arg) { return to_string(static_cast<const void *>(arg)); }
#if D3D_HAS_RAY_TRACING
  static eastl::string to_string(ID3D12StateObject *arg) { return to_string(static_cast<const void *>(arg)); }
#endif

  static eastl::string to_string(const D3D12_SUBRESOURCE_FOOTPRINT &footprint)
  {
    eastl::string result;
    result.sprintf("{%u, %u, %u, %u, %u}", footprint.Format, footprint.Width, footprint.Height, footprint.Depth, footprint.RowPitch);
    return result;
  }

  static eastl::string to_string(const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint)
  {
    eastl::string result;
    result.sprintf("{0x%" PRIX64, footprint.Offset);
    result.append(to_string(footprint.Footprint));
    result.append("}");
    return result;
  }

  static eastl::string to_string(const D3D12_TEXTURE_COPY_LOCATION &location)
  {
    eastl::string result;
    result.sprintf("{0x%p, %u, ", location.pResource, location.Type);
    if (location.Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)
      result.append(to_string(location.PlacedFootprint));
    else
      result.append_sprintf("%u", location.SubresourceIndex);
    result.append("}");
    return result;
  }

  static eastl::string to_string(const D3D12_BOX &box)
  {
    eastl::string result;
    result.sprintf("{%u, %u, %u, %u, %u, %u}", box.left, box.top, box.front, box.right, box.bottom, box.back);
    return result;
  }

  static eastl::string to_string(const D3D12_INDEX_BUFFER_VIEW &index_buffer_view)
  {
    eastl::string result;
    result.sprintf("{0x%" PRIX64 ", %u, %u}", index_buffer_view.BufferLocation, index_buffer_view.SizeInBytes,
      index_buffer_view.Format);
    return result;
  }

  static eastl::string to_string(const D3D12_GPU_VIRTUAL_ADDRESS_RANGE &gpu_virtual_address_range)
  {
    eastl::string result;
    result.sprintf("{0x%" PRIX64 ", %u}", gpu_virtual_address_range.StartAddress, gpu_virtual_address_range.SizeInBytes);
    return result;
  }

  static eastl::string to_string(const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE &gpu_virtual_address_range_and_stride)
  {
    eastl::string result;
    result.sprintf("{0x%" PRIX64 ", %u, %u}", gpu_virtual_address_range_and_stride.StartAddress,
      gpu_virtual_address_range_and_stride.SizeInBytes, gpu_virtual_address_range_and_stride.StrideInBytes);
    return result;
  }

#if D3D_HAS_RAY_TRACING
  static eastl::string to_string(const D3D12_DISPATCH_RAYS_DESC &dispatch_rays_desc)
  {
    eastl::string result;
    result.sprintf("{");
    result.append_sprintf("%s, ", to_string(dispatch_rays_desc.RayGenerationShaderRecord).c_str());
    result.append_sprintf("%s, ", to_string(dispatch_rays_desc.MissShaderTable).c_str());
    result.append_sprintf("%s, ", to_string(dispatch_rays_desc.HitGroupTable).c_str());
    result.append_sprintf("%s, ", to_string(dispatch_rays_desc.CallableShaderTable).c_str());
    result.append_sprintf("{%u, %u, %u}", dispatch_rays_desc.Width, dispatch_rays_desc.Height, dispatch_rays_desc.Depth);
    return result;
  }

  static eastl::string to_string(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &inputs)
  {
    eastl::string result;
    result.sprintf("{");
    result.append_sprintf("%s, ", to_string(inputs.Type).c_str());
    result.append_sprintf("%s, ", to_string(inputs.Flags).c_str());
    result.append_sprintf("%s, ", to_string(inputs.NumDescs).c_str());
    result.append_sprintf("%s, ", to_string(inputs.DescsLayout).c_str());
    result.append("{...}}"); /// TODO
    return result;
  }

  static eastl::string to_string(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &desc)
  {
    eastl::string result;
    result.sprintf("{");
    result.append_sprintf("%s, ", to_string(desc.DestAccelerationStructureData).c_str());
    result.append_sprintf("%s, ", to_string(desc.Inputs).c_str());
    result.append_sprintf("%s, ", to_string(desc.SourceAccelerationStructureData).c_str());
    result.append_sprintf("%s", to_string(desc.ScratchAccelerationStructureData).c_str());
    result.sprintf("}");
    return result;
  }
#endif

  static eastl::string to_string(const D3D12_DISCARD_REGION &discard_region)
  {
    eastl::string result;
    result.sprintf("{%u, {...}, %u, %u}", discard_region.NumRects /*, discard_region.pRects*/, discard_region.FirstSubresource,
      discard_region.NumSubresources); /// TODO
    return result;
  }

  eastl::tuple<ARGS...> args;
};

template <DX12_COMMAND_LOG_ITEM_TYPE command_type, class... ARGS>
inline uint32_t full_log_item_size(ARGS... args)
{
  constexpr uint32_t staticCommandSize = CommandLogItem<command_type, ARGS...>::static_command_size();
  auto getDynamicArgSize = [](auto arg) {
    if constexpr (IsSpan<decltype(arg)>::value)
      return arg.size_bytes() + sizeof(uint32_t);
    else
      return 0;
  };
  const uint32_t dynamicCommandSize = (getDynamicArgSize(args) + ... + 0);
  return staticCommandSize + dynamicCommandSize;
}

using AnyCommandItemType = eastl::variant<eastl::monostate
#define DX12_HARDWARE_COMMAND(TYPE, COMMAND_NAME, ...) , CommandLogItem<DX12_COMMAND_LOG_ITEM_TYPE::TYPE, __VA_ARGS__>
#include <command_list_cmd.inc.h>
#undef DX12_HARDWARE_COMMAND
  >;

class CommandLogDecoder
{
public:
  static void dump_commands_to_log(eastl::optional<eastl::span<const unsigned char>> commands_logs)
  {
    if (!commands_logs)
    {
      logdbg("Commands log is disabled");
      return;
    }

    logdbg("Hardware command list log begin");
    uint32_t offset = 0;
    while (offset < commands_logs->size())
    {
      auto cmdVariant = decode_command(commands_logs->data() + offset);
      logdbg("%s", eastl::visit(CommandToStringVisitor{}, cmdVariant));
      offset += eastl::visit(CommandToSizeVisitor{}, cmdVariant);
    }
    logdbg("Hardware command list log end");
  }

private:
  struct CommandToStringVisitor
  {
    template <class COMMAND>
    eastl::string operator()(const COMMAND &cmd) const
    {
      return cmd.toString();
    }
    eastl::string operator()(const eastl::monostate &) const { return "Unknown command"; }
  };

  struct CommandToSizeVisitor
  {
    template <class COMMAND>
    uint32_t operator()(const COMMAND &cmd) const
    {
      return cmd.command_size();
    }
    uint32_t operator()(const eastl::monostate &) const { return sizeof(DX12_COMMAND_LOG_ITEM_TYPE); }
  };

  template <class ARG_TYPE>
  static ARG_TYPE decode_argument(const unsigned char *&args)
  {
    if constexpr (IsSpan<ARG_TYPE>::value)
    {
      const uint32_t size = *reinterpret_cast<const uint32_t *>(args);
      args += sizeof(uint32_t);
      const auto begin = reinterpret_cast<const typename IsSpan<ARG_TYPE>::ElementType *>(args);
      args += size * sizeof(typename IsSpan<ARG_TYPE>::ElementType);
      return {begin, size};
    }
    else
    {
      const auto result = *reinterpret_cast<const ARG_TYPE *>(args);
      args += sizeof(ARG_TYPE);
      return result;
    }
  }

  template <DX12_COMMAND_LOG_ITEM_TYPE command_type, class... ARG_TYPE>
  static AnyCommandItemType decode_command(const unsigned char *args)
  {
    return CommandLogItem<command_type, ARG_TYPE...>(decode_argument<ARG_TYPE>(args)...);
  }

  static AnyCommandItemType decode_command(const unsigned char *ptr)
  {
    const DX12_COMMAND_LOG_ITEM_TYPE cmdType = *reinterpret_cast<const DX12_COMMAND_LOG_ITEM_TYPE *>(ptr);
    const auto args = ptr + sizeof(DX12_COMMAND_LOG_ITEM_TYPE);
    switch (cmdType)
    {
#define DX12_HARDWARE_COMMAND(TYPE, COMMAND_NAME, ...) \
  case DX12_COMMAND_LOG_ITEM_TYPE::TYPE:               \
    return decode_command<DX12_COMMAND_LOG_ITEM_TYPE::TYPE, __VA_ARGS__>(args);
#include <command_list_cmd.inc.h>
#undef DX12_HARDWARE_COMMAND
      default:
        D3D_ERROR("Unknown command type %d", static_cast<eastl::underlying_type_t<DX12_COMMAND_LOG_ITEM_TYPE>>(cmdType));
        return eastl::monostate{};
    }
  }
};

class CommandLogStorage
{
public:
  eastl::optional<eastl::span<const unsigned char>> getCommands() const
  {
    return eastl::span{commandsLogs.begin(), commandsLogs.end()};
  }

protected:
  template <DX12_COMMAND_LOG_ITEM_TYPE command_type, class... ARG_TYPE>
  auto logCommand(ARG_TYPE... args)
    -> decltype(AnyCommandItemType{eastl::declval<CommandLogItem<command_type, ARG_TYPE...>>()}, void())
  {
    const uint32_t commandSize = full_log_item_size<command_type>(args...);
    uint32_t offset = commandsLogs.size();
    commandsLogs.resize(offset + commandSize);
    writeToByteStream(command_type, offset);
    (writeToByteStream(args, offset), ...);
  }

  void reset() { commandsLogs.clear(); }

private:
  template <class ARG_TYPE>
  void writeToByteStream(eastl::span<ARG_TYPE> args, uint32_t &offset)
  {
    writeToByteStream((uint32_t)args.size(), offset);
    for (const auto &arg : args)
      writeToByteStream(arg, offset);
  }

  template <class ARG_TYPE>
  void writeToByteStream(ARG_TYPE arg, uint32_t &offset)
  {
    constexpr uint32_t size = sizeof(ARG_TYPE);
    memcpy(&commandsLogs[offset], &arg, size);
    offset += size;
  }

  dag::Vector<unsigned char> commandsLogs;
};

} // namespace core

namespace null
{
class CommandLogDecoder
{
public:
  static constexpr void dump_commands_to_log(const eastl::optional<eastl::span<const unsigned char>> &) {}
};

class CommandLogStorage
{
public:
  eastl::optional<eastl::span<const unsigned char>> getCommands() const { return {}; }

protected:
  template <DX12_COMMAND_LOG_ITEM_TYPE, class... ARG_TYPE>
  constexpr void logCommand(ARG_TYPE...)
  {}
  constexpr void reset() {}
};

} // namespace null

#if DX12_ENABLE_COMMAND_LIST_LOGGER
using namespace core;
#else
using namespace null;
#endif

class CommandListLogger : public CommandLogStorage
{
public:
  void copyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
    const D3D12_BOX *src_box)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::COPY_TEXTURE_REGION>(*dst, x, y, z, *src,
      src_box ? eastl::optional{*src_box} : eastl::nullopt);
  }

  void copyBufferRegion(ID3D12Resource *dst, UINT64 dst_offset, ID3D12Resource *src, UINT64 src_offset, UINT64 num_bytes)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::COPY_BUFFER_REGION>(dst, dst_offset, src, src_offset, num_bytes);
  }

  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER * /*barriers*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RESOURCE_BARRIER>(num_barriers /*, barriers*/);
  }

  void copyResource(ID3D12Resource *dst, ID3D12Resource *src) { logCommand<DX12_COMMAND_LOG_ITEM_TYPE::COPY_RESOURCE>(dst, src); }

  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::END_QUERY>(query_heap, type, index);
  }

  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RESOLVE_QUERY_DATA>(query_heap, type, start_index, num_queries, destination_buffer,
      aligned_destination_buffer_offset);
  }

  void setDescriptorHeaps(UINT num_descriptor_heaps, ID3D12DescriptorHeap *const *descriptor_heaps)
  {
    auto heaps = eastl::span(descriptor_heaps, num_descriptor_heaps);
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_DESCRIPTOR_HEAPS>(num_descriptor_heaps, heaps);
  }

  void clearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE view_GPU_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const FLOAT /*values*/[4], UINT num_rects,
    const D3D12_RECT * /*rects*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::CLEAR_UNORDERED_ACCESS_VIEW_FLOAT>(view_GPU_handle_in_current_heap, view_CPU_handle,
      resource,
      /*values, */ num_rects /*, rects*/);
  }

  void clearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE view_gpu_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const UINT /*values*/[4], UINT num_rects,
    const D3D12_RECT * /*rects*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::CLEAR_UNORDERED_ACCESS_VIEW_UINT>(view_gpu_handle_in_current_heap, view_CPU_handle,
      resource /*, values*/, num_rects /*, rects*/);
  }

  void setComputeRootSignature(ID3D12RootSignature *root_signature)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_COMPUTE_ROOT_SIGNATURE>(root_signature);
  }

  void setPipelineState(ID3D12PipelineState *pipeline_state)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_PIPELINE_STATE>(pipeline_state);
  }

  void dispatch(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DISPATCH>(thread_group_count_x, thread_group_count_y, thread_group_count_z);
  }

  void executeIndirect(ID3D12CommandSignature *command_signature, UINT max_command_count, ID3D12Resource *argument_buffer,
    UINT64 argument_buffer_offset, ID3D12Resource *count_buffer, UINT64 count_buffer_offset)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::EXECUTE_INDIRECT>(command_signature, max_command_count, argument_buffer,
      argument_buffer_offset, count_buffer, count_buffer_offset);
  }

  void setComputeRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_COMPUTE_ROOT_CONSTANT_BUFFER_VIEW>(root_parameter_index, buffer_location);
  }

  void setComputeRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_COMPUTE_ROOT_DESCRIPTOR_TABLE>(root_parameter_index, base_descriptor);
  }

  void setComputeRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_COMPUTE_ROOT_32BIT_CONSTANTS>(root_parameter_index, num_32bit_values_to_set, src_data,
      dest_offset_in_32bit_values); // exact values don't matter
  }


  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DISCARD_RESOURCE>(resource, region ? eastl::optional{*region} : eastl::nullopt);
  }

  void setPredication(ID3D12Resource *buffer, UINT64 aligned_buffer_offset, D3D12_PREDICATION_OP operation)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_PREDICATION>(buffer, aligned_buffer_offset, operation);
  }


  void clearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE render_target_view, const FLOAT /*color_rgba*/[4], UINT num_rects,
    const D3D12_RECT * /*rects*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::CLEAR_RENDER_TARGET_VIEW>(render_target_view, /*color_rgba, */ num_rects /*, rects*/);
  }

  void resolveSubresource(ID3D12Resource *dst_resource, UINT dst_subresource, ID3D12Resource *src_resource, UINT src_subresource,
    DXGI_FORMAT format)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RESOLVE_SUBRESOURCE>(dst_resource, dst_subresource, src_resource, src_subresource, format);
  }

  void clearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view, D3D12_CLEAR_FLAGS clear_flags, FLOAT depth, UINT8 stencil,
    UINT num_rects, const D3D12_RECT * /*rects*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::CLEAR_DEPTH_STENCIL_VIEW>(depth_stencil_view, clear_flags, depth, stencil,
      num_rects /*, rects*/);
  }

  void beginQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::BEGIN_QUERY>(query_heap, type, index);
  }

  void writeBufferImmediate(UINT count, const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER * /*params*/,
    const D3D12_WRITEBUFFERIMMEDIATE_MODE * /*modes*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::WRITE_BUFFER_IMMEDIATE>(count /*, params, modes*/);
  }

  void omSetRenderTargets(UINT num_render_target_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE * /*render_target_descriptors*/,
    BOOL rts_single_handle_to_descriptor_range, const D3D12_CPU_DESCRIPTOR_HANDLE * /*depth_stencil_descriptor*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::OM_SET_RENDER_TARGETS>(num_render_target_descriptors /*, render_target_descriptors*/,
      rts_single_handle_to_descriptor_range /*, depth_stencil_descriptor*/);
  }

  void setGraphicsRootSignature(ID3D12RootSignature *root_signature)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_GRAPHICS_ROOT_SIGNATURE>(root_signature);
  }
  void iaSetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_topology)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::IA_SET_PRIMITIVE_TOPOLOGY>(primitive_topology);
  }
  void omSetDepthBounds(FLOAT min_value, FLOAT max_value)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::OM_SET_DEPTH_BOUNDS>(min_value, max_value);
  }
  void omSetBlendFactor(const FLOAT /*blend_factor*/[4])
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::OM_SET_BLEND_FACTOR>(/*, blend_factor*/);
  }
  void rsSetScissorRects(UINT num_rects, const D3D12_RECT * /*rects*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RS_SET_SCISSOR_RECTS>(num_rects /*, rects*/);
  }
  void rsSetViewports(UINT num_viewports, const D3D12_VIEWPORT * /*viewports*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RS_SET_VIEWPORTS>(num_viewports /*, viewports*/);
  }
  void omSetStencilRef(UINT stencil_ref) { logCommand<DX12_COMMAND_LOG_ITEM_TYPE::OM_SET_STENCIL_REF>(stencil_ref); }
  void drawInstanced(UINT vertex_count_per_instance, UINT instance_count, UINT start_vertex_location, UINT start_instance_location)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DRAW_INSTANCED>(vertex_count_per_instance, instance_count, start_vertex_location,
      start_instance_location);
  }
  void drawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, UINT start_index_location, INT base_vertex_location,
    UINT start_instance_location)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DRAW_INDEXED_INSTANCED>(index_count_per_instance, instance_count, start_index_location,
      base_vertex_location, start_instance_location);
  }
  void setGraphicsRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_GRAPHICS_ROOT_32BIT_CONSTANTS>(root_parameter_index, num_32bit_values_to_set, src_data,
      dest_offset_in_32bit_values); // exact values don't matter
  }
  void setGraphicsRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_GRAPHICS_ROOT_DESCRIPTOR_TABLE>(root_parameter_index, base_descriptor);
  }

  void setGraphicsRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_GRAPHICS_ROOT_CONSTANT_BUFFER_VIEW>(root_parameter_index, buffer_location);
  }

  void iaSetVertexBuffers(UINT start_slot, UINT num_views, const D3D12_VERTEX_BUFFER_VIEW * /*views*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::IA_SET_VERTEX_BUFFERS>(start_slot, num_views /*, views*/);
  }

  void iaSetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW *view)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::IA_SET_INDEX_BUFFER>(view ? eastl::optional{*view} : eastl::nullopt);
  }

#if !_TARGET_XBOXONE
  void rsSetShadingRate(D3D12_SHADING_RATE base_shading_rate, const D3D12_SHADING_RATE_COMBINER * /*combiners*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RS_SET_SHADING_RATE>(base_shading_rate /*, combiners*/);
  }
  void rsSetShadingRateImage(ID3D12Resource *shading_rate_image)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::RS_SET_SHADING_RATE_IMAGE>(shading_rate_image);
  }

  void dispatchMesh(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DISPATCH_MESH>(thread_group_count_x, thread_group_count_y, thread_group_count_z);
  }
#endif

#if D3D_HAS_RAY_TRACING
  void buildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *dst,
    UINT num_postbuild_info_descs, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC * /*postbuild_info_descs*/)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::BUILD_RAYTRACING_ACCELERATION_STRUCTURE>(*dst,
      num_postbuild_info_descs /*, postbuild_info_descs*/);
  }

#if _TARGET_XBOX
  COMMAND_LIST_LOGGERS_XBOX()
#endif

  void copyRaytracingAccelerationStructure(D3D12_GPU_VIRTUAL_ADDRESS dst, D3D12_GPU_VIRTUAL_ADDRESS src,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::COPY_RAYTRACING_ACCELERATION_STRUCTURE>(dst, src, mode);
  }

  void setPipelineState1(ID3D12StateObject *state_object)
  {
    logCommand<DX12_COMMAND_LOG_ITEM_TYPE::SET_PIPELINE_STATE1>(state_object);
  }

  void dispatchRays(const D3D12_DISPATCH_RAYS_DESC *desc) { logCommand<DX12_COMMAND_LOG_ITEM_TYPE::DISPATCH_RAYS>(*desc); }
#endif
};

} // namespace drv3d_dx12::debug