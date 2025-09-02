//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>
#include <EASTL/string.h>
#include <util/dag_stdint.h>

namespace dafg
{

/**
 * \brief Describes the way a node uses a resource.
 * Part of the information required for daFG to place barriers.
 */
enum class Usage : uint16_t
{
  /**
   * For legacy code only, will suppress a barrier before this usage
   * and force a barrier after this usage.
   */
  UNKNOWN = 0,

  /// Regular render target
  COLOR_ATTACHMENT = 1u << 0,

  /// Depth buffer
  DEPTH_ATTACHMENT = 1u << 1,

  /// Vulkan only: automatic MSAA resolve inside a subpass
  RESOLVE_ATTACHMENT = 1u << 2,

  /**
   * Any type of shader resource, i.e. a sampled image or a storage
   * buffer/texture in terms of vulkan and an SRV or a UAV in terms of
   * directX.
   */
  SHADER_RESOURCE = 1u << 3,

  /**
   * Special type of shader resource that is limited in size but is
   * faster to read. Marked as `cbuffer` in shaders.
   */
  CONSTANT_BUFFER = 1u << 4,

  /// Index buffer for a draw primitive
  INDEX_BUFFER = 1u << 5,

  /// Vertex buffer for a draw primitive
  VERTEX_BUFFER = 1u << 6,

  /// Either the source or the destination of a copy operation
  COPY = 1u << 7,

  /// Either the source or the destination of a blit operation
  BLIT = 1u << 8,

  /// A buffer used for an indirect draw/dispatch/raytrace primitive
  INDIRECTION_BUFFER = 1u << 9,

  /// A texture used for determining variable shading rate for regions
  VRS_RATE_TEXTURE = 1u << 10,

  /// Vulkan only: subpass input attachment
  INPUT_ATTACHMENT = COLOR_ATTACHMENT | SHADER_RESOURCE,

  /**
   * A special read-only usage type that allows a texture to be used as
   * a read-only depth target as well as a sampled image simultaneously.
   */
  DEPTH_ATTACHMENT_AND_SHADER_RESOURCE = DEPTH_ATTACHMENT | SHADER_RESOURCE,
};

constexpr inline Usage operator|(Usage a, Usage b) { return static_cast<Usage>(eastl::to_underlying(a) | eastl::to_underlying(b)); }

constexpr inline Usage &operator|=(Usage &a, Usage b)
{
  a = a | b;
  return a;
}

constexpr inline Usage operator&(Usage a, Usage b) { return static_cast<Usage>(eastl::to_underlying(a) & eastl::to_underlying(b)); }

eastl::string to_string(dafg::Usage usage);


} // namespace dafg
