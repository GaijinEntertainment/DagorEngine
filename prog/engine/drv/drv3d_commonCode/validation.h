// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Contains common validation routines

// Usually its best to align stuff to 16 bytes - eg one float4
constexpr unsigned OPTIMAL_STRUCTURE_ALIGN = 16;
// On VK this could be even optionally 2 or 1 with proper extensions
constexpr unsigned MINIMAL_STRUCTURE_ALIGN = 4;

// Returns true if the structure size follows the basic alignment rules for structured buffer.
//
// Buffers will work when this returns false, but they are probably less performant and on some
// platforms it requires either the shader compiler to restructure the structure layout (VK) or on
// others is requires the memory manager to waste memory to properly align the buffer in memory
// (DX12).
inline bool is_good_buffer_structure_size(unsigned size)
{
  if (OPTIMAL_STRUCTURE_ALIGN < size)
    return 0 == (size % OPTIMAL_STRUCTURE_ALIGN);
  else if (MINIMAL_STRUCTURE_ALIGN < size)
    return 0 == (size % MINIMAL_STRUCTURE_ALIGN);
  return true;
}

#define ENABLE_GENERIC_RENDER_PASS_VALIDATION (DAGOR_DBGLEVEL > 0)

#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
#define VALIDATE_GENERIC_RENDER_PASS_CONDITION(condition, message) \
  do                                                               \
  {                                                                \
    if (!(condition))                                              \
    {                                                              \
      LOGERR_ONCE(message);                                        \
    }                                                              \
  } while (false)
#else
#define VALIDATE_GENERIC_RENDER_PASS_CONDITION(condition, message) ((void)0)
#endif

#if defined(__d3dcommon_h__) || defined(__d3d11_x_h__) || defined(__d3d12_xs_h__)
// Check is a primitive topology is valid when certain shader stages are active or not
// NOTE For some platforms the api/driver will ignore draw calls when this returns false
//      on other platforms this will lead to a device lost state and crash.
// Direct3D version
inline bool validate_primitive_topology(D3D_PRIMITIVE_TOPOLOGY topology, bool has_geometry_stage, bool has_tesselation_stage)
{
  switch (topology)
  {
#if defined(__d3d11_x_h__) || defined(__d3d12_xs_h__)
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN:
    case D3D_PRIMITIVE_TOPOLOGY_RECTLIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINELOOP:
    case D3D_PRIMITIVE_TOPOLOGY_QUADLIST:
    case D3D_PRIMITIVE_TOPOLOGY_QUADSTRIP: return !has_tesselation_stage;
#endif
    default:
    case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED: return false;
    case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP: return !has_tesselation_stage;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ: return !has_tesselation_stage && has_geometry_stage;
    case D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST: return has_tesselation_stage;
  }
}
#endif

#ifdef VK_HEADER_VERSION
// Check is a primitive topology is valid when certain shader stages are active or not
// NOTE For some platforms the api/driver will ignore draw calls when this returns false
//      on other platforms this will lead to a device lost state and crash.
// Vulkan version
inline bool validate_primitive_topology(VkPrimitiveTopology topology, bool has_geometry_stage, bool has_tesselation_stage)
{
  switch (topology)
  {
    default: return false;
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return !has_tesselation_stage;
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: return !has_tesselation_stage && has_geometry_stage;
    case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST: return has_tesselation_stage;
  }
}
#endif
