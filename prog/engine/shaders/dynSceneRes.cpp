// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_dynSceneRes.h>
#include <3d/dag_render.h>
#include <shaders/dag_shSkinMesh.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include "scriptSElem.h"
#include <math/dag_traceRayTriangle.h>
#include <math/dag_bounds3.h>
#include <math/dag_frustum.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_texIdSet.h>
#include <memory/dag_framemem.h>
#include "scriptSMat.h"
#include "shaders/sh_vars.h"
#include <shaders/dag_shaderBlock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <stdio.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#if DAGOR_DBGLEVEL > 0
#include <debug/dag_log.h>
#endif

using namespace shglobvars;
static constexpr unsigned VDATA_MT_DYNMODEL = 1;


static bool hasBindPoseBuffer(const ShaderSkinnedMesh &skin)
{
  static ShaderVariableInfo bindposeVar("bind_pose_indicator_dummy", /*opt*/ true);
  for (const ShaderMesh::RElem &elem : skin.getShaderMesh().getAllElems())
  {
    if (elem.mat->hasVariable((int)bindposeVar))
      return true;
  }
  return false;
}


BindPoseBufferManager bindPoseBufferManager;


void DynamicRenderableSceneLodsResource::updateShaderElems()
{
  for (int i = 0; i < lods.size(); ++i)
    lods[i].scene->updateShaderElems();
}
void DynamicRenderableSceneLodsResource::updateBindposes()
{
  for (int i = 0; i < lods.size(); ++i)
    lods[i].scene->updateBindposes();
}

void DynamicRenderableSceneLodsResource::closeBindposeBuffer() { bindPoseBufferManager.closeHeap(); }

int DynamicRenderableSceneResource::getBindposeBufferIndex(int bone_idx) const
{
  if (bindPoseElemPtrArr.size() != skins.size()) // no bindpose
    return 0;
  const Ptr<BindPoseElem> &ptr = bindPoseElemPtrArr[bone_idx];
  return ptr ? ptr->getBufferOffset() : 0;
}

void DynamicRenderableSceneResource::updateBindposes()
{
  if (bindPoseElemPtrArr.size() == skins.size()) // already updated
    return;
  G_ASSERT_RETURN(bindPoseElemPtrArr.size() == 0, ); // must be cleared before update

  bool needsBindpose = false;
  for (int i = 0; i < skins.size() && !needsBindpose; ++i)
    needsBindpose = hasBindPoseBuffer(*skins[i]->getMesh());

  if (!needsBindpose)
    return;

  bindPoseElemPtrArr.resize(skins.size());
  for (int i = 0; i < skins.size(); ++i)
  {
    ShaderSkinnedMesh &mesh = *skins[i]->getMesh();
    if (hasBindPoseBuffer(mesh))
      bindPoseElemPtrArr[i] = bindPoseBufferManager.createOrGetBindposeElem(mesh.getBoneOrgTmArr());
  }
}


float DynamicRenderableSceneInstance::lodDistanceScale = 1.f;
void (*DynamicRenderableSceneLodsResource::on_higher_lod_required)(DynamicRenderableSceneLodsResource *res, unsigned req_lod,
  unsigned cur_lod) = nullptr;

static OSSpinlock dynres_clones_list_spinlock;

static on_dynsceneres_beforedrawmeshes_cb on_dynsceneres_beforedrawmeshes = nullptr;
on_dynsceneres_beforedrawmeshes_cb set_dynsceneres_beforedrawmeshes_cb(on_dynsceneres_beforedrawmeshes_cb cb)
{
  on_dynsceneres_beforedrawmeshes_cb prev = on_dynsceneres_beforedrawmeshes;
  on_dynsceneres_beforedrawmeshes = cb;
  return prev;
}

DynSceneResNameMapResource *DynSceneResNameMapResource::loadResource(IGenLoad &crd, int res_sz)
{
  if (res_sz == -1)
    res_sz = crd.readInt();

  DynSceneResNameMapResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) DynSceneResNameMapResource;
  crd.read(res->dumpStartPtr(), res_sz);
  res->patchData();

  return res;
}

void DynSceneResNameMapResource::patchData()
{
  void *base = &node;

  node.patchData(base);
}


DynamicRenderableSceneResource *DynamicRenderableSceneResource::loadResource(IGenLoad &, int) { return NULL; }

DynamicRenderableSceneResource *DynamicRenderableSceneResource::loadResourceInternal(IGenLoad &crd, DynSceneResNameMapResource *nm,
  int srl_flags, ShaderMatVdata &smvd, int res_sz)
{
  if (res_sz == -1)
    res_sz = crd.readInt();

  DynamicRenderableSceneResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) DynamicRenderableSceneResource;
  crd.read(res->dumpStartPtr(), res_sz);
  res->names = nm;
  res->patchAndLoadData(crd, srl_flags, res_sz, smvd);

  return res;
}
void DynamicRenderableSceneResource::patchAndLoadData(IGenLoad &crd, int flags, int res_sz, ShaderMatVdata &smvd)
{
  G_ASSERT(bindPoseElemPtrArr.size() == 0); // must be cleared before (or remain unused)

  void *base = &rigids;
  resSize = res_sz;
  rigids.patch(base);
  skins.patch(base);

  for (RigidObject &ro : rigids)
  {
    ro.mesh = ShaderMeshResource::loadResource(crd, smvd, ro.mesh.toInt());
    ro.mesh->addRef();
    if (!(flags & SRLOAD_NO_TEX_REF))
      ro.mesh->getMesh()->acquireTexRefs();
  }
  // #if DAGOR_DBGLEVEL > 0
  bool is_valid_bones = true;
  int num_bones_varId = get_shader_variable_id("num_bones", true);
  for (RigidObject &ro : rigids)
  {
    ShaderMesh *sm = ro.mesh->getMesh();
    for (int ei = 0; ei < sm->getAllElems().size(); ++ei)
    {
      ShaderMaterial *mat = sm->getAllElems()[ei].mat;
      int num_bones = 0;
      if (mat->getIntVariable(num_bones_varId, num_bones) && num_bones != 0)
      {
#if DAGOR_DBGLEVEL > 0
        logerr("%s: material with shader class <%s> info <%s> has not zero num_bones (%d)", crd.getTargetName(),
          mat->getShaderClassName(), mat->getInfo().str(), num_bones);
#endif
        mat->set_int_param(num_bones_varId, 0); // workaround for invalid build;
        is_valid_bones = false;
      }
    }
  }
  if (!is_valid_bones)
  {
    // G_ASSERTF(0, "Invalid bones");
  }
  // #endif
}
void DynamicRenderableSceneResource::loadSkins(IGenLoad &crd, int /*flags*/, ShaderMatVdata &skin_smvd)
{
  skinNodes.resize(skins.size());
  for (unsigned int skinNo = 0; skinNo < skins.size(); skinNo++)
    skinNodes[skinNo] = crd.readInt();

  crd.readTabData(skins);
  for (int i = 0; i < skins.size(); ++i)
    skins[i] = ShaderSkinnedMeshResource::loadResource(crd, skin_smvd, skins[i].toInt());

  for (int i = skins.size() - 1; i >= 0; i--)
    if (skins[i])
      skins[i]->addRef();
    else
    {
      if (skins.size() > i + 1)
        memmove(&skins[i], &skins[i + 1], (skins.size() - i - 1) * elem_size(skins));
      skins.init(skins.data(), skins.size() - 1);
    }
}

DynamicRenderableSceneResource::DynamicRenderableSceneResource(const DynamicRenderableSceneResource &from)
{
  resSize = from.resSize;
  names = from.names;
  skinNodes = from.skinNodes;

  void *new_base = &rigids;
  const void *old_base = &from.rigids;

  memcpy(new_base, old_base, resSize);
  rigids.rebase(new_base, old_base);
  skins.rebase(new_base, old_base);

  for (RigidObject &ro : rigids)
    ro.mesh->addRef();

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i])
      skins[i]->addRef();

  cloneMeshes();
}


DynamicRenderableSceneResource *DynamicRenderableSceneResource::clone() const
{
  void *mem = memalloc(dumpStartOfs() + resSize, midmem);
  return new (mem, _NEW_INPLACE) DynamicRenderableSceneResource(*this);
}

void DynamicRenderableSceneResource::clearData()
{
  clear_and_shrink(bindPoseElemPtrArr);

  for (int i = 0; i < rigids.size(); ++i)
    rigids[i].mesh->delRef();
  for (int i = 0; i < skins.size(); ++i)
    skins[i]->delRef();

  rigids.init(NULL, 0);
  skins.init(NULL, 0);
  names = NULL;
}

void DynamicRenderableSceneResource::acquireTexRefs()
{
  for (RigidObject &ro : rigids)
    ro.mesh->getMesh()->acquireTexRefs();

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i])
      skins[i]->getMesh()->getShaderMesh().acquireTexRefs();
}
void DynamicRenderableSceneResource::releaseTexRefs()
{
  for (RigidObject &ro : rigids)
    ro.mesh->getMesh()->releaseTexRefs();

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i])
      skins[i]->getMesh()->getShaderMesh().releaseTexRefs();
}


void DynamicRenderableSceneResource::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  for (const RigidObject &ro : rigids)
    ro.mesh->gatherUsedTex(tex_id_list);

  for (auto s : skins)
    if (s)
      s->gatherUsedTex(tex_id_list);
}

void DynamicRenderableSceneResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const
{
  for (const RigidObject &ro : rigids)
    ro.mesh->gatherUsedMat(mat_list);

  for (auto s : skins)
    if (s)
      s->gatherUsedMat(mat_list);
}

bool DynamicRenderableSceneResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  bool ok = false;

  for (RigidObject &ro : rigids)
    if (ro.mesh->replaceTexture(tex_id_old, tex_id_new))
      ok = true;

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i])
      if (skins[i]->replaceTexture(tex_id_old, tex_id_new))
        ok = true;

  return ok;
}


void DynamicRenderableSceneResource::duplicateMaterial(TEXTUREID tex_id, Tab<ShaderMaterial *> &old_mat,
  Tab<ShaderMaterial *> &new_mat)
{
  for (RigidObject &ro : rigids)
    if (tex_id == BAD_TEXTUREID || ro.mesh->replaceTexture(tex_id, tex_id))
      if (ShaderMesh *mesh = ro.mesh->getMesh())
        mesh->duplicateMaterial(tex_id, old_mat, new_mat);

  for (int mi = 0; mi < skins.size(); ++mi)
    if (tex_id == BAD_TEXTUREID || skins[mi]->replaceTexture(tex_id, tex_id))
      skins[mi]->getMesh()->duplicateMaterial(tex_id, old_mat, new_mat);
}

void DynamicRenderableSceneResource::duplicateMat(ShaderMaterial *prev_m, Tab<ShaderMaterial *> &old_mat,
  Tab<ShaderMaterial *> &new_mat)
{
  for (RigidObject &ro : rigids)
    if (ShaderMesh *mesh = ro.mesh->getMesh())
      mesh->duplicateMat(prev_m, old_mat, new_mat);

  for (int mi = 0; mi < skins.size(); ++mi)
    skins[mi]->getMesh()->duplicateMat(prev_m, old_mat, new_mat);
}

void DynamicRenderableSceneResource::cloneMeshes()
{
  for (RigidObject &ro : rigids)
  {
    ShaderMeshResource *m = ro.mesh;
    ro.mesh = m->clone();
    ro.mesh->addRef();
    m->delRef();
  }

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i])
    {
      ShaderSkinnedMeshResource *m = skins[i];
      skins[i] = m->clone();
      skins[i]->addRef();
      m->delRef();
    }
}

void DynamicRenderableSceneResource::updateShaderElems()
{
  updateBindposes();

  for (RigidObject &ro : rigids)
    if (ro.mesh->getMesh())
      ro.mesh->getMesh()->updateShaderElems();

  for (int i = 0; i < skins.size(); ++i)
    if (skins[i] && skins[i]->getMesh())
      skins[i]->getMesh()->updateShaderElems();
}

bool DynamicRenderableSceneResource::matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm,
  DynamicRenderableSceneInstance &cb) const
{
  if (skins.size())
    return skins[0]->getMesh()->matchBetterBoneIdxAndItmForPoint(pos, bone_idx, bone_itm, cb);
  return false;
}

int DynamicRenderableSceneResource::getNodeForBone(uint32_t bone_idx) const
{
  if (skins.size())
    return skins[0]->getMesh()->getNodeForBone(bone_idx);
  return -1;
}

int DynamicRenderableSceneResource::getBoneForNode(int node_idx) const
{
  if (skins.size())
    return skins[0]->getMesh()->getBoneForNode(node_idx);
  return -1;
}


