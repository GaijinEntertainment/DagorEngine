// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsSorter.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <render/lights/renderLightsConsts.hlsli>
#include <render/lights/lightsSorter.hlsli>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resourceTags.h>
#include <util/dag_string.h>

#define LIGHTS_SORTER_GLOBAL_VARS_LIST \
  VAR(sort_lights_dispatch_args_buf)   \
  VAR(sort_lights_stage_data_buf)      \
  VAR(visible_lights_data_buf)         \
  VAR(visible_lights_counts_buf)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
LIGHTS_SORTER_GLOBAL_VARS_LIST
#undef VAR
#undef LIGHTS_SORTER_GLOBAL_VARS_LIST

LightsSorter::LightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights) :
  omniLights(&omni_lights), spotLights(&spot_lights)
{}

void LightsSorter::init(DynLightsFeatures requested_features)
{
  if (static_cast<bool>(requested_features & DynLightsFeatures::GpuLightsSort))
  {
    prepareSortCS = ComputeShader("prepare_sort_lights_cs", true);
    sortWaveImplCS = ComputeShader("sort_lights_in_wave_cs", true);
    sortScalarBasicImplCS = ComputeShader("sort_scalar_basic_impl_lights_cs", true);
    sortScalarMediumImplCS = ComputeShader("sort_scalar_medium_impl_lights_cs", true);
    sortScalarHighImplCS = ComputeShader("sort_scalar_high_impl_lights_cs", true);
    sortBatchedImplCS = ComputeShader("sort_batched_high_impl_lights_cs", true);
    finalizeSortCS = ComputeShader("finalize_sort_lights_cs", true);

    static uint32_t sort_lights_dispatch_args_buf_counter = 0;
    String argsBufName, stageDataBufName;
    argsBufName.printf(0, "lights_sorter_dispatch_args_buf_%u", sort_lights_dispatch_args_buf_counter);
    stageDataBufName.printf(0, "lights_sorter_stage_data_buf_%u", sort_lights_dispatch_args_buf_counter++);
    sortDispatchArgsBuf = dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, SORT_LIGHTS_DISPATCH_ARGS_RECORDS_COUNT,
      argsBufName, RESTAG_LIGHTS);
    sortLightsStageDataBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), LIGHTS_SORTER_STAGE_DATA_RECORDS_COUNT,
      stageDataBufName, d3d::buffers::Init::No, RESTAG_LIGHTS);
  }
}

DynLightsFeatures LightsSorter::getRuntimeFeatures() const
{
  const bool isGpuPathConfigured = (prepareSortCS && sortScalarBasicImplCS && sortScalarMediumImplCS && sortScalarHighImplCS &&
                                    sortBatchedImplCS && finalizeSortCS && sortDispatchArgsBuf && sortLightsStageDataBuf);
  return isGpuPathConfigured ? DynLightsFeatures::GpuLightsSort : DynLightsFeatures::CpuOnly;
}

template <typename LightsManager>
static void sort_lights_by_distance(LightsManager &lights, Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  TIME_D3D_PROFILE(lightsSorter_cpu_impl);
  stlsort::sort(visible_ids.begin(), visible_ids.end(), [&lights, cur_view_pos](uint16_t i, uint16_t j) {
    const vec3f diffI = v_sub(cur_view_pos, lights.getBoundingSphere(i));
    const vec3f diffJ = v_sub(cur_view_pos, lights.getBoundingSphere(j));
    return v_test_vec_x_lt(v_dot3_x(diffI, diffI), v_dot3_x(diffJ, diffJ));
  });
}

void LightsSorter::sortLightsCPU(Tab<uint16_t> &omni_visible_ids, Tab<uint16_t> &spot_visible_ids, vec4f cur_view_pos)
{
  TIME_D3D_PROFILE(lightsSorter_cpu_sortLights);
  sort_lights_by_distance(*omniLights, omni_visible_ids, cur_view_pos);
  sort_lights_by_distance(*spotLights, spot_visible_ids, cur_view_pos);
}

void LightsSorter::bindStageBuffers()
{
  ShaderGlobal::set_buffer(sort_lights_dispatch_args_bufVarId, sortDispatchArgsBuf.getBuf());
  ShaderGlobal::set_buffer(sort_lights_stage_data_bufVarId, sortLightsStageDataBuf.getBuf());
}

void LightsSorter::unbindStageBuffers()
{
  ShaderGlobal::set_buffer(sort_lights_dispatch_args_bufVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(sort_lights_stage_data_bufVarId, BAD_D3DRESID);
}

void LightsSorter::dispatchPrepareSort()
{
  if (!prepareSortCS)
    return;

  prepareSortCS.dispatchThreads(MAX_SCENE_OMNI_LIGHTS + MAX_SCENE_SPOT_LIGHTS, 1, 1);
}

void LightsSorter::dispatchSortImpl(Sbuffer *data_buf)
{
  if (!sortScalarBasicImplCS || !sortScalarMediumImplCS || !sortScalarHighImplCS || !sortBatchedImplCS)
    return;

  Sbuffer *dispatch_args_buf = sortDispatchArgsBuf.getBuf();

  if (sortWaveImplCS)
  {
    sortWaveImplCS.dispatchIndirect(dispatch_args_buf, SORT_LIGHTS_DISPATCH_ARGS_WAVE_OFFSET);
    d3d::resource_barrier({data_buf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  }

  sortScalarBasicImplCS.dispatchIndirect(dispatch_args_buf, SORT_LIGHTS_DISPATCH_ARGS_BASIC_OFFSET);
  d3d::resource_barrier({data_buf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  sortScalarMediumImplCS.dispatchIndirect(dispatch_args_buf, SORT_LIGHTS_DISPATCH_ARGS_MEDIUM_OFFSET);
  d3d::resource_barrier({data_buf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  sortScalarHighImplCS.dispatchIndirect(dispatch_args_buf, SORT_LIGHTS_DISPATCH_ARGS_SCALAR_HIGH_OFFSET);
  sortBatchedImplCS.dispatchIndirect(dispatch_args_buf, SORT_LIGHTS_DISPATCH_ARGS_BATCHED_HIGH_OFFSET);
}

void LightsSorter::dispatchFinalizeSort()
{
  if (!finalizeSortCS)
    return;

  finalizeSortCS.dispatchIndirect(sortDispatchArgsBuf.getBuf(), SORT_LIGHTS_DISPATCH_ARGS_FINALIZE_OFFSET);
}

void LightsSorter::sortLightsGPU(Sbuffer *data_buf, Sbuffer *counts_buf, bool update_variables)
{
  TIME_D3D_PROFILE(lightsSorter_gpu_sortLights);

  Sbuffer *argsBuf = sortDispatchArgsBuf.getBuf();

  if (update_variables)
  {
    bindStageBuffers();
    ShaderGlobal::set_buffer(visible_lights_data_bufVarId, data_buf);
    ShaderGlobal::set_buffer(visible_lights_counts_bufVarId, counts_buf);
  }

  {
    TIME_D3D_PROFILE(lightsSorter_gpu_prepare);
    dispatchPrepareSort();
  }

  d3d::resource_barrier({{data_buf, counts_buf, sortLightsStageDataBuf.getBuf(), argsBuf},
    {RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE,
      RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, RB_RO_INDIRECT_BUFFER}});

  {
    TIME_D3D_PROFILE(lightsSorter_gpu_impl);
    dispatchSortImpl(data_buf);
  }

  d3d::resource_barrier({data_buf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  {
    TIME_D3D_PROFILE(lightsSorter_gpu_finalize);
    dispatchFinalizeSort();
  }

  if (update_variables)
  {
    unbindStageBuffers();
    ShaderGlobal::set_buffer(visible_lights_data_bufVarId, BAD_D3DRESID);
    ShaderGlobal::set_buffer(visible_lights_counts_bufVarId, BAD_D3DRESID);
  }
}
