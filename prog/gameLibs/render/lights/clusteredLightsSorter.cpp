// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/clusteredLightsSorter.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>
#include <shaders/dag_shaderVar.h>
#include <render/lights/renderLightsConsts.hlsli>

static int lights_sort_view_pos_varId = -1;
static int omni_lights_structured_buf_varId = -1;
static int spot_lights_structured_buf_varId = -1;

static void init_sort_shader_vars()
{
  lights_sort_view_pos_varId = get_shader_variable_id("lights_sort_view_pos", true);
  omni_lights_structured_buf_varId = get_shader_variable_id("omni_lights_structured_buf", true);
  spot_lights_structured_buf_varId = get_shader_variable_id("spot_lights_structured_buf", true);
}

ClusteredLightsSorter::ClusteredLightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights) :
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

void ClusteredLightsSorter::sortOmniLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  sort_lights_by_distance(*omniLights, visible_ids, cur_view_pos);
}

void ClusteredLightsSorter::sortSpotLightsCPU(Tab<uint16_t> &visible_ids, vec4f cur_view_pos)
{
  sort_lights_by_distance(*spotLights, visible_ids, cur_view_pos);
}

static void dispatch_bitonic_sort(ComputeShader &cs, Sbuffer *buf, int buf_var_id, int light_count, const Point3 &view_pos,
  int max_lights_count)
{
  G_ASSERTF((max_lights_count & (max_lights_count - 1)) == 0, "max_lights_count must be a power of two, got %d", max_lights_count);
  G_ASSERTF(light_count <= max_lights_count, "light_count %d exceeds max_lights_count %d", light_count, max_lights_count);

  if (!cs)
    return;
  if (light_count <= 1)
    return;

  ShaderGlobal::set_float4(lights_sort_view_pos_varId, view_pos.x, view_pos.y, view_pos.z, 0.f);
  ShaderGlobal::set_buffer(buf_var_id, buf);
  cs.dispatchThreads(max_lights_count / 2, 1, 1);
  ShaderGlobal::set_buffer(buf_var_id, BAD_D3DRESID);
}

void ClusteredLightsSorter::sortOmniLightsGPU(Sbuffer *buf, const Point3 &view_pos, int light_count)
{
  dispatch_bitonic_sort(sortOmniCS, buf, omni_lights_structured_buf_varId, light_count, view_pos, MAX_OMNI_LIGHTS);
}

void ClusteredLightsSorter::sortSpotLightsGPU(Sbuffer *buf, const Point3 &view_pos, int light_count)
{
  dispatch_bitonic_sort(sortSpotCS, buf, spot_lights_structured_buf_varId, light_count, view_pos, MAX_SPOT_LIGHTS);
}
