// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsPartition.h>
#include <render/lights/lightsEncoding.h>
#include <scene/dag_occlusion.h>
#include <EASTL/type_traits.h>
#include <generic/dag_align.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_resourceTags.h>
#include "smallLights.h"

LightsPartition::LightsPartition(OmniLightsManager &omni_lights, SpotLightsManager &spot_lights,
  const LightsResourcesManager *lights_res_mgr) :
  omniLights(&omni_lights), spotLights(&spot_lights), lightsResMgr(lights_res_mgr), lightsSorter(omni_lights, spot_lights)
{}

void LightsPartition::init()
{
  if (VariableMap::isVariablePresent(VariableMap::getVariableId("spot_lights_flags")))
  {
    static constexpr uint32_t spotMaskSizeInDwords = dag::divide_align_up(MAX_CLUSTERED_SPOT_LIGHTS, 4);
    visibleClusteredSpotLightsMasksSB = dag::buffers::create_one_frame_sr_byte_address(spotMaskSizeInDwords,
      lightsResMgr->getResName("spot_lights_flags"), RESTAG_LIGHTS);
  }

  if (VariableMap::isVariablePresent(VariableMap::getVariableId("omni_lights_flags")))
  {
    static constexpr uint32_t omniMaskSizeInDwords = dag::divide_align_up(MAX_CLUSTERED_OMNI_LIGHTS, 4);
    visibleClusteredOmniLightsMasksSB = dag::buffers::create_one_frame_sr_byte_address(omniMaskSizeInDwords,
      lightsResMgr->getResName("omni_lights_flags"), RESTAG_LIGHTS);
  }

  visibleClusteredOmniLightsCB.reallocate(0, MAX_CLUSTERED_OMNI_LIGHTS, lightsResMgr->getResName("clustered_omni_lights"), true);
  visibleClusteredOmniLightsCB.update(nullptr, 0);
  visibleClusteredSpotLightsCB.reallocate(0, MAX_CLUSTERED_SPOT_LIGHTS, lightsResMgr->getResName("clustered_spot_lights"), true);
  visibleClusteredSpotLightsCB.update(nullptr, 0);

  visibleFarOmniLightsCB.reallocate(0, MAX_VISIBLE_FAR_LIGHTS, lightsResMgr->getResName("far_omni_lights"), true);
  visibleFarOmniLightsCB.update(nullptr, 0);
  visibleFarSpotLightsCB.reallocate(0, MAX_VISIBLE_FAR_LIGHTS, lightsResMgr->getResName("far_spot_lights"), true);
  visibleFarSpotLightsCB.update(nullptr, 0);
}

template <typename LightsManager>
void LightsPartition::executeLightsCPUPartition(const LightsManager *lights_manager, const Frustum &frustum,
  Tab<uint16_t> &lights_inside, Tab<uint16_t> &lights_outside, eastl::bitset<LightsManager::MAX_LIGHTS> *visible_id_bitset,
  Occlusion *occlusion, vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos,
  typename LightsManager::MaskType require_any_mask, float cutoff_dist_sq)
{

  lights_inside.clear();
  lights_outside.clear();
  if (visible_id_bitset)
    visible_id_bitset->reset();

  const int maxIdx = lights_manager->maxIndex();
  const int reserveSize = (maxIdx + 1) / 2;
  lights_inside.reserve(reserveSize);
  lights_outside.reserve(reserveSize);

  vec3f cutoff_dist_sq_v = cutoff_dist_sq > 0 ? v_splats(cutoff_dist_sq) : V_C_INF;

  for (int i = 0; i <= maxIdx; ++i)
  {
    if (require_any_mask && !(require_any_mask & lights_manager->masks[i]))
      continue;

    const typename LightsManager::RawLight &l = lights_manager->rawLights[i];
    if (l.pos_radius.w <= 0)
      continue;

    vec4f lightPosRad = lights_manager->getBoundingSphere(i);
    vec3f rad = v_splat_w(lightPosRad);

    if (!frustum.testSphereB(lightPosRad, rad))
      continue;

    if (occlusion)
    {
      if constexpr (eastl::is_same_v<LightsManager, OmniLightsManager>)
      {
        if (occlusion->isOccludedSphere(lightPosRad, rad))
          continue;
      }
      else if constexpr (eastl::is_same_v<LightsManager, SpotLightsManager>)
      {
        if (occlusion->isOccludedBox(lights_manager->boundingBoxes[i]))
          continue;
      }
    }

    if (visible_id_bitset)
      visible_id_bitset->set(i, true);


    vec3f radScaled = rad;
    if constexpr (eastl::is_same_v<LightsManager, OmniLightsManager>)
    {
      radScaled = v_mul_x(v_splats(1.1f), rad);
    }

    vec4f res = v_add_x(v_sub_x(v_dot3_x(lightPosRad, znear_plane), radScaled), v_splat_w(znear_plane));
    vec4f length_sq = v_length3_sq(v_sub(camera_pos, lightPosRad));

    if (v_test_vec_x_gt(length_sq, cutoff_dist_sq_v))
      continue;
    vec4f camInSphereVec = v_sub_x(length_sq, v_mul(rad, rad));

#if _TARGET_SIMD_SSE
    bool intersectsNear = _mm_movemask_ps(res) & 1;
    bool camInSphere = _mm_movemask_ps(camInSphereVec) & 1;
#else
    bool intersectsNear = v_test_vec_x_lt_0(res);
    bool camInSphere = v_test_vec_x_lt_0(camInSphereVec);
#endif

    const bool small = lights_manager->getShadowId(i) == BaseLightsManager::INVALID_SHADOW_VOLUME_ID &&
                       is_viewed_small(lightPosRad, length_sq, mark_small_lights_as_far_limit);

    if ((intersectsNear || small) && !camInSphere)
      lights_inside.push_back(i);
    else
      lights_outside.push_back(i);
  };
};

