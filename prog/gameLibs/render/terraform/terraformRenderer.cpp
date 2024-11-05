// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/terraform/terraformRenderer.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_quadIndexBuffer.h>
#include <generic/dag_carray.h>
#include <math/dag_math3d.h>
#include <render/debug3dSolid.h>
#include <heightmap/heightmapHandler.h>
#include <memory/dag_framemem.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <gameRes/dag_gameResources.h>

static const int PATCH_SIZE = Terraform::PATCH_SIZE;
static const int PATCH_MAX_BUFFER_SIZE = 128;
G_STATIC_ASSERT(PATCH_MAX_BUFFER_SIZE > 0);

struct TerraformRenderer::Patch
{
  UniqueTexHolder texData;
  bool invalid;
  uint32_t timestep;
};

struct TerraformRenderer::Prim
{
  ska::flat_hash_map<uint16_t, uint8_t> altMap;
  bool invalidPatch;
};


TerraformRenderer::TerraformRenderer(Terraform &in_tform, const Desc &in_desc) : tform(in_tform), desc(in_desc), patches()
{
  tform.registerRenderer(this);

  if (desc.diffuseTexId != BAD_TEXTUREID)
  {
    acquire_managed_tex(desc.diffuseTexId);
    tform_diffuse_texVarId = ::get_shader_glob_var_id("tform_diffuse_tex");
  }
  if (desc.diffuseAboveTexId != BAD_TEXTUREID)
  {
    acquire_managed_tex(desc.diffuseAboveTexId);
    tform_diffuse_above_texVarId = ::get_shader_glob_var_id("tform_diffuse_above_tex");
  }
  if (desc.detailRTexId != BAD_TEXTUREID)
  {
    acquire_managed_tex(desc.detailRTexId);
    tform_detail_rtexVarId = ::get_shader_glob_var_id("tform_detail_rtex");
  }
  if (desc.normalsTexId != BAD_TEXTUREID)
  {
    acquire_managed_tex(desc.normalsTexId);
    tform_normals_texVarId = ::get_shader_glob_var_id("tform_normals_tex");
  }

  ShaderGlobal::set_color4(::get_shader_glob_var_id("tform_above_ground_color"), desc.aboveGroundColor);
  ShaderGlobal::set_color4(::get_shader_glob_var_id("tform_under_ground_color"), desc.underGroundColor);
  ShaderGlobal::set_real(::get_shader_glob_var_id("tform_dig_ground_alpha"), desc.digGroundAlpha);
  ShaderGlobal::set_real(::get_shader_glob_var_id("tform_dig_roughness"), desc.digRoughness);
  ShaderGlobal::set_color4(::get_shader_glob_var_id("tform_texture_tile"), Color4(desc.textureTile.x, desc.textureTile.y, 0, 0));
  ShaderGlobal::set_real(::get_shader_glob_var_id("tform_texture_blend"), desc.textureBlend);
  ShaderGlobal::set_real(::get_shader_glob_var_id("tform_texture_blend_above"), desc.textureBlendAbove);
  ShaderGlobal::set_real(::get_shader_glob_var_id("terraform_max_physics_error"), desc.maxPhysicsError);
  ShaderGlobal::set_real(::get_shader_glob_var_id("terraform_min_physics_error"), desc.minPhysicsError);

  patchShader.init("terraform_patch", NULL, 0, "terraform_patch");

  const IPoint2 hmapSizes = tform.getHMapSizes();
  hmapSavedTex = UniqueTexHolder(dag::create_tex(NULL, hmapSizes.x, hmapSizes.y, TEXFMT_L16 | TEXCF_MAYBELOST, 1, "hmap_saved"),
    "terraform_hmap_saved");
  uint16_t *texDataPtr = NULL;
  int texStride = 0;
  hmapSavedTex.getTex2D()->lockimg((void **)&texDataPtr, texStride, 0, TEXLOCK_WRITE | TEXLOCK_DISCARD);
  G_ASSERT(texDataPtr);
  if (texDataPtr)
    tform.copyOriginalHeightMap(texDataPtr);
  hmapSavedTex.getTex2D()->unlockimg();
  hmapSavedTex.getTex2D()->texaddr(TEXADDR_CLAMP);

  index_buffer::init_quads_32bit(1);

  terraform_tex_data_pivot_sizeVarId = get_shader_glob_var_id("terraform_tex_data_pivot_size");
  terraform_min_max_levelVarId = get_shader_glob_var_id("terraform_min_max_level");
  terraform_render_modeVarId = get_shader_glob_var_id("terraform_render_mode");
  terraform_tex_data_sizeVarId = get_shader_glob_var_id("terraform_tex_data_size");
  terraform_enabledVarId = get_shader_glob_var_id("terraform_enabled", true);

  ShaderGlobal::set_int(terraform_enabledVarId, 1);
}

