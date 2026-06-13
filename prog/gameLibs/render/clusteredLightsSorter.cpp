// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/clusteredLightsSorter.h>
#include <util/dag_stlqsort.h>
#include <vecmath/dag_vecMath.h>

ClusteredLightsSorter::ClusteredLightsSorter(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights) :
  omniLights(&omni_lights), spotLights(&spot_lights)
{}

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
