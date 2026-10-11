//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/lightsBase.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMathDecl.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <3d/dag_resPtr.h>

class Sbuffer;
class LightsPartition;

class LightsSorter final : public IDynLightsPipelineSection
{
  friend class LightsPartition;

public:
  LightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights);
  void init(DynLightsFeatures requested_features) override;
  DynLightsFeatures getRuntimeFeatures() const override;
  void sortLightsCPU(Tab<uint16_t> &omni_visible_ids, Tab<uint16_t> &spot_visible_ids, vec4f cur_view_pos);
  void sortLightsGPU(Sbuffer *data_buf, Sbuffer *counts_buf, bool update_variables = true);

private:
  void bindStageBuffers();
  void unbindStageBuffers();

  void dispatchPrepareSort();
  void dispatchSortImpl(Sbuffer *data_buf);
  void dispatchFinalizeSort();

  OmniLightsManager *omniLights;
  SpotLightsManager *spotLights;

  ComputeShader prepareSortCS;
  ComputeShader sortWaveImplCS;
  ComputeShader sortScalarBasicImplCS;
  ComputeShader sortScalarMediumImplCS;
  ComputeShader sortScalarHighImplCS;
  ComputeShader sortBatchedImplCS;
  ComputeShader finalizeSortCS;

  UniqueBuf sortDispatchArgsBuf;
  UniqueBuf sortLightsStageDataBuf;
};
