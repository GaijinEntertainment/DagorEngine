// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstExtra.h>

#include "bvh_context.h"

namespace bvh::ri
{

using TreeMapper = bvh::ReferencedTransformData &(*)(ContextId context_id, uint64_t mesh_id, vec4f pos, rendinst::riex_handle_t handle,
  int lod_ix, uint64_t bvh_id, void *user_data, bool &recycled);

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

template <TreeMapper mapper>
inline void handle_tree(ContextId context_id, ShaderMesh::RElem &elem, uint64_t mesh_id, int lod_ix, bool is_pos_inst, mat44f_cref tm,
  const E3DCOLOR *colors, rendinst::riex_handle_t handle, eastl::optional<TMatrix4> &inv_world_tm, TreeInfo &treeInfo,
  MeshMetaAllocator::AllocId &metaAllocId, uint64_t bvh_id, void *user_data)
{
  if (!inv_world_tm.has_value())
  {
    inv_world_tm = TMatrix4::IDENT;

    mat44f inv44;
    v_mat44_inverse(inv44, tm);
    v_mat44_transpose(inv44, inv44);
    v_stu(&inv_world_tm.value().row[0], inv44.col0);
    v_stu(&inv_world_tm.value().row[1], inv44.col1);
    v_stu(&inv_world_tm.value().row[2], inv44.col2);
    v_stu(&inv_world_tm.value().row[3], inv44.col3);
  }

  auto &data = mapper(context_id, mesh_id, tm.col3, handle, lod_ix, bvh_id, user_data, treeInfo.recycled);

  if (data.metaAllocId < 0)
    data.metaAllocId = context_id->allocateMetaRegion();

  metaAllocId = data.metaAllocId;

  treeInfo.invWorldTm = inv_world_tm.value();
  treeInfo.transformedBuffer = &data.buffer;
  treeInfo.transformedBlas = &data.blas;

  static int is_pivotedVarId = VariableMap::getVariableId("is_pivoted");
  static int wind_channel_strengthVarId = VariableMap::getVariableId("wind_channel_strength");
  static int tree_wind_branch_ampVarId = VariableMap::getVariableId("tree_wind_branch_amp");
  static int tree_wind_detail_ampVarId = VariableMap::getVariableId("tree_wind_detail_amp");
  static int tree_wind_speedVarId = VariableMap::getVariableId("tree_wind_speed");
  static int tree_wind_timeVarId = VariableMap::getVariableId("tree_wind_time");
  static int tree_wind_blend_params1VarId = VariableMap::getVariableId("tree_wind_blend_params1");
  static int wind_noise_speed_baseVarId = VariableMap::getVariableId("wind_noise_speed_base");
  static int wind_noise_speed_level_mulVarId = VariableMap::getVariableId("wind_noise_speed_level_mul");
  static int wind_angle_rot_baseVarId = VariableMap::getVariableId("wind_angle_rot_base");
  static int wind_angle_rot_level_mulVarId = VariableMap::getVariableId("wind_angle_rot_level_mul");
  static int wind_per_level_angle_rot_maxVarId = VariableMap::getVariableId("wind_per_level_angle_rot_max");
  static int wind_parent_contribVarId = VariableMap::getVariableId("wind_parent_contrib");
  static int wind_motion_damp_baseVarId = VariableMap::getVariableId("wind_motion_damp_base");
  static int wind_motion_damp_level_mulVarId = VariableMap::getVariableId("wind_motion_damp_level_mul");

  treeInfo.data.windBranchAmp = ShaderGlobal::get_real(tree_wind_branch_ampVarId);
  treeInfo.data.windDetailAmp = ShaderGlobal::get_real(tree_wind_detail_ampVarId);
  treeInfo.data.windSpeed = ShaderGlobal::get_real(tree_wind_speedVarId);
  treeInfo.data.windTime = ShaderGlobal::get_real(tree_wind_timeVarId);
  treeInfo.data.windBlendParams = ShaderGlobal::get_color4(tree_wind_blend_params1VarId);

  int isPivoted;
  if (!elem.mat->getIntVariable(is_pivotedVarId, isPivoted))
    isPivoted = 0;
  if (!elem.mat->getColor4Variable(wind_channel_strengthVarId, treeInfo.data.windChannelStrength))
    treeInfo.data.windChannelStrength = Color4(1, 1, 1, 1);
  if (!elem.mat->getColor4Variable(wind_per_level_angle_rot_maxVarId, treeInfo.data.ppWindPerLevelAngleRotMax))
    treeInfo.data.ppWindPerLevelAngleRotMax = Color4(60, 60, 60, 60);
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

  treeInfo.data.ppPosition = elem.mat->get_texture(7);
  treeInfo.data.ppDirection = elem.mat->get_texture(8);

  treeInfo.data.isPivoted = !!isPivoted;
  treeInfo.data.isPosInstance = is_pos_inst;

  treeInfo.data.color = colors ? random_color_from_pos(tm.col3, colors[0], colors[1]) : E3DCOLOR(0x40404040U);
  if (treeInfo.data.isPivoted && !treeInfo.data.isPosInstance)
    treeInfo.data.perInstanceRenderDataOffset = rendinst::getRiExtraPerInstanceRenderDataOffset(handle);
  else
    treeInfo.data.perInstanceRenderDataOffset = 0;
}

} // namespace bvh::ri