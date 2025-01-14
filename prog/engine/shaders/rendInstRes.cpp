// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_rendInstRes.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_IRenderWrapperControl.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_resMgr.h>
#include <drv/3d/dag_resId.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include "shaders/sh_vars.h"
#include <stdio.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <startup/dag_globalSettings.h>

using namespace shglobvars;
static constexpr unsigned VDATA_MT_RENDINST = 2;

int (*RenderableInstanceLodsResource::get_skip_first_lods_count)(const char *name, bool has_impostors, int total_lods) = NULL;
void (*RenderableInstanceLodsResource::on_higher_lod_required)(RenderableInstanceLodsResource *res, unsigned req_lod,
  unsigned cur_lod) = nullptr;

RenderableInstanceLodsResource::ImpostorParams RenderableInstanceLodsResource::ImpostorRtData::null_params;
RenderableInstanceLodsResource::ImpostorTextures RenderableInstanceLodsResource::ImpostorRtData::null_tex;

//--- RenderableInstanceResource -------------------------------------------------------------------//

#define GLOBAL_OPTIONAL_VARS_LIST   \
  VAR(impostor_parallax_mode)       \
  VAR(impostor_view_mode)           \
  VAR(impostor_albedo_alpha)        \
  VAR(impostor_normal_translucency) \
  VAR(impostor_ao_smoothness)       \
  VAR(impostor_preshadow)           \
  VAR(cross_dissolve_mul)           \
  VAR(cross_dissolve_add)

#define VAR(a) static int a##VarId = -1;
GLOBAL_OPTIONAL_VARS_LIST
#undef VAR

static bool parallax_allowed = false;
static bool tri_view_allowed = false;
static bool rendinst_impostor_shader_initialized = false;

void init_rendinst_impostor_shader()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_OPTIONAL_VARS_LIST
#undef VAR
  rendinst_impostor_shader_initialized = true;
}

void rendinst_impostor_set_parallax_mode(bool allowed) { parallax_allowed = allowed; }

void rendinst_impostor_set_view_mode(bool allowed) { tri_view_allowed = allowed; }

bool rendinst_impostor_is_parallax_allowed() { return parallax_allowed; }

bool rendinst_impostor_is_tri_view_allowed() { return tri_view_allowed; }

RenderableInstanceResource *RenderableInstanceResource::loadResourceInternal(IGenLoad &crd, int srl_flags, ShaderMatVdata &smvd)
{
  RenderableInstanceResource *res = new RenderableInstanceResource;
  crd.read(res->dumpStartPtr(), sizeof(RenderableInstanceResource) - dumpStartOfs());

  int m_sz = (int)(intptr_t)res->rigid.mesh.get();
  res->rigid.mesh.init(InstShaderMeshResource::loadResource(crd, smvd, m_sz));
  if (!(srl_flags & SRLOAD_NO_TEX_REF))
    res->rigid.mesh->getMesh()->getMesh()->acquireTexRefs();

  return res;
}

RenderableInstanceResource *RenderableInstanceResource::clone() const
{
  RenderableInstanceResource *res = new RenderableInstanceResource(*this);
  res->rigid.mesh = res->rigid.mesh ? res->rigid.mesh->clone() : NULL;
  return res;
}


void RenderableInstanceResource::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  if (rigid.mesh)
    rigid.mesh->getMesh()->gatherUsedTex(tex_id_list);
}
void RenderableInstanceResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const
{
  if (rigid.mesh)
    rigid.mesh->getMesh()->gatherUsedMat(mat_list);
}
bool RenderableInstanceResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  return rigid.mesh ? rigid.mesh->getMesh()->replaceTexture(tex_id_old, tex_id_new) : false;
}


void RenderableInstanceResource::render(const TMatrix &tm, IRenderWrapperControl &, real trans)
{
  if (trans < 0.99f)
    return;

  d3d::settm(TM_WORLD, tm);
  TMatrix itm = inverse(tm);

  if (VariableMap::isGlobVariablePresent(localWorldXGvId) && VariableMap::isGlobVariablePresent(localWorldYGvId) &&
      VariableMap::isGlobVariablePresent(localWorldZGvId))
  {
    Point3 vn = normalize(itm.getcol(0));
    ShaderGlobal::set_color4_fast(localWorldXGvId, Color4(P3D(vn), 0));
    vn = normalize(itm.getcol(1));
    ShaderGlobal::set_color4_fast(localWorldYGvId, Color4(P3D(vn), 0));
    vn = normalize(itm.getcol(2));
    ShaderGlobal::set_color4_fast(localWorldZGvId, Color4(P3D(vn), 0));
  }

  ShaderGlobal::set_color4_fast(worldViewPosGvId, P3D(itm.getcol(3)), 1.f);


  rigid.mesh->getMesh()->render();
}

