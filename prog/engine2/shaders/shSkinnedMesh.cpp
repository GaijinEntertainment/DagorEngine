// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <math/dag_Point4.h>
#include <shaders/dag_shSkinMesh.h>
#include <shaders/dag_dynSceneRes.h>
#include "scriptSMat.h"
// #include <debug/dag_debug.h>

/*********************************
 *
 * class ShaderSkinnedMesh
 *
 *********************************/

void ShaderSkinnedMesh::patchData(void *base, ShaderMatVdata &smvd)
{
  boneOrgTm.patch(base);
  matTab.patch(base);

  boneNodeId.patch(base);
  shMat.patch(base);

  for (int i = 0; i < matTab.size(); i++)
  {
    matTab[i].boneIndices.patch(base);

    shMat[i] = smvd.getMaterial(shMat[i].toInt());
    shMat[i]->addRef();
  }

  shaderMesh.patchData(base, smvd);

  workData.setPtr(nullptr);
  allocateWorkData();
}
void ShaderSkinnedMesh::allocateWorkData()
{
  workData.setPtr(new WorkData);

  clear_and_resize(workData->tm, boneOrgTm.size());
  mem_set_0(workData->tm);

  clear_and_resize(workData->matTabVprArray, matTab.size());
  int va_sz = 0;
  for (int i = 0; i < matTab.size(); i++)
    va_sz += (matTab[i].boneIndices.size() ? matTab[i].boneIndices.size() : boneOrgTm.size()) * 3;
  clear_and_resize(workData->vprArrayStor, va_sz);
  mem_set_0(workData->vprArrayStor);

  va_sz = 0;
  for (int i = 0; i < matTab.size(); i++)
  {
    int vec_cnt = (matTab[i].boneIndices.size() ? matTab[i].boneIndices.size() : boneOrgTm.size()) * 3;
    workData->matTabVprArray[i].set(make_span(workData->vprArrayStor).subspan(va_sz, vec_cnt));
    va_sz += vec_cnt;
  }
}

void ShaderSkinnedMesh::rebaseAndClone(void *new_base, const void *old_base)
{
  boneOrgTm.rebase(new_base, old_base);
  matTab.rebase(new_base, old_base);

  boneNodeId.rebase(new_base, old_base);
  shMat.rebase(new_base, old_base);

  for (int i = 0; i < matTab.size(); i++)
  {
    matTab[i].boneIndices.rebase(new_base, old_base);
    shMat[i]->addRef();
  }

  shaderMesh.rebaseAndClone(new_base, old_base);
  allocateWorkData();
}

void ShaderSkinnedMesh::clearData()
{
  shaderMesh.clearData();

  for (int i = 0; i < matTab.size(); i++)
    if (shMat[i])
    {
      shMat[i]->delRef();
      shMat[i] = NULL;
    }

  boneOrgTm.init(NULL, 0);
  matTab.init(NULL, 0);
  boneNodeId = NULL;
  shMat = NULL;
  delete workData.get();
  workData.setPtr(nullptr);
}

void ShaderSkinnedMesh::render() { shaderMesh.render(); }


void ShaderSkinnedMesh::render_trans() { shaderMesh.render_trans(); }


void ShaderSkinnedMesh::render_distortion() { shaderMesh.render_distortion(); }


bool ShaderSkinnedMesh::matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm,
  DynamicRenderableSceneInstance &cb) const
{
  int bestNodeIdx = -1;
  float bestDistSq = 0;

  for (int i = 0; i < cb.getNodeCount(); ++i)
  {
    const float newDistSq = (cb.getNodeWtm(i).getcol(3) - pos).lengthSq();
    if (bestNodeIdx == -1 || bestDistSq > newDistSq)
    {
      bestNodeIdx = i;
      bestDistSq = newDistSq;
    }
  }

  bestDistSq = bone_idx == -1 ? 0 : (bone_itm * pos).lengthSq();

  bool updated = false;
  dag::ConstSpan<TMatrix> work_tm = workData->tm;
  for (int i = 0; i < work_tm.size(); i++)
  {
    if (boneNodeId[i] != bestNodeIdx)
      continue;
    float det = work_tm[i].det();
    if (fabsf(det) < 1e-6f)
      continue;
    TMatrix itm = inverse(work_tm[i], det);

    const float newDistSq = (itm * pos).lengthSq();

    if (bone_idx == -1 || bestDistSq > newDistSq)
    {
      bone_idx = i;
      bestDistSq = newDistSq;
      bone_itm = itm;
      updated = true;
    }
  }
  return updated;
}