void DynamicRenderableSceneResource::render(DynamicRenderableSceneInstance &cb, real opacity /*= 1.0*/)
{
  if (opacity < 0.99f)
    return;

  int i;

  for (i = 0; i < rigids.size(); ++i)
  {
    RigidObject &o = rigids[i];
    if (cb.isNodeHidden(o.nodeId))
      continue;
    TMatrix &tm = (TMatrix &)cb.getNodeWtm(o.nodeId);
    float nodeOpacity = cb.getNodeOpacity(o.nodeId);
    if (nodeOpacity < 1.f)
      continue;

    d3d::settm(TM_WORLD, tm);

    if (on_dynsceneres_beforedrawmeshes)
      on_dynsceneres_beforedrawmeshes(cb, o.nodeId);

    if (!o.mesh->getMesh())
      DAG_FATAL("DynamicRenderableSceneResource:render - mesh in NULL! (object build not called?)");

    o.mesh->getMesh()->render();
  }

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  for (i = 0; i < skins.size(); i++)
  {
    ShaderSkinnedMesh &mesh = *skins[i]->getMesh();
    if (cb.isNodeHidden(skinNodes[i]) || cb.getNodeOpacity(skinNodes[i]) < 1.f)
      continue;

    mesh.recalcBones(cb);
    mesh.render();
  }
}


void DynamicRenderableSceneResource::renderTrans(DynamicRenderableSceneInstance &cb, real opacity /*= 1.0*/)
{
  bool need_transp = opacity < 0.99f;

  if (need_transp)
    ShaderGlobal::set_real_fast(globalTransRGvId, opacity);

  int i;


  for (i = 0; i < rigids.size(); ++i)
  {
    RigidObject &o = rigids[i];
    if (cb.isNodeHidden(o.nodeId))
      continue;

    TMatrix &tm = (TMatrix &)cb.getNodeWtm(o.nodeId);
    real nodeOpacity = cb.getNodeOpacity(o.nodeId);
    real totalOpacity = opacity * nodeOpacity;
    if (totalOpacity < FLT_EPSILON)
      continue;

    d3d::settm(TM_WORLD, tm);

    bool need_node_opacity = nodeOpacity < 0.99f;
    if (need_node_opacity)
      ShaderGlobal::set_real_fast(globalTransRGvId, totalOpacity);

    if (totalOpacity < 1)
      o.mesh->getMesh()->render();

    o.mesh->getMesh()->render_trans();

    if (need_node_opacity)
      ShaderGlobal::set_real_fast(globalTransRGvId, opacity);
  }

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  // d3d::settm(TM_WORLD, &TMatrix4::IDENT);

  for (i = 0; i < skins.size(); i++)
  {
    ShaderSkinnedMesh &mesh = *skins[i]->getMesh();
    if (cb.isNodeHidden(skinNodes[i]))
      continue;

    mesh.recalcBones(cb);

    if (need_transp)
      mesh.render();
    mesh.render_trans();
  }

  if (need_transp)
    ShaderGlobal::set_real_fast(globalTransRGvId, 1.f);
}


void DynamicRenderableSceneResource::renderDistortion(DynamicRenderableSceneInstance &cb)
{
  for (RigidObject &o : rigids)
  {
    if (cb.isNodeHidden(o.nodeId))
      continue;

    TMatrix &tm = (TMatrix &)cb.getNodeWtm(o.nodeId);
    d3d::settm(TM_WORLD, tm);

    o.mesh->getMesh()->render_distortion();
  }

  d3d::settm(TM_WORLD, TMatrix::IDENT);

  for (int i = 0; i < skins.size(); i++)
  {
    ShaderSkinnedMesh &mesh = *skins[i]->getMesh();
    if (cb.isNodeHidden(skinNodes[i]))
      continue;

    mesh.recalcBones(cb);
    mesh.render_distortion();
  }
}


DynamicRenderableSceneResource::RigidObject *DynamicRenderableSceneResource::getRigidObject(int node_id) const
{
  if (node_id < 0)
    return NULL;

  for (int i = 0; i < rigids.size(); ++i)
  {
    const RigidObject &r = rigids[i];
    if (r.nodeId == node_id)
      return const_cast<RigidObject *>(&r);
  }

  return NULL;
}

bool DynamicRenderableSceneResource::hasRigidMesh(int node_id)
{
  RigidObject *ro = getRigidObject(node_id);
  return ro && ro->mesh;
}