void RenderableInstanceResource::renderTrans(const TMatrix &tm, IRenderWrapperControl &, real trans)
{
  bool need_transp = trans < 0.99f;
  if (need_transp)
    ShaderGlobal::set_real_fast(globalTransRGvId, trans);

  d3d::settm(TM_WORLD, tm);

  TMatrix itm = inverse(tm);
  Point3 vn = normalize(itm.getcol(0));
  ShaderGlobal::set_color4_fast(localWorldXGvId, Color4(P3D(vn), 0));
  vn = normalize(itm.getcol(1));
  ShaderGlobal::set_color4_fast(localWorldYGvId, Color4(P3D(vn), 0));
  vn = normalize(itm.getcol(2));
  ShaderGlobal::set_color4_fast(localWorldZGvId, Color4(P3D(vn), 0));

  ShaderGlobal::set_color4_fast(worldViewPosGvId, P3D(itm.getcol(3)), 1.f);

  if (trans < 1)
    rigid.mesh->getMesh()->render();
  rigid.mesh->getMesh()->renderTrans();

  if (need_transp)
    ShaderGlobal::set_real_fast(globalTransRGvId, 1.0f);
}


//--- RenderableInstanceLodsResource ------------------------------------//
RenderableInstanceLodsResource *RenderableInstanceLodsResource::loadResource(IGenLoad &crd, int flags, const char *name,
  const DataBlock *desc)
{
  int res_sz_w = crd.readInt();
  int res_sz = res_sz_w & 0x00FFFFFF;
  bool has_impostor = (res_sz_w >> 24) & 1;
  unsigned sz = dumpStartOfs() + res_sz + (has_impostor ? (int)sizeof(ImpostorRtData) : 0);

  RenderableInstanceLodsResource *res = NULL;
  void *mem = memalloc(max(sz, (unsigned)sizeof(*res)), midmem);
  res = new (mem, _NEW_INPLACE) RenderableInstanceLodsResource;
  res->setResSizeNonTS(res_sz);
  if (has_impostor)
  {
    G_ASSERTF(((char *)res->impRtdPtr() > (char *)res &&
                sizeof(ImpostorRtData) + (char *)res->impRtdPtr() <= midmem->getSize(res) + (char *)res),
      "res=%p  impostorRtDataPtr=%p", res, res->impRtdPtr());

    new (res->impRtdPtr(), _NEW_INPLACE) ImpostorRtData;
  }

  int tmp[4];
  crd.read(tmp, sizeof(int) * 4);
  if (tmp[0] == 0xFFFFFFFF && tmp[1] == 0xFFFFFFFF)
  {
    // model matVdata saved separately from materials (mat & tex must be set in desc BLK)
    const DataBlock *texBlk = desc ? desc->getBlockByName("tex") : nullptr;
    const DataBlock *matBlk = desc ? desc->getBlockByName("mat") : nullptr;
    if (!desc || !texBlk || !matBlk)
    {
      logerr("RI <%s> was saved with sep. materials, but mat data is not found in riDesc.bin: desc=%p texBlk=%p matBlk=%p", name, desc,
        texBlk, matBlk);
      res->addRef();
      res->delRef(); // effectively deletes DObject
      return nullptr;
    }
    res->smvd = ShaderMatVdata::create(texBlk->paramCount(), matBlk->blockCount(), tmp[2], tmp[3], VDATA_MT_RENDINST);
    res->smvd->makeTexAndMat(*texBlk, *matBlk);
  }
  else
  {
    res->smvd = ShaderMatVdata::create(tmp[0], tmp[1], tmp[2], tmp[3], VDATA_MT_RENDINST);
    res->smvd->loadTexStr(crd, flags & SRLOAD_SYMTEX);
  }
  unsigned vdataFlags = 0;
  if (flags & SRLOAD_TO_SYSMEM)
    vdataFlags |= VDATA_NO_IBVB;
  if (flags & SRLOAD_SRC_ONLY)
    vdataFlags |= VDATA_SRC_ONLY;
#if DAGOR_DBGLEVEL > 0 || _TARGET_PC_WIN
  char vname[64];
  _snprintf(vname, sizeof(vname), "%s ldRes", crd.getTargetName());
  vname[sizeof(vname) - 1] = 0;
#else
  const char *vname = "";
#endif
  res->smvd->loadMatVdata(vname, crd, vdataFlags);

  crd.read(res->dumpStartPtr(), res_sz);
  res->patchAndLoadData(res_sz_w, crd, flags, name);
  if (res->hasImpostor())
    res->loadImpostorData(name);
  if (desc)
  {
    const DataBlock *texqBlk = desc->getBlockByName("texScale_data");
    if (texqBlk)
      for (int lodIdx = 0; lodIdx < res->lods.size(); lodIdx++)
        res->lods[lodIdx].texScale = texqBlk->getReal(String(0, "texScale%d", lodIdx), 0);
  }
  return res;
}

RenderableInstanceLodsResource *RenderableInstanceLodsResource::makeStubRes(const DataBlock *b)
{
  struct StubRendInst : public RenderableInstanceLodsResource
  {
    Lod lod0;
    struct StubLod : public RenderableInstanceResource
    {
      StubLod()
      {
        rigid.mesh.init(nullptr);
        addRef();
      }
      ~StubLod() {}
    };
    StubRendInst(const DataBlock *b = NULL)
    {
      packedFields = sizeof(*this) - dumpStartOfs(); // Set all other fields to zero.
      impostorDataOfs = 0;
      extraFlags = 0;
      bbox[0].set(-1e-3, -1e-3, -1e-3);
      bbox[1].set(1e-3, 1e-3, 1e-3);
      bsphCenter.set(0, 0, 0);
      bsphRad = 0;
      bound0rad = 0;

      lods.init(&lod0, 1);
      lod0.scene.setPtr(new StubLod);
      lod0.range = 1e5;
      bool hasImpostorMat = false;
      if (b)
      {
        hasImpostorMat = b->getBool("hasImpostor", false);
        bbox[0] = b->getPoint3("bbox0", bbox[0]);
        bbox[1] = b->getPoint3("bbox1", bbox[1]);
        Point4 bs = b->getPoint4("bsph", Point4::xyzV(bsphCenter, bsphRad));
        bsphCenter.set_xyz(bs);
        bsphRad = bs.w;
      }
      if (hasImpostorMat)
      {
        setHasImpostorMat();
        new (impRtdPtr(), _NEW_INPLACE) ImpostorRtData;
      }
    }
  };
  size_t sz = sizeof(StubRendInst) + ((b && b->getBool("hasImpostor", false)) ? sizeof(ImpostorRtData) : 0);
  return new (memalloc(sz, midmem), _NEW_INPLACE) StubRendInst(b);
}

static RenderableInstanceLodsResource::ImpostorTextureCallback impostor_texture_callback = nullptr;

void RenderableInstanceLodsResource::setImpostorTextureCallback(ImpostorTextureCallback callback)
{
  impostor_texture_callback = callback;
}

bool RenderableInstanceLodsResource::isBakedImpostor() const { return getImpostorParams().hasBakedTexture(); }

bool RenderableInstanceLodsResource::setImpostorVars(ShaderMaterial *mat) const
{
  auto &impostorTextures = getImpostorTextures();
  bool res = true;
  if (!rendinst_impostor_shader_initialized)
  {
    logerr("Impostor texture manager is not initialized");
    return false;
  }

  res = mat->set_texture_param(impostor_albedo_alphaVarId, impostorTextures.albedo_alpha) && res;
  res = mat->set_texture_param(impostor_normal_translucencyVarId, impostorTextures.normal_translucency) && res;
  res = mat->set_texture_param(impostor_ao_smoothnessVarId, impostorTextures.ao_smoothness) && res;
  res = mat->set_texture_param(impostor_preshadowVarId, impostorTextures.shadowAtlas) && res;

  if (isBakedImpostor())
    G_ASSERT(impostorTextures.albedo_alpha != BAD_TEXTUREID && impostorTextures.normal_translucency != BAD_TEXTUREID &&
             impostorTextures.ao_smoothness != BAD_TEXTUREID);

  if (impostor_texture_callback)
    impostor_texture_callback(this);

  return res;
}

bool RenderableInstanceLodsResource::hasImpostorVars(ShaderMaterial *mat) const
{
  bool res = true;
  TEXTUREID textureIdStub;
  res = res && mat->getTextureVariable(impostor_albedo_alphaVarId, textureIdStub);
  res = res && mat->getTextureVariable(impostor_normal_translucencyVarId, textureIdStub);
  res = res && mat->getTextureVariable(impostor_ao_smoothnessVarId, textureIdStub);
  res = res && mat->getTextureVariable(impostor_preshadowVarId, textureIdStub);
  return res;
}

bool RenderableInstanceLodsResource::setImpostorTransitionRange(ShaderMaterial *mat, float transition_lod_start,
  float transition_range) const
{
  bool res = true;
  res = mat->set_real_param(cross_dissolve_mulVarId, 1 / transition_range) && res;
  res = mat->set_real_param(cross_dissolve_addVarId, -transition_lod_start / transition_range) && res;
  return res;
}

void RenderableInstanceLodsResource::loadImpostorData(const char *name)
{
  if (!hasImpostor())
    return;
  auto &params = getImpostorParamsE();
  if (params.horizontalSamples > 0)
    return;
  if (get_resource_type_id("impostor_data") == 0) // when no impostor_data resource present
    return;

  DataBlock *impostorData =
    reinterpret_cast<DataBlock *>(get_one_game_resource_ex(GAMERES_HANDLE(impostor_data), ImpostorDataGameResClassId));
  if (!impostorData)
  {
    params.horizontalSamples = params.verticalSamples = 0;
    return;
  }
  DataBlock *impostorBlk = impostorData->getBlockByName(name);
  Tab<ShaderMaterial *> mats;
  gatherUsedMat(mats);

  static bool impostorPreshadowForceDisable = false;
  if (!mats.empty()) // Do not access shader dump on dedicated
  {
    static const int baked_impostor_preshadowsVarId = get_shader_variable_id("baked_impostor_preshadows", true);
    if (ShaderGlobal::has_associated_interval(baked_impostor_preshadowsVarId) &&
        ShaderGlobal::is_var_assumed(baked_impostor_preshadowsVarId) &&
        ShaderGlobal::get_interval_assumed_value(baked_impostor_preshadowsVarId) == 0)
      impostorPreshadowForceDisable = true;
  }

  bool runtimeImpostor = false;
  bool bakedImpostor = false;
  for (const auto &itr : mats)
  {
    runtimeImpostor = runtimeImpostor || (strcmp(itr->getShaderClassName(), "rendinst_impostor") == 0);
    bakedImpostor = bakedImpostor || (strcmp(itr->getShaderClassName(), "rendinst_baked_impostor") == 0);
  }
  if (runtimeImpostor == bakedImpostor && !mats.empty()) // avoid reporting errors for stubbed RI on dedicated server
  {
    String matNames;
    for (const auto &itr : mats)
      matNames = String(0, "%s, %s", itr->getShaderClassName(), matNames.c_str());
    if (!runtimeImpostor)
      logerr("An asset is supposed to be an impostor, but doesn't have any impostor materials: <%s>. Materials: %s", name,
        matNames.c_str());
    else
      logerr("An asset has both runtime and baked impostor materials at the same time: <%s>. Materials: %s", name, matNames.c_str());
  }
  else if (impostorBlk)
  {
    if (bakedImpostor || mats.empty()) // accept stubbed RI on dedicated server
    {
      params.horizontalSamples = impostorBlk->getInt("horizontalSamples", 0);
      params.verticalSamples = impostorBlk->getInt("verticalSamples", 0);
      if (params.horizontalSamples > 0 && params.verticalSamples > 0)
      {
        rotationPaletteSize = impostorBlk->getInt("rotationPaletteSize", rotationPaletteSize);
        setTiltLimit(impostorBlk->getPoint3("tiltLimit", getTiltLimit()));

        Point2 scale = impostorBlk->getPoint2("scale", Point2{0, 0});

        params.preshadowEnabled = impostorBlk->getBool("preshadowEnabled", true) && !impostorPreshadowForceDisable;
        params.bottomGradient = impostorBlk->getReal("bottomGradient", 0.0);
        params.scale = Point4(scale.x, scale.y, 1.0f / scale.x, 1.0f / scale.y);
        params.boundingSphere = Point4(bsphCenter.x, bsphCenter.y, bsphCenter.z, bsphRad);
        params.vertexOffsets[0] = impostorBlk->getPoint4("vertexOffsets1_2", Point4(1, 1, 1, 1));
        params.vertexOffsets[1] = impostorBlk->getPoint4("vertexOffsets3_4", Point4(1, 1, 1, 1));
        params.vertexOffsets[2] = impostorBlk->getPoint4("vertexOffsets5_6", Point4(1, 1, 1, 1));
        params.vertexOffsets[3] = impostorBlk->getPoint4("vertexOffsets7_8", Point4(1, 1, 1, 1));
        params.crownCenter1 = impostorBlk->getPoint4("crownCenter1", Point4(0, 0, 0, 0));
        params.invCrownRad1 = impostorBlk->getPoint4("crownRad1", Point4(0, 0, 0, 0));
        if (params.invCrownRad1.x * params.invCrownRad1.y * params.invCrownRad1.z > 0.)
        {
          params.invCrownRad1.x = 1. / params.invCrownRad1.x;
          params.invCrownRad1.y = 1. / params.invCrownRad1.y;
          params.invCrownRad1.z = 1. / params.invCrownRad1.z;
        }
        params.crownCenter2 = impostorBlk->getPoint4("crownCenter2", Point4(0, 0, 0, 0));
        params.invCrownRad2 = impostorBlk->getPoint4("crownRad2", Point4(0, 0, 0, 0));
        if (params.invCrownRad2.x * params.invCrownRad2.y * params.invCrownRad2.z > 0.)
        {
          params.invCrownRad2.x = 1. / params.invCrownRad2.x;
          params.invCrownRad2.y = 1. / params.invCrownRad2.y;
          params.invCrownRad2.z = 1. / params.invCrownRad2.z;
        }
        for (int i = 0; i < params.horizontalSamples * params.verticalSamples; i++)
        {
          String tmName = String(0, "tm_%d", i);
          String linesName = String(0, "lines_%d", i);
          G_ASSERTF(impostorBlk->findParam(tmName) && impostorBlk->findParam(linesName), "Missing impostor slice data for: %s", name);
          params.perSliceParams[i].sliceTm = impostorBlk->getPoint4(tmName, Point4(1, 1, 0, 0));
          Point4 clippingLines = impostorBlk->getPoint4(linesName, Point4(0, 0, 1, 1));
          clippingLines =
            Point4(clippingLines.y - clippingLines.x, clippingLines.w - clippingLines.z, clippingLines.x, clippingLines.z);
          params.perSliceParams[i].clippingLines = clippingLines;
        }
      }
      else
      {
        params.horizontalSamples = params.verticalSamples = 0;
        logerr("An ri <%s> has baked impostor params (in impostor data) but it's invalid", name);
      }
    }
    else
      logerr("An asset is supposed to have baked impostor, but it has runtime impostor shader."
             " Try rebaking impostors and then rebuilding assets using dabuild: <%s>",
        name);
  }
  else if (!runtimeImpostor)
    logerr("An asset is supposed to have baked impostor, but it has runtime impostor shader."
           " Try rebaking impostors and then rebuilding assets using dabuild: <%s>",
      name);
  release_game_resource(reinterpret_cast<GameResource *>(impostorData));
}

void RenderableInstanceLodsResource::patchAndLoadData(int res_sz, IGenLoad &crd, int flags, const char *name)
{
  packedFields = res_sz & 0x0FFFFFFF; // We call it on a resource creation so it could be non thread safe.
  bool hasImpostorMat = (res_sz & (1 << HAS_IMPOSTOR_MAT_SHIFT)) != 0;
  G_ASSERT(!(hasOccluderBox() && hasOccluderQuad()));
  lods.patch(&lods);

  if (hasImpostorMat && impostorDataOfs)
    if (ImpostorData *id = const_cast<ImpostorData *>(getImpostorData()))
    {
      id->convexPointsPtr.patch(&lods);
      id->shadowPointsPtr.patch(&lods);
    }

  for (int i = 0; i < lods.size(); i++)
  {
    lods[i].scene = RenderableInstanceResource::loadResourceInternal(crd, flags, *smvd);
    lods[i].scene->addRef();
  }
  for (int i = lods.size() - 1; i >= 0; i--)
    if (lods[i].scene->getMesh() && lods[i].scene->getMesh()->getMesh()->getMesh()->getAllElems().size())
      break;
    else
    {
      char buf[4 << 10];
      if (i == 0)
      {
        logwarn("empty mesh in trailing lod%02d, remains; %s", i, dgs_get_fatal_context(buf, sizeof(buf)));
        break;
      }
      lods[i].scene->delRef();
      lods[i].scene = nullptr;
      logerr("empty mesh in trailing lod%02d, dropped; %s", i, dgs_get_fatal_context(buf, sizeof(buf)));
      lods.init(lods.data(), i);
    }

  uint32_t qlBestLod = 0;
  if (RenderableInstanceLodsResource::get_skip_first_lods_count && !(flags & SRLOAD_TO_SYSMEM))
    qlMinAllowedLod = qlBestLod =
      clamp<int>(RenderableInstanceLodsResource::get_skip_first_lods_count(name, hasImpostorMat, lods.size()), 0, lods.size());
  if (qlBestLod)
  {
    debug("%s: skipping %d neares LODs (impostor=%d totalLODs=%d)", name, qlBestLod, hasImpostorMat, lods.size());
    G_ASSERT(qlBestLod <= lods.size());
  }
  if (RenderableInstanceLodsResource::on_higher_lod_required && !hasImpostorData() && !(flags & SRLOAD_TO_SYSMEM))
  {
    bool discardable = true;
    for (const ShaderMatVdata *smvd : getSmvd())
      if (smvd && (!smvd->isReloadable() || !smvd->areLodsSplit()))
      {
        discardable = false;
        break;
      }
    if (discardable)
      qlBestLod = max((int)lods.size() - 1, 0);
  }
  applyQlBestLod(qlBestLod);

  smvd->applyFirstLodsSkippedCount(qlBestLod);
}

void RenderableInstanceLodsResource::prepareTextures(const char *name, uint32_t shadow_atlas_size, int shadow_atlas_mip_offset,
  int texture_format_flags)
{
  auto &params = getImpostorParamsE();
  auto &impostorTextures = getImpostorTexturesE();
  if (hasImpostorData())
  {
    if (impostorTextures.isInitialized())
      return;
    impostorTextures.albedo_alpha = ::get_tex_gameres(String(0, "%s_aa", name));
    if (impostorTextures.albedo_alpha != BAD_TEXTUREID)
    {
      if (BaseTexture *tex = D3dResManagerData::getD3dTex<RES3D_TEX>(impostorTextures.albedo_alpha))
      {
        add_anisotropy_exception(impostorTextures.albedo_alpha);
        tex->disableSampler();
        TextureInfo texInfo;
        tex->getinfo(texInfo);
        impostorTextures.height = texInfo.h;
        impostorTextures.width = texInfo.w;

        // Only create the shadow texture if pre-baked textures are used and preshadows enabled
        if (params.preshadowEnabled)
        {
          String textureName(0, "%s_s", name);
          impostorTextures.shadowAtlas = ::get_tex_gameres(textureName);
          if (shadow_atlas_size == 0)
          {
            // Level binary didn't have any exported palettes. Default: palette = {identity rotation}
            shadow_atlas_size = 1;
          }
          if (impostorTextures.shadowAtlas == BAD_TEXTUREID)
          {
            int levelCount = max(tex->level_count() - shadow_atlas_mip_offset, 1);
            int preshadowWidth = max(texInfo.w >> shadow_atlas_mip_offset, 1);
            int preshadowHeight = max(texInfo.h >> shadow_atlas_mip_offset, 1);

            Texture *shadowTex = d3d::create_array_tex(preshadowWidth, preshadowHeight, shadow_atlas_size,
              texture_format_flags | TEXCF_CLEAR_ON_CREATE, levelCount, textureName.c_str());
            if (shadowTex)
            {
              impostorTextures.shadowAtlas = register_managed_tex(textureName, shadowTex);
              shadowTex->texlod(0);
              shadowTex->texfilter(TEXFILTER_DEFAULT);
              shadowTex->setAnisotropy(1);
              add_anisotropy_exception(impostorTextures.shadowAtlas);

              impostorTextures.shadowAtlasTex = acquire_managed_tex(impostorTextures.shadowAtlas);
              release_managed_tex(impostorTextures.shadowAtlas); // get_tex_gameres() or register_managed_tex() hold main ref to tex
            }
          }
        }
      }
    }
    impostorTextures.normal_translucency = ::get_tex_gameres(String(0, "%s_nt", name));
    if (impostorTextures.normal_translucency != BAD_TEXTUREID)
    {
      if (BaseTexture *tex = D3dResManagerData::getD3dTex<RES3D_TEX>(impostorTextures.normal_translucency))
      {
        tex->texaddr(TEXADDR_CLAMP);
        tex->texlod(0);
        tex->setAnisotropy(1);
        add_anisotropy_exception(impostorTextures.normal_translucency);
      }
    }
    impostorTextures.ao_smoothness = ::get_tex_gameres(String(0, "%s_as", name));
    if (impostorTextures.ao_smoothness != BAD_TEXTUREID)
    {
      if (BaseTexture *tex = D3dResManagerData::getD3dTex<RES3D_TEX>(impostorTextures.ao_smoothness))
      {
        tex->texaddr(TEXADDR_CLAMP);
        tex->texlod(0);
        tex->setAnisotropy(1);
        add_anisotropy_exception(impostorTextures.ao_smoothness);
      }
    }
  }
  else
  {
    impostorTextures.albedo_alpha = BAD_TEXTUREID;
    impostorTextures.normal_translucency = BAD_TEXTUREID;
    impostorTextures.ao_smoothness = BAD_TEXTUREID;
  }
}

RenderableInstanceLodsResource::RenderableInstanceLodsResource(const RenderableInstanceLodsResource &from) : //-V730
  bsphRad(0.f),
  bound0rad(0.f),
  impostorDataOfs(from.impostorDataOfs),
  packedFields(interlocked_relaxed_load(from.packedFields) & ~(RES_LD_FLG_MASK << RES_LD_FLG_SHIFT)),
  smvd(from.smvd)
{
  uint32_t resSize = (packedFields >> RES_SIZE_SHIFT) & RES_SIZE_MASK;
  // don't touch (write) occl unless (hasOccluderBox|hasOccluderQuad)!=0; resSize MAY BE smaller than offset between lods and end of
  // occl!
  void *new_base = &lods;
  const void *old_base = &from.lods;

  bvhId = from.bvhId;

  memcpy(new_base, old_base, resSize);
  lods.rebase(new_base, old_base);
  if (hasImpostor())
    new (impRtdPtr(), _NEW_INPLACE) ImpostorRtData;

  for (int i = 0; i < lods.size(); ++i)
    if (lods[i].scene)
    {
      lods[i].scene = lods[i].scene->clone();
      lods[i].scene->addRef();
    }

  if (hasImpostorData() && impostorDataOfs)
    if (ImpostorData *id = const_cast<ImpostorData *>(getImpostorData()))
    {
      id->convexPointsPtr.rebase(new_base, old_base);
      id->shadowPointsPtr.rebase(new_base, old_base);
    }
}


RenderableInstanceLodsResource *RenderableInstanceLodsResource::clone() const
{
  unsigned sz = dumpStartOfs() + getResSize() + (hasImpostor() ? (int)sizeof(ImpostorRtData) : 0);
  void *mem = memalloc(max(sz, unsigned(sizeof(*this))), midmem);
  auto copy = new (mem, _NEW_INPLACE) RenderableInstanceLodsResource(*this);
  copy->bvhId = 0;
  return copy;
}

void RenderableInstanceLodsResource::clearData()
{
  for (int i = 0; i < lods.size(); i++)
    lods[i].scene->delRef();
  lods.init(NULL, 0);
  smvd = NULL;
  auto &impostorTextures = getImpostorTexturesE();
  impostorTextures.close();
}


void RenderableInstanceLodsResource::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  for (int i = 0; i < lods.size(); ++i)
    if (lods[i].scene)
      lods[i].scene->gatherUsedTex(tex_id_list);
}
void RenderableInstanceLodsResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const
{
  for (int i = 0; i < lods.size(); ++i)
    if (lods[i].scene)
      lods[i].scene->gatherUsedMat(mat_list);
}
bool RenderableInstanceLodsResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  bool ok = false;

  for (int i = 0; i < lods.size(); ++i)
    if (lods[i].scene)
      if (lods[i].scene->replaceTexture(tex_id_old, tex_id_new))
        ok = true;

  return ok;
}
int RenderableInstanceLodsResource::Lod::getAllElems(Tab<dag::ConstSpan<ShaderMesh::RElem>> &out_elems) const
{
  out_elems.clear();
  out_elems.push_back(scene->getMesh()->getMesh()->getMesh()->getAllElems());
  return out_elems.size();
}
