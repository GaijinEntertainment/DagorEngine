// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <libTools/shaderResBuilder/lodsEqMatGather.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/meshUtil.h>
#include <libTools/ambientOcclusion/ambientOcclusion.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <libTools/dagFileRW/normalizeNodeTm.h>
#include <shaders/dag_shaderMesh.h>
#include <math/dag_mesh.h>
#include <math/dag_boundingSphere.h>
#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include <obsolete/dag_cfg.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zlibIo.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <startup/dag_restart.h>
#include <util/dag_bitArray.h>
#include <math/random/dag_random.h>
#include <sceneRay/dag_sceneRay.h>
#include "collapseData.h"
#include <debug/dag_log.h>
#include <debug/dag_debug.h>

// 0 - means no AO
#define RAYS_PER_VERTEX_AMBIENT_OCCLUSION 8

static int issued_fatal_count = 0;

#define CLEAR_FATALS_ISSUED() issued_fatal_count = 0

#define ISSUE_FATAL(...)                               \
  do                                                   \
  {                                                    \
    issued_fatal_count++;                              \
    if (log)                                           \
      log->addMessage(ILogWriter::FATAL, __VA_ARGS__); \
    else                                               \
      DAG_FATAL(__VA_ARGS__);                          \
  } while (0)

#define WERE_FATALS_ISSUED() (issued_fatal_count > 0 || (log ? log->hasErrors() : false))

enum
{
  AO_INDEX1 = 100,
  AO_INDEX2 = 101,
  AO_INDEX3 = 102,
};

extern bool dynmodel_use_direct_bones_array;
extern bool dynmodel_use_direct_bones_array_combined_lods;
extern bool dynmodel_optimize_direct_bones;
extern bool shadermeshbuilder_strip_d3dres;
extern void prepare_billboard_mesh(Mesh &mesh, const TMatrix &wtm, dag::ConstSpan<Point3> org_verts);


//************************************************************************
//* variants for skinned meshes
//************************************************************************
int dynmodel_max_vpr_const_count = 256 * 3;

static unsigned max_vpr_const_for_skinned_mesh() { return dynmodel_max_vpr_const_count; }

StaticSceneRayTracer *theRayTracer = NULL;

//=== DynamicRenderableSceneLodsResSrc =====================================//

bool dynmodel_exp_empty_bone_names_allowed = false;
bool DynamicRenderableSceneLodsResSrc::optimizeForCache = true;
int DynamicRenderableSceneLodsResSrc::limitBonePerVertex = -1;
int DynamicRenderableSceneLodsResSrc::setBonePerVertex = -1;
bool (*DynamicRenderableSceneLodsResSrc::process_scene)(const char *fn, int lod_n, AScene &sc, float start_range,
  const DataBlock &props) = NULL;
DataBlock *DynamicRenderableSceneLodsResSrc::buildResultsBlk = NULL;
bool DynamicRenderableSceneLodsResSrc::sepMatToBuildResultsBlk = false;


static DagorMatShaderSubstitute matSubst;
static const char *curDagFname = NULL;


int DynamicRenderableSceneLodsResSrc::Lod::addRigid(int node_id)
{
  int i;
  for (i = 0; i < rigids.size(); ++i)
    if (rigids[i].nodeId == node_id)
      return i;

  i = append_items(rigids, 1);
  rigids[i].nodeId = node_id;
  return i;
}


int DynamicRenderableSceneLodsResSrc::Lod::addSkin(const char *node_name)
{
  skins.push_back(SkinnedObj());
  skins.back().nodeId = -1;
  skins.back().nodeName = node_name;
  return skins.size() - 1;
}


DynamicRenderableSceneLodsResSrc::SkinnedObj::~SkinnedObj() { tabutils::deleteAll(meshData); }

DynamicRenderableSceneLodsResSrc::DynamicRenderableSceneLodsResSrc(IProcessMaterialData *pm) :
  lods(tmpmem), matSaverSkins(tmpmem), processMat(pm), textureSlots(tmpmem)
{
  matSaverSkins.resize(1);
}


void DynamicRenderableSceneLodsResSrc::splitRealTwoSided(Mesh &m, Bitarray &is_material_real_two_sided_array, int side_channel_id)
{
  split_real_two_sided(m, is_material_real_two_sided_array, side_channel_id);
}

class HasAOChanCB : public ShaderChannelsEnumCB
{
public:
  bool hasAO;
  HasAOChanCB() : hasAO(false) {}

  void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int) override
  {
    if (u == SCUSAGE_EXTRA)
      if (ui == AO_INDEX1 || ui == AO_INDEX2 || ui == AO_INDEX3)
        hasAO = true;
  }
};

void DynamicRenderableSceneLodsResSrc::calcMeshNodeBbox(Node *n_)
{
  if (!n_)
    return;
  Node &n = *n_;
  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (!mh.bones && mh.mesh)
    {
      // add mesh
      Mesh &mesh = *mh.mesh;
      const Node *key_node = n_;
      TMatrix tm = inverse(key_node->wtm) * n.wtm;

      if (forceLeftSideMatrices && (key_node->wtm.det() > 0)) //< if linked to root whose wtm==tm and is right-sided
      {
        TMatrix ptm = key_node->wtm;
        ptm.setcol(1, key_node->wtm.getcol(2));
        ptm.setcol(2, key_node->wtm.getcol(1));
        tm = inverse(ptm) * n.wtm;
      }
      //== billboards are not handled properly

      for (int i = 0; i < mesh.getVert().size(); ++i)
        bbox += n.wtm * mesh.getVert()[i];
      for (int i = 0; i < mesh.getVert().size(); ++i)
        lbbox += tm * mesh.getVert()[i];

      if (wAllVert && mesh.getVert().size())
      {
        int cnt = mesh.getVert().size();
        int base = append_items(*wAllVert, cnt);
        for (int i = 0; i < cnt; ++i)
          wAllVert[0][base + i] = n.wtm * mesh.getVert()[i];
        bsph += ::mesh_bounding_sphere(&wAllVert[0][base], cnt);
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    calcMeshNodeBbox(n.child[i]);
}
void DynamicRenderableSceneLodsResSrc::calcSkinNodeBbox(Node *n_)
{
  if (!n_)
    return;
  Node &n = *n_;
  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.bones && mh.mesh)
    {
      for (int i = 0; i < mh.bones->boneNames.size(); ++i)
      {
        G_ASSERTF(mh.bones->boneNames[i][0] || dynmodel_exp_empty_bone_names_allowed, "unnamed bone #%d in skinmesh node <%s>", i,
          n.name);
        nodeNameMap.addNameId(mh.bones->boneNames[i]);
      }

      // add skinned object
      Mesh &mesh = *mh.mesh;

      for (int i = 0; i < mesh.getVert().size(); ++i)
      {
        bbox += n.wtm * mesh.getVert()[i]; //== has little sense since skin is surely animated; but this is better than nothing!
        lbbox += n.wtm * mesh.getVert()[i];
      }
      if (wAllVert)
      {
        int cnt = mesh.getVert().size();
        int base = append_items(*wAllVert, cnt);
        for (int i = 0; i < cnt; ++i)
          wAllVert[0][base + i] = n.wtm * mesh.getVert()[i];
        bsph += ::mesh_bounding_sphere(&wAllVert[0][base], cnt);
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    calcSkinNodeBbox(n.child[i]);
}

// add mesh node
void DynamicRenderableSceneLodsResSrc::addMeshNode(Lod &lod, Node *n_, LodsEqualMaterialGather &mat_gather)
{
  if (!n_)
    return;
  Node &n = *n_;

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (!mh.bones && mh.mesh && mh.mesh->vert.size())
    {
      // add mesh
      Mesh &mesh = *mh.mesh;

      if (shadermeshbuilder_strip_d3dres)
        return;

      // Process real_two_sided.

      bool meshHasAO = false;

      Bitarray isMaterialRealTwoSidedArray;
      isMaterialRealTwoSidedArray.resize(n.mat->subMatCount());
      for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(materialNo);
        if (!subMat)
        {
          isMaterialRealTwoSidedArray.reset(materialNo);
          continue;
        }
        if (theRayTracer && !meshHasAO)
        {
          matSubst.substMatClass(*subMat);
          subMat = processMaterial(subMat, false);
          Ptr<ShaderMaterial> m = new_shader_material(*subMat, true, false);
          if (m)
          {
            HasAOChanCB hasAOcb;
            int flags;
            if (m->enum_channels(hasAOcb, flags))
              meshHasAO = hasAOcb.hasAO;
          }
        }

        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", subMat->matScript.str()), "q");

        if (c.getbool("real_two_sided", false))
          isMaterialRealTwoSidedArray.set(materialNo);
        else
          isMaterialRealTwoSidedArray.reset(materialNo);
      }

      MeshData &meshData = mesh.getMeshData();
      int sideChannelId = meshData.add_extra_channel(MeshData::CHT_FLOAT1, SCUSAGE_EXTRA, 55);
      G_ASSERT(sideChannelId >= 0 && sideChannelId < meshData.extra.size());
      MeshData::ExtraChannel &sideChannel = meshData.extra[sideChannelId];
      sideChannel.resize_verts(2);
      G_ASSERT(data_size(sideChannel.vt) >= 8);
      ((float *)sideChannel.vt.data())[0] = 1.f;
      ((float *)sideChannel.vt.data())[1] = -1.f;
      for (unsigned int faceNo = 0; faceNo < meshData.face.size(); faceNo++)
      {
        for (unsigned int cornerNo = 0; cornerNo < 3; cornerNo++)
          sideChannel.fc[faceNo].t[cornerNo] = 0;
      }


      splitRealTwoSided(mesh, isMaterialRealTwoSidedArray, sideChannelId);


      const Node *key_node = n_;

      // debug("\n\nmesh '%s'", (const char*)n.name);

      TMatrix tm = inverse(key_node->wtm) * n.wtm;
      TMatrix toWorld = key_node->wtm;

      bool billboard = false;
      DataBlock blk;
      if (dblk::load_text(blk, make_span_const(n.script), dblk::ReadFlag::ROBUST, curDagFname))
      {
        billboard = blk.getBool("billboard", false);
      }


      if (forceLeftSideMatrices && (key_node->wtm.det() > 0)) //< if linked to root whose wtm==tm and is right-sided
      {
        TMatrix ptm = key_node->wtm;
        ptm.setcol(1, key_node->wtm.getcol(2));
        ptm.setcol(2, key_node->wtm.getcol(1));
        tm = inverse(ptm) * n.wtm;
        toWorld = ptm;

        debug("swapped axes");
      }

      // transform mesh to key node space
      int i;
      // prepare mesh for building
      int numMat = n.mat->subMatCount();
      mesh.clampMaterials(numMat - 1);

      G_VERIFY(mesh.sort_faces_by_mat());

      int t0 = get_time_msec();
      int tc0 = mesh.tvert[0].size(), tc1 = mesh.tvert[1].size(), tc2 = mesh.tvert[2].size(), tc3 = mesh.tvert[3].size(),
          cvert = mesh.cvert.size(), vnorm = mesh.vertnorm.size();

      mesh.optimize_tverts();
      if (get_time_msec() > t0 + 1000)
        logwarn("node=\"%s\" mesh.optimize_tverts() takes %.2f sec; "
                "tc0=%d->%d tc1=%d->%d tc2=%d->%d tc3=%d->%d  cvert=%d->%d vertnorm=%d->%d",
          n.name, (get_time_msec() - t0) / 1000.f, tc0, mesh.tvert[0].size(), tc1, mesh.tvert[1].size(), tc2, mesh.tvert[2].size(),
          tc3, mesh.tvert[3].size(), cvert, mesh.cvert.size(), vnorm, mesh.vertnorm.size());
      if (!mesh.vertnorm.size())
      {
        G_VERIFY(mesh.calc_ngr());
        if (mesh.face.size())
          G_VERIFY(mesh.calc_vertnorms());
        else
          logerr("no faces in mesh");
      }
      mesh.transform(tm);
      create_vertex_color_data(mesh, SCUSAGE_EXTRA, 53);

      int prt1, prt2, prt3;
      if (meshHasAO)
      {
        prt1 = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, AO_INDEX1);
        prt2 = mesh.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, AO_INDEX2);
        prt3 = mesh.add_extra_channel(MeshData::CHT_FLOAT1, SCUSAGE_EXTRA, AO_INDEX3);
        MeshData::ExtraChannel &prt1e = meshData.extra[prt1];
        MeshData::ExtraChannel &prt2e = meshData.extra[prt2];
        MeshData::ExtraChannel &prt3e = meshData.extra[prt3];
        prt1e.resize_verts(meshData.vertnorm.size());
        prt2e.resize_verts(meshData.vertnorm.size());
        prt3e.resize_verts(meshData.vertnorm.size());
      }
      if (meshHasAO && theRayTracer)
      {
        float maxDist = theRayTracer->getSphere().r * 2;
        calculatePRT(meshData, toWorld, theRayTracer, RAYS_PER_VERTEX_AMBIENT_OCCLUSION, 1000, 100, maxDist, prt1, prt2, prt3);
      }
      else if (meshHasAO)
      {
        MeshData::ExtraChannel &prt1e = meshData.extra[prt1];
        MeshData::ExtraChannel &prt2e = meshData.extra[prt2];
        MeshData::ExtraChannel &prt3e = meshData.extra[prt3];
        for (i = 0; i < meshData.vertnorm.size(); ++i)
        {
          ((Point4 *)&prt1e.vt[0])[i] = Point4(1, meshData.vertnorm[i].y, meshData.vertnorm[i].z, meshData.vertnorm[i].x);
          ((Point4 *)&prt2e.vt[0])[i] = Point4(0, 0, 0, 0);
          ((float *)&prt3e.vt[0])[i] = 0;
        }

        for (i = 0; i < meshData.facengr.size(); ++i)
        {
          memcpy(&prt1e.fc[i].t[0], &meshData.facengr[i], sizeof(TFace));
          memcpy(&prt2e.fc[i].t[0], &meshData.facengr[i], sizeof(TFace));
          memcpy(&prt3e.fc[i].t[0], &meshData.facengr[i], sizeof(TFace));
        }
      }

      if (mh.morphTargets.size())
      {
        logwarn("morph meshes support dropped on 03.03.09");
        goto add_children;
      }

      Tab<ShaderMaterial *> shmat(tmpmem);
      shmat.resize(numMat);
      mem_set_0(shmat);

      bool has_any_mat = false;
      for (i = 0; i < numMat; ++i)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(i);
        if (!subMat)
        {
          logwarn("invalid sub-material #%d in '%s'", i + 1, lod.fileName);
          continue;
        }

        if (!matSubst.substMatClass(*subMat))
          continue;

        subMat = processMaterial(subMat);
        shmat[i] = mat_gather.addEqual((MaterialData *)subMat);
        if (shmat[i])
          has_any_mat = true;
      }
      if (!has_any_mat || !mesh.getVert().size() || !mesh.getFace().size()) // no renderable geometry
        goto add_children;

      int rigidId = lod.addRigid(nodeNameMap.addNameId(n.parent ? n.name.c_str() : "@root"));

      for (i = 0; i < mesh.getVert().size(); i++)
        lod.rigids[rigidId].sph += mesh.getVert()[i];

      if (billboard)
      {
        SmallTab<Point3, TmpmemAlloc> org_verts;
        org_verts = mesh.getVert();
        prepare_billboard_mesh(mesh, key_node->wtm, org_verts);
      }


      {
        // build ShaderMeshData
        ShaderMeshData &md = lod.rigids[rigidId].meshData;
        G_ASSERTF(md.elems.size() == 0, "md.elem.size()=%d node=<%s> key_node=<%s>", md.elems.size(), n.name, key_node->name);

        ::collapse_materials(mesh, shmat);
        md.build(mesh, shmat.data(), numMat, IdentColorConvert::object, false, rigidId);

        if (optimizeForCache)
          md.optimizeForCache();
        if (!md.elems.size())
        {
          G_ASSERTF(rigidId == lod.rigids.size() - 1, "empty rigid: rigidId=%d lod.rigids.count=%d", rigidId, lod.rigids.size());
          lod.rigids.pop_back();
        }
      }
    }
  }

add_children:
  for (int i = 0; i < n.child.size(); ++i)
  {
    addMeshNode(lod, n.child[i], mat_gather);
  }
}

static void gatherSkinNodeUsedBones(Node *n_, MeshBones &nmb)
{
  if (!n_)
    return;
  Node &n = *n_;

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.bones && mh.mesh)
    {
      Mesh &mesh = *mh.mesh;

      DataBlock node_blk;
      dblk::load_text(node_blk, make_span_const(n.script), dblk::ReadFlag::ROBUST, "Node::script");
      int limit_bpv = node_blk.getInt("limitBonePerVertex", DynamicRenderableSceneLodsResSrc::limitBonePerVertex);

      int bcount = mh.bones->boneNames.size();
      int vcount = mesh.getVert().size();
      Tab<bool> bone_used;
      bone_used.resize(bcount);
      mem_set_0(bone_used);
      Tab<real> boneinf(tmpmem);
      boneinf = mh.bones->boneinf;

      for (int i = 0; i < vcount; i++)
      {
        for (int j = 0; j < MAX_SKINBONES; j++)
        {
          real bw = 0;
          int bb = -1;
          for (int bi = 0; bi < bcount; ++bi)
          {
            real w = boneinf[i + bi * vcount];
            if (w > bw)
            {
              bw = w;
              bb = bi;
            }
          }
          if (bb < 0)
            break;
          bone_used[bb] = true;
          if (limit_bpv > 0 && j + 1 >= limit_bpv)
            break;
          boneinf[i + bb * vcount] = 0;
        }
      }

      mh.bones->bonetm.resize(bcount);
      mem_set_0(mh.bones->bonetm);
      for (int bi = 0; bi < bcount; ++bi)
        if (bone_used[bi])
        {
          mh.bones->bonetm[bi].m[0][0] = -1;
          for (int k = 0; k < nmb.boneNames.size(); k++)
            if (strcmp(nmb.boneNames[k], mh.bones->boneNames[bi]) == 0 &&
                memcmp(&nmb.orgtm[k], &mh.bones->orgtm[bi], sizeof(TMatrix)) == 0) //-V1014
            {
              mh.bones->bonetm[bi].m[0][0] = k;
              break;
            }
          if (mh.bones->bonetm[bi].m[0][0] < 0)
          {
            mh.bones->bonetm[bi].m[0][0] = nmb.boneNames.size();
            nmb.boneNames.push_back() = mh.bones->boneNames[bi];
            nmb.orgtm.push_back(mh.bones->orgtm[bi]);
            nmb.bonetm.push_back(TMatrix::IDENT);
          }
        }
        else
          mh.bones->bonetm[bi].m[0][0] = -1;
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    gatherSkinNodeUsedBones(n.child[i], nmb);
}

static void remapMeshSkinning(Node *n_, MeshBones &nmb, ILogWriter *log)
{
  if (!n_)
    return;
  Node &n = *n_;

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.bones && mh.mesh)
    {
      Mesh &mesh = *mh.mesh;

      int bcount = mh.bones->boneNames.size();
      int vcount = mesh.getVert().size();

      MeshBones old(*mh.bones);
      bool found_any_used_bone = false;
      for (const SimpleString &s : mh.bones->boneNames)
        if (find_value_idx(nmb.boneNames, s) >= 0)
        {
          found_any_used_bone = true;
          break;
        }
      if (!found_any_used_bone)
        ISSUE_FATAL("skinmesh in node <%s> doesn't use any bone", n.name.str());
      *mh.bones = nmb;

      mh.bones->boneinf.resize(nmb.boneNames.size() * vcount);
      mem_set_0(mh.bones->boneinf);
      for (int bi = 0; bi < bcount; ++bi)
      {
        int new_bi = int(old.bonetm[bi].m[0][0]);
        if (new_bi >= 0)
          mem_copy_to(make_span(old.boneinf).subspan(bi * vcount, vcount), &mh.bones->boneinf[new_bi * vcount]);
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    remapMeshSkinning(n.child[i], nmb, log);
}

// add skin node
void DynamicRenderableSceneLodsResSrc::addSkinNode(Lod &lod, Node *n_, LodsEqualMaterialGather &mat_gather)
{
  if (!n_)
    return;
  Node &n = *n_;

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.bones && mh.mesh)
    {
      // add skinned object
      Mesh &mesh = *mh.mesh;

      if (shadermeshbuilder_strip_d3dres)
        return;

      debug("skin found '%s'", (const char *)n.name);

      // enum materials
      int numMat = n.mat->subMatCount();

      Tab<ShaderMaterial *> shmat(tmpmem);
      shmat.resize(numMat);
      mem_set_0(shmat);

      int i;
      Bitarray isMaterialRealTwoSidedArray;
      isMaterialRealTwoSidedArray.resize(numMat);

      Bitarray isMaterialUsed;
      isMaterialUsed.resize(numMat);
      isMaterialUsed.reset();

      MeshData &meshData = mesh.getMeshData();
      for (i = 0; i < meshData.face.size(); ++i)
        if (meshData.face[i].mat < numMat)
          isMaterialUsed.set(meshData.face[i].mat);
        else if (numMat == 1)
          isMaterialUsed.set(0);

      for (i = 0; i < numMat; ++i)
      {
        if (!isMaterialUsed.get(i))
        {
          logwarn("node \"%s\" with %d mat(s): unused mat[%d]=\"%s\" class=\"%s\"", n.name, numMat, i, n.mat->getSubMat(i)->matName,
            n.mat->getSubMat(i)->className); //-V522
          isMaterialRealTwoSidedArray.reset(i);
          continue;
        }

        Ptr<MaterialData> subMat = n.mat->getSubMat(i);
        if (!subMat)
          DAG_FATAL("invalid sub-material #%d of '%s'", i + 1, (const char *)n.name);

        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", subMat->matScript.str()), "q");

        if (c.getbool("real_two_sided", false))
          isMaterialRealTwoSidedArray.set(i);
        else
          isMaterialRealTwoSidedArray.reset(i);

        if (!matSubst.substMatClass(*subMat))
          continue;
        subMat = processMaterial(subMat);
        ShaderMaterial *m = new_shader_material(*subMat, true);
        G_ASSERTF(m, "node=%s (%d verts, %d faces) mat[%d] name=\"%s\" shader=\"%s\" script=\"%s\"", n.name, meshData.vert.size(),
          meshData.face.size(), i, subMat->matName, subMat->className, subMat->matScript);
        shmat[i] = m;
      }

      int sideChannelId = meshData.add_extra_channel(MeshData::CHT_FLOAT1, SCUSAGE_EXTRA, 55);
      G_ASSERT(sideChannelId >= 0 && sideChannelId < meshData.extra.size());
      MeshData::ExtraChannel &sideChannel = meshData.extra[sideChannelId];
      sideChannel.resize_verts(2);
      G_ASSERT(data_size(sideChannel.vt) >= 8);
      ((float *)sideChannel.vt.data())[0] = 1.f;
      ((float *)sideChannel.vt.data())[1] = -1.f;
      for (unsigned int faceNo = 0; faceNo < meshData.face.size(); faceNo++)
      {
        for (unsigned int cornerNo = 0; cornerNo < 3; cornerNo++)
          sideChannel.fc[faceNo].t[cornerNo] = 0;
      }


      splitRealTwoSided(mesh, isMaterialRealTwoSidedArray, sideChannelId);

      // convert mesh data
      // debug("skin wtm: " FMT_TM "", TMD(n.wtm));
      mesh.transform(n.wtm);

      int skinId = lod.addSkin(n.name);

      // build and save skins for different VPRConst count
      for (i = 0; i < matSaverSkins.size(); i++)
      {
        int curCount = max_vpr_const_for_skinned_mesh();

        if (curCount < 0)
        {
          logwarn("try to build skin with no VPR consts!");
          continue;
        }

        ShaderSkinnedMeshData *meshData = new ShaderSkinnedMeshData;

        DataBlock node_blk;
        dblk::load_text(node_blk, make_span_const(n.script), dblk::ReadFlag::ROBUST, "Node::script");
        int limit_bpv = node_blk.getInt("limitBonePerVertex", limitBonePerVertex);
        int set_bpv = node_blk.getInt("setBonePerVertex", setBonePerVertex);

        if (limit_bpv > 0)
          meshData->limitBonesPerVertex(limit_bpv);
        if (set_bpv > 0)
          meshData->setBonesPerVertex(set_bpv);

        Mesh localMesh = mesh;
        bool needToPackVcolor = false;
        for (int j = 0; j < n.mat->subMatCount(); ++j)
          if (auto *submat = n.mat->getSubMat(j))
            if (strstr(submat->matScript, "cut_mesh=1"))
              needToPackVcolor = true;
        if (!meshData->build(localMesh, *mh.bones, shmat.data(), shmat.size(), curCount, &nodeNameMap, needToPackVcolor))
        {
          DAG_FATAL("cannot build skinned mesh data! (node='%s'; VPRConstCount=%d)", (const char *)n.name, curCount);
        }

        // enum new materials
        const ShaderSkinnedMeshData::MaterialDescTab &mdTab = meshData->getMaterialDescTab();

        for (int j = 0; j < mdTab.size(); j++)
        {
          if (matSaverSkins[i].getmatindex(mdTab[j].shMat) < 0)
          {
            matSaverSkins[i].addMaterial(mdTab[j].shMat, texSaver, n.mat->getSubMat(mdTab[j].srcMatIdx));
          }
        }

        if (optimizeForCache)
          meshData->optimizeForCache();

        lod.skins[skinId].addSkinVariant(meshData);
      }

      for (i = 0; i < shmat.size(); i++)
      {
        if (!shmat[i])
          continue;
        if (shmat[i]->getRefCount())
        {
          shmat[i]->delRef();
        }
        else
        {
          delete shmat[i];
        }
      }
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
  {
    addSkinNode(lod, n.child[i], mat_gather);
  }
}

static bool meshesHaveAO(Node &n, DynamicRenderableSceneLodsResSrc *dm)
{
  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (!mh.bones && mh.mesh)
    {
      Mesh &mesh = *mh.mesh;
      for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(materialNo);
        if (!subMat)
          continue;
        matSubst.substMatClass(*subMat);
        subMat = dm->processMaterial(subMat, false);
        Ptr<ShaderMaterial> m = new_shader_material(*subMat, true, false);
        if (!m)
          continue;
        int flags = 0;
        HasAOChanCB hasAOcb;
        if (!m->enum_channels(hasAOcb, flags))
          continue;
        if (hasAOcb.hasAO)
          return true;
      }
    }
  }
  for (int i = 0; i < n.child.size(); ++i)
  {
    if (meshesHaveAO(*n.child[i], dm))
      return true;
  }
  return false;
}

static void addMeshToBuilder(Node &n, BuildableStaticSceneRayTracer *rayTracer, DynamicRenderableSceneLodsResSrc *dm)
{
  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (!mh.bones && mh.mesh)
    {
      // add mesh
      const Mesh &mesh = *mh.mesh;
      SmallTab<Point3, TmpmemAlloc> vert;
      clear_and_resize(vert, mesh.getVert().size());
      for (int i = 0; i < mesh.getVert().size(); ++i)
        vert[i] = n.wtm * mesh.getVert()[i];


      Bitarray isMaterialTrans;
      isMaterialTrans.resize(n.mat->subMatCount());
      isMaterialTrans.reset();
      int totalTrans = 0;
      for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
      {
        Ptr<MaterialData> subMat = n.mat->getSubMat(materialNo);
        if (!subMat)
          continue;
        matSubst.substMatClass(*subMat);
        subMat = dm->processMaterial(subMat, false);
        Ptr<ShaderMaterial> m = new_shader_material(*subMat, true, false);
        if (!m)
          continue;
        int flags = 0;
        class NullChanCB : public ShaderChannelsEnumCB
        {
        public:
          NullChanCB() {}

          void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int) override {}
        } ncb;
        if (!m->enum_channels(ncb, flags))
          continue;
        if ((flags & SC_STAGE_IDX_MASK) >= ShaderMesh::STG_decal)
        {
          isMaterialTrans.set(materialNo);
          totalTrans++;
        }
      }
      if (totalTrans < n.mat->subMatCount())
      {
        SmallTab<unsigned, TmpmemAlloc> flags;
        clear_and_resize(flags, mesh.getFace().size());
        for (int i = 0; i < flags.size(); ++i)
        {
          int mat = mesh.getFaceMaterial(i);
          if (mat >= isMaterialTrans.size())
            mat = isMaterialTrans.size() - 1;
          flags[i] = isMaterialTrans[mat] ? rayTracer->USER_INVISIBLE : rayTracer->CULL_BOTH;
        }
        rayTracer->addmesh(&vert[0], vert.size(), (const unsigned int *)&mesh.getFace()[0].v[0], elem_size(mesh.getFace()),
          mesh.getFace().size(), flags.data(), false);
      }
    }
  }
  for (int i = 0; i < n.child.size(); ++i)
  {
    addMeshToBuilder(*n.child[i], rayTracer, dm);
  }
}

void DynamicRenderableSceneLodsResSrc::addNode(Lod &lod, Node *n_, LodsEqualMaterialGather &mat_gather)
{
  if (!n_)
    return;
  Node &n = *n_;

  BuildableStaticSceneRayTracer *rayTracer = NULL;
  bool hasAO = !shadermeshbuilder_strip_d3dres && meshesHaveAO(n, this);
  theRayTracer = NULL;
  if (hasAO)
    debug("Ambient Occlusion needed.");
    //__int64 refTime = ref_time_ticks();
#if RAYS_PER_VERTEX_AMBIENT_OCCLUSION
  if (hasAO)
  {
    debug("Computing PRT...");
    rayTracer = create_buildable_staticmeshscene_raytracer(Point3(0.20, 0.20, 0.20), 6);
    addMeshToBuilder(n, rayTracer, this);
    // debug("adding %dus", get_time_usec ( refTime ));
    // refTime = ref_time_ticks();
    rayTracer->rebuild();
  }
#endif
  // debug("building %dus",get_time_usec ( refTime ));

  // enum skins
  addSkinNode(lod, &n, mat_gather);
  if (rayTracer)
  {
    theRayTracer = rayTracer;
    theRayTracer->setCullFlags(StaticSceneRayTracer::CULL_BOTH);
  }
  // enum meshes
  addMeshNode(lod, &n, mat_gather);
  theRayTracer = NULL;
  if (rayTracer)
    delete rayTracer;
}

bool DynamicRenderableSceneLodsResSrc::addLod(const char *filename, real range, LodsEqualMaterialGather &mat_gather,
  Tab<AScene *> &scene_list, bool all_animated, const DataBlock &props, bool need_reset_nodes_tm_scale,
  const DataBlock &material_overrides)
{
  PtrTab<MaterialData> matList;
  scene_list.push_back(new AScene());
  AScene &sc = *scene_list.back();
  if (!load_ascene((char *)filename, sc, LASF_NULLMATS | LASF_MATNAMES, false, &matList))
  {
    ISSUE_FATAL("Failed to load %s", filename);
    return false;
  }

  override_materials(matList, material_overrides, nullptr);

  sc.root->calc_wtm();
  if (log)
    if (!matSubst.checkMatClasses(sc.root, filename, *log))
      return false;

  if (need_reset_nodes_tm_scale)
    normalizeNodeTreeTm(*sc.root, TMatrix::IDENT, true);

  int lodId = append_items(lods, 1);
  Lod &lod = lods[lodId];

  lod.fileName = filename;
  lod.range = range;

  curDagFname = filename;
  {
    OAHashNameMap<true> skinNodes;
    ::add_skin_node_names(sc.root, skinNodes);
    ::collapse_nodes(sc.root, sc.root, true, all_animated, curDagFname, skinNodes, false);
  }
  if (DynamicRenderableSceneLodsResSrc::process_scene)
    if (!DynamicRenderableSceneLodsResSrc::process_scene(filename, lodId, sc, lodId > 0 ? lods[lodId - 1].range : 0, props))
    {
      ISSUE_FATAL("Failed to process %s", filename);
      return false;
    }
  curDagFname = NULL;
  return true;
}

void DynamicRenderableSceneLodsResSrc::setupMatSubst(const DataBlock &blk) { matSubst.setupMatSubst(blk); }

bool DynamicRenderableSceneLodsResSrc::build(const DataBlock &blk)
{
  CLEAR_FATALS_ISSUED();
  useSymTex = blk.getBool("useSymTex", false);
  forceLeftSideMatrices = blk.getBool("forceLeftSideMatrices", false);
  bool all_animated = blk.getBool("all_animated_nodes", false);
  doNotSplitLods = blk.getBool("dont_split_lods", false);

  int lodNameId = blk.getNameId("lod");

  LodsEqualMaterialGather matGather;
  Tab<AScene *> curScenes(tmpmem);

  const bool needResetNodesTmScale = blk.getBool("needResetNodesTmScale", false);

  int numBlk = blk.blockCount();
  for (int blkId = 0; blkId < numBlk; ++blkId)
  {
    const DataBlock &lodBlk = *blk.getBlock(blkId);
    if (lodBlk.getBlockNameId() != lodNameId)
      continue;

    const char *fileName = lodBlk.getStr("scene", NULL);
    real range = lodBlk.getReal("range", MAX_REAL);
    const DataBlock *materialOverrides = lodBlk.getBlockByNameEx("materialOverrides");

    if (!addLod(fileName, range, matGather, curScenes, all_animated, lodBlk, needResetNodesTmScale, *materialOverrides))
    {
      clear_all_ptr_items(curScenes);
      return false;
    }
  }

  bbox.setempty();
  lbbox.setempty();
  bsph.setempty();
  for (int i = 0; i < curScenes.size(); i++)
  {
    curScenes[i]->root->calc_wtm();

    calcSkinNodeBbox(curScenes[i]->root);
    calcMeshNodeBbox(curScenes[i]->root);
  }

  nodeNameMap.clear();

  if (bbox.isempty())
  {
    bbox[0].zero();
    bbox[1].set(0.001f, 0.001f, 0.001f);
  }
  if (lbbox.isempty())
  {
    lbbox[0].zero();
    lbbox[1].set(0.001f, 0.001f, 0.001f);
  }
#define MIN_EXPAND(B, C)            \
  if (fabs(B[1].C - B[0].C) < 2e-6) \
  {                                 \
    B[0].C -= 1e-6;                 \
    B[1].C += 1e-6;                 \
  }

  MIN_EXPAND(lbbox, x);
  MIN_EXPAND(lbbox, y);
  MIN_EXPAND(lbbox, z);
  MIN_EXPAND(bbox, x);
  MIN_EXPAND(bbox, y);
  MIN_EXPAND(bbox, z);
#undef MIN_EXPAND

  if (bsph.isempty())
    bsph = bbox;
  if (blk.getBool("boundsOnly", false)) // wAllVert and bsph are used from assetExp/exporters/exp_animChar.cpp to estimate animChar
                                        // sphere
  {
    clear_all_ptr_items(curScenes);
    return true;
  }

  // debug("bbox=" FMT_P3 "-" FMT_P3 ", local_bbox=" FMT_P3 "-" FMT_P3 "",
  //   P3D(bbox[0]), P3D(bbox[1]), P3D(lbbox[0]), P3D(lbbox[1]));
  Point3 bsz = lbbox.width();
  ShaderChannelId::setPosBounding(-lbbox[0], Point3(1.0f / bsz.x, 1.0f / bsz.y, 1.0f / bsz.z));
  ShaderChannelId::resetBoundingUsed();


  G_ASSERT(curScenes.size() == lods.size());
  if (dynmodel_use_direct_bones_array && !dynmodel_optimize_direct_bones && !shadermeshbuilder_strip_d3dres)
  {
    MeshBones nmb_combined;
    for (int i = 0; i < curScenes.size(); i++)
    {
      MeshBones nmb;
      curDagFname = lods[i].fileName;

      gatherSkinNodeUsedBones(curScenes[i]->root, dynmodel_use_direct_bones_array_combined_lods ? nmb_combined : nmb);

      if (!dynmodel_use_direct_bones_array_combined_lods)
        remapMeshSkinning(curScenes[i]->root, nmb, log);
    }
    if (dynmodel_use_direct_bones_array_combined_lods)
      for (int i = 0; i < curScenes.size(); i++)
        remapMeshSkinning(curScenes[i]->root, nmb_combined, log);
  }
  if (WERE_FATALS_ISSUED())
  {
    clear_all_ptr_items(curScenes);
    return false;
  }
  for (int i = 0; i < curScenes.size(); i++)
  {
    curDagFname = lods[i].fileName;

    addNode(lods[i], curScenes[i]->root, matGather);
    curDagFname = NULL;
  }

  ShaderChannelId::setPosBounding(Point3(0, 0, 0), Point3(1, 1, 1));

  matGather.copyToMatSaver(matSaver, texSaver);
  clear_all_ptr_items(curScenes);

  int elemnum = 0;
  for (int i = 0; i < lods.size(); i++)
    for (int j = 0; j < lods[i].rigids.size(); j++)
      elemnum += lods[i].rigids[j].meshData.elems.size();

  debug("%d lods, %d mats, %d texs, %d elems", lods.size(), matSaver.mats.size(), texSaver.textures.size(), elemnum);
  for (int i = 0; i < matSaverSkins.size(); i++)
    debug("skinvar %d: %d mats", max_vpr_const_for_skinned_mesh(), matSaverSkins[i].mats.size());

  if (const DataBlock *texBlk = blk.getBlockByName("textures"))
  {
    int id = texBlk->getNameId("tex");

    for (int i = 0; i < texBlk->paramCount(); ++i)
    {
      if (texBlk->getParamType(i) != DataBlock::TYPE_STRING || texBlk->getParamNameId(i) != id)
        continue;
      const char *name = texBlk->getStr(i);
      textureSlots.push_back() = name;
    }
  }

  drsHdrAdd = blk.getInt("addHdrResv", 0);

  if (ShaderChannelId::getBoundingUsed())
  {
    // encode 'bounding-used' flags into bbox representation
    Point3 tmp = bbox[0];
    bbox[0] = bbox[1];
    bbox[1] = tmp;
    boundingPackUsed = true;
    drsHdrAdd += 2 * 4 * 4;
  }
  ShaderChannelId::resetBoundingUsed();
  return true;
}


void DynamicRenderableSceneLodsResSrc::RigidObj::presave(ShaderMeshDataSaveCB &mdcb) { meshData.enumVertexData(mdcb); }


void DynamicRenderableSceneLodsResSrc::SkinnedObj::presave(int var, ShaderMeshDataSaveCB &mdcb)
{
  if (meshData[var])
    meshData[var]->enumVertexData(mdcb);
}


int DynamicRenderableSceneLodsResSrc::Lod::saveRigids(mkbindump::BinDumpSaveCB &cwr, ShaderMaterialsSaver &mdcb)
{
  mkbindump::PatchTabRef rig_pt, skin_pt;

  cwr.setOrigin();
  rig_pt.reserveTab(cwr);
  skin_pt.reserveTab(cwr);

  rig_pt.startData(cwr, rigids.size());
  for (RigidObj &ro : rigids)
  {
    ro.meshOfs = cwr.tell();
    cwr.writePtr64e(0);
    cwr.write32ex(&ro.sph.c, sizeof(ro.sph.c));
    cwr.writeReal(ro.sph.r);
    cwr.writeInt32e(ro.nodeId);
    cwr.writeInt32e(0);
  }
  rig_pt.finishTab(cwr);

  skin_pt.startData(cwr, skins.size());
  cwr.writeZeroes(skins.size() * cwr.PTR_SZ);
  skin_pt.finishTab(cwr);

  int sz = cwr.tell();

  for (RigidObj &ro : rigids)
    if (ro.meshData.elems.size())
      cwr.writePtr32eAt(ro.meshData.save(cwr, mdcb, true), ro.meshOfs);

  cwr.popOrigin();
  return sz;
}

void DynamicRenderableSceneLodsResSrc::Lod::saveSkins(mkbindump::BinDumpSaveCB &cwr, int var, ShaderMaterialsSaver &mdcb)
{
  for (int skinNo = 0; skinNo < skins.size(); skinNo++)
    cwr.writeInt32e(skins[skinNo].nodeId);

  int pos = cwr.tell();
  cwr.writeZeroes(skins.size() * cwr.PTR_SZ);

  for (int i = 0; i < skins.size(); ++i)
    cwr.writePtr32eAt(skins[i].meshData[var]->save(cwr, mdcb), pos + i * cwr.PTR_SZ);
}

void DynamicRenderableSceneLodsResSrc::presave()
{
  // remap names to remove unused nodes
  NameMap newNm;
  newNm.addNameId("@root");
  for (int i = 0; i < lods.size(); i++)
  {
    Lod &l = lods[i];

    for (int j = l.rigids.size() - 1; j >= 0; j--)
      if (!l.rigids[j].meshData.elems.size())
      {
        debug("removed empty rigid for <%s> (lod #%d, rigid #%d)", nodeNameMap.getName(l.rigids[j].nodeId), i, j);
        G_ASSERTF(j == l.rigids.size() - 1, "not last! j=%d l.rigids.size()=%d", j, l.rigids.size());
        erase_items(l.rigids, j, 1);
      }

    for (int j = 0; j < l.rigids.size(); j++)
      l.rigids[j].nodeId = newNm.addNameId(nodeNameMap.getName(l.rigids[j].nodeId));

    for (int j = 0; j < l.skins.size(); j++)
    {
      l.skins[j].nodeId = newNm.addNameId((const char *)l.skins[j].nodeName);
      for (int k = 0; k < l.skins[j].meshData.size(); k++)
        if (l.skins[j].meshData[k])
          l.skins[j].meshData[k]->remap(nodeNameMap, newNm);
    }
    debug("lod[%d] %d rigids, %d skins", i, l.rigids.size(), l.skins.size());
  }
  nodeNameMap = eastl::move(newNm);
  debug("%d nodes", nodeNameMap.nameCount());


  // connect vertex datas for all LODs
  GlobalVertexDataConnector gvd;
  Tab<GlobalVertexDataConnector> gvdSkins(tmpmem);

  gvdSkins.resize(matSaverSkins.size());

  for (int i = lods.size() - 1; i >= 0; i--)
  {
    Lod &l = lods[i];

    for (int j = 0; j < l.rigids.size(); j++)
      gvd.addMeshData(&l.rigids[j].meshData, Point3(0, 0, 0), 0);
    for (int j = 0; j < l.skins.size(); j++)
      for (int k = 0; k < matSaverSkins.size(); k++)
        if (l.skins[j].meshData[k])
          gvdSkins[k].addMeshData(&l.skins[j].meshData[k]->getShaderMeshData(), Point3(0, 0, 0), 0);

    if (doNotSplitLods && i > 0)
      continue;
    unsigned f = VDATA_PACKED_IB | ((i << __bsf(VDATA_LOD_MASK)) & VDATA_LOD_MASK);
    gvd.connectData(false, NULL, f);
    for (int k = 0; k < matSaverSkins.size(); k++)
      gvdSkins[k].connectData(false, NULL, f);

    gvd.clear();
    for (int k = 0; k < matSaverSkins.size(); k++)
      gvdSkins[k].clear();
  }

  for (int i = lods.size() - 1; i >= 0; i--)
  {
    Lod &l = lods[i];

    for (int j = 0; j < l.rigids.size(); j++)
      l.rigids[j].presave(matSaver);

    for (int j = 0; j < l.skins.size(); j++)
      for (int k = 0; k < matSaverSkins.size(); k++)
        l.skins[j].presave(k, matSaverSkins[k]);
  }
}

bool DynamicRenderableSceneLodsResSrc::save(mkbindump::BinDumpSaveCB &cwr, const LodValidationSettings *lvs, ILogWriter &log_writer)
{
  SmallTab<int, TmpmemAlloc> ofs;

  presave();
  texSaver.prepareTexNames(textureSlots);

  // validate LODs
  bool validation_passed = true;
  if (lvs)
    for (unsigned l = 0, prev_lod_faces = ~0u, faces = 0; l < lods.size(); l++)
    {
      if (!lvs->checkLodValid(l, faces = lods[l].countFaces(), prev_lod_faces, lods[l].range, bbox, lods.size(), log_writer))
        validation_passed = false;
      prev_lod_faces = faces;
    }
  if (!validation_passed)
    return false;

  // write DynamicRenderableSceneLodsResource resSize
  cwr.writeInt32e(int(cwr.TAB_SZ + drsHdrAdd + lods.size() * cwr.TAB_SZ + sizeof(bbox)));

  // write materials
  unsigned total_mats = matSaver.mats.size();
  for (const auto &m : matSaverSkins)
    total_mats += m.mats.size();
  bool write_mat_sep = sepMatToBuildResultsBlk && buildResultsBlk != nullptr && total_mats;
  if (write_mat_sep)
  {
    matSaver.writeMatVdataCombinedNoMat(cwr);
    texSaver.writeTexToBlk(*buildResultsBlk);
    matSaver.writeMatToBlk(*buildResultsBlk, texSaver, "matR");
  }
  else
  {
    matSaver.writeMatVdataHdr(cwr, texSaver);
    texSaver.writeTexStr(cwr);
    matSaver.writeMatVdata(cwr, texSaver);
  }

  {
    for (int lodIdx = 0; lodIdx < lods.size(); lodIdx++)
    {
      DynamicRenderableSceneLodsResSrc::Lod &lod = lods[lodIdx];
      float minTextureScale = FLT_MAX;
      for (DynamicRenderableSceneLodsResSrc::RigidObj &rigid : lod.rigids)
        for (ShaderMeshData::RElem &elem : rigid.meshData.elems)
          minTextureScale = min(minTextureScale, elem.textureScale);
      for (DynamicRenderableSceneLodsResSrc::SkinnedObj &skined : lod.skins)
        for (ShaderSkinnedMeshData *skinnedMeshData : skined.meshData)
        {
          ShaderMeshData &meshData = skinnedMeshData->getShaderMeshData();
          for (ShaderMeshData::RElem &elem : meshData.elems)
            minTextureScale = min(minTextureScale, elem.textureScale);
        }

      lod.texScale = 1.0f / (minTextureScale * minTextureScale);
    }
    if (write_mat_sep)
    {
      DataBlock *b = buildResultsBlk->addBlock("texScale_data");
      b->clearData();
      for (int lodIdx = 0; lodIdx < lods.size(); lodIdx++)
        b->setReal(String(0, "texScale%d", lodIdx), lods[lodIdx].texScale);
    }
  }

  // write DynamicRenderableSceneLodsResource data
  int skin_count = 0;
  cwr.writeRef(cwr.TAB_SZ + drsHdrAdd + sizeof(bbox), lods.size());

  // write BoundingBox
  cwr.write32ex(&bbox, sizeof(bbox));
  if (boundingPackUsed)
  {
    Point3 mul = lbbox.width();
    Point3 ofs = lbbox.center();
    cwr.writeReal(ofs.x);
    cwr.writeReal(ofs.y);
    cwr.writeReal(ofs.z);
    cwr.writeReal(0);
    cwr.writeReal(mul.x * 0.5f);
    cwr.writeReal(mul.y * 0.5f);
    cwr.writeReal(mul.z * 0.5f);
    cwr.writeReal(1);
  }

  cwr.writeZeroes(drsHdrAdd - (boundingPackUsed ? 2 * 4 * 4 : 0));
  for (int i = 0; i < lods.size(); ++i)
  {
    lods[i].szPos = cwr.tell();

    cwr.writePtr64e(0);
    cwr.writeReal(lods[i].range);
    cwr.writeReal((!write_mat_sep) ? lods[i].texScale : 0);

    skin_count += lods[i].skins.size();
  }

  // write DynSceneResNameMapResource
  mkbindump::StrCollector all_names;
  mkbindump::RoNameMapBuilder node_nm;

  node_nm.prepare(nodeNameMap, all_names);

  cwr.beginBlock();
  cwr.setOrigin();

  node_nm.writeHdr(cwr);

  all_names.writeStrings(cwr);
  cwr.align8();

  node_nm.writeMap(cwr);

  cwr.popOrigin();
  cwr.endBlock();

  // write referenced resources (rigids only)
  for (int i = 0; i < lods.size(); ++i)
    cwr.writePtr32eAt(lods[i].saveRigids(cwr, matSaver), lods[i].szPos);

  // write referenced resources (skins only)
  if (skin_count)
  {
    clear_and_resize(ofs, matSaverSkins.size());

    cwr.writeInt32e(matSaverSkins.size());
    cwr.beginBlock();
    for (int j = 0; j < matSaverSkins.size(); j++)
    {
      cwr.writeInt32e(max_vpr_const_for_skinned_mesh());
      cwr.writeInt32e(0);
      ofs[j] = cwr.tell();
    }

    for (int j = 0; j < matSaverSkins.size(); j++)
    {
      cwr.writeInt32eAt(cwr.tell() - ofs[j], ofs[j] - 4);

      if (write_mat_sep)
      {
        matSaverSkins[j].writeMatVdataCombinedNoMat(cwr);
        matSaverSkins[j].writeMatToBlk(*buildResultsBlk, texSaver, "matS");
      }
      else
      {
        matSaverSkins[j].writeMatVdataHdr(cwr, texSaver);
        matSaverSkins[j].writeMatVdata(cwr, texSaver);
      }

      {
        mkbindump::BinDumpSaveCB cwr_s(1 << 20, cwr.getTarget(), cwr.WRITE_BE);

        for (int i = 0; i < lods.size(); ++i)
          lods[i].saveSkins(cwr_s, j, matSaverSkins[j]);

        ZlibSaveCB z_cwr(cwr.getRawWriter(), ZlibSaveCB::CL_BestCompression);
        cwr_s.copyDataTo(z_cwr);
        z_cwr.finish();
      }
    }
    cwr.endBlock();
  }
  return true;
}