TerraformRenderer::~TerraformRenderer()
{
  tform.unregisterRenderer(this);

  deletePrims();
  deletePatches();
  closeHeightMask();

  release_managed_tex(desc.diffuseAboveTexId);
  release_managed_tex(desc.diffuseTexId);
  release_managed_tex(desc.detailRTexId);
  release_managed_tex(desc.normalsTexId);

  ShaderGlobal::set_int(terraform_enabledVarId, 0);

  hmapSavedTex.close();
  index_buffer::release_quads_32bit();
  patchShader.close();
}

void TerraformRenderer::initHeightMask()
{
  if (heightMaskTex.getTex2D())
    return;

  tform_height_mask_scale_offsetVarId = get_shader_glob_var_id("tform_height_mask_scale_offset");

  heightMaskTex =
    dag::create_tex(NULL, desc.heightMaskSize.x, desc.heightMaskSize.y, TEXFMT_L8 | TEXCF_RTARGET, 1, "tform_height_mask");
  heightMaskTex.getTex2D()->texfilter(TEXFILTER_LINEAR);
  heightMaskTex.getTex2D()->texaddr(TEXADDR_WRAP);
}

void TerraformRenderer::closeHeightMask() { heightMaskTex.close(); }

void TerraformRenderer::update() { ++curTimestep; }

