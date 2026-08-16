// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsSorter.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <shaders/dag_shaderVar.h>
#include <render/lights/renderLightsConsts.hlsli>

static int visible_clustered_omni_lights_structured_buf_varId = -1;
static int visible_clustered_spot_lights_structured_buf_varId = -1;
static int visible_clustered_omni_lights_structured_buf_count_varId = -1;
static int visible_clustered_spot_lights_structured_buf_count_varId = -1;
static int sort_lights_param_varId = -1;

static void init_sort_shader_vars()
{
  visible_clustered_omni_lights_structured_buf_varId = get_shader_variable_id("visible_clustered_omni_lights_structured_buf", true);
  visible_clustered_spot_lights_structured_buf_varId = get_shader_variable_id("visible_clustered_spot_lights_structured_buf", true);
  visible_clustered_omni_lights_structured_buf_count_varId =
    get_shader_variable_id("visible_clustered_omni_lights_structured_buf_count", true);
  visible_clustered_spot_lights_structured_buf_count_varId =
    get_shader_variable_id("visible_clustered_spot_lights_structured_buf_count", true);
  sort_lights_param_varId = get_shader_variable_id("sort_lights_param", true);
}

LightsSorter::LightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights) :
  omniLights(&omni_lights), spotLights(&spot_lights), sortOmniCS("sort_omni_lights_cs", true), sortSpotCS("sort_spot_lights_cs", true)
{
  init_sort_shader_vars();
}

template <typename LightsManager>
static void sort_lights_by_distance(LightsManager &lights, Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  stlsort::sort(visible_ids.begin(), visible_ids.end(), [&lights, cur_view_pos](uint16_t i, uint16_t j) {
    const vec3f diffI = v_sub(cur_view_pos, lights.getBoundingSphere(i));
    const vec3f diffJ = v_sub(cur_view_pos, lights.getBoundingSphere(j));
    return v_test_vec_x_lt(v_dot3_x(diffI, diffI), v_dot3_x(diffJ, diffJ));
  });
}

void LightsSorter::sortOmniLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  sort_lights_by_distance(*omniLights, visible_ids, cur_view_pos);
}

void LightsSorter::sortSpotLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  sort_lights_by_distance(*spotLights, visible_ids, cur_view_pos);
}

// sort_omni/spot_lights_cs (lights_partition.dshl) does the entire bitonic sort itself in one
// dispatch of a single threadgroup: it reads the real count from visible_clustered_lights_buf_count,
// pads it up to a power of two on the GPU, and sorts in groupshared memory. max_lights_count is
// only used here to check it matches the shader's kMaxLightsCount (the type's scene capacity,
// which sizes the shader's groupshared array) - it no longer drives a CPU-side dispatch loop.
static void dispatch_bitonic_sort(ComputeShader &cs, Sbuffer *buf, int buf_var_id, Sbuffer *count_buf, int count_buf_var_id,
  const Point3 &view_pos, float zfar, int max_lights_count)
{
  G_ASSERTF((max_lights_count & (max_lights_count - 1)) == 0, "max_lights_count must be a power of two, got %d", max_lights_count);
  // max_lights_count doubles as the "invalid" scene index sentinel packed into the low 16 bits of
  // the shader's gs_sort_data (see lights_partition.dshl) - it must fit there.
  G_ASSERTF(max_lights_count <= 0xFFFF, "max_lights_count must fit in 16 bits, got %d", max_lights_count);
  G_ASSERT(zfar > 0.f);

  if (!cs)
    return;

  ShaderGlobal::set_float4(sort_lights_param_varId, view_pos, zfar);
  ShaderGlobal::set_buffer(buf_var_id, buf);
  ShaderGlobal::set_buffer(count_buf_var_id, count_buf);

  cs.dispatchThreads(1, 1, 1);
  d3d::resource_barrier({buf, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  ShaderGlobal::set_buffer(buf_var_id, BAD_D3DRESID);
  ShaderGlobal::set_buffer(count_buf_var_id, BAD_D3DRESID);
}

// buf/count_buf are the visible_clustered_omni/spot_lights_structured_buf index array and its
// count (as filled by partition_omni/spot_lights_cs); the caller must still separately bind the
// matching scene_omni/spot_lights_structured_buf.
void LightsSorter::sortOmniLightsGPU(Sbuffer *buf, Sbuffer *count_buf, const Point3 &view_pos, float zfar)
{
  dispatch_bitonic_sort(sortOmniCS, buf, visible_clustered_omni_lights_structured_buf_varId, count_buf,
    visible_clustered_omni_lights_structured_buf_count_varId, view_pos, zfar, MAX_SCENE_OMNI_LIGHTS);
}

void LightsSorter::sortSpotLightsGPU(Sbuffer *buf, Sbuffer *count_buf, const Point3 &view_pos, float zfar)
{
  dispatch_bitonic_sort(sortSpotCS, buf, visible_clustered_spot_lights_structured_buf_varId, count_buf,
    visible_clustered_spot_lights_structured_buf_count_varId, view_pos, zfar, MAX_SCENE_SPOT_LIGHTS);
}
