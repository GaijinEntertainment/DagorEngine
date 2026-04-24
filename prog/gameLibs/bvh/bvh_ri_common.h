// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstExtra.h>

#include "bvh_context.h"
#include "bvh_math_util.h"
#include "bvh_color_from_pos.h"

namespace bvh::ri
{

using TreeMapper = bvh::ReferencedTransformData &(*)(ContextId context_id, uint64_t object_id, vec4f pos,
  rendinst::riex_handle_t handle, int lod_ix, uint64_t bvh_id, void *user_data, bool &recycled);

inline bool is_tree(ContextId context_id, ShaderMesh::RElem &elem)
{
  return context_id->has(Features::RIFull) && strncmp(elem.mat->getShaderClassName(), "rendinst_tree", 13) == 0;
}

inline bool is_leaves(ContextId context_id, ShaderMesh::RElem &elem)
{
  return context_id->has(Features::RIFull) && strncmp(elem.mat->getShaderClassName(), "rendinst_facing_leaves", 22) == 0;
}

inline bool is_flag(ContextId context_id, ShaderMesh::RElem &elem)
{
  return context_id->has(Features::RIFull) && strncmp(elem.mat->getShaderClassName(), "rendinst_flag", 13) == 0;
}

using map_tree_fn = ReferencedTransformData *(ContextId, uint64_t, vec4f, rendinst::riex_handle_t, int, void *, bool &, int &);

template <map_tree_fn mapper>
inline bool handle_tree(ContextId context_id, ShaderMesh::RElem &elem, uint64_t object_id, int lod_ix, bool is_pos_inst,
  mat44f_cref tm, vec4f_const originalPos, const E3DCOLOR *colors, rendinst::riex_handle_t handle,
  eastl::optional<TMatrix4> &inv_world_tm, TreeInfo &treeInfo, MeshMetaAllocator::AllocId &metaAllocId, void *user_data,
  bool stationary, bool is_burning, uint32_t palette_id)
{
  if (!inv_world_tm.has_value())
    inv_world_tm = make_packed_precise_itm(tm);

  auto data = mapper(context_id, object_id, tm.col3, handle, lod_ix, user_data, treeInfo.recycled, treeInfo.animIndex);

  treeInfo.stationary = stationary;

  if (stationary)
  {
    auto buffers = context_id->stationaryTreeBuffers.find(object_id);
    if (buffers == context_id->stationaryTreeBuffers.end())
      return false;

    treeInfo.recycled = false;
    treeInfo.stationary = true;
    data = &buffers->second;

    G_ASSERT(data->metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID);
  }
  else if (!data)
    return false;

  if (data->metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID)
    data->metaAllocId = context_id->allocateMetaRegion(1);

  metaAllocId = data->metaAllocId;

  treeInfo.invWorldTm = inv_world_tm.value();
  treeInfo.transformedBuffer = &data->buffer;
  treeInfo.transformedBlas = &data->blas;

  static int is_pivotedVarId = VariableMap::getVariableId("is_pivoted");
  static int small_plantVarId = VariableMap::getVariableId("small_plant");
  static int wind_channel_strengthVarId = VariableMap::getVariableId("wind_channel_strength");
  static int wind_noise_speed_baseVarId = VariableMap::getVariableId("wind_noise_speed_base");
  static int wind_noise_speed_level_mulVarId = VariableMap::getVariableId("wind_noise_speed_level_mul");
  static int wind_angle_rot_baseVarId = VariableMap::getVariableId("wind_angle_rot_base");
  static int wind_angle_rot_level_mulVarId = VariableMap::getVariableId("wind_angle_rot_level_mul");
  static int wind_per_level_angle_rot_maxVarId = VariableMap::getVariableId("wind_per_level_angle_rot_max");
  static int wind_parent_contribVarId = VariableMap::getVariableId("wind_parent_contrib");
  static int wind_motion_damp_baseVarId = VariableMap::getVariableId("wind_motion_damp_base");
  static int wind_motion_damp_level_mulVarId = VariableMap::getVariableId("wind_motion_damp_level_mul");
  static int AnimWindScaleVarId = VariableMap::getVariableId("AnimWindScale");
  static int atestVarId = VariableMap::getVariableId("atest");
  static int ground_snap_height_softVarId = VariableMap::getVariableId("ground_snap_height_soft");
  static int ground_snap_height_fullVarId = VariableMap::getVariableId("ground_snap_height_full");
  static int ground_snap_normal_offsetVarId = VariableMap::getVariableId("ground_snap_normal_offset");
  static int ground_snap_limitVarId = VariableMap::getVariableId("ground_snap_limit");
  static int ground_bend_heightVarId = VariableMap::getVariableId("ground_bend_height");
  static int ground_bend_normal_offsetVarId = VariableMap::getVariableId("ground_bend_normal_offset");
  static int ground_bend_tangent_offsetVarId = VariableMap::getVariableId("ground_bend_tangent_offset");

  int atest = 0;
  elem.mat->getIntVariable(atestVarId, atest);
  bool isTrunk = atest == 0; // rendinst_tree_perlin_layered doesn't have this shadervar

  if (is_burning && !isTrunk)
    return false; // Just drop all burning tree leaves from BVH, eventually they'll disappear entirely anyway

  int smallPlant = 0;
  if (elem.mat->getIntVariable(small_plantVarId, smallPlant) && smallPlant > 0)
    return false;

  int isPivoted;
  if (!elem.mat->getIntVariable(is_pivotedVarId, isPivoted))
    isPivoted = 0;
  if (!elem.mat->getColor4Variable(wind_channel_strengthVarId, *reinterpret_cast<Color4 *>(&treeInfo.data.windChannelStrength)))
    treeInfo.data.windChannelStrength = Point4(1, 1, 0, 1);
  if (!elem.mat->getColor4Variable(wind_per_level_angle_rot_maxVarId,
        *reinterpret_cast<Color4 *>(&treeInfo.data.ppWindPerLevelAngleRotMax)))
    treeInfo.data.ppWindPerLevelAngleRotMax = Point4(60, 60, 60, 60);
  if (!elem.mat->getRealVariable(wind_noise_speed_baseVarId, treeInfo.data.ppWindNoiseSpeedBase))
    treeInfo.data.ppWindNoiseSpeedBase = 0.1;
  if (!elem.mat->getRealVariable(wind_noise_speed_level_mulVarId, treeInfo.data.ppWindNoiseSpeedLevelMul))
    treeInfo.data.ppWindNoiseSpeedLevelMul = 1.666;
  if (!elem.mat->getRealVariable(wind_angle_rot_baseVarId, treeInfo.data.ppWindAngleRotBase))
    treeInfo.data.ppWindAngleRotBase = 5;
  if (!elem.mat->getRealVariable(wind_angle_rot_level_mulVarId, treeInfo.data.ppWindAngleRotLevelMul))
    treeInfo.data.ppWindAngleRotLevelMul = 5;
  if (!elem.mat->getRealVariable(wind_parent_contribVarId, treeInfo.data.ppWindParentContrib))
    treeInfo.data.ppWindParentContrib = 0.25;
  if (!elem.mat->getRealVariable(wind_motion_damp_baseVarId, treeInfo.data.ppWindMotionDampBase))
    treeInfo.data.ppWindMotionDampBase = 3;
  if (!elem.mat->getRealVariable(wind_motion_damp_level_mulVarId, treeInfo.data.ppWindMotionDampLevelMul))
    treeInfo.data.ppWindMotionDampLevelMul = 0.8;
  if (!elem.mat->getRealVariable(AnimWindScaleVarId, treeInfo.data.AnimWindScale))
    treeInfo.data.AnimWindScale = 0.25;
  if (!elem.mat->getRealVariable(ground_snap_height_softVarId, treeInfo.data.groundSnapHeightSoft))
    treeInfo.data.groundSnapHeightSoft = -1;
  if (!elem.mat->getRealVariable(ground_snap_height_fullVarId, treeInfo.data.groundSnapHeightFull))
    treeInfo.data.groundSnapHeightFull = 0.5;
  if (!elem.mat->getRealVariable(ground_snap_normal_offsetVarId, treeInfo.data.groundSnapNormalOffset))
    treeInfo.data.groundSnapNormalOffset = 0.1;
  if (!elem.mat->getRealVariable(ground_snap_limitVarId, treeInfo.data.groundSnapLimit))
    treeInfo.data.groundSnapLimit = 1.0;
  if (!elem.mat->getRealVariable(ground_bend_heightVarId, treeInfo.data.groundBendHeight))
    treeInfo.data.groundBendHeight = -1;
  if (!elem.mat->getRealVariable(ground_bend_normal_offsetVarId, treeInfo.data.groundBendNormalOffset))
    treeInfo.data.groundBendNormalOffset = 0.075;
  if (!elem.mat->getRealVariable(ground_bend_tangent_offsetVarId, treeInfo.data.groundBendTangentOffset))
    treeInfo.data.groundBendTangentOffset = 4;
  treeInfo.data.apply_tree_wind = !isTrunk;

  // It will be filled later from mesh!
  treeInfo.data.ppPositionBindless = 0xFFFFFFFFU;
  treeInfo.data.ppDirectionBindless = 0xFFFFFFFFU;

  treeInfo.data.isPivoted = !!isPivoted;
  treeInfo.data.isPosInstance = is_pos_inst;

  const E3DCOLOR WHITE = E3DCOLOR(0x40404040U);
  if (isTrunk)
    treeInfo.data.color = is_burning ? E3DCOLOR(0x10101010U) : WHITE;
  else
    treeInfo.data.color = colors ? random_color_from_pos(originalPos, palette_id, colors[0], colors[1]) : WHITE;
  if (treeInfo.data.isPivoted && !treeInfo.data.isPosInstance)
    treeInfo.data.perInstanceRenderAdditionalData = rendinst::getRiExtraPerInstanceRenderEncodedAdditionalData(handle);
  else
    treeInfo.data.perInstanceRenderAdditionalData = 0;

  return true;
}

float get_ri_lod_dist_bias();

void debug_update();

} // namespace bvh::ri