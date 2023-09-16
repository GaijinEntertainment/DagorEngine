//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/************************************************************************
  shader skinned mesh - interface classes
************************************************************************/

#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <math/dag_TMatrix.h>

//************************************************************************
//* forwards
//************************************************************************
class DynamicRenderableSceneInstance;
class IGenLoad;


/*********************************
 *
 * class ShaderSkinnedMesh
 *
 *********************************/
class ShaderSkinnedMesh
{
public:
  DAG_DECLARE_NEW(midmem)

  struct BoneForElem
  {
    TMatrix origTm;
    int nodeId;
  };

  ~ShaderSkinnedMesh() { clearData(); }


  // patch mesh data after loading from dump
  void patchData(void *base, ShaderMatVdata &smvd);

  // rebase and clone data (useful for data copies)
  void rebaseAndClone(void *new_base, const void *old_base);

  // explicit destructor
  void clearData();

  // render mesh
  void render();
  void render_trans();
  void render_distortion();

  // update bones
  void recalcBones(const DynamicRenderableSceneInstance &cb, const TMatrix *render_space_tm = NULL);

  void duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat);
  void duplicateMat(ShaderMaterial *prev_m, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat);

  void updateShaderElems() { shaderMesh.updateShaderElems(); }

  // return internal shader mesh object
  inline ShaderMesh &getShaderMesh() { return shaderMesh; }
  inline const ShaderMesh &getShaderMesh() const { return shaderMesh; }

  void getBonesForElem(ShaderElement *elem, Tab<BoneForElem> &out_bones) const;
  int bonesCount() const { return boneOrgTm.size(); }
  bool hasBone(int bone_id) const;

  bool matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm, DynamicRenderableSceneInstance &cb) const;
  int getBoneForNode(int node_idx) const;
  uint32_t getNodeForBone(uint32_t bone_idx) const
  {
    G_FAST_ASSERT(bone_idx < bonesCount());
    return boneNodeId[bone_idx];
  }
  dag::ConstSpan<int16_t> getBoneNodeIds() const { return make_span_const(boneNodeId.get(), bonesCount()); }
  const TMatrix &getBoneOrgTm(uint32_t bone_idx) const { return boneOrgTm[bone_idx]; }
  dag::ConstSpan<TMatrix> getBoneOrgTmArr() const { return make_span_const(boneOrgTm.data(), bonesCount()); }

private:
  void allocateWorkData();

  struct WorkData
  {
    SmallTab<TMatrix, MidmemAlloc> tm;                       //< parallel to boneOrgTm[]
    SmallTab<dag::Span<Color4>, MidmemAlloc> matTabVprArray; //< parallel to matTab[]
    SmallTab<Color4, MidmemAlloc> vprArrayStor;
  };
  struct MaterialDesc
  {
    PatchableTab<uint8_t> boneIndices;
  };

  PatchableTab<TMatrix> boneOrgTm;
  PatchableTab<MaterialDesc> matTab;

  PatchablePtr<int16_t> boneNodeId;                 //< parallel to boneOrgTm[]
  PatchablePtr<PatchablePtr<ShaderMaterial>> shMat; //< parallel to matTab[]
  PatchablePtr<WorkData> workData;

  ShaderMesh shaderMesh;
}; // class ShaderSkinnedMesh
//
