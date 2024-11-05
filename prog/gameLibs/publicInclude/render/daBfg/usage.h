//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>


namespace dabfg
{

/**
 * \brief Describes the way a node uses a resource.
 * Part of the information required for daBfg to place barriers.
 */
enum class Usage : uint8_t
{
  /**
   * For legacy code only, will suppress a barrier before this usage
   * and force a barrier after this usage.
   */
  UNKNOWN,

  /// Regular render target
  COLOR_ATTACHMENT,

  /// Vulkan only: subpass input attachment
  INPUT_ATTACHMENT,

  /// Depth buffer
  DEPTH_ATTACHMENT,

  /**
   * A special read-only usage type that allows a texture to be used as
   * a read-only depth target as well as a sampled image simultaneously.
   */
  DEPTH_ATTACHMENT_AND_SHADER_RESOURCE,

  /// Vulkan only: automatic MSAA resolve inside a subpass
  RESOLVE_ATTACHMENT,

  /**
   * Any type of shader resource, i.e. a sampled image or a storage
   * buffer/texture in terms of vulkan and an SRV or a UAV in terms of
   * directX.
   */
  SHADER_RESOURCE,

  /**
   * Special type of shader resource that is limited in size but is
   * faster to read. Marked as `cbuffer` in shaders.
   */
  CONSTANT_BUFFER,

  /// Index buffer for a draw primitive
  INDEX_BUFFER,

  /// Vertex buffer for a draw primitive
  VERTEX_BUFFER,

  /// Either the source or the destination of a copy operation
  COPY,

  /// Either the source or the destination of a blit operation
  BLIT,

  /// A buffer used for an indirect draw/dispatch/raytrace primitive
  INDIRECTION_BUFFER,

  /// A texture used for determining variable shading rate for regions
  VRS_RATE_TEXTURE,
};

} // namespace dabfg
