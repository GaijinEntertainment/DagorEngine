// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <render/omm.h>
#include <image/dag_texPixel.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <math/dag_mesh.h>
#include <math/dag_check_nan.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/shaderResBuilder/validateAlphaTest.h>

using AlphaTestValidation::BakeResult;

struct AlphaTestValidation::Context
{
  bool isWarningOnly = true;
  bool isStrict = false;
  AssetMgrProvider assetMgrProvider;
  ImageLoader imageLoader;
  render::omm::CpuContext ommCpuContext;
};

AlphaTestValidation::Context *AlphaTestValidation::init(const DataBlock &blk, AssetMgrProvider provider, ImageLoader loader)
{
  if (!blk.getBool("validate", false))
    return nullptr;
  Context *ctx = new Context;
  ctx->isWarningOnly = blk.getBool("warnOnly", true);
  ctx->isStrict = blk.getBool("strict", false);
  ctx->assetMgrProvider = provider;
  ctx->imageLoader = loader;
  if (!render::omm::init_cpu(ctx->ommCpuContext))
  {
    delete ctx;
    return nullptr;
  }
  return ctx;
}

void AlphaTestValidation::shutdown(Context *ctx)
{
  if (!ctx)
    return;
  render::omm::shutdown_cpu(ctx->ommCpuContext);
  delete ctx;
}

// based on prog/daNetGame/render/world/bvhES.cpp.inl implementation
static int get_texture_slot_by_shader(const char *cls, const MaterialData &mat, uint32_t &alpha_channel)
{
  int slot = 0;
  alpha_channel = 3;
  if (cls)
  {
    if (strncmp(cls, "rendinst_vcolor_layered", 24) == 0)
      return -1;
    if (strncmp(cls, "rendinst_layered", 16) == 0 || strncmp(cls, "dynamic_layered", 15) == 0 ||
        strncmp(cls, "rendinst_mask_layered", 21) == 0 || strncmp(cls, "rendinst_perlin_layered", 23) == 0 ||
        strncmp(cls, "rendinst_tree_perlin_layered", 28) == 0)
      slot = 3;
    else if (strncmp(cls, "rendinst_tree", 13) == 0)
    {
      slot = 1;
      alpha_channel = 0;
    }
    if (mat.mtex[slot] == BAD_TEXTUREID)
      slot = 0;
  }
  return (mat.mtex[slot] == BAD_TEXTUREID) ? -1 : slot;
}

static bool is_degenerate(const Point2 &v0, const Point2 &v1, const Point2 &v2, const Point2 &tex_size)
{
  if (!check_finite(v0.x) || !check_finite(v0.y) || !check_finite(v1.x) || !check_finite(v1.y) || !check_finite(v2.x) ||
      !check_finite(v2.y))
    return true;
  Point2 e0 = mul(v1 - v0, tex_size);
  Point2 e1 = mul(v2 - v0, tex_size);
  float area = std::abs(cross(e0, e1)) * 0.5f;
  return area < 0.1f;
}

static String resolve_texture_source(const char *tex_name, DagorAssetMgr *mgr)
{
  String tmp_stor;
  const char *decoded_name = TextureMetaData::decodeFileName(tex_name, &tmp_stor);
  String file_path(decoded_name);
  if (mgr)
  {
    String asset_name = DagorAsset::fpath2asset(decoded_name);
    if (DagorAsset *tex_a = mgr->findAsset(asset_name, mgr->getTexAssetTypeId()))
      file_path = tex_a->getTargetFilePath();
  }
  return file_path;
}