template <typename TRenderLight>
static void update_render_lights_const_buffer(int elem_count, int max_count, Tab<TRenderLight> &render_lights,
  ReallocatableLightsConstBuffer<sizeof(TRenderLight) / sizeof(vec4f), true> &cb, bool persistent, const char *name,
  const LightsResourcesManager *lights_res_mgr)
{
  G_ASSERT(elem_count <= render_lights.size());
  G_ASSERT(sizeof(TRenderLight) % sizeof(vec4f) == 0);

  cb.reallocate(elem_count, max_count, lights_res_mgr->getResName(name), persistent);
  cb.update(render_lights.data(), elem_count * sizeof(TRenderLight));
};

template <typename TMaskType>
static void update_masks_buffer(int elem_count, int max_count, Tab<TMaskType> &masks_list, Sbuffer *buf, TMaskType stub_mask_value)
{
  if (buf)
  {
    const TMaskType stubMask[1] = {stub_mask_value};
    G_ASSERT(masks_list.size() <= ((max_count + 3) & ~3));
    dag::Span<const TMaskType> masks = elem_count > 0 ? make_span_const(masks_list) : make_span_const(stubMask);
    // bound & used framemem buffer must be updated every frame
    buf->updateDataWithLock(0, data_size(masks), masks.data(), VBLOCK_DISCARD);
  }
};


void LightsPartition::executeOmniLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane,
  Tab<uint16_t> &lights_outside_plane, eastl::bitset<OmniLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occlusion,
  vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos, OmniLightMaskType require_any_mask,
  float cutoff_dist_sq) const
{
  executeLightsCPUPartition<OmniLightsManager>(omniLights, frustum, lights_inside_plane, lights_outside_plane, visible_id_bitset,
    occlusion, znear_plane, mark_small_lights_as_far_limit, camera_pos, require_any_mask, cutoff_dist_sq);
};

void LightsPartition::executeOmniLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside,
  Tab<uint16_t> &lights_outside, Occlusion *occlusion, vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos,
  OmniLightMaskType require_any_mask, float cutoff_dist_sq) const
{
  executeOmniLightsCPUPartition(frustum, lights_inside, lights_outside, nullptr, occlusion, znear_plane,
    mark_small_lights_as_far_limit, camera_pos, require_any_mask, cutoff_dist_sq);
};

void LightsPartition::executeSpotLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane,
  Tab<uint16_t> &lights_outside_plane, eastl::bitset<SpotLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occ,
  vec4f znear_plane, float mark_small_lights_as_far_limit, vec3f camera_pos, SpotLightMaskType require_any_mask,
  float cutoff_dist_sq) const
{
  executeLightsCPUPartition<SpotLightsManager>(spotLights, frustum, lights_inside_plane, lights_outside_plane, visible_id_bitset, occ,
    znear_plane, mark_small_lights_as_far_limit, camera_pos, require_any_mask, cutoff_dist_sq);
};

void LightsPartition::executeSpotLightsCPUPartition(const Frustum &frustum, Tab<uint16_t> &lights_inside_plane,
  Tab<uint16_t> &lights_outside_plane, eastl::bitset<SpotLightsManager::MAX_LIGHTS> *visible_id_bitset, Occlusion *occ,
  vec4f znear_plane, SpotLightMaskType require_any_mask, float cutoff_dist_sq)
{
  executeLightsCPUPartition<SpotLightsManager>(spotLights, frustum, lights_inside_plane, lights_outside_plane, visible_id_bitset, occ,
    znear_plane, 0, v_zero(), require_any_mask, cutoff_dist_sq);
};

