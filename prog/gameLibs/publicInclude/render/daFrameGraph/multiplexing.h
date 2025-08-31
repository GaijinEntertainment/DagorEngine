//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>
#include <math/integer/dag_IPoint2.h>

/**
 * \brief Multiplexing allows a single run_nodes call to result in
 * several consequent executions of frameGraph with some nodes being
 * run only once.
 * As an example, when rendering for VR, shadows need to be rendered
 * only once, not for every eye/viewport, while most scene geometry
 * should be rendered for every viewport/eye.
 */
namespace dafg::multiplexing
{

/**
 * \brief A struct describing the amount of multiplexing that occurs
 * over every dimension.
 * \details The application is free to interpret these dimensions
 * however it pleases, but the intended interpretation is
 * - superSamples -- used when rendering the entire scene multiple times
 *   to achieve a higher resolution result than the user's VRAM would
 *   allow otherwise, which is useful for screenshots and cinematics;
 * - subSamples -- used for rendering the entire scene multiple times
 *   and then blending the results to achieve SSAA without spending any
 *   additional VRAM required;
 * - viewports -- used for VR/AR.
 */
struct Extents
{
  uint32_t superSamples{1}; ///< Number of super-samples.
  uint32_t subSamples{1};   ///< Number of sub-samples.
  uint32_t viewports{1};    ///< Number of viewports.
  uint32_t subCameras{1};   ///< Number of main camera areas.

  /// \brief Equality comparison operator.
  bool operator==(const Extents &other) const = default;
};

/**
 * \brief Identifies a concrete iteration of multiplexing.
 * \details Can be accessed from an execution callback by simply adding
 * an argument of this type to it. It is then possible to achieve
 * different behavior for different eyes or sub/super samples.
 */
struct Index
{
  uint32_t superSample{0}; ///< Index of the super-sample.
  uint32_t subSample{0};   ///< Index of the sub-sample.
  uint32_t viewport{0};    ///< Index of the viewport.
  uint32_t subCamera{0};   ///< Index of the camera in camera area.

  /// \brief Equality comparison operator.
  bool operator==(const Index &other) const = default;
};

/// \brief Describes the dimensions along which a node shall be multiplexed.
enum class Mode : uint32_t
{
  /// Run once, ignore multiplexing (i.e. global setup)
  None = 0b000,

  /// Run the node for every super-sample
  SuperSampling = 0b001,
  /// Run the node for every sub-sample
  SubSampling = 0b010,
  /// Run the node once for every viewport
  Viewport = 0b100,
  /// Run the node for every sub area in the main camera
  CameraInCamera = 0b1000,


  /// Default: everything is multiplexed (normal scene rendering nodes)
  FullMultiplex = SuperSampling | SubSampling | Viewport | CameraInCamera,
};

/// \brief Bitwise OR operator for combining several modes.
inline Mode operator|(Mode a, Mode b) { return static_cast<Mode>(eastl::to_underlying(a) | eastl::to_underlying(b)); }

/// \brief Bitwise AND operator to check if mode set.
inline Mode operator&(Mode a, Mode b) { return static_cast<Mode>(eastl::to_underlying(a) & eastl::to_underlying(b)); }

} // namespace dafg::multiplexing