static BakeResult bake_alpha_test(AlphaTestValidation::Context *ctx, const Mesh &mesh, int mat_id, const MaterialData &mat,
  const ShaderMaterial &sh_mat, int &atest, const char *&tex_name, render::omm::CpuBakeStats &stats, Tab<int> *out_transparent_faces)
{
  if (!ctx || !ctx->imageLoader)
    return BakeResult::Error;

  atest = 0;
  const int atest_varId = VariableMap::getVariableId("atest", true);
  if (atest_varId < 0 || !sh_mat.getIntVariable(atest_varId, atest) || atest <= 0)
    return BakeResult::Error;

  if (mesh.face.empty() || mesh.tvert[0].empty() || mesh.tface[0].size() < mesh.face.size())
    return BakeResult::Error;

  uint32_t alpha_channel;
  int slot = get_texture_slot_by_shader(sh_mat.getShaderClassName(), mat, alpha_channel);
  if (slot < 0 || slot == 3)
    return BakeResult::Error;
  tex_name = get_managed_texture_name(mat.mtex[slot]);
  if (!tex_name || !*tex_name)
    return BakeResult::Error;

  DagorAssetMgr *mgr = ctx->assetMgrProvider ? ctx->assetMgrProvider() : nullptr;
  String tex_path = resolve_texture_source(tex_name, mgr);
  TexImage32 *img = ctx->imageLoader(tex_path, tmpmem);
  if (!img)
    return BakeResult::Error;

  short width = img->w;
  short height = img->h;
  Tab<uint8_t> alpha(tmpmem);
  alpha.resize_noinit(width * height);
  const TexPixel32 *pixels = img->getPixels();
  if (alpha_channel == 0)
    for (int i = 0; i < alpha.size(); ++i)
      alpha[i] = pixels[i].r;
  else
    for (int i = 0; i < alpha.size(); ++i)
      alpha[i] = pixels[i].a;
  memfree(img, tmpmem);
  img = nullptr;

  dag::ConstSpan<Point2> tvert = mesh.getTVert(0);
  Tab<uint32_t> indices(tmpmem);
  Tab<int> faces(tmpmem);
  for (int k = 0; k < mesh.face.size(); ++k)
  {
    if (mesh.face[k].mat != mat_id)
      continue;
    const TFace &tf = mesh.tface[0][k];
    G_ASSERT(tf.t[0] < tvert.size() && tf.t[1] < tvert.size() && tf.t[2] < tvert.size());
    if (is_degenerate(tvert[tf.t[0]], tvert[tf.t[1]], tvert[tf.t[2]], Point2(width, height)))
      continue;
    for (int v = 0; v < 3; v++)
      indices.push_back(tf.t[v]);
    faces.push_back(k);
  }
  if (indices.empty())
    return BakeResult::Error;

  String cache_key(0, "%s:%c", tex_name, (alpha_channel == 0) ? 'r' : 'a');
  Tab<render::omm::OpacityState> states(tmpmem);
  if (out_transparent_faces)
    states.resize(faces.size());
  if (!render::omm::cpu_bake_alpha_stats(ctx->ommCpuContext, cache_key, width, height, alpha.data(), &tvert[0].x, indices.data(),
        indices.size(), stats, make_span(states)))
    return BakeResult::Error;

  if (stats.descCount)
    return BakeResult::Cutout;
  if (stats.totalFullyOpaqueCount == stats.triangles)
    return BakeResult::FullyOpaque;
  if (stats.totalFullyTransparentCount == stats.triangles)
    return BakeResult::FullyTransparent;

  if (out_transparent_faces)
    for (int i = 0; i < states.size(); ++i)
      if (states[i] == render::omm::OpacityState::Transparent)
        out_transparent_faces->push_back(faces[i]);
  return BakeResult::Mixed;
}

void AlphaTestValidation::validate(Context *ctx, const Mesh &mesh, int mat_id, const MaterialData &mat, const ShaderMaterial &sh_mat,
  const char *dag_fn, const char *node_name, ILogWriter &log)
{
  int atest;
  const char *tex_name;
  render::omm::CpuBakeStats stats;
  const BakeResult result = bake_alpha_test(ctx, mesh, mat_id, mat, sh_mat, atest, tex_name, stats, nullptr);
  if (result == BakeResult::Error || result == BakeResult::Cutout)
    return;
  if (!ctx->isStrict && result != BakeResult::Mixed) // same skips as bvh_strict_asset_checks from bvh_omm.cpp
    return;

  const char *fn = dd_get_fname(dag_fn);
  ILogWriter::MessageType type = ctx->isWarningOnly ? ILogWriter::WARNING : ILogWriter::ERROR;
  if (result == BakeResult::FullyOpaque)
  {
    log.addMessage(type,
      "alpha test does nothing: %s node <%s> mat <%s> has atest=%d, but the alpha of texture <%s> cuts nothing out of any of the "
      "%d triangles. Remove atest from the material.",
      fn, node_name, mat.matName.c_str(), atest, tex_name, stats.triangles);
  }
  else
  {
    log.addMessage(type,
      "alpha test makes no OMM: %s node <%s> mat <%s> has atest=%d with texture <%s>, and every micro-triangle came out uniform "
      "[opaque=%d transparent=%d unknown=%d of %d triangles]. Fix the texcoords or drop the invisible triangles.",
      fn, node_name, mat.matName.c_str(), atest, tex_name, stats.totalFullyOpaqueCount, stats.totalFullyTransparentCount,
      stats.totalFullyUnknownCount, stats.triangles);
  }
}

BakeResult AlphaTestValidation::retrieve_invisible_faces(Context *ctx, const Mesh &mesh, int mat_id, const MaterialData &mat,
  const ShaderMaterial &sh_mat, Tab<int> &out_faces)
{
  int atest;
  const char *tex_name;
  render::omm::CpuBakeStats stats;
  return bake_alpha_test(ctx, mesh, mat_id, mat, sh_mat, atest, tex_name, stats, &out_faces);
}
