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
 * \brief Describes a GPU pipeline stage at which a resource is accessed.
 * Part of the information required for daFG to place barriers.
 * Note that this is a flags enum, so you can combine multiple stages.
 */
enum class Stage : uint8_t
{
  /**
   * For legacy code only. Will suppress a barrier before this usage
   * and force a barrier after this usage.
   */
  UNKNOWN = 0,

  /**
   * All pre-rasterization stages: indirect draw reading, vertex input,
   * vertex shader, geometry shader, tesselation shaders, mesh shaders.
   */
  PRE_RASTER = 1u << 0,

  /**
   * All post-rasterization stages: early tests, fragment shader,
   * late tests, attachment output.
   */
  POST_RASTER = 1u << 1,

  /// Copute dispatches
  COMPUTE = 1u << 2,

  /// Blit and copy operations
  TRANSFER = 1u << 3,

  /// All raytrace shaders
  RAYTRACE = 1u << 4,

  /**
   * Aliases for simple use-cases. Note however that even though all PS
   * usages are POST_RASTER, but not all POST_RASTER usages are from
   * the pixel shader (PS). Use with caution so as not to confuse the
   * reader of your code.
   */
  ///@{
  PS = POST_RASTER,
  VS = PRE_RASTER,
  CS = COMPUTE,
  PS_OR_CS = PS | CS,
  ///@}

  /// Anything graphics-related
  ALL_GRAPHICS = PRE_RASTER | POST_RASTER,

  /// Reads from inderection buffers can only happen during one of these stages.
  ALL_INDIRECTION = PRE_RASTER | COMPUTE | RAYTRACE,
};

constexpr inline Stage operator|(Stage a, Stage b) { return static_cast<Stage>(eastl::to_underlying(a) | eastl::to_underlying(b)); }

constexpr inline Stage &operator|=(Stage &a, Stage b)
{
  a = a | b;
  return a;
}

constexpr inline Stage operator&(Stage a, Stage b) { return static_cast<Stage>(eastl::to_underlying(a) & eastl::to_underlying(b)); }

constexpr inline Stage &operator&=(Stage &a, Stage b)
{
  a = a & b;
  return a;
}

constexpr inline Stage operator~(Stage a) { return static_cast<Stage>(~eastl::to_underlying(a)); }

constexpr inline bool stagesSubsetOf(Stage subset, Stage superset) { return (subset & superset) == subset; }

eastl::string to_string(Stage stage);

} // namespace dafg