void LightsPartition::executeFrustumOmniLightsCPUPartition(const Frustum &frustum, Occlusion *occlusion, vec4f znear_plane,
  float mark_small_lights_as_far_limit, vec3f camera_pos, OmniLightMaskType require_any_mask, float cutoff_dist_sq)
{
  executeLightsCPUPartition<OmniLightsManager>(omniLights, frustum, visibleFarOmniLightsIds, visibleClusteredOmniLightsIds,
    &visibleOmniLightsIdSet, occlusion, znear_plane, mark_small_lights_as_far_limit, camera_pos, require_any_mask, cutoff_dist_sq);

  lightsSorter.sortOmniLightsCPU(visibleClusteredOmniLightsIds, camera_pos);
  trimVisibleLightsIdLists(visibleClusteredOmniLightsIds, visibleFarOmniLightsIds, MAX_CLUSTERED_OMNI_LIGHTS);
  fillDerivativeLightsLists(omniLights, visibleClusteredOmniLightsIds, visibleFarOmniLightsIds, renderOmniLightsClustered,
    renderOmniLightsFar, visibleOmniLightsMasks, visibleClusteredOmniLightsBounds, OmniLightMaskType::OMNI_LIGHT_MASK_NONE);
}

void LightsPartition::executeFrustumSpotLightsCPUPartition(const Frustum &frustum, Occlusion *occ, vec4f znear_plane,
  float mark_small_lights_as_far_limit, vec3f camera_pos, SpotLightMaskType require_any_mask, float cutoff_dist_sq)
{
  executeLightsCPUPartition<SpotLightsManager>(spotLights, frustum, visibleFarSpotLightsIds, visibleClusteredSpotLightsIds,
    &visibleSpotLightsIdSet, occ, znear_plane, mark_small_lights_as_far_limit, camera_pos, require_any_mask, cutoff_dist_sq);

  lightsSorter.sortSpotLightsCPU(visibleClusteredSpotLightsIds, camera_pos);
  trimVisibleLightsIdLists(visibleClusteredSpotLightsIds, visibleFarSpotLightsIds, MAX_CLUSTERED_SPOT_LIGHTS);
  fillDerivativeLightsLists(spotLights, visibleClusteredSpotLightsIds, visibleFarSpotLightsIds, renderSpotLightsClustered,
    renderSpotLightsFar, visibleSpotLightsMasks, visibleClusteredSpotLightsBounds, SpotLightMaskType::SPOT_LIGHT_MASK_NONE);
}

void LightsPartition::trimVisibleLightsIdLists(Tab<uint16_t> &clustered_lights, Tab<uint16_t> &far_lights, int max_clustered_count)
{
  if (clustered_lights.size() > max_clustered_count)
  {
    auto excessSize = clustered_lights.size() - max_clustered_count;
    append_items(far_lights, excessSize, clustered_lights.begin() + max_clustered_count);
  }
  clustered_lights.resize(min(int(clustered_lights.size()), int(max_clustered_count)));
  far_lights.resize(min<int>(far_lights.size(), MAX_VISIBLE_FAR_LIGHTS));
}

template <typename LightsManager>
void LightsPartition::fillDerivativeLightsLists(const LightsManager *lights_manager, const Tab<uint16_t> &clustered_lights_id,
  const Tab<uint16_t> &far_lights_ids, Tab<typename LightsManager::RenderLight> &clustered_render_lights,
  Tab<typename LightsManager::RenderLight> &far_render_lights, Tab<typename LightsManager::MaskType> &clustered_lights_masks,
  Tab<vec4f> &clustered_lights_bounds, typename LightsManager::MaskType default_mask_type)
{
  clustered_render_lights.resize(clustered_lights_id.size());
  clustered_lights_masks.resize((clustered_lights_id.size() + 3) & ~3);
  clustered_lights_bounds.resize(clustered_lights_id.size());

  far_render_lights.resize(far_lights_ids.size());
  for (int i = 0, e = far_lights_ids.size(); i < e; ++i)
    far_render_lights[i] = lights_manager->getRenderLight(far_lights_ids[i]);

  for (int i = 0, e = clustered_lights_id.size(); i < e; ++i)
  {
    uint32_t id = clustered_lights_id[i];
    clustered_render_lights[i] = lights_manager->getRenderLight(id);
    clustered_lights_bounds[i] = lights_manager->getBoundingSphere(id);
    clustered_lights_masks[i] = lights_manager->getLightMask(id);
  }

  for (int i = clustered_lights_id.size(), e = (clustered_lights_id.size() + 3) & ~3; i < e; ++i)
    clustered_lights_masks[i] = default_mask_type;
}