void DynamicRenderableSceneResource::findRigidNodesWithMaterials(dag::ConstSpan<const char *> material_names,
  const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const
{
  auto checkRigid = [&](const RigidObject &ro) {
    for (auto &elem : ro.mesh->getMesh()->getAllElems())
      for (auto name : material_names)
        if (strcmp(elem.mat->getShaderClassName(), name) == 0)
        {
          cb(ro.nodeId);
          return;
        }
  };

  for (auto &ro : rigids)
    checkRigid(ro);
}

void DynamicRenderableSceneResource::findSkinNodesWithMaterials(dag::ConstSpan<const char *> material_names,
  const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const
{
  auto checkSkin = [&](int skinIndex) {
    const auto &sm = *skins[skinIndex];
    for (auto &elem : sm.getMesh()->getShaderMesh().getAllElems())
      for (auto name : material_names)
        if (strcmp(elem.mat->getShaderClassName(), name) == 0)
        {
          cb(skinNodes[skinIndex]);
          return;
        }
  };

  for (int i = 0; i < skins.size(); i++)
    checkSkin(i);
}

void DynamicRenderableSceneResource::getMeshes(Tab<ShaderMeshResource *> &meshes, Tab<int> &nodeIds, Tab<BSphere3> *bsph)
{
  for (RigidObject &ro : rigids)
  {
    meshes.push_back(ro.mesh);
    nodeIds.push_back(ro.nodeId);
    if (bsph)
      bsph->push_back(BSphere3(ro.sph_c, ro.sph_r));
  }
}

static inline Point3 c_to_p3(const E3DCOLOR &c) { return Point3(c.r / 127.5f - 1.0f, c.g / 127.5f - 1.0f, c.b / 127.5f - 1.0f); }


bool DynamicRenderableSceneResource::traceRayRigids(DynamicRenderableSceneInstance &scene, const Point3 &from, const Point3 &dir,
  float &currentT, Point3 &normal, bool *trans, Point2 *uv)
{
  bool collision = false;
  for (RigidObject &ro : rigids)
  {
    const TMatrix &tm = scene.getNodeWtm(ro.nodeId);

    dag::ConstSpan<ShaderMesh::RElem> elems = ro.mesh->getMesh()->getAllElems();
    dag::ConstSpan<ShaderMesh::RElem> tr_elems = ro.mesh->getMesh()->getElems(ShaderMesh::STG_trans);
    for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
    {
      const ShaderMesh::RElem &elem = elems[elemNo];
      if (!elem.vertexData)
        continue;


      // Parse vertex format for normal offset and vertex buffer stride.

      ScriptedShaderElement *shaderElem = (ScriptedShaderElement *)elem.e.get();
      unsigned int vbStride = 0;
      unsigned int normalOffset = 0xFFFFFFFF;
      int normalType = 0;
      unsigned int tcOffset = 0xFFFFFFFF;
      int tcType = 0;
      int tcMod = 0;
      int posType = 0;
      int posOffset = 0xFFFFFFFF;
      for (unsigned int channelNo = 0; channelNo < shaderElem->code.channel.size(); channelNo++)
      {
        const ShaderChannelId &channel = shaderElem->code.channel[channelNo];
        if (channel.u == SCUSAGE_POS && channel.ui == 0)
        {
          if (channel.t != VSDT_FLOAT3 && channel.t != VSDT_HALF4)
            DAG_FATAL("Invalid pos channel type");

          posType = channel.t;
          posOffset = vbStride;
        }
        else if (channel.u == SCUSAGE_NORM && channel.ui == 0)
        {
          if (channel.t != VSDT_FLOAT3 && channel.t != VSDT_E3DCOLOR)
            DAG_FATAL("Invalid normal channel type");

          normalType = channel.t;
          normalOffset = vbStride;
        }
        else if (uv && channel.u == SCUSAGE_TC && tcOffset == 0xFFFFFFFF)
        {
          if (channel.t != VSDT_FLOAT2 && channel.t != VSDT_SHORT2 && channel.t != VSDT_HALF2)
            DAG_FATAL("Invalid texture channel type");

          tcType = channel.t;
          tcOffset = vbStride;
          tcMod = channel.mod;
        }

        unsigned int channelSize = 0;
        if (!channel_size(channel.t, channelSize))
        {
          DAG_FATAL("Unknown channel type");
        }
        vbStride += channelSize;
      }
      if (normalOffset == 0xFFFFFFFF)
        continue;
      if (posOffset == 0xFFFFFFFF)
        continue;


      // Lock VB/IB.

      unsigned short int *indexBufferData;
      elem.vertexData->getIB()->lock(elem.si * sizeof(unsigned short int), elem.numf * 3 * sizeof(unsigned short int),
        &indexBufferData, VBLOCK_READONLY);
      unsigned char *vertexBufferData;
      elem.vertexData->getVB()->lockEx((elem.baseVertex + elem.sv) * vbStride, elem.numv * vbStride, &vertexBufferData,
        VBLOCK_READONLY);


      // Iterate over triangles.

      for (unsigned int faceNo = 0; faceNo < elem.numf; faceNo++)
      {
        unsigned int index[3];
        index[0] = indexBufferData[3 * faceNo + 0] - elem.sv;
        index[1] = indexBufferData[3 * faceNo + 1] - elem.sv;
        index[2] = indexBufferData[3 * faceNo + 2] - elem.sv;
        Point3 corner[3];

        if (posType == VSDT_FLOAT3)
        {
          corner[0] = *(Point3 *)&vertexBufferData[vbStride * index[0] + posOffset];
          corner[1] = *(Point3 *)&vertexBufferData[vbStride * index[1] + posOffset];
          corner[2] = *(Point3 *)&vertexBufferData[vbStride * index[2] + posOffset];
        }
        else if (posType == VSDT_HALF4)
        {
          for (int ii = 0; ii < 3; ++ii)
            for (int jj = 0; jj < 3; ++jj)
            {
              unsigned short stc = ((unsigned short *)&vertexBufferData[index[ii] * vbStride + posOffset])[jj];
              corner[ii][jj] = half_to_float(stc);
            }
        }
        corner[0] = tm * corner[0];
        corner[1] = tm * corner[1];
        corner[2] = tm * corner[2];


        float u, v;
        bool res = traceRayToTriangleCullCCW(from, dir, currentT, corner[0], corner[1] - corner[0], corner[2] - corner[0], u, v);

        if (res)
        {
          // debug("rigid=%d,mesh=0x%08x,elemNo=%d,faceNo=%d", &ro-rigid.data(), ro.mesh, elemNo, faceNo);
          collision = true;
          if (normalType == VSDT_FLOAT3)
            normal = *(Point3 *)&vertexBufferData[index[0] * vbStride + normalOffset];
          else if (normalType == VSDT_E3DCOLOR)
          {
            E3DCOLOR packedNormal = *(unsigned int *)&vertexBufferData[index[0] * vbStride + normalOffset];
            normal = c_to_p3(packedNormal);
          }
          normal = tm % normal;
          if (trans)
            *trans = tr_elems.size() && (&elem >= tr_elems.data()) && (&elem < tr_elems.data() + tr_elems.size());
          if (uv)
          {
            Point2 tc[3];
            if (tcType == VSDT_FLOAT2)
            {
              tc[0] = *(Point2 *)&vertexBufferData[index[0] * vbStride + tcOffset];
              tc[1] = *(Point2 *)&vertexBufferData[index[1] * vbStride + tcOffset];
              tc[2] = *(Point2 *)&vertexBufferData[index[2] * vbStride + tcOffset];
            }
            else if (tcType == VSDT_HALF2)
            {
              for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 2; ++j)
                {
                  unsigned short stc = ((unsigned short *)&vertexBufferData[index[i] * vbStride + tcOffset])[j];
                  tc[i][j] = half_to_float(stc);
                }
            }
            else if (tcType == VSDT_SHORT2)
            {
              for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 2; ++j)
                {
                  short stc = ((short *)&vertexBufferData[index[i] * vbStride + tcOffset])[j];
                  tc[i][j] = stc;
                }
            }
            else
            {
              tc[0] = tc[1] = tc[2] = Point2(0, 0);
            }
            float div = 1.0;
            switch (tcMod)
            {
              case CMOD_MUL_1K: // multiply by  1024
                div = 1024;
                break;
              case CMOD_MUL_2K: // multiply by  2048
                div = 2048;
                break;
              case CMOD_MUL_4K: // multiply by  4096
                div = 4096;
                break;
              case CMOD_MUL_8K: // multiply by  8192
                div = 8192;
                break;
              case CMOD_MUL_16K: // multiply by 16384
                div = 16384;
                break;
              default: break;
            }
            for (int cornerNo = 0; cornerNo < 3; cornerNo++)
            {
              tc[cornerNo].x /= div;
              tc[cornerNo].y /= div;
            }
            *uv = tc[0] + (tc[1] - tc[0]) * u + (tc[2] - tc[0]) * v;
          }
        }
      }

      elem.vertexData->getIB()->unlock();
      elem.vertexData->getVB()->unlock();
    }
  }

  return collision;
}


DynamicRenderableSceneLodsResource *DynamicRenderableSceneLodsResource::loadResource(IGenLoad &crd, int flags, int res_sz,
  const DataBlock *desc)
{
  if (res_sz == -1)
    res_sz = crd.readInt();

  Ptr<ShaderMatVdata> smvd;
  int tmp[4];
  crd.read(tmp, sizeof(int) * 4);

  if (tmp[0] == 0xFFFFFFFF && tmp[1] == 0xFFFFFFFF)
  {
    // model matVdata saved separately from materials (mat & tex must be set in desc BLK)
    const DataBlock *texBlk = desc ? desc->getBlockByName("tex") : nullptr;
    const DataBlock *matRBlk = desc ? desc->getBlockByName("matR") : nullptr;
    if (!desc || !texBlk || !matRBlk)
    {
      logerr("dynModel in %s was saved with sep. materials, but mat data is not found in dynModelDesc.bin:"
             " desc=%p texBlk=%p matRBlk=%p",
        crd.getTargetName(), desc, texBlk, matRBlk);
      return nullptr;
    }
    smvd = ShaderMatVdata::create(texBlk->paramCount(), matRBlk->blockCount(), tmp[2], tmp[3], VDATA_MT_DYNMODEL);
    smvd->makeTexAndMat(*texBlk, *matRBlk);
  }
  else
  {
    smvd = ShaderMatVdata::create(tmp[0], tmp[1], tmp[2], tmp[3], VDATA_MT_DYNMODEL);
    smvd->loadTexStr(crd, flags & SRLOAD_SYMTEX);
  }
#if DAGOR_DBGLEVEL > 0 || _TARGET_PC_WIN
  char vname[64];
  _snprintf(vname, sizeof(vname), "%s ldRes", crd.getTargetName());
  vname[sizeof(vname) - 1] = 0;
#else
  const char *vname = "";
#endif

  unsigned vdataFlags = 0;
  if (flags & SRLOAD_SRC_ONLY)
    vdataFlags |= VDATA_SRC_ONLY;
  if (flags & SRLOAD_BIND_SHADER_RES)
    vdataFlags |= VDATA_BIND_SHADER_RES;
  smvd->loadMatVdata(vname, crd, vdataFlags);

  bool modern_data_fmt_detected = (smvd->getGlobVDataCount() == 0);
  for (int i = 0; i < smvd->getGlobVDataCount(); i++)
    if (smvd->getGlobVData(i)->testFlags(VDATA_PACKED_IB | VDATA_LOD_MASK) ||
        smvd->getGlobVData(i)->getIbSize() < 6 * 3 * smvd->getGlobVData(i)->getIbElemSz())
      modern_data_fmt_detected = true;
  if (!modern_data_fmt_detected)
  {
    char buf[4 << 10];
    logerr("old dynModel format detected (replacing with stub)%s", dgs_get_fatal_context(buf, sizeof(buf)));
    crd.seekrel(res_sz);
    return makeStubRes();
  }

  DynamicRenderableSceneLodsResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) DynamicRenderableSceneLodsResource;
  res->smvdR = smvd;
  crd.read(res->dumpStartPtr(), res_sz);

  if (int skins_count = res->patchAndLoadData(crd, flags, res_sz))
  {
    int endpos = crd.tell();
    int maxVPRConst = d3d::get_driver_desc().maxvpconsts;

    crd.read(tmp, sizeof(int) * 2);
    endpos += tmp[1];

    for (int i = 0; i < tmp[0]; i++)
    {
      crd.read(&tmp[2], sizeof(int) * 2);
      if (tmp[2] <= maxVPRConst)
      {
        crd.seekrel(tmp[3]);
        break;
      }
    }

    res->loadSkins(crd, flags, desc);
    crd.seekto(endpos);
  }

  if (desc)
  {
    const DataBlock *texqBlk = desc->getBlockByName("texScale_data");
    if (texqBlk)
      for (int lodIdx = 0; lodIdx < res->lods.size(); lodIdx++)
        res->lods[lodIdx].texScale = texqBlk->getReal(String(0, "texScale%d", lodIdx), 0);
  }

  if (res->bbox[0].x > res->bbox[1].x || res->bbox[0].y > res->bbox[1].y || res->bbox[0].z > res->bbox[1].z)
  {
    Point3 tmp_p3 = res->bbox[0];
    res->bbox[0] = res->bbox[1];
    res->bbox[1] = tmp_p3;
    res->setBoundPackUsed(true);
  }

  for (int i = res->lods.size() - 1; i >= 0; i--)
    if (res->lods[i].scene && (res->lods[i].scene->getRigidsConst().size() || res->lods[i].scene->getSkinsCount()))
      break;
    else
    {
      char buf[4 << 10];
      if (i == 0)
      {
        logwarn("empty mesh in trailing lod%02d, remains; %s", i, dgs_get_fatal_context(buf, sizeof(buf)));
        break;
      }
      res->lods[i].scene->delRef();
      res->lods[i].scene = nullptr;
      logerr("empty mesh in trailing lod%02d, dropped; %s", i, dgs_get_fatal_context(buf, sizeof(buf)));
      res->lods.init(res->lods.data(), i);
    }

  uint32_t qlBestLod = DynamicRenderableSceneLodsResource::on_higher_lod_required ? max(res->lods.size() - 1, 0u) : 0u;
  if (res->areLodsSplit())
  {
    if (res->smvdR)
      res->smvdR->setLodsAreSplit();
    if (res->smvdS)
      res->smvdS->setLodsAreSplit();
  }
  else
    qlBestLod = 0;
  res->applyQlBestLod(qlBestLod);
  if (res->smvdR)
    res->smvdR->applyFirstLodsSkippedCount(qlBestLod);
  if (res->smvdS)
    res->smvdS->applyFirstLodsSkippedCount(qlBestLod);
  smvd->finalizeMatRefs();
  return res;
}

DynamicRenderableSceneLodsResource *DynamicRenderableSceneLodsResource::makeStubRes()
{
  struct StubDynModel : public DynamicRenderableSceneLodsResource
  {
    Lod lod0;

    struct Names : public DynSceneResNameMapResource
    {
      PatchablePtr<const char> name0;
      uint16_t id0;
      Names()
      {
        name0.setPtr("root");
        id0 = 0;
        node.map.init(&name0, 1);
        node.id.init(&id0, 1);
      }
    };
    struct StubLod : public DynamicRenderableSceneResource
    {
      StubLod(DynSceneResNameMapResource *nm)
      {
        resSize = sizeof(*this) - dumpStartOfs();
        names = nm;
        rigids.init(NULL, 0);
        skins.init(NULL, 0);
        addRef();
      }
      ~StubLod() {}
    };
    StubDynModel()
    {
      uint32_t resSize = sizeof(*this) - dumpStartOfs();
      interlocked_release_store(packedFields, resSize); // set other fileds to zero.
      names = new Names;

      bbox[0].set(-1e-3, -1e-3, -1e-3);
      bbox[1].set(1e-3, 1e-3, 1e-3);
      bpC254[0] = bpC254[1] = bpC254[2] = bpC254[3] = bpC255[0] = bpC255[1] = bpC255[2] = bpC255[3] = 0;

      lods.init(&lod0, 1);
      lod0.scene.setPtr(new StubLod(names));
      lod0.range = 1e5;
    }
  };
  return new StubDynModel;
}

int DynamicRenderableSceneLodsResource::patchAndLoadData(IGenLoad &crd, int flags, int res_sz)
{
  setResSizeNonTS(res_sz); // Non thread safe version is ok here, because we call patchAndLoadData just after an object creation.
  lods.patch(&lods);

  names = DynSceneResNameMapResource::loadResource(crd);

  int skins_count = 0;
  for (int i = 0; i < lods.size(); i++)
  {
    lods[i].scene = DynamicRenderableSceneResource::loadResourceInternal(crd, names, flags, *smvdR, lods[i].scene.toInt());
    lods[i].scene->addRef();
    skins_count += lods[i].scene->getSkinsCount();
  }
  return skins_count;
}
void DynamicRenderableSceneLodsResource::loadSkins(IGenLoad &crd, int flags, const DataBlock *desc)
{
  int tmp[4];
  crd.read(tmp, sizeof(int) * 4);
  if (tmp[0] == 0xFFFFFFFF && tmp[1] == 0xFFFFFFFF)
  {
    // model matVdata saved separately from materials (mat & tex must be set in desc BLK)
    const DataBlock *texBlk = desc ? desc->getBlockByName("tex") : nullptr;
    const DataBlock *matSBlk = desc ? desc->getBlockByName("matS") : nullptr;
    if (!desc || !texBlk || !matSBlk)
    {
      logerr("dynModel in %s was saved with sep. materials, but mat data is not found in dynModelDesc.bin:"
             " desc=%p texBlk=%p matSBlk=%p",
        crd.getTargetName(), desc, texBlk, matSBlk);
      return;
    }
    smvdS = ShaderMatVdata::create(texBlk->paramCount(), matSBlk->blockCount(), tmp[2], tmp[3], VDATA_MT_DYNMODEL);
    smvdS->makeTexAndMat(*texBlk, *matSBlk);
  }
  else
  {
    smvdS = ShaderMatVdata::create(tmp[0], tmp[1], tmp[2], tmp[3], VDATA_MT_DYNMODEL);
    smvdS->getTexIdx(*smvdR);
  }
  unsigned vdataFlags = 0;
  if (flags & SRLOAD_SRC_ONLY)
    vdataFlags |= VDATA_SRC_ONLY;
  vdataFlags |= VDATA_BIND_SHADER_RES;
  smvdS->loadMatVdata(String(200, "%s ldSkin", crd.getTargetName()).str(), crd, vdataFlags);

  ZlibLoadCB z_crd(crd, 0x7FFFFFFF /*unlimited*/);
  for (int i = 0; i < lods.size(); i++)
    lods[i].scene->loadSkins(z_crd, flags, *smvdS);
  z_crd.close();
  smvdS->finalizeMatRefs();
}

void DynamicRenderableSceneLodsResource::addInstanceRef()
{
  int newRefCount = interlocked_increment(instanceRefCount);
  G_ASSERT(newRefCount > 0);
  if (newRefCount == 1)
    acquireTexRefs();
}
void DynamicRenderableSceneLodsResource::delInstanceRef()
{
  int newRefCount = interlocked_decrement(instanceRefCount);
  G_ASSERT(newRefCount >= 0);
  if (newRefCount == 0)
    releaseTexRefs();
}

void DynamicRenderableSceneLodsResource::addToCloneList(const DynamicRenderableSceneLodsResource &from)
{
  originalRes = const_cast<DynamicRenderableSceneLodsResource &>(from).getFirstOriginal();
  nextClonedRes = from.nextClonedRes;
  from.nextClonedRes = this;
}

DynamicRenderableSceneLodsResource::DynamicRenderableSceneLodsResource(const DynamicRenderableSceneLodsResource &from) //-V730
{
  OSSpinlockScopedLock lock(dynres_clones_list_spinlock);

  instanceRefCount = 0;
  uint32_t srcPackedFields =
    interlocked_relaxed_load(from.packedFields) & ~((RESERVED_MASK << RESERVED_SHIFT) | (RES_LD_FLG_MASK << RES_LD_FLG_SHIFT));
  interlocked_relaxed_store(packedFields, srcPackedFields);
  uint32_t resSize = (srcPackedFields >> RES_SIZE_SHIFT) & RES_SIZE_MASK;
  names = from.names;
  smvdR = from.smvdR;
  smvdS = from.smvdS;
  // don't touch (write) bpC254/bpC255 unless boundPackUsed==1; resSize MAY BE smaller than offset between lods and end of bpC255!

  bvhId = from.bvhId;

  void *new_base = &lods;
  const void *old_base = &from.lods;

  memcpy(new_base, old_base, resSize);
  lods.rebase(new_base, old_base);

  for (int i = 0; i < lods.size(); ++i)
    if (lods[i].scene)
    {
      lods[i].scene = lods[i].scene->clone();
      lods[i].scene->addRef();
    }

  addToCloneList(from);
}

DynamicRenderableSceneLodsResource *DynamicRenderableSceneLodsResource::clone() const
{
  void *mem = memalloc(dumpStartOfs() + getResSize(), midmem);
  auto copy = new (mem, _NEW_INPLACE) DynamicRenderableSceneLodsResource(*this);
  copy->bvhId = 0;
  return copy;
}

void DynamicRenderableSceneLodsResource::clearData()
{
  OSSpinlockScopedLock lock(dynres_clones_list_spinlock);

  for (int i = 0; i < lods.size(); i++)
    lods[i].scene->delRef();
  lods.init(NULL, 0);
  names = NULL;

  if (originalRes)
  {
    Ptr<DynamicRenderableSceneLodsResource> tmp_holder = originalRes;

    for (DynamicRenderableSceneLodsResource *r = originalRes; r; r = r->nextClonedRes)
      if (r->nextClonedRes == this)
      {
        r->nextClonedRes = nextClonedRes;
        break;
      }
    originalRes = nullptr;
    nextClonedRes = nullptr;
  }
  else if (getRefCount() > 0) // expect nextClonedRes == nullptr when refCount is suitable for destruction, so code under branch shall
                              // not be called
  {
    Ptr<DynamicRenderableSceneLodsResource> tmp_holder = this;

    for (DynamicRenderableSceneLodsResource *r = nextClonedRes; r; r = r->nextClonedRes)
      r->originalRes = nextClonedRes;
  }
  G_ASSERT(!originalRes && !nextClonedRes);
}
void DynamicRenderableSceneLodsResource::lockClonesList() DAG_TS_ACQUIRE(dynres_clones_list_spinlock)
{
  dynres_clones_list_spinlock.lock();
}
void DynamicRenderableSceneLodsResource::unlockClonesList() DAG_TS_RELEASE(dynres_clones_list_spinlock)
{
  dynres_clones_list_spinlock.unlock();
}


DynamicRenderableSceneResource *DynamicRenderableSceneLodsResource::getLod(real range)
{
  for (int i = 0; i < lods.size(); ++i)
    if (range <= lods[i].range)
    {
      updateReqLod(i);
      return lods[i].scene;
    }
  return NULL;
}


void DynamicRenderableSceneLodsResource::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  for (auto l : lods)
    if (l.scene)
      l.scene->gatherUsedTex(tex_id_list);
}
void DynamicRenderableSceneLodsResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const
{
  for (auto l : lods)
    if (l.scene)
      l.scene->gatherUsedMat(mat_list);
}

void DynamicRenderableSceneLodsResource::gatherLodUsedMat(Tab<ShaderMaterial *> &mat_list, int lod) const
{
  if (0 <= lod && lod < lods.size() && lods[lod].scene)
    lods[lod].scene->gatherUsedMat(mat_list);
}

bool DynamicRenderableSceneLodsResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  bool ok = false;

  for (auto l : lods)
    if (l.scene)
      if (l.scene->replaceTexture(tex_id_old, tex_id_new))
        ok = true;

  return ok;
}
void DynamicRenderableSceneLodsResource::finalizeDuplicateMaterial(dag::Span<ShaderMaterial *> new_mat)
{
  for (int i = 0; i < new_mat.size(); i++)
    new_mat[i]->native().setNonSharedRefCount(new_mat[i]->getRefCount());
}

int DynamicRenderableSceneLodsResource::Lod::getAllElems(Tab<dag::ConstSpan<ShaderMesh::RElem>> &out_elems) const
{
  out_elems.clear();
  for (const DynamicRenderableSceneResource::RigidObject &r : scene->getRigidsConst())
    out_elems.push_back(r.mesh->getMesh()->getAllElems());
  for (const PatchablePtr<ShaderSkinnedMeshResource> &s : scene->getSkins())
    out_elems.push_back(s->getMesh()->getShaderMesh().getAllElems());
  return out_elems.size();
}

// ZZZ  DynamicRenderableSceneInstance  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

static uint32_t dynres_unique_id_gen = 0;

DynamicRenderableSceneInstance::DynamicRenderableSceneInstance(DynamicRenderableSceneLodsResource *res, bool activate_instance) :
  lods(res),
  autoCalcBBox(1),
  disableAutoChooseLod(0),
  instanceActive(0),
  _resv(0),
  forceLod(-1),
  bestLodLimit(-1),
  curLodNo(-1),
  origin(0.f, 0.f, 0.f),
  originPrev(0, 0, 0),
  uniqueId(interlocked_increment(dynres_unique_id_gen))
{
  G_ASSERT(lods);
  if (activate_instance)
    activateInstance();

  G_ASSERT(lods->lods.size());
  // sceneResource left nullptr intentionally to force using setLod(), setCurrentLod() or chooseLodByDistSq() before rendering
  // sceneResource=lods->lods[curLodNo=lods->getQlBestLod()].scene;

  count = lods->getNames().node.nameCount();
  allNodesData.reset(new uint8_t[ALL_DATA_SIZE * count]);
  mem_set_0(hidden());
  for (int i = 0; i < getNodeCount(); i++)
  {
    nodeWtm()[i].identity();
    nodePrevWtm()[i].identity();
    // put initial node pos 100 m below camera for case if node tm not setted properly
    nodeWtm()[i].setcol(3, 0, -100, 0);
    wtmToOriginVec()[i].set(0, -100, 0);
    nodePrevWtm()[i].setcol(3, 0, -100, 0);
    wtmToOriginVecPrev()[i].set(0, -100, 0);
    opacity()[i] = 1;
  }
  clearNodeCollapser();
}


BSphere3 DynamicRenderableSceneInstance::getBoundingSphere() const
{
  if (!lods || lods->lods.size() == 0 || !lods->lods[0].scene)
    return BSphere3(Point3(0.f, 0.f, 0.f), 1.f);

  if (!lods->lods[0].scene->getSkinsCount())
    return lods->lods[0].scene->getBoundingSphere(nodeWtm_ptr());

  //== when skin is present, we add whole bbox also; is not very accurate, but definitely better than doing nothing
  BSphere3 bsph;
  bsph = getBoundingBox();
  bsph += lods->lods[0].scene->getBoundingSphere(nodeWtm_ptr());
  return bsph;
}


DynamicRenderableSceneInstance::~DynamicRenderableSceneInstance()
{
  sceneResource = NULL;
  deactivateInstance();
  lods = NULL;
}

static OSSpinlock dynres_activate_instance_spinlock;

void DynamicRenderableSceneInstance::activateInstance(bool a)
{
  dynres_activate_instance_spinlock.lock();
  if (a && !instanceActive)
  {
    instanceActive = 1;
    lods->addInstanceRef();
  }
  else if (!a && instanceActive)
  {
    instanceActive = 0;
    lods->delInstanceRef();
  }
  dynres_activate_instance_spinlock.unlock();
}