int ShaderSkinnedMesh::getBoneForNode(int node_idx) const
{
  for (int i = 0; i < workData->tm.size(); i++)
  {
    if (boneNodeId[i] != node_idx)
      continue;
    return i;
  }
  return -1;
}

// update bones
void ShaderSkinnedMesh::recalcBones(const DynamicRenderableSceneInstance &cb, const TMatrix *render_space_tm)
{
#define CPY(c)                      \
  dest[c * 4 + 0] = src[0 * 3 + c]; \
  dest[c * 4 + 1] = src[1 * 3 + c]; \
  dest[c * 4 + 2] = src[2 * 3 + c]; \
  dest[c * 4 + 3] = src[3 * 3 + c];

  dag::Span<TMatrix> work_tm = make_span(workData->tm);
  for (int i = 0, bonesCount = work_tm.size(); i < bonesCount; i++)
  {
    TMatrix tm = cb.getNodeWtm(boneNodeId[i]);
    work_tm[i] = tm * boneOrgTm[i];
    if (render_space_tm)
      work_tm[i] = *render_space_tm * work_tm[i];
  }
  for (int i = 0, matTabNum = matTab.size(); i < matTabNum; i++)
  {
    MaterialDesc &materialDesc = matTab[i];
    float *__restrict dest = &workData->matTabVprArray[i][0].r;
    bool direct_bone_map = materialDesc.boneIndices.size() == 0;
    for (int j = 0, je = direct_bone_map ? boneOrgTm.size() : materialDesc.boneIndices.size(); j < je; j++, dest += 3 * 4)
    {
      int boneIndex = direct_bone_map ? j : materialDesc.boneIndices[j];
      const TMatrix &tm = work_tm[boneIndex];
      const float *__restrict src = &tm.m[0][0];

      CPY(0);
      CPY(1);
      CPY(2);
    }
  }
#undef CPY
}

void ShaderSkinnedMesh::duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat)
{
  shaderMesh.duplicateMaterial(tex_id, old_mat, new_mat);
  for (int i = 0; i < matTab.size(); i++)
    if (shMat[i])
      for (int j = 0; j < old_mat.size(); j++)
        if (shMat[i] == old_mat[j])
        {
          shMat[i]->delRef();
          shMat[i] = new_mat[j];
          shMat[i]->addRef();
          break;
        }
}
void ShaderSkinnedMesh::duplicateMat(ShaderMaterial *prev_m, Tab<ShaderMaterial *> &old_mat, Tab<ShaderMaterial *> &new_mat)
{
  if (ShaderMaterial *m = replace_shader_mat(shaderMesh, prev_m, old_mat, new_mat))
    for (int i = 0; i < matTab.size(); i++)
      if (shMat[i] == prev_m)
      {
        shMat[i]->delRef();
        shMat[i] = m;
        shMat[i]->addRef();
      }
}

void ShaderSkinnedMesh::getBonesForElem(ShaderElement *elem, Tab<BoneForElem> &out_bones) const
{
  G_ASSERT(elem);

  for (unsigned int matNo = 0, me = matTab.size(); matNo < me; matNo++)
  {
    if ((ShaderElement *)shMat[matNo]->native().getElem() == elem)
    {
      const bool direct_bone_map = matTab[matNo].boneIndices.size() == 0;
      const int boneE = direct_bone_map ? boneOrgTm.size() : matTab[matNo].boneIndices.size();
      int at = append_items(out_bones, boneE);
      BoneForElem *boneForElemPtr = &out_bones[at];
      for (unsigned int boneNo = 0; boneNo < boneE; boneNo++, ++boneForElemPtr)
      {
        BoneForElem &boneForElem = *boneForElemPtr;
        int boneIndex = direct_bone_map ? boneNo : matTab[matNo].boneIndices[boneNo];

        boneForElem.nodeId = boneNodeId[boneIndex];
        boneForElem.origTm = boneOrgTm[boneIndex];
      }

      break;
    }
  }
}

bool ShaderSkinnedMesh::hasBone(int bone_id) const
{
  for (int i = 0; i < boneOrgTm.size(); ++i)
    if (boneNodeId[i] == bone_id)
      return true;
  return false;
}
