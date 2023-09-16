//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <EASTL/variant.h>
#include <3d/dag_drv3d_buffers.h>


namespace dabfg
{

/**
 * \brief Named slots do not represent any resource by default and must be
 * explicitly set to point to a different resource through dabfg::fill_slot
 */
struct NamedSlot
{
  /// String name for this slot, must not match any other resource's name
  const char *name;

  /// Enables using regular parentheses for construction
  explicit NamedSlot(const char *slot_name) : name(slot_name) {}
};

/**
 * \brief This structure is used for specifying a daBfg-managed
 * resolution to a 2D texture. The resulting resolution at runtime
 * will be the dynamic resolution scaled by the multiplier, but the
 * consumed memory will always be equal to the static resolution times
 * the multiplier.
 * See dabfg::set_resolution.
 */
struct AutoResolution
{
  /// A string name for this resolution
  const char *type = nullptr;
  /// The multiplier for this particular resource
  float multiplier = 1.f;
};

/**
 * \brief Special value for Texture2dCreateInfo::mipLevels. daBfg will
 * automatically generate all mip levels for such a texture.
 */
inline constexpr int AUTO_MIP_COUNT = 0;

/**
 * \brief Specifies the creation parameters for a 2D texture.
 * Note that resources will ALWAYS contain garbage right after they've
 * been created. Use virtual passes to clear them if you need to,
 * or write explicit clears.
 */
struct Texture2dCreateInfo
{
  /// Use TEXCF_ prefixed flags
  uint32_t creationFlags = 0;
  /// See \ref AutoResolution
  eastl::variant<IPoint2, AutoResolution> resolution;
  /// Use 0 for automatic mip levels
  uint32_t mipLevels = 1;
};

/**
 * \details An effort to create a less error-prone buffers API is currently
 * ongoing, this structure exposes the to-be-deprecated
 * \inlinerst :cpp:func:`d3d::create_sbuffer` \endrst buffer creation
 * mechanism. You are on your own trying to figure out how to use this.
 */
struct BufferCreateInfo
{
  /// Size of a single element in bytes
  uint32_t elementSize;
  /// Amount of elements in this buffer
  uint32_t elementCount;
  /// Use SBCF_ flags here
  uint32_t flags;
  /**
   * This is for so-called T-buffers, which are basically 1d textures
   * masquerading as buffers. If you are not targeting DX10, don't use these
   */
  uint32_t format;
};

} // namespace dabfg
