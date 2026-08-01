//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_hlsl_floatx.h>
#include <render/lights/reallocatableLightsConstBuffer.h>
#include <render/lights/renderLights.hlsli>
#include <render/lights/lightsResources.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>

class ShaderMaterial;
class ShaderElement;

// Draws proxy geometry (sphere for omni, cone for spot) for far/clustered lights via
// indirect draws, swapping in whichever light data buffer is being rendered.
struct LightsRenderer
{
  using OmniLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderOmniLight) / 16, true>;
  using SpotLightsCB = ReallocatableLightsConstBuffer<sizeof(RenderSpotLight) / 16, true>;

  // Bundles far/clustered CB refs for a single call. The buffers are owned by the caller
  // (ClusteredLights); LightsRenderer does not keep these pointers past the call.
  struct OmniLightsCBs
  {
    const OmniLightsCB *far = nullptr;
    const OmniLightsCB *clustered = nullptr;
  };
  struct SpotLightsCBs
  {
    const SpotLightsCB *far = nullptr;
    const SpotLightsCB *clustered = nullptr;
  };

  LightsRenderer(const LightsResourcesManager *lights_res_mgr);
  ~LightsRenderer();

  void init();
  void close();
  void beforeResetDevice();
  void afterResetDevice();

  void copyInstanceCountsToIndirectArgs(const OmniLightsCBs &omni_lights_cb, const SpotLightsCBs &spot_lights_cb);

  void renderFarOmniLights(const OmniLightsCB &far_omni_lights_cb);
  void renderFarSpotLights(const SpotLightsCB &far_spot_lights_cb);
  void renderDebugOmniLights(const OmniLightsCBs &omni_lights_cb);
  void renderDebugSpotLights(const SpotLightsCBs &spot_lights_cb);

private:
  enum IndirectIndices : size_t
  {
    OMNI_CLUSTERED = 0,
    OMNI_FAR = 1,
    SPOT_CLUSTERED = 2,
    SPOT_FAR = 3,
    COUNT = 4
  };

  void renderPrims(ShaderElement *elem, int buffer_var_id, D3DRESID lights_cb_id, IndirectIndices argsIndex);

  void initConeSphere();
  void closeConeSphere();
  void initOmni();
  void initSpot();
  void initDebugOmni();
  void initDebugSpot();
  void closeOmni();
  void closeSpot();
  void closeDebugOmni();
  void closeDebugSpot();

  const char *getResName(const char *name) const;

  ShaderMaterial *pointLightsMat = nullptr, *pointLightsDebugMat = nullptr;
  ShaderElement *pointLightsElem = nullptr, *pointLightsDebugElem = nullptr;
  ShaderMaterial *spotLightsMat = nullptr, *spotLightsDebugMat = nullptr;
  ShaderElement *spotLightsElem = nullptr, *spotLightsDebugElem = nullptr;

  uint32_t v_count = 0, f_count = 0;
  UniqueBuf coneSphereVb;
  UniqueBuf coneSphereIb;

  UniqueBuf indirectArgsBuf;

  int omniLightsVarId = -1;
  int spotLightsVarId = -1;

  const LightsResourcesManager *lightsResMgr = nullptr;
};