void DynamicRenderableSceneInstance::changeLodsResource(DynamicRenderableSceneLodsResource *new_lods)
{
  if (instanceActive)
  {
    G_ASSERTF_RETURN(lods && new_lods, , "lods=%p -> new_lods=%p while instance active!", lods, new_lods);
    new_lods->addInstanceRef();
    lods->delInstanceRef();
  }
  lods = new_lods;
}

int DynamicRenderableSceneInstance::getNodeId(const char *name) const
{
  if (!lods)
    return -1;
  return lods->getNames().node.getNameId(name);
}

void DynamicRenderableSceneInstance::setLod(int lod)
{
  forceLod = lod;
  if (validateLod(uint32_t(forceLod)))
  {
    lods->updateReqLod(forceLod);
    sceneResource = lods->lods[curLodNo = (forceLod < lods->getQlBestLod() ? lods->getQlBestLod() : forceLod)].scene;
  }
  else
    forceLod = -1;
}

void DynamicRenderableSceneInstance::setBestLodLimit(int lod) { bestLodLimit = min(lod, max((int)lods->lods.size() - 1, 0)); }

int DynamicRenderableSceneInstance::chooseLod(const Point3 &camera_pos, float dist_mul)
{
  if (getNodeCount() > 0)
    return chooseLodByDistSq((camera_pos - nodeWtm()[0].getcol(3)).lengthSq(), dist_mul);
  return lods->lods.size() - 1;
}
int DynamicRenderableSceneInstance::chooseLodByDistSq(float dist_sq, float dist_mul)
{
  distSq = dist_sq;
  if (forceLod >= 0)
  {
    lods->updateReqLod(forceLod);
    sceneResource = lods->lods[curLodNo = (forceLod < lods->getQlBestLod() ? lods->getQlBestLod() : forceLod)].scene;
  }
  else
  {
    curLodNo = lods->getLodNoForDistSq(sqr(lodDistanceScale * dist_mul) * dist_sq);
    if (curLodNo >= 0)
    {
      curLodNo = max(curLodNo, bestLodLimit);
      sceneResource = lods->lods[curLodNo].scene.get();
    }
    else
      sceneResource = NULL;
  }
  return curLodNo;
}

bool DynamicRenderableSceneInstance::matchBetterBoneIdxAndItmForPoint(const Point3 &pos, int &bone_idx, TMatrix &bone_itm)
{
  if (!sceneResource)
    return false;
  return sceneResource->matchBetterBoneIdxAndItmForPoint(pos, bone_idx, bone_itm, *this);
}

int DynamicRenderableSceneInstance::getNodeForBone(uint32_t bone_idx) const
{
  if (!sceneResource)
    return -1;
  return sceneResource->getNodeForBone(bone_idx);
}

int DynamicRenderableSceneInstance::getBoneForNode(int node_idx) const
{
  if (!sceneResource)
    return -1;
  return sceneResource->getBoneForNode(node_idx);
}

void DynamicRenderableSceneInstance::beforeRender(const Point3 &view_pos)
{
  if (!disableAutoChooseLod)
    chooseLod(view_pos);
}

#define RENDER_HEADER          \
  if (!sceneResource)          \
    return;                    \
  if (lods->isBoundPackUsed()) \
    d3d::set_vs_const(shglobvars::dynamic_pos_unpack_reg, lods->bpC254, 2); /* pos_unp_s_ofs, pos_unp_s_mul. */
#define RENDER_FOOTER

void DynamicRenderableSceneInstance::render(real opacity_)
{
  RENDER_HEADER;
  sceneResource->render(*this, opacity_);
  RENDER_FOOTER;
}

void DynamicRenderableSceneInstance::renderTrans(real opacity_)
{
  RENDER_HEADER;
  sceneResource->renderTrans(*this, opacity_);
  RENDER_FOOTER;
}

void DynamicRenderableSceneInstance::renderDistortion()
{
  RENDER_HEADER;
  sceneResource->renderDistortion(*this);
  RENDER_FOOTER;
}

void DynamicRenderableSceneInstance::showSkinnedNodesConnectedToBone(int bone_id, bool need_show)
{
  if (!sceneResource)
    return;

  for (int i = 0; i < sceneResource->getSkins().size(); ++i)
  {
    const ShaderSkinnedMesh &mesh = *sceneResource->getSkins()[i]->getMesh();
    const int skinnedNodeId = sceneResource->getSkinNodes()[i];
    if (isNodeHidden(skinnedNodeId) || getNodeOpacity(skinnedNodeId) < 1.f)
      continue;

    if (mesh.hasBone(bone_id))
      setNodeOpacity(skinnedNodeId, need_show ? 1.f : 0.f);
  }
}

void DynamicRenderableSceneInstance::clipNodes(const Frustum &frustum, dag::Vector<int, framemem_allocator> &node_list)
{
  node_list.clear();

  if (!sceneResource)
    return;

  auto rigids = sceneResource->getRigids();
  auto skinNodes = sceneResource->getSkinNodes();

  if (rigids.empty() && skinNodes.empty())
    return;

  node_list.reserve(rigids.size() + skinNodes.size());

  // Test all nodes against the frustum. Note that it will not work
  // well with non-uniform scaling, as the sphere radius is only
  // scaled by the length of the X axis. But hopefully cockpits
  // are not scaled non-uniformly.
  for (auto &rigid : rigids)
  {
    if (isNodeHidden(rigid.nodeId))
      continue;

    TMatrix nodeTm = getNodeWtm(rigid.nodeId);
    Point3 sphC = rigid.sph_c * nodeTm;
    float sphR = rigid.sph_r * nodeTm.getcol(0).length();

    if (frustum.testSphereB(sphC, sphR) != Frustum::OUTSIDE)
      node_list.push_back(rigid.nodeId);
  }

  // Don't bother with clipping skinned meshes. Just render them.
  for (auto &node : skinNodes)
  {
    if (isNodeHidden(node))
      continue;

    node_list.push_back(node);
  }
}

void DynamicRenderableSceneInstance::findRigidNodesWithMaterials(dag::ConstSpan<const char *> material_names,
  const eastl::fixed_function<2 * sizeof(void *), void(int)> &cb) const
{
  if (!sceneResource)
    return;

  return sceneResource->findRigidNodesWithMaterials(material_names, cb);
}