void TerraformRenderer::render(RenderMode render_mode, const BBox2 &region_box)
{
  ShaderGlobal::set_texture(tform_diffuse_texVarId, desc.diffuseTexId);
  ShaderGlobal::set_texture(tform_diffuse_above_texVarId, desc.diffuseAboveTexId);
  ShaderGlobal::set_texture(tform_detail_rtexVarId, desc.detailRTexId);
  ShaderGlobal::set_texture(tform_normals_texVarId, desc.normalsTexId);

  heightMaskTex.setVar();
  hmapSavedTex.setVar();

  if (heightMaskTex.getBaseTex() && (render_mode != TerraformRenderer::RENDER_MODE_HEIGHT_MASK))
    d3d::resource_barrier({heightMaskTex.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  ShaderGlobal::set_color4(terraform_min_max_levelVarId, tform.getDesc().getMinLevel(), tform.getDesc().getMaxLevel(),
    tform.getZeroAlt() / 255.0f, 0);
  ShaderGlobal::set_int(terraform_render_modeVarId, render_mode);
  ShaderGlobal::set_color4(terraform_tex_data_sizeVarId, PATCH_SIZE, PATCH_SIZE, 1.0f / PATCH_SIZE, 1.0f / PATCH_SIZE);

  index_buffer::Quads32BitUsageLock quads32Lock;
  d3d::setvsrc(0, NULL, 0);

  IPoint2 patchXY0 = tform.getPatchXYFromCoord(tform.getCoordFromPos(region_box.lim[0]));
  IPoint2 patchXY1 = tform.getPatchXYFromCoord(tform.getCoordFromPos(region_box.lim[1]));
  for (int patchX = patchXY0.x; patchX <= patchXY1.x; ++patchX)
    for (int patchY = patchXY0.y; patchY <= patchXY1.y; ++patchY)
    {
      int patchNo = tform.getPatchNoFromXY(IPoint2(patchX, patchY));
      if (patchNo < 0 || patchNo >= tform.getPatchNum() || !tform.isPatchExist(patchNo))
        continue;
      IPoint2 coord(tform.getCoordFromPcd(Pcd(patchNo, 0)));
      Point2 pos1(tform.getPosFromCoord(coord));
      Point2 pos2(tform.getPosFromCoord(coord + IPoint2(PATCH_SIZE, PATCH_SIZE)));
      BBox2 box = BBox2(pos1, pos2);
      if (!(region_box & box))
        continue;
      Patch *patch = getPatch(patchNo);
      if (!patch)
        continue;

      ShaderGlobal::set_color4(terraform_tex_data_pivot_sizeVarId, Color4(pos1.x, pos1.y, pos2.x - pos1.x, pos2.y - pos1.y));
      patch->texData.setVar();

      if (!patchShader.shader->setStates())
        break;
      d3d_err(d3d::drawind(PRIM_TRILIST, 0, 2, 0));
    }

  if (heightMaskTex.getBaseTex() && (render_mode == TerraformRenderer::RENDER_MODE_HEIGHT_MASK))
    d3d::resource_barrier({heightMaskTex.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void TerraformRenderer::renderHeightMask(const Point2 &pivot)
{
  if (!heightMaskTex.getTex2D())
    return;

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  // align pivot to avoid flikering
  IPoint2 pivotCell(pivot.x / desc.heightMaskSize.x * tform.getResolution().x,
    pivot.y / desc.heightMaskSize.y * tform.getResolution().y);
  Point2 pivotPos(pivotCell.x * desc.heightMaskSize.x / tform.getResolution().x,
    pivotCell.y * desc.heightMaskSize.y / tform.getResolution().y);

  Point2 worldSize = desc.heightMaskWorldSize;
  BBox2 bbox;
  bbox.lim[0] = pivotPos - worldSize * 0.5f;
  bbox.lim[1] = pivotPos + worldSize * 0.5f;

  TMatrix viewTm = TMatrix::IDENT;
  viewTm.setcol(0, 1, 0, 0);
  viewTm.setcol(1, 0, 0, 1);
  viewTm.setcol(2, 0, 1, 0);
  d3d::settm(TM_VIEW, viewTm);

  TMatrix4 projTm;
  projTm = matrix_ortho_off_center_lh(bbox[0].x, bbox[1].x, bbox[1].y, bbox[0].y, -10.0f, 10.0f);
  d3d::settm(TM_PROJ, &projTm);

  d3d::set_render_target(heightMaskTex.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(tform.getZeroAlt(), 0, 0, 0), 0.f, 0);
  ShaderGlobal::set_color4(tform_height_mask_scale_offsetVarId, safediv(1.0f, bbox.width().x), safediv(1.0f, bbox.width().y),
    safediv(-bbox.lim[0].x, bbox.width().x), safediv(-bbox.lim[0].y, bbox.width().y));

  render(TerraformRenderer::RENDER_MODE_HEIGHT_MASK, bbox);
}

void TerraformRenderer::invalidate()
{
  deletePatches();
  update();
}

void TerraformRenderer::drawDebug(const Point3 &pos, int mode)
{
  const int PATCH_NUM = 8;
  const int CLIP_SIZE = 64;

  Tab<Point3> vertices;
  vertices.resize((CLIP_SIZE + 1) * (CLIP_SIZE + 1));
  Tab<uint16_t> indices;
  indices.resize(CLIP_SIZE * CLIP_SIZE * 6);

  for (int px = 0; px < PATCH_NUM; ++px)
    for (int py = 0; py < PATCH_NUM; ++py)
    {
      IPoint2 pivotPos = tform.getCoordFromPos(Point2::xz(pos));
      if (mode == 3 || mode == 4)
        pivotPos = tform.getHmapCoordFromCoord(pivotPos);
      pivotPos = pivotPos - IPoint2((px - PATCH_NUM / 2) * CLIP_SIZE, (py - PATCH_NUM / 2) * CLIP_SIZE);

      for (int j = 0; j <= CLIP_SIZE; ++j)
        for (int i = 0; i <= CLIP_SIZE; ++i)
        {
          int x = pivotPos.x + i;
          int y = pivotPos.y + j;
          float h = 0.0f;

          IPoint2 coord = IPoint2(x, y);
          if (mode == 3 || mode == 4)
            coord = tform.getCoordFromHmapCoord(coord);
          Point2 vpos = tform.getPosFromCoord(coord);
          if (mode == 2 || mode == 4)
            h = tform.sampleHmapHeightCur(coord);
          else
            tform.getHMap().getHeight(vpos, h, NULL);

          vertices[j * (CLIP_SIZE + 1) + i] = Point3::xVy(vpos, h);
        }

      for (int j = 0; j < CLIP_SIZE; ++j)
        for (int i = 0; i < CLIP_SIZE; ++i)
        {
          int index = (j * CLIP_SIZE + i) * 6;
          indices[index++] = j * (CLIP_SIZE + 1) + i;
          indices[index++] = j * (CLIP_SIZE + 1) + i + 1;
          indices[index++] = (j + 1) * (CLIP_SIZE + 1) + i + 1;

          indices[index++] = j * (CLIP_SIZE + 1) + i;
          indices[index++] = (j + 1) * (CLIP_SIZE + 1) + i + 1;
          indices[index++] = (j + 1) * (CLIP_SIZE + 1) + i;
        }

      TMatrix tm = TMatrix::IDENT;
      tm.setcol(3, 0, 0.0f, 0);

      draw_debug_solid_mesh(indices.data(), indices.size() / 3, (const float *)vertices.data(), sizeof(Point3), vertices.size(), tm,
        Color4(1, 0, 0, 0.5f));
    }
}

TerraformRenderer::Patch *TerraformRenderer::getPatch(int patch_no)
{
  dag::ConstSpan<uint8_t> tformPatch = tform.getPatch(patch_no);
  if (!tformPatch.data())
  {
    auto iter = patches.find(patch_no);
    if (iter != patches.end())
    {
      del_it(iter->second);
      patches.erase(iter);
    }
    return NULL;
  }

  auto iter = patches.find(patch_no);
  if (iter == patches.end())
  {
    Patch *patch = NULL;
    if (patches.size() >= PATCH_MAX_BUFFER_SIZE)
    {
      auto iter = patches.begin();
      int minPatchNo = iter->first;
      int minTimestep = iter->second->timestep;
      for (auto &elem : patches)
        if (elem.second->timestep < minTimestep)
        {
          minTimestep = elem.second->timestep;
          minPatchNo = elem.first;
        }
      iter = patches.find(minPatchNo);
      G_ASSERT(iter != patches.end());
      patch = iter->second;
      patches.erase(iter);
    }
    else
    {
      patch = new Patch();
      String name(0, "terraform_tex_data_%d", patch_no);
      patch->texData = UniqueTexHolder(dag::create_tex(NULL, PATCH_SIZE, PATCH_SIZE, TEXFMT_R8 | TEXCF_MAYBELOST, 1, name.str()),
        "terraform_tex_data");
      d3d_err(patch->texData.getTex2D());
      patch->texData.getTex2D()->texaddr(TEXADDR_CLAMP);
      patch->texData.getTex2D()->texfilter(TEXFILTER_LINEAR);
    }
    patch->invalid = true;
    iter = patches.insert(iter, eastl::make_pair(patch_no, patch));
  }

  Patch &patch = *iter->second;
  patch.timestep = curTimestep;

  auto primIter = prims.find(patch_no);
  bool invalidPrim = primIter != prims.end() && primIter->second->invalidPatch;

  if (patch.invalid || invalidPrim)
  {
    patch.invalid = false;

    uint8_t *texDataPtr;
    int texStride;
    patch.texData.getTex2D()->lockimg((void **)&texDataPtr, texStride, 0, TEXLOCK_WRITE | TEXLOCK_DISCARD);
    G_ASSERT(texDataPtr);

    if (texDataPtr)
      for (int j = 0; j < PATCH_SIZE; ++j)
        memcpy(texDataPtr + j * texStride / sizeof(uint8_t), tformPatch.data() + j * PATCH_SIZE, PATCH_SIZE * sizeof(uint8_t));

    if (texDataPtr && primIter != prims.end())
    {
      Prim &prim = *primIter->second;
      prim.invalidPatch = false;
      for (auto &item : prim.altMap)
      {
        IPoint2 patchCoord(tform.getCoordFromPcd(Pcd(0, item.first)));
        texDataPtr[patchCoord.y * texStride / sizeof(uint8_t) + patchCoord.x] = item.second;
      }
    }

    patch.texData.getTex2D()->unlockimg();
  }

  return iter->second;
}

void TerraformRenderer::invalidatePatch(int patch_no)
{
  auto iter = patches.find(patch_no);
  if (iter != patches.end())
    iter->second->invalid = true;
}

void TerraformRenderer::deletePatches()
{
  for (auto &elem : patches)
    del_it(elem.second);
  patches.clear();
}

void TerraformRenderer::invalidatePrims()
{
  deletePrims();

  for (auto &item : tform.getPrimAltMap())
  {
    Pcd pcd(item.first);
    auto iter = prims.find(pcd.patchNo);
    if (iter == prims.end())
    {
      Prim *prim = new Prim();
      prim->invalidPatch = true;
      iter = prims.insert(iter, eastl::make_pair(pcd.patchNo, prim));
    }

    Prim &prim = *iter->second;
    prim.altMap.emplace(pcd.dataIndex, tform.packAlt(item.second));
  }
}

void TerraformRenderer::deletePrims()
{
  for (auto &elem : prims)
    del_it(elem.second);
  prims.clear();
}