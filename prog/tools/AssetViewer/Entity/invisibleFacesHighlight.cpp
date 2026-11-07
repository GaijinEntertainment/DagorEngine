// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "invisibleFacesHighlight.h"

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <de3_interface.h>
#include <debug/dag_debug3d.h>
#include <generic/dag_ptrTab.h>
#include <image/dag_loadImage.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/validateAlphaTest.h>
#include <math/dag_mesh.h>
#include <math/dag_TMatrix.h>
#include <osApiWrappers/dag_direct.h>
#include <shaders/dag_shaders.h>
#include "entityMatEditor.h"
#include "../av_appwnd.h"

static const E3DCOLOR WIREFRAME_COLOR(0xFB, 0x00, 0x00, 0xFF);
static const E3DCOLOR FILL_COLOR(0x8A, 0x68, 0x13, 0xF2);

static void make_process_mat_blk(const DataBlock &type_blk, const DataBlock &a_props, DataBlock &out)
{
  out.reset();
  if (const DataBlock *b = type_blk.getBlockByName("processMat"))
    out.setFrom(b);
  if (const DataBlock *b = a_props.getBlockByName("processMat"))
    for (int i = 0; i < b->blockCount(); ++i)
      out.addNewBlock(b->getBlock(i), b->getBlock(i)->getBlockName());
}

static void traverse_node(AlphaTestValidation::Context *ctx, const Node &node, const TMatrix &inv_root_wtm,
  DagorMatShaderSubstitute &mat_subst, IProcessMaterialData &pm, bool skip_skinned, Tab<Point3> &out_verts)
{
  for (const Node *child : node.child)
    if (child)
      traverse_node(ctx, *child, inv_root_wtm, mat_subst, pm, skip_skinned, out_verts);

  if (!(node.flags & NODEFLG_RENDERABLE) || !node.mat || !node.mat->subMatCount() || !node.obj || !node.obj->isSubOf(OCID_MESHHOLDER))
    return;

  MeshHolderObj &meshHolder = static_cast<MeshHolderObj &>(*node.obj);
  if (!meshHolder.mesh || (skip_skinned && meshHolder.bones))
    return;
  Mesh &mesh = *meshHolder.mesh;
  const int matCount = node.mat->subMatCount();
  mesh.clampMaterials(matCount - 1);

  Tab<int> faces(tmpmem);
  for (int i = 0; i < matCount; ++i)
  {
    Ptr<MaterialData> subMat = node.mat->getSubMat(i);
    if (!subMat || !mat_subst.substMatClass(*subMat))
      continue;
    subMat = pm.processMaterial(subMat, true);
    Ptr<ShaderMaterial> shMat = new_shader_material(*subMat, false, false);
    if (shMat)
      AlphaTestValidation::retrieve_invisible_faces(ctx, mesh, i, *subMat, *shMat, faces);
  }
  if (faces.empty())
    return;

  const TMatrix tm = inv_root_wtm * node.wtm;
  for (int f : faces)
    for (int v = 0; v < 3; ++v)
      out_verts.push_back(tm * mesh.vert[mesh.face[f].v[v]]);
}

void InvisibleFacesHighlight::setAsset(const DagorAsset *in_asset)
{
  supported = false;
  asset = in_asset;
  clear_and_shrink(lodCache);
  if (!asset)
    return;
  isRendInst = strcmp(asset->getTypeStr(), "rendInst") == 0;
  supported = (isRendInst || strcmp(asset->getTypeStr(), "dynModel") == 0);
}

void InvisibleFacesHighlight::retrieveGeometry(int lod)
{
  lodCache[lod].cached = true;

  const DataBlock *lodBlk = nullptr;
  const String dagPath = get_asset_dag_file_path(*asset, lod, &lodBlk);
  if (dagPath.empty() || !dd_file_exists(dagPath))
    return;

  DataBlock appBlk;
  if (!appBlk.load(::get_app().getWorkspace().getAppBlkPath()))
    return;
  const DataBlock &typeBlk = *appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx(asset->getTypeStr());

  DagorMatShaderSubstitute matSubst;
  if (const DataBlock *remapBlk = typeBlk.getBlockByName("remapShaders"))
    matSubst.setupMatSubst(*remapBlk);

  DataBlock processMatBlk;
  make_process_mat_blk(typeBlk, asset->getProfileTargetProps(_MAKE4C('PC'), nullptr), processMatBlk);
  GenericTexSubstProcessMaterialData pm(asset->getName(), processMatBlk,
    asset->props.getBool("allowProxyMat", false) ? &asset->getMgr() : nullptr, nullptr);

  AScene scene;
  PtrTab<MaterialData> matList;
  if (!load_ascene(dagPath, scene, LASF_MATNAMES | LASF_NOSPLINES | LASF_NOLIGHTS, false, &matList) || !scene.root)
    return;
  const DataBlock &materialOverrides = *lodBlk->getBlockByNameEx("materialOverrides");
  override_materials(matList, materialOverrides, nullptr);
  scene.root->calc_wtm();

  DataBlock validationBlk;
  validationBlk.setBool("validate", true);
  AlphaTestValidation::Context *ctx = AlphaTestValidation::init(
    validationBlk, [this]() { return &asset->getMgr(); }, [](const char *path, IMemAlloc *mem) { return ::load_image(path, mem); });
  traverse_node(ctx, *scene.root, inverse(scene.root->wtm), matSubst, pm, isRendInst, lodCache[lod].verts);
  AlphaTestValidation::shutdown(ctx);
  if (stateCb)
    stateCb(State::Ready, lodCache[lod].verts.size() / 3);
}

void InvisibleFacesHighlight::render(int lod, const TMatrix &wtm)
{
  if (!supported)
    return;

  if (lod < 0)
    lod = 0;
  if (lod >= lodCache.size())
    lodCache.resize(lod + 1);
  if (!lodCache[lod].cached)
    retrieveGeometry(lod);
  const Tab<Point3> &verts = lodCache[lod].verts;
  if (verts.empty())
    return;

  begin_draw_cached_debug_lines(true, true);
  set_cached_debug_lines_wtm(wtm);
  draw_cached_debug_trilist(verts.data(), verts.size() / 3, FILL_COLOR);
  for (int i = 0; i + 2 < verts.size(); i += 3)
  {
    draw_cached_debug_line(verts[i + 0], verts[i + 1], WIREFRAME_COLOR);
    draw_cached_debug_line(verts[i + 1], verts[i + 2], WIREFRAME_COLOR);
    draw_cached_debug_line(verts[i + 2], verts[i + 0], WIREFRAME_COLOR);
  }
  end_draw_cached_debug_lines();
}