void LightsPartition::close()
{
  visibleClusteredSpotLightsMasksSB.close();
  visibleClusteredOmniLightsMasksSB.close();

  visibleClusteredOmniLightsCB.close();
  visibleFarOmniLightsCB.close();
  visibleClusteredSpotLightsCB.close();
  visibleFarSpotLightsCB.close();
}

void LightsPartition::updateBuffersForVisibleFarLights()
{
  update_render_lights_const_buffer(renderSpotLightsFar.size(), MAX_VISIBLE_FAR_LIGHTS, renderSpotLightsFar, visibleFarSpotLightsCB,
    false, "far_spot_lights", lightsResMgr);
  update_render_lights_const_buffer(renderOmniLightsFar.size(), MAX_VISIBLE_FAR_LIGHTS, renderOmniLightsFar, visibleFarOmniLightsCB,
    false, "far_omni_lights", lightsResMgr);
}

void LightsPartition::updateBuffersForVisibleClusteredLights(int omni_count, int spot_count)
{
  // FIXME: (workaround) buffers are persistent as it referenced by volume lights and eye caustics when data is not updated in
  // clustered lights
  update_render_lights_const_buffer(spot_count, MAX_CLUSTERED_SPOT_LIGHTS, renderSpotLightsClustered, visibleClusteredSpotLightsCB,
    true, "clustered_spot_lights", lightsResMgr);
  update_render_lights_const_buffer(omni_count, MAX_CLUSTERED_OMNI_LIGHTS, renderOmniLightsClustered, visibleClusteredOmniLightsCB,
    true, "clustered_omni_lights", lightsResMgr);


  update_masks_buffer(omni_count, MAX_CLUSTERED_OMNI_LIGHTS, visibleOmniLightsMasks, visibleClusteredOmniLightsMasksSB.getBuf(),
    OmniLightMaskType::OMNI_LIGHT_MASK_NONE);
  update_masks_buffer(spot_count, MAX_CLUSTERED_SPOT_LIGHTS, visibleSpotLightsMasks, visibleClusteredSpotLightsMasksSB.getBuf(),
    SpotLightMaskType::SPOT_LIGHT_MASK_NONE);
}

const Tab<uint16_t> &LightsPartition::getVisibleClusteredSpotLightsIds() const { return visibleClusteredSpotLightsIds; }
const Tab<uint16_t> &LightsPartition::getVisibleClusteredOmniLightsIds() const { return visibleClusteredOmniLightsIds; }

const Tab<RenderOmniLight> &LightsPartition::getRenderOmniLightsFar() const { return renderOmniLightsFar; }
const Tab<RenderSpotLight> &LightsPartition::getRenderSpotLightsFar() const { return renderSpotLightsFar; }

const Tab<vec4f> &LightsPartition::getVisibleClusteredSpotLightsBounds() const { return visibleClusteredSpotLightsBounds; }
const Tab<vec4f> &LightsPartition::getVisibleClusteredOmniLightsBounds() const { return visibleClusteredOmniLightsBounds; }

const LightsPartition::OmniLightsCB &LightsPartition::getVisibleClusteredOmniLightsCB() const { return visibleClusteredOmniLightsCB; }
const LightsPartition::OmniLightsCB &LightsPartition::getVisibleFarOmniLightsCB() const { return visibleFarOmniLightsCB; }
const LightsPartition::SpotLightsCB &LightsPartition::getVisibleClusteredSpotLightsCB() const { return visibleClusteredSpotLightsCB; }
const LightsPartition::SpotLightsCB &LightsPartition::getVisibleFarSpotLightsCB() const { return visibleFarSpotLightsCB; }

const UniqueBuf &LightsPartition::getVisibleClusteredSpotLightsMasksSB() const { return visibleClusteredSpotLightsMasksSB; }
const UniqueBuf &LightsPartition::getVisibleClusteredOmniLightsMasksSB() const { return visibleClusteredOmniLightsMasksSB; }


bool LightsPartition::isLightVisible(uint32_t id) const
{
  const auto typeId = LightsEncoder::decodeLightId(id);
  switch (typeId.type)
  {
    case LightType::Spot: G_ASSERT_RETURN(typeId.id <= spotLights->maxIndex(), false); return visibleSpotLightsIdSet.test(typeId.id);
    case LightType::Omni: G_ASSERT_RETURN(typeId.id <= omniLights->maxIndex(), false); return visibleOmniLightsIdSet.test(typeId.id);
    case LightType::Invalid: return false;
    default: G_ASSERT_FAIL("unknown light type");
  }
  return false;
}