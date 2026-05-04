//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <dag/dag_vector.h>
#include <generic/dag_carray.h>
#include <shaders/dag_computeShaders.h>
#include <render/toroidalVoxelGrid.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

class Point3;

class VoxelShadows
{
public:
  static constexpr int maxCascadeCount = 4;

  struct CascadeSettings
  {
    uint32_t sparseDiv = 2; // 0 = no sparse, only toroidal, 1 - full update every frame, N - 1 dithered update in NxNxN box
    float temporalSpeed = 0.1;
    float voxelSize = 0.f;          // 0 - autotedect (Settings::voxelSize * Settings::voxelSizeMul^cascade_no
    uint32_t moveThreshold = 0;     // 0 - same is in Settings::moveThreshold
    uint32_t computeGroupOrder = 0; // cascades with same group will be executed on same frame. examples - c0:0, c1:1, c2:2, c3:2
    bool allowDynamic = true;       // allow CSM + FOM, not recomended for cascades with big sparseDiv (ghosting)

    bool operator==(const CascadeSettings &) const = default;
  };

  struct Settings
  {
    uint32_t cascadeCount = 4;
    uint32_t xzDim = 64;
    uint32_t yDim = 32;
    float voxelSize = 1.f;
    float voxelSizeMul = 2.f;
    float heightOffsetRatio = 0.5;
    uint32_t moveThreshold = 4; // texels
    bool oneToroidalUpdatePerFrame = true;
    bool allowSparseAfterToroidal = false;
    dag::Vector<CascadeSettings> cascades; // optional

    bool operator==(const Settings &) const = default;
  };

  VoxelShadows(const Settings &cfg);
  ~VoxelShadows();
  void updateOrigin(const Point3 &pos);
  void calculateVolumes();
  void invalidate();
  void renderDebugOpt();

private:
  bool is_force_full_update() const;

  int rndSeed = 0;
  bool haveSparse = false;
  bool invalidatedLastFrame = true;
  uint32_t originUpdateCascadeIdx = 0;
  uint32_t computeOrderCascadeIdx = 0;
  uint32_t computeOrderLastIdx = 0;
  Point3 lastViewPos = {0, 0, 0};
  Settings cfg;
  UniqueTexHolder volTexAtlas;

  carray<eastl::string, maxCascadeCount> sparseProfilerNames;
  carray<eastl::string, maxCascadeCount> toroidalProfilerNames;
  dag::Vector<ToroidalVoxelGrid> grids;
  dag::Vector<ToroidalVoxelGrid::UpdateResult> gridStates;

  eastl::unique_ptr<ComputeShaderElement> fullUpdateCs;
  eastl::unique_ptr<ComputeShaderElement> partialUpdateCs;
  eastl::unique_ptr<ComputeShaderElement> sparseFullUpdateCs;
  eastl::unique_ptr<ComputeShaderElement> copyPaddingCs;
  eastl::unique_ptr<PostFxRenderer> voxelDebugRenderer;
  eastl::unique_ptr<DynamicShaderHelper> probesDebugShader;

  ShaderVariableInfo voxel_shadows_rndVarId;
  ShaderVariableInfo voxel_shadows_cascade_countVarId;
  ShaderVariableInfo voxel_shadows_cascadeIdxVarId;
  ShaderVariableInfo voxel_shadows_originVarId;
  ShaderVariableInfo voxel_shadows_origin_in_tc_spaceVarId;
  ShaderVariableInfo voxel_shadows_wvp_in_tc_spaceVarId;
  ShaderVariableInfo voxel_shadows_voxel_sizeVarId;
  ShaderVariableInfo voxel_shadows_world_to_tcVarId;
  ShaderVariableInfo voxel_shadows_update_box_ofsVarId;
  ShaderVariableInfo voxel_shadows_update_box_sizeVarId;
  ShaderVariableInfo voxel_shadows_sparse_divVarId;
  ShaderVariableInfo voxel_shadows_allow_dynamicVarId;
  ShaderVariableInfo voxel_shadows_debug_typeVarId;
  ShaderVariableInfo voxel_shadows_debug_thresholdVarId;
  ShaderVariableInfo voxel_shadows_debug_cascadeVarId;
};