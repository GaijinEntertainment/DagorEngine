#pragma once

#include <supp/dag_comPtr.h>

#include "driver.h"
#include "util.h"

/*!
 * @file
 * @brief This file contains utility functions for DirectX types.
 */

/**
 * @brief Compares two CPU descriptor handles.
 * 
 * @param [in] l, r The compared handles.
 * @return          \c true if both arguments handle same descriptors.
 *
 */
inline bool operator==(D3D12_CPU_DESCRIPTOR_HANDLE l, D3D12_CPU_DESCRIPTOR_HANDLE r) { return l.ptr == r.ptr; }

/**
 * @brief Compares two CPU descriptor handles.
 * 
 * @param [in] l, r The compared descriptors.
 * @return          \c true if both arguments handle different descriptors.
 */
inline bool operator!=(D3D12_CPU_DESCRIPTOR_HANDLE l, D3D12_CPU_DESCRIPTOR_HANDLE r) { return !(l == r); }

/**
 * @brief Compares two GPU descriptor handles.
 * 
 * @param [in] l, r The compared descriptors.
 * @return          \c true if both arguments handle same descriptors
 *
 */
inline bool operator==(D3D12_GPU_DESCRIPTOR_HANDLE l, D3D12_GPU_DESCRIPTOR_HANDLE r) { return l.ptr == r.ptr; }

/**
 * @brief Compares two GPU descriptor handles.
 * 
 * @param [in] l, r The compared descriptors.
 * @return          \c true if both arguments handle different descriptors.
 */
inline bool operator!=(D3D12_GPU_DESCRIPTOR_HANDLE l, D3D12_GPU_DESCRIPTOR_HANDLE r) { return !(l == r); }

/**
 * @brief Creates a D3D12_VIEWPORT instance from D3D12_RECT instance.
 * 
 * @param [in] vp   The viewport to create from.
 * @return          The created instance.
 */
inline D3D12_RECT asRect(const D3D12_VIEWPORT &vp)
{
  D3D12_RECT rect;
  rect.left = vp.TopLeftX;
  rect.top = vp.TopLeftY;
  rect.right = vp.TopLeftX + vp.Width;
  rect.bottom = vp.TopLeftY + vp.Height;
  return rect;
}
#if !_TARGET_XBOXONE

/**
 * @brief Compares two D3D12_RECT instances.
 * 
 * @param [in] l, r The compared rectangles.
 * @return          \c true if both rectangles coincide, i.e. all of their corresponding corners have same coordinates
 */
inline bool operator==(const D3D12_RECT &l, const D3D12_RECT &r)
{
  return l.left == r.left && l.top == r.top && l.right == r.right && l.bottom == r.bottom;
}

/**
 * @brief Compares two D3D12_RECT instances.
 * 
 * @param [in] l, r The compared rectangles.
 * @return          \c true if both rectangles don't coincide, i.e. any of their corresponding corners have different coordinates.
 */
inline bool operator!=(const D3D12_RECT &l, const D3D12_RECT &r) { return !(l == r); }
#endif

/**
 * @brief Returns a string representation of a resource type.
 * 
 * @param [in] dim  The resource type.
 * @return          A pointer to the start of the buffer, storing the string representation the resource type.
 */
inline const char *to_string(D3D12_RESOURCE_DIMENSION dim)
{
  switch (dim)
  {
    default: return "<unknown>";
    case D3D12_RESOURCE_DIMENSION_UNKNOWN: return "unknown";
    case D3D12_RESOURCE_DIMENSION_BUFFER: return "buffer";
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D: return "texture 1D";
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D: return "texture 2D";
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return "texture 3D";
  }
}

/**
 * @brief Returns a string representation of a heap type.
 * 
 * @param [in] type     The heap type
 * @return              A pointer to the start of the buffer, storing the string representation of the heap type. 
 * 
 * In case \b type is not equal to one of D3D12_HEAP_TYPE enumerators, "??" is placed in the buffer.
 */
inline const char *to_string(D3D12_HEAP_TYPE type)
{
  switch (type)
  {
    case D3D12_HEAP_TYPE_DEFAULT: return "default";
    case D3D12_HEAP_TYPE_UPLOAD: return "upload";
    case D3D12_HEAP_TYPE_READBACK: return "read back";
    case D3D12_HEAP_TYPE_CUSTOM: return "custom";
  }
  return "??";
}

/**
 * @brief Returns a string representation of a CPU-page property.
 * 
 * @param [in] property The property CPU-page property
 * @return              A pointer to the start of the buffer, storing the string representation of the CPU-page property.
 * 
 * In case \b type is not one of D3D12_CPU_PAGE_PROPERTY enumerators, "??" is placed in the buffer.
 */
inline const char *to_string(D3D12_CPU_PAGE_PROPERTY property)
{
  switch (property)
  {
    case D3D12_CPU_PAGE_PROPERTY_UNKNOWN: return "unknown";
    case D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE: return "not available";
    case D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE: return "write combine";
    case D3D12_CPU_PAGE_PROPERTY_WRITE_BACK: return "write back";
  }
  return "??";
}

/**
 * @brief Returns a string representation of a memory pool specification.
 * 
 * @param [in] pool memory pool specification
 * @return          A pointer to the start of the buffer, storing the string representation of the memory pool specification.
 * 
 * In case \b type is not equal to one of D3D12_MEMORY_POOL enumerators, "??" is placed in the buffer.
 */
inline const char *to_string(D3D12_MEMORY_POOL pool)
{
  switch (pool)
  {
    case D3D12_MEMORY_POOL_UNKNOWN: return "unknown";
    case D3D12_MEMORY_POOL_L0: return "L0";
    case D3D12_MEMORY_POOL_L1: return "L1";
  }
  return "??";
}

/**
 * @brief Adds offset to a GPU descriptor handle.
 * 
 * @param [in] l    The descriptor handle to increment.
 * @param [in] r    The offset to increment by.
 * @return          The resulting handler.
 */
inline D3D12_GPU_DESCRIPTOR_HANDLE operator+(D3D12_GPU_DESCRIPTOR_HANDLE l, uint64_t r) { return {l.ptr + r}; }

/**
 * @brief Adds offset to a CPU descriptor handle.
 * 
 * @param [in] l    The descriptor handle to increment.
 * @param [in] r    The offset to increment by.
 * @return          The resulting handler.
 */
inline D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE l, size_t r) { return {l.ptr + r}; }

/**
 * @brief Returns string representation of a resource state mask.
 * 
 * @tparam N        The size of the character buffer
 * @param [in] res  The resource state mask.
 * @param [in] cbuf The address of the character buffer for the string.
 * @return          A pointer to the start of the buffer, storing the string representation of the resource state mask.
 */
template <size_t N>
inline char *resource_state_mask_as_string(D3D12_RESOURCE_STATES mask, char (&cbuf)[N])
{
  auto at = cbuf;
  auto ed = cbuf + N - 1;
  if (mask == D3D12_RESOURCE_STATE_COMMON)
  {
    at = append_literal(at, ed, "COMMON");
  }
  else
  {
// @cond
#define CHECK_MASK(name)                                                   \
  if (D3D12_RESOURCE_STATE_##name == (mask & D3D12_RESOURCE_STATE_##name)) \
  {                                                                        \
    at = append_or_mask_value_name(cbuf, at, ed, #name);                   \
    mask ^= D3D12_RESOURCE_STATE_##name;                                   \
  }
// @endcond
    // combined state, has to be first
    CHECK_MASK(GENERIC_READ)
    // single state
    CHECK_MASK(VERTEX_AND_CONSTANT_BUFFER)
    CHECK_MASK(INDEX_BUFFER)
    CHECK_MASK(RENDER_TARGET)
    CHECK_MASK(UNORDERED_ACCESS)
    CHECK_MASK(DEPTH_WRITE)
    CHECK_MASK(DEPTH_READ)
    CHECK_MASK(NON_PIXEL_SHADER_RESOURCE)
    CHECK_MASK(PIXEL_SHADER_RESOURCE)
    CHECK_MASK(STREAM_OUT)
    CHECK_MASK(INDIRECT_ARGUMENT)
    CHECK_MASK(COPY_DEST)
    CHECK_MASK(COPY_SOURCE)
    CHECK_MASK(RESOLVE_DEST)
    CHECK_MASK(RESOLVE_SOURCE)
#if !_TARGET_XBOXONE
    CHECK_MASK(RAYTRACING_ACCELERATION_STRUCTURE)
    CHECK_MASK(SHADING_RATE_SOURCE)
#endif
    CHECK_MASK(PREDICATION)
    CHECK_MASK(VIDEO_DECODE_READ)
    CHECK_MASK(VIDEO_DECODE_WRITE)
    CHECK_MASK(VIDEO_PROCESS_READ)
    CHECK_MASK(VIDEO_PROCESS_WRITE)
    CHECK_MASK(VIDEO_ENCODE_READ)
    CHECK_MASK(VIDEO_ENCODE_WRITE)
#undef CHECK_MASK
  }
  *at = '\0';
  return cbuf;
}

/**
 * @brief Overrides a primitive topology value depending on the primitive type interpreted by a geometry or hull shader.
 * 
 * @param [in] pt       The primitive type value.
 * @param [in] initial  The value of the primitive topology to override.
 * @return              The overriden primitive topology value.
 */
inline D3D12_PRIMITIVE_TOPOLOGY pimitive_type_to_primtive_topology(D3D_PRIMITIVE pt, D3D12_PRIMITIVE_TOPOLOGY initial)
{
  if (pt >= D3D_PRIMITIVE_1_CONTROL_POINT_PATCH && pt <= D3D_PRIMITIVE_32_CONTROL_POINT_PATCH)
    return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(
      D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + pt - D3D_PRIMITIVE_1_CONTROL_POINT_PATCH);
  return initial;
}

/**
 * @brief Converts a primitive topology value to a primitive topology type value.
 * 
 * @param [in] top  The primitive topology value to convert.
 * @return          The converted value.
 */
inline D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_to_topology_type(D3D12_PRIMITIVE_TOPOLOGY top)
{
  if (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST <= top && D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ >= top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  if (D3D_PRIMITIVE_TOPOLOGY_POINTLIST == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  if (D3D_PRIMITIVE_TOPOLOGY_LINELIST == top || D3D_PRIMITIVE_TOPOLOGY_LINESTRIP == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  if (D3D_PRIMITIVE_TOPOLOGY_UNDEFINED == top)
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
#if _TARGET_XBOX
  if (D3D_PRIMITIVE_TOPOLOGY_QUADLIST == top)
    return PRIMITIVE_TOPOLOGY_TYPE_QUAD;
#endif
  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
}

#if _TARGET_PC_WIN

/**
 * @brief Evaluates maximal values for the members of DXGI_QUERY_VIDEO_MEMORY_INFO instances.
 * 
 * @param [in] l, r The instances to evaluate.
 * @return          An instance containing maximal values of each member.
 *
 */
inline DXGI_QUERY_VIDEO_MEMORY_INFO max(const DXGI_QUERY_VIDEO_MEMORY_INFO &l, const DXGI_QUERY_VIDEO_MEMORY_INFO &r)
{
  DXGI_QUERY_VIDEO_MEMORY_INFO result;
  result.Budget = max(l.Budget, r.Budget);
  result.CurrentUsage = max(l.CurrentUsage, r.CurrentUsage);
  result.AvailableForReservation = max(l.AvailableForReservation, r.AvailableForReservation);
  result.CurrentReservation = max(l.CurrentReservation, r.CurrentReservation);
  return result;
}
#endif

/**
 * @brief Checks resource allocation info struct instance for errors.
 * 
 * @param [in] info The struct instance to check.
 * @return          \c true if no errors have been detected.
 */
inline bool is_valid_allocation_info(const D3D12_RESOURCE_ALLOCATION_INFO &info)
{
  // On error DX12 returns ~0 in the SizeInBytes member.
  return 0 != ~info.SizeInBytes;
}

/**
 * @brief Adjusts resource alignment.
 * 
 * @param [in] alignment    The current (\e small) alignment value.
 * @param [in] samples      The number of multisamples per pixel for the resource.
 * @return                  The adjusted (\e default) resource alignment value.
 *
 * The function takes into account the number of samples and raises alignment value from \e small up to \e default.
 * The adjusted value can be used as an argument for \c get_resource_allocation_info call in case 
 * the resource has not been granted \e small aligment.
 */
inline uint64_t get_next_resource_alignment(uint64_t alignment, uint32_t samples)
{
  if (D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT == alignment)
  {
    return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }
  if (samples > 1 && (D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT == alignment))
  {
    return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
  }

  return alignment;
}

/**
 * @brief Returns resource allocation information.
 * 
 * @param [in]      device  A pointer to the device object.
 * @param [in, out] desc    The resource description.
 * @return                  The struct instance storing the resource allocation information.
 *
 * \c desc should store the resource desired (\e small) alignment, but its value might be adjusted (to \e default) 
 * if not applicable to the resource. The adjusted value is assigned to \c desc.Alignment.
 */
inline D3D12_RESOURCE_ALLOCATION_INFO get_resource_allocation_info(ID3D12Device *device, D3D12_RESOURCE_DESC &desc)
{
  G_ASSERTF(desc.Alignment != 0, "DX12: desc.Alignment should not be 0!");
  auto result = device->GetResourceAllocationInfo(0, 1, &desc);
  if (!is_valid_allocation_info(result))
  {
    auto nextAlignment = get_next_resource_alignment(desc.Alignment, desc.SampleDesc.Count);
    if (nextAlignment != desc.Alignment)
    {
      desc.Alignment = nextAlignment;
      result = device->GetResourceAllocationInfo(0, 1, &desc);
    }
  }
  return result;
}

/**
 * @brief Returns the name of a DirectX resource data format.
 * 
 * @param [in] fmt  The resource data format.
 * @return          A pointer to the start of the buffer, storing the name of the format.
 */
const char *dxgi_format_name(DXGI_FORMAT fmt);
