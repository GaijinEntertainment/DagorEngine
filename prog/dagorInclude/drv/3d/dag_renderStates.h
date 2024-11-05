//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>

#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_renderStateId.h>
#include <math/dag_e3dColor.h>
#include <shaders/dag_overrideStates.h>

namespace shaders
{

/**
 * @brief Description of render state.
 */
struct RenderStateBits //-V730
{
  uint32_t zwrite : 1;            ///< Enable depth write.
  uint32_t ztest : 1;             ///< Enable depth test.
  uint32_t zFunc : 4;             ///< Relationship operation #CMPF used for depth test (less, greater, equal and etc.).
  uint32_t stencilRef : 8;        ///< Reference value to perform against when doing a stencil test.
  uint32_t cull : 2;              ///< Specifies triangle face culling #CULL_TYPE.
  uint32_t depthBoundsEnable : 1; ///< Enable depth-bounds testing. Samples outside the range are discarded.
  /// \brief The sample count that is forced while UAV rendering or rasterizing.
  /// \warning Requires caps.hasForcedSamplerCount (DX 11.1).
  /// \warning Depth test must be disabled.
  uint32_t forcedSampleCount : 4;
  /// \brief Identifies whether conservative rasterization is on or off.
  /// \warning Requires caps.hasConservativeRassterization (DX 11.3).
  /// \warning Only triangle topology is allowed (DX 12).
  uint32_t conservativeRaster : 1;
  uint32_t zClip : 1;          ///< Enable depth clipping based on distance.
  uint32_t scissorEnabled : 1; ///< Enable scissor-rectangle culling. All pixels outside an active scissor rectangle are culled.
  uint32_t independentBlendEnabled : 1; ///< Enable independent blending in simultaneous render targets.
  uint32_t alphaToCoverage : 1;         ///< Specifies whether to use alpha-to-coverage.
  uint32_t viewInstanceCount : 2;       ///< Specifies the number of used views.
  uint32_t colorWr = 0xFFFFFFFF;        ///< Specifies mask which is used for color write.
  float zBias = 0;                      ///< Specifies depth value added to a given pixel.
  float slopeZBias = 0;                 ///< Specifies scalar on a given pixel's slope.
  StencilState stencil;                 ///< Specifies stencil state.
};

/**
 * @brief Extended render state with blending params.
 */
struct RenderState : public RenderStateBits
{
  // increase if more independent blend parameters are needed
  static constexpr uint32_t NumIndependentBlendParameters = 4;

  /**
   * @brief Description of blending factors.
   */
  struct BlendFactors
  {
    uint8_t src : 4; ///< Configures source alpha #BLEND_FACTOR.
    uint8_t dst : 4; ///< Configures destination alpha #BLEND_FACTOR.
  };
  /**
   * @brief Description of blending parameters.
   */
  struct BlendParams
  {
    BlendFactors ablendFactors;    ///< Configures alpha blend factor.
    BlendFactors sepablendFactors; ///< Configures separate alpha blend factor.

    uint8_t blendOp : 3;     ///< Specifies #BLENDOP operation for alpha blending.
    uint8_t sepablendOp : 3; ///< Specifies #BLENDOP operation for separate alpha blending.
    uint8_t ablend : 1;      ///< Enables alpha blending.
    uint8_t sepablend : 1;   ///< Enables separate alpha blending.
  };

  BlendParams blendParams[NumIndependentBlendParameters];

  // ensure that no padding occured
  static_assert(sizeof(BlendParams) == 3);
  static_assert(sizeof(blendParams) == NumIndependentBlendParameters * sizeof(BlendParams));

  RenderState()
  {
    memset(this, 0, sizeof(RenderState));
    zwrite = 1;
    ztest = 1;
    zFunc = CMPF_GREATEREQUAL;
    stencilRef = 0;
    for (BlendParams &blendParam : blendParams)
    {
      blendParam.ablendFactors.src = BLEND_ONE;
      blendParam.ablendFactors.dst = BLEND_ZERO;
      blendParam.sepablendFactors.src = BLEND_ONE;
      blendParam.sepablendFactors.dst = BLEND_ZERO;
      blendParam.blendOp = BLENDOP_ADD;
      blendParam.sepablendOp = BLENDOP_ADD;
      // done by memset with zero
      // blendParam.ablend = 0;
      // blendParam.sepablend = 0;
    }
    cull = CULL_NONE;
    depthBoundsEnable = 0;
    forcedSampleCount = 0;
    conservativeRaster = 0;
    viewInstanceCount = 0;
    colorWr = 0xFFFFFFFF;
    zClip = 1;
    alphaToCoverage = 0;
    independentBlendEnabled = 0;
  }

  bool operator==(const RenderState &s) const
  {
    if (auto r = memcmp(static_cast<const RenderStateBits *>(this), static_cast<const RenderStateBits *>(&s), //-V1014
          sizeof(RenderStateBits));
        r != 0)
      return false;

    size_t blendParamBytesToCompare = independentBlendEnabled ? sizeof(blendParams) : sizeof(BlendParams);
    return memcmp(blendParams, s.blendParams, blendParamBytesToCompare) == 0;
  }
  bool operator!=(const RenderState &s) const { return !(*this == s); }
};
} // namespace shaders

namespace d3d
{
/**
 * @brief Set the blend factor object
 *
 * @param color blend factor to set
 * @return true if successful, false otherwise
 */
bool set_blend_factor(E3DCOLOR color);

/**
 * @brief Set the stencil reference value
 *
 * @param ref reference value to set
 * @return true if successful, false otherwise
 */
bool setstencil(uint32_t ref);

/**
 * @brief Set the wireframe mode. Works only in dev build.
 *
 * @param in true to enable wireframe mode, false to disable
 * @return true if successful, false otherwise
 */
bool setwire(bool in);

/**
 * @brief Set the depth bounds
 *
 * @param zmin minimum depth value
 * @param zmax maximum depth value
 * @return true if successful, false otherwise
 */
bool set_depth_bounds(float zmin, float zmax);

/**
 * @brief Create a render state object in driver
 *
 * @param state parameters of the render state object to create
 * @return shaders::DriverRenderStateId id of the created render state object
 */
shaders::DriverRenderStateId create_render_state(const shaders::RenderState &state);

/**
 * @brief Set the render state object in driver
 *
 * @param state_id id of the render state object to set
 * @return true if successful, false otherwise
 */
bool set_render_state(shaders::DriverRenderStateId state_id);

/**
 * @brief Remove all render state objects allocated in driver
 */
void clear_render_states();
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool set_blend_factor(E3DCOLOR color) { return d3di.set_blend_factor(color); }
inline bool setstencil(uint32_t ref) { return d3di.setstencil(ref); }
inline bool setwire(bool in) { return d3di.setwire(in); }
inline bool set_depth_bounds(float zmin, float zmax) { return d3di.set_depth_bounds(zmin, zmax); }
inline shaders::DriverRenderStateId create_render_state(const shaders::RenderState &state) { return d3di.create_render_state(state); }
inline bool set_render_state(shaders::DriverRenderStateId state_id) { return d3di.set_render_state(state_id); }
inline void clear_render_states() { d3di.clear_render_states(); }
} // namespace d3d
#endif
