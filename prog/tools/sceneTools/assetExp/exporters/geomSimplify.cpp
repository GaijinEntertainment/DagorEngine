#include <math/SimplygonSDK/SimplygonSDK.h>
#include <math/SimplygonSDK/SimplygonSDKLoader.h>
#include <assets/asset.h>
#include <assets/assetExpCache.h>

#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <3d/dag_materialData.h>
#include <math/dag_mesh.h>
#include <math/dag_meshBones.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastIntList.h>
#include "modelExp.h"

static SimplygonSDK::ISimplygonSDK *sg = NULL;
static String sgsceneOutDir, cacheBase;

static void addSkinNodes(Node *n, FastNameMap &skinNodes)
{
  if (n->obj && n->obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n->obj;
    if (mh.bones)
      for (int i = 0; i < mh.bones->boneNames.size(); i++)
        skinNodes.addNameId(mh.bones->boneNames[i]);
  }

  for (int i = 0; i < n->child.size(); i++)
    addSkinNodes(n->child[i], skinNodes);
}

static void copy_ascene_to_sg(Node &n, SimplygonSDK::ISceneNode *n_parent, MaterialDataList &all_mat,
  SimplygonSDK::ISceneBoneTable *btable)
{
  using namespace SimplygonSDK;
  spSceneNode n_dest;

  Tab<int> mat_remap;
  if (n.mat && n.mat->subMatCount())
  {
    mat_remap.resize(n.mat->subMatCount());
    mem_set_ff(mat_remap);
    for (int i = 0; i < mat_remap.size(); i++)
    {
      for (int j = 0; j < all_mat.list.size(); j++)
        if (n.mat->list[i]->isEqual(*all_mat.list[j]))
        {
          mat_remap[i] = j;
          break;
        }
      if (mat_remap[i] < 0)
        mat_remap[i] = all_mat.list.size();
      all_mat.list.push_back(n.mat->list[i]);
    }
  }

  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.mesh)
    {
      Mesh m = *mh.mesh;
      m.clampMaterials(mat_remap.size() - 1);
      m.sort_faces_by_mat();
      m.optimize_tverts();

      if (!m.facengr.size())
      {
        G_VERIFY(m.calc_ngr());
        G_VERIFY(m.calc_vertnorms());
        // DEBUG_DUMP_VAR(m.facengr.size());
        // DEBUG_DUMP_VAR(m.vertnorm.size());
      }

      spPackedGeometryData gd = sg->CreatePackedGeometryData();
      gd->SetVertexCount(m.face.size() * 3);
      gd->SetTriangleCount(m.face.size());
      gd->AddMaterialIds();
      gd->AddNormals();

      spRealArray Coords = gd->GetCoords();
      spRealArray Normals = gd->GetNormals();
      spRidArray VertexIds = gd->GetVertexIds();
      spRidArray MatIds = gd->GetMaterialIds();

      for (int j = 0; j < m.face.size(); j++)
        for (int k = 0; k < 3; k++)
        {
          int corner = j * 3 + k;
          int i = m.face[j].v[k];
          Coords->SetTuple(corner, &m.vert[i].x);
        }
      for (int j = 0; j < m.facengr.size(); ++j)
        for (int k = 0; k < 3; k++)
        {
          int corner = j * 3 + k;
          int i = m.facengr[j][k];
          Normals->SetTuple(corner, &m.vertnorm[i].x);
        }
      for (int t = 0; t < NUMMESHTCH; t++)
      {
        if (!m.tface[t].size())
          continue;
        G_ASSERT(m.tface[t].size() == m.face.size());

        gd->AddTexCoords(t);
        spRealArray TexCoords = gd->GetTexCoords(t);
        for (int j = 0; j < m.tface[t].size(); j++)
          for (int k = 0; k < 3; k++)
          {
            int corner = j * 3 + k;
            int i = m.tface[t][j].t[k];
            TexCoords->SetTuple(corner, &m.tvert[t][i].x);
          }
      }
      if (m.cface.size())
      {
        gd->AddColors(0);
        spRealArray VertCol = gd->GetColors(0);
        for (int j = 0; j < m.cface.size(); ++j)
          for (int k = 0; k < 3; k++)
          {
            int corner = j * 3 + k;
            int i = m.cface[j].t[k];
            VertCol->SetTuple(corner, &m.cvert[i].r);
          }
      }

      for (int i = 0; i < m.face.size(); i++)
      {
        VertexIds->SetItem(i * 3 + 0, i * 3 + 0);
        VertexIds->SetItem(i * 3 + 1, i * 3 + 1);
        VertexIds->SetItem(i * 3 + 2, i * 3 + 2);
        MatIds->SetItem(i, mat_remap[m.face[i].mat]);
      }

      if (mh.bones)
      {
        Tab<int> bone_ids;
        bone_ids.resize(mh.bones->boneNames.size());
        for (int j = 0; j < bone_ids.size(); j++)
          bone_ids[j] = btable->FindBoneId(mh.bones->boneNames[j]);

        gd->AddBoneWeights(bone_ids.size());
        spRealArray BoneWeights = gd->GetBoneWeights();
        spRidArray BoneIds = gd->GetBoneIds();
        for (int j = 0; j < m.vert.size(); j++)
        {
          float *wt = &mh.bones->boneinf[bone_ids.size() * j];
          BoneWeights->SetTuple(j, wt);
          for (int k = 0; k < bone_ids.size(); k++)
            BoneIds->SetItem(j * bone_ids.size() + k, fabsf(wt[k]) > 1e-12 ? bone_ids[k] : -1);
        }
      }
      n_dest = ISceneNode::SafeCast(n_parent->CreateChildMesh(gd->NewUnpackedCopy()));
    }
  }
  if (!n_dest)
    n_dest = sg->CreateSceneNode();

  n_dest->SetName(n.name);
  spMatrix4x4 tm = n_dest->GetRelativeTransform();
  TMatrix4 tm4(n.tm);
  for (int c = 0; c < 4; c++)
    for (int r = 0; r < 4; r++)
      tm->SetElement(c, r, tm4.m[c][r]);
  n_parent->AddChild(n_dest);

  for (int i = 0; i < n.child.size(); ++i)
    copy_ascene_to_sg(*n.child[i], n_dest, all_mat, btable);
}
static void update_ascene_from_sg(Node &n, SimplygonSDK::ISceneNode *n_sg, MaterialDataList &all_mat,
  SimplygonSDK::ISceneBoneTable *btable)
{
  using namespace SimplygonSDK;
  G_ASSERT(n_sg->GetChildCount() == n.child.size());

  // DEBUG_DUMP_VAR(n.name);
  // DEBUG_DUMP_VAR(n_sg->GetName()->GetText());
  if ((n.flags & NODEFLG_RENDERABLE) && n.mat && n.mat->subMatCount() && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.mesh)
    {
      Mesh &m = *mh.mesh;

      /*
      debug("vert=%d face=%d  nvert=%d nface=%d  mat=%d",
        m.vert.size(), m.face.size(), m.vertnorm.size(), m.facengr.size(), n.mat->list.size());
      for (int i = 0; i < m.face.size(); i ++)
        debug("v(%d,%d,%d)=%@ %@ %@", m.face[i].v[0], m.face[i].v[1], m.face[i].v[2],
          m.vert[m.face[i].v[0]], m.vert[m.face[i].v[1]], m.vert[m.face[i].v[2]]);
      for (int t = 0; t < NUMMESHTCH; t ++)
        if (m.tface[t].size())
        {
          debug("%d: tvert=%d tface=%d", t, m.tvert[t].size(), m.tface[t].size());
          for (int i = 0; i < m.tface[t].size(); i ++)
            debug("t(%d,%d,%d)=%@ %@ %@", m.tface[t][i].t[0], m.tface[t][i].t[1], m.tface[t][i].t[2],
              m.tvert[t][m.tface[t][i].t[0]], m.tvert[t][m.tface[t][i].t[1]], m.tvert[t][m.tface[t][i].t[2]]);
        }
      */

      spPackedGeometryData gd = ISceneMesh::SafeCast(n_sg)->GetGeometry()->NewPackedCopy();
      // DEBUG_DUMP_VAR(gd.GetPointer());
      m.setnumfaces(gd->GetTriangleCount());

      m.vert.resize(gd->GetVertexCount());
      m.face.resize(gd->GetTriangleCount());
      m.vertnorm.resize(gd->GetVertexCount());
      m.facengr.resize(gd->GetTriangleCount());
      mem_set_0(m.face);

      spRealArray Coords = gd->GetCoords();
      spRealArray Normals = gd->GetNormals();
      spRidArray VertexIds = gd->GetVertexIds();
      spRidArray MatIds = gd->GetMaterialIds();

      Tab<int> mat_remap;
      n.mat->list.clear();
      mat_remap.resize(all_mat.subMatCount());
      mem_set_ff(mat_remap);
      for (int i = 0; i < m.face.size(); i++)
      {
        int mid = MatIds->GetItem(i);
        if (mat_remap[mid] < 0)
        {
          mat_remap[mid] = n.mat->list.size();
          n.mat->list.push_back(all_mat.list[mid]);
        }
        m.face[i].mat = mat_remap[mid];
      }

      for (int i = 0; i < m.vert.size(); i++)
      {
        m.vert[i].set(Coords->GetItem(i * 3 + 0), Coords->GetItem(i * 3 + 1), Coords->GetItem(i * 3 + 2));
        m.vertnorm[i].set(Normals->GetItem(i * 3 + 0), Normals->GetItem(i * 3 + 1), Normals->GetItem(i * 3 + 2));
      }
      for (int i = 0; i < m.face.size(); i++)
      {
        m.face[i].v[0] = m.facengr[i][0] = VertexIds->GetItem(i * 3 + 0),
        m.face[i].v[1] = m.facengr[i][1] = VertexIds->GetItem(i * 3 + 1),
        m.face[i].v[2] = m.facengr[i][2] = VertexIds->GetItem(i * 3 + 2);
      }
      for (int t = 0; t < NUMMESHTCH; t++)
      {
        spRealArray TexCoords = gd->GetTexCoords(t);
        if (!TexCoords)
        {
          clear_and_shrink(m.tvert[t]);
          clear_and_shrink(m.tface[t]);
          continue;
        }
        m.tvert[t].resize(gd->GetVertexCount());
        m.tface[t].resize(gd->GetTriangleCount());
        mem_set_0(m.tvert[t]);
        mem_set_0(m.tface[t]);

        for (int i = 0; i < m.tvert[t].size(); i++)
          m.tvert[t][i].set(TexCoords->GetItem(i * 2 + 0), TexCoords->GetItem(i * 2 + 1));
        for (int i = 0; i < m.tface[t].size(); i++)
        {
          m.tface[t][i].t[0] = m.face[i].v[0];
          m.tface[t][i].t[1] = m.face[i].v[1];
          m.tface[t][i].t[2] = m.face[i].v[2];
        }
      }

      if (spRealArray VertCol = gd->GetColors(0))
      {
        m.cvert.resize(m.vert.size());
        m.cface.resize(m.face.size());
        for (int i = 0; i < m.cvert.size(); i++)
          m.cvert[i].set(VertCol->GetItem(i * 4 + 0), VertCol->GetItem(i * 4 + 1), VertCol->GetItem(i * 4 + 2),
            VertCol->GetItem(i * 4 + 3));
        for (int i = 0; i < m.cface.size(); i++)
        {
          m.cface[i].t[0] = m.face[i].v[0];
          m.cface[i].t[1] = m.face[i].v[1];
          m.cface[i].t[2] = m.face[i].v[2];
        }
      }

      if (mh.bones)
      {
        FastIntList all_bones;
        spRealArray BoneWeights = gd->GetBoneWeights();
        spRidArray BoneIds = gd->GetBoneIds();
        // debug("bones: BoneWeights=%p, BoneIds=%p", BoneWeights.GetPointer(), BoneIds.GetPointer());
        int tuple_sz = BoneIds->GetTupleSize();

        for (int j = 0; j < tuple_sz * BoneIds->GetTupleCount(); j++)
          if (BoneIds->GetItem(j) >= 0)
            all_bones.addInt(BoneIds->GetItem(j));

        Tab<int> bone_id_map;
        bone_id_map.resize(all_bones.getList().back() + 1);
        mem_set_ff(bone_id_map);
        for (int j = 0; j < all_bones.getList().size(); j++)
        {
          spSceneBone b = btable->GetBone(all_bones.getList()[j]);
          int new_id = -1;
          for (int k = 0; k < mh.bones->boneNames.size(); k++)
            if (strcmp(mh.bones->boneNames[k], b->GetName()->GetText()) == 0)
            {
              bone_id_map[all_bones.getList()[j]] = k;
              break;
            }
        }
        // debug("mh.bones->boneinf=%d -> %d", mh.bones->boneinf.size(), mh.bones->boneNames.size()*BoneIds->GetTupleCount());
        // debug("BoneIds->getTupleCount()=%d m.vert.size()=%d", BoneIds->GetTupleCount(), m.vert.size());

        mh.bones->boneinf.resize(m.vert.size() * mh.bones->boneNames.size());
        for (int j = 0; j < tuple_sz * BoneIds->GetTupleCount(); j++)
        {
          int id = BoneIds->GetItem(j);
          if (id < 0)
            continue;
          id = bone_id_map[id];
          if (id < 0)
            continue;
          int v = j / tuple_sz;
          mh.bones->boneinf[v * mh.bones->boneNames.size() + id] = BoneWeights->GetItem(j);
        }
      }

      m.sort_faces_by_mat();
      m.optimize_tverts();
      /*
      debug("vert=%d face=%d  nvert=%d nface=%d  cvert=%d, cface=%d  mat=%d extra=%d",
        m.vert.size(), m.face.size(), m.vertnorm.size(), m.facengr.size(),
        m.cvert.size(), m.cface.size(), n.mat->list.size(), m.extra.size());
      for (int i = 0; i < m.face.size(); i ++)
        debug("v(%d,%d,%d)=%@ %@ %@", m.face[i].v[0], m.face[i].v[1], m.face[i].v[2],
          m.vert[m.face[i].v[0]], m.vert[m.face[i].v[1]], m.vert[m.face[i].v[2]]);
      for (int t = 0; t < NUMMESHTCH; t ++)
        if (m.tface[t].size())
        {
          debug("%d: tvert=%d tface=%d", t, m.tvert[t].size(), m.tface[t].size());
          for (int i = 0; i < m.tface[t].size(); i ++)
            debug("t(%d,%d,%d)=%@ %@ %@", m.tface[t][i].t[0], m.tface[t][i].t[1], m.tface[t][i].t[2],
              m.tvert[t][m.tface[t][i].t[0]], m.tvert[t][m.tface[t][i].t[1]], m.tvert[t][m.tface[t][i].t[2]]);
        }
      */
    }
  }
  for (int i = 0; i < n.child.size(); ++i)
    update_ascene_from_sg(*n.child[i], n_sg->GetChild(i), all_mat, btable);
}

static bool check_sg_cache_changed(AssetExportCache &c4, const DataBlock &p, const char *fn)
{
  bool curChanged = false;
  if (c4.checkDataBlockChanged(cur_asset->getNameTypified(), const_cast<DataBlock &>(p)))
    curChanged = true;

  if (c4.checkFileChanged(fn))
    curChanged = true;
  return curChanged;
}
static bool process_scene(const char *fn, int lod_n, AScene &sc, float start_range, const DataBlock &props)
{
  struct BuildMutexAutoAcquire
  {
    void *m;
    BuildMutexAutoAcquire(const char *asset_nm) { global_mutex_enter(m = global_mutex_create(asset_nm)); }
    ~BuildMutexAutoAcquire()
    {
      global_mutex_leave(m);
      global_mutex_destroy(m, NULL);
    }
  };

  using namespace SimplygonSDK;
  using ::real;
  if (!props.getBool("optimizeGeom", false) || !cur_asset)
    return true;

  int64_t reft = ref_time_ticks_qpc();
  AssetExportCache c4;
  int cacheEndPos = 0;
  String cacheFname(260, "%s/%s.lod%dsg.c4.bin", cacheBase, cur_asset->getName(), lod_n);
  String sgfn(0, "%s.sgscene", cacheFname);

  spScene lodScene = sg->CreateScene();
  MaterialDataList all_mat;
  FastNameMap skinNodes;
  addSkinNodes(sc.root, skinNodes);
  bool failed = false;
  iterate_names_in_lexical_order(skinNodes, [&](int, const char *name) {
    Node *n = sc.root->find_node(name);
    if (!n)
    {
      cur_log->addMessage(cur_log->ERROR, "skin bone node <%s> not found!", name);
      failed = true;
      return;
    }
    spSceneBone bone = sg->CreateSceneBone();
    bone->SetName(n->name);
    spMatrix4x4 tm = bone->GetRelativeTransform();
    TMatrix4 tm4(n->wtm);
    for (int c = 0; c < 4; c++)
      for (int r = 0; r < 4; r++)
        tm->SetElement(c, r, tm4.m[c][r]);
    lodScene->GetBoneTable()->AddBone(bone);
  });
  if (failed)
    return false;

  for (int i = 0; i < sc.root->child.size(); ++i)
    copy_ascene_to_sg(*sc.root->child[i], lodScene->GetRootNode(), all_mat, lodScene->GetBoneTable());
  // DEBUG_DUMP_VAR(all_mat.subMatCount());
  spMaterialTable mat = lodScene->GetMaterialTable();
  for (int i = 0; i < all_mat.subMatCount(); i++)
  {
    spMaterial m = sg->CreateMaterial();
    m->SetDiffuseColor(0.2, 0.2, 0.2);
    m->SetTextureLevel(SG_MATERIAL_CHANNEL_DIFFUSE, 0);
    m->SetTexture(SG_MATERIAL_CHANNEL_DIFFUSE, String(0, "tex%d.jpg", i));
    mat->AddMaterial(m);
  }

  if (!sgsceneOutDir.empty())
  {
    dd_mkdir(sgsceneOutDir);
    lodScene->SaveToFile(String(0, "%s/%s.lod%d.in.sgscene", sgsceneOutDir, cur_asset->getName(), lod_n));
  }

  BuildMutexAutoAcquire bmaa(cacheFname);
  if (c4.load(cacheFname, cur_asset->getMgr(), &cacheEndPos) && !check_sg_cache_changed(c4, props, fn))
  {
    FullFileLoadCB crd(cacheFname);
    Tab<char> dag_data;

    DAGOR_TRY
    {
      crd.seekto(cacheEndPos);
      dag_data.resize(df_length(crd.fileHandle) - crd.tell());
      crd.read(dag_data.data(), data_size(dag_data));
      crd.close();
    }
    DAGOR_CATCH(IGenLoad::LoadException exc)
    {
      logerr("failed to read '%s'", cacheFname);
      debug_flush(false);
      crd.close();
      dd_erase(cacheFname);
      goto rebuild;
    }

    if (c4.isTimeChanged())
    {
      c4.removeUntouched();
      c4.save(cacheFname, cur_asset->getMgr());

      FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
      cwr.writeInt(data_size(dag_data));
      cwr.write(dag_data.data(), data_size(dag_data));
    }

    InPlaceMemLoadCB mcrd(dag_data.data(), data_size(dag_data));
    ZlibLoadCB zcrd(mcrd, data_size(dag_data) - 4, false, false);
    FullFileSaveCB cwr(sgfn);
    copy_stream_to_stream(zcrd, cwr, mcrd.readInt());
    zcrd.ceaseReading();
    cwr.close();

    lodScene->LoadFromFile(sgfn);
    dd_erase(sgfn);

    update_ascene_from_sg(*sc.root, lodScene->GetRootNode(), all_mat, lodScene->GetBoneTable());
    cur_log->addMessage(ILogWriter::NOTE, "read LOD%d asset <%s> from cache for %.3f sec", lod_n, cur_asset->getName(),
      get_time_usec_qpc(reft) / 1e6f);
    return true;
  }

rebuild:
  spReductionProcessor reductionProcessor = sg->CreateReductionProcessor();
  reductionProcessor->SetScene(lodScene);

  spReductionSettings reductionSettings = reductionProcessor->GetReductionSettings();
  reductionSettings->SetDataCreationPreferences(props.getBool("preferOptResult", true)
                                                  ? SG_DATACREATIONPREFERENCES_PREFER_OPTIMIZED_RESULT
                                                  : SG_DATACREATIONPREFERENCES_PREFER_ORIGINAL_DATA);

  bool use_symm = props.getBool("useSymmetry", true);
  reductionSettings->SetKeepSymmetry(use_symm);
  reductionSettings->SetUseAutomaticSymmetryDetection(use_symm);
  reductionSettings->SetUseHighQualityNormalCalculation(true);
  reductionSettings->SetReductionHeuristics(
    props.getBool("heurFast", false) ? SG_REDUCTIONHEURISTICS_FAST : SG_REDUCTIONHEURISTICS_CONSISTENT);
  reductionSettings->SetUseSymmetryQuadRetriangulator(props.getBool("symmQuadRetriangle", true));
  reductionProcessor->GetNormalCalculationSettings()->SetReplaceNormals(false);
  reductionProcessor->GetNormalCalculationSettings()->SetHardEdgeAngleInRadians(props.getReal("hardEdgeAng", 60) * DEG_TO_RAD);

  int srcSz = props.getInt("onScrSize", -1);
  reductionSettings->SetStopCondition(SG_STOPCONDITION_ANY);
  reductionSettings->SetReductionTargets(SG_REDUCTIONTARGET_TRIANGLECOUNT | SG_REDUCTIONTARGET_TRIANGLERATIO |
                                         (props.paramExists("maxDeviation") ? SG_REDUCTIONTARGET_MAXDEVIATION : 0) |
                                         (srcSz > 1 ? SG_REDUCTIONTARGET_ONSCREENSIZE : 0));
  reductionSettings->SetTriangleRatio(props.getReal("triRatio", 0.5));
  reductionSettings->SetTriangleCount(props.getInt("minTri", 100));
  reductionSettings->SetMaxDeviation(props.getReal("maxDeviation", 1e6));
  reductionSettings->SetOnScreenSize(srcSz);

  spRepairSettings repairSettings = reductionProcessor->GetRepairSettings();
  repairSettings->SetUseWelding(true);
  repairSettings->SetWeldDist(0.0);
  repairSettings->SetUseTJunctionRemover(true);
  repairSettings->SetTjuncDist(0.0f);

  reductionProcessor->RunProcessing();

  lodScene->SaveToFile(sgfn);
  update_ascene_from_sg(*sc.root, lodScene->GetRootNode(), all_mat, lodScene->GetBoneTable());

  check_sg_cache_changed(c4, props, fn);
  c4.removeUntouched();

  c4.save(cacheFname, cur_asset->getMgr());

  FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
  FullFileLoadCB crd(sgfn);
  cwr.writeInt(df_length(crd.fileHandle));
  zlib_compress_data(cwr, 5, crd, df_length(crd.fileHandle));
  crd.close();
  cwr.close();

  if (!sgsceneOutDir.empty())
    dd_rename(sgfn, String(0, "%s/%s.lod%d.out.sgscene", sgsceneOutDir, cur_asset->getName(), lod_n));
  else
    dd_erase(sgfn);

  cur_log->addMessage(ILogWriter::NOTE, "simplified LOD%d asset <%s> for %.3f sec", lod_n, cur_asset->getName(),
    get_time_usec_qpc(reft) / 1e6f);
  debug_flush(false);
  return true;
}

void init_simplygon()
{
  using namespace SimplygonSDK;
  if (SimplygonSDK::Initialize(&sg) != 0 || !sg)
  {
    logerr("failed to init SimplygonSDK");
    return;
  }
  sg->SetGlobalSetting("DefaultTBNType", SG_TANGENTSPACEMETHOD_ORTHONORMAL);

  DynamicRenderableSceneLodsResSrc::process_scene = process_scene;

  cacheBase.printf(0, "%s/%s/dynModel~genlod", appBlkCopy.getStr("appDir", "."),
    appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("export")->getStr("cache", "/develop/.cache"));
  simplify_fname(cacheBase);
  dd_mkdir(cacheBase);

  sgsceneOutDir = "";
  if (appBlkCopy.getBlockByNameEx("assets")
        ->getBlockByNameEx("build")
        ->getBlockByNameEx("dynModel")
        ->getBool("saveSimplygonScenes", false))
  {
    sgsceneOutDir.printf(0, "%s/%s/SGtemp", appBlkCopy.getStr("appDir", "."),
      appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("export")->getStr("cache", "/develop/.cache"));
    simplify_fname(sgsceneOutDir);
  }
}

void term_simplygon() { SimplygonSDK::Deinitialize(); }

#include <math/SimplygonSDK/SimplygonSDKLoader.cpp>
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
