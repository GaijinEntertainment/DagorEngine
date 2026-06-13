// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Land-class / virtual-texture rendering and texture preparation for LandMeshRenderer.
// Split out of lmeshRenderer.cpp; shares private state via lmeshRendererCommon.h.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IBBox2.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include "heightmap/heightmapHandler.h"
#include "lmeshRendererGlue.h"
#include <memory/dag_framemem.h>
#include <EASTL/string.h>
#include <EASTL/bitset.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <perfMon/dag_cpuFreq.h>
#include "lmeshRendererCommon.h"

static int create_new_land_shader(const char *shader_name, const char *landclass_name)
{
  ShaderMaterial *m = new_shader_material_by_name(shader_name, "");
  if (!m)
  {
    G_ASSERT_LOG(0, "can't create ShaderMaterial '%s' for land class <%s>", shader_name, landclass_name);
    return LC_SIMPLE;
  }
  landclassShader.push_back();
  const int id = (int)landclassShader.size() - 1;
  landclassShader[id].material.reset(m);
  G_VERIFY(landclassShader[id].elem = m->make_elem());

  landclassShader[id].vs_const_offset = get_shader_int_constant(String(64, "%s_vs_const_offset", shader_name), -1);
  landclassShader[id].ps_const_offset = get_shader_int_constant(String(64, "%s_ps_const_offset", shader_name), -1);
  landclassShader[id].lc_detail_const_offset = get_shader_int_constant(String(64, "%s_lc_detail_const_offset", shader_name), -1);
  landclassShader[id].lc_textures_sampler = get_shader_int_constant(String(64, "%s_lc_textures_sampler", shader_name), -1);
  landclassShader[id].lc_ps_details_cb_register = get_shader_int_constant(String(64, "%s_lc_ps_details_cb_register", shader_name), -1);
  // debug("create new land class shader %d <%s> (%d %d %d)",  id, shader_name,
  //   landclassShader[id].vs_const_offset, landclassShader[id].lc_detail_const_offset, landclassShader[id].lc_textures_sampler);
  return id;
}
static LandClassType insert_land_shader(const char *shader_name, const char *landclass_name)
{
  auto it = shadersNames.find_as(shader_name);
  if (it != shadersNames.end()) // not inserted (e.g. value exist)
    return (LandClassType)it->second;

  LandClassType lc = (LandClassType)create_new_land_shader(shader_name, landclass_name);
  shadersNames[shader_name] = lc;
  return lc;
}
static inline TEXTUREID query_tex_loading(TEXTUREID id)
{
  if (id == BAD_TEXTUREID)
    return id;
  mark_managed_tex_lfu(id);
  return id;
}

bool LandMeshRenderer::reloadGrassMaskTex(int land_class_id, TEXTUREID newGrassMaskTexId)
{
  landClasses[land_class_id].grassMaskTexId = newGrassMaskTexId;
  if (land_class_id < landClassesLoaded.size())
    landClassesLoaded[land_class_id].grassMask.tid = query_tex_loading(newGrassMaskTexId);
  return true;
}

// Landclasses are rendered on clipmap or on grassmask. We use seperate sampler in order to avoid additional mipbias.
d3d::SamplerInfo landmesh::get_texture_sampler_info(TEXTUREID tid)
{
  if (tid == BAD_TEXTUREID)
    return {};

  return get_sampler_info(get_texture_meta_data(tid));
}

d3d::SamplerInfo landmesh::get_texture_sampler_info(TEXTUREID tid, float anisotropic_max)
{
  d3d::SamplerInfo samplerInfo = landmesh::get_texture_sampler_info(tid);
  samplerInfo.anisotropic_max = anisotropic_max;
  return samplerInfo;
}

void LandMeshRenderer::prepareLandClasses(LandMeshManager &provider)
{
  landClassesLoaded.resize(0);
  landClassesLoaded.resize(landClasses.size());
  for (int i = 0; i < landClassesLoaded.size(); ++i)
  {
    landClassesLoaded[i].lcDetailParamsVS = landClasses[i].lcDetailParamsVS;
    landClassesLoaded[i].detailsCB = &landClasses[i].detailsCB;
    landClassesLoaded[i].weighMapMulOffset = landClasses[i].weighMapMulOffset;
    landClassesLoaded[i].weightMapNoiseScale = landClasses[i].weightMapNoiseScale;
    // landClassesLoaded[i].lcDetailParamsPS = landClasses[i].lcDetailParamsPS;

    landClassesLoaded[i].colorMap.tid = query_tex_loading(landClasses[i].colormapId);
    landClassesLoaded[i].colorMap.sampler =
      d3d::request_sampler(landmesh::get_texture_sampler_info(landClassesLoaded[i].colorMap.tid, 1.f));

    if (BaseTexture *t = D3dResManagerData::getBaseTex(landClassesLoaded[i].colorMap.tid))
    {
      TextureInfo tinfo;
      t->getinfo(tinfo, 0);
      landClassesLoaded[i].invColormapSize = Point2(safediv(1.0f, tinfo.w), safediv(1.0f, tinfo.h));
    }

    landClassesLoaded[i].grassMask.tid = query_tex_loading(landClasses[i].grassMaskTexId);
    landClassesLoaded[i].grassMask.sampler =
      d3d::request_sampler(landmesh::get_texture_sampler_info(landClassesLoaded[i].grassMask.tid, 1.f));

    landClassesLoaded[i].lcType = landClasses[i].lcType;
    if (landClassesLoaded[i].lcType != LC_SIMPLE)
    {
      clear_and_resize(landClassesLoaded[i].lcTextures, landClasses[i].lcTextures.size());
      for (int j = 0; j < landClasses[i].lcTextures.size(); ++j)
      {
        landClassesLoaded[i].lcTextures[j].tid = query_tex_loading(landClasses[i].lcTextures[j]);

        d3d::SamplerInfo lcSamplerInfo = landmesh::get_texture_sampler_info(landClassesLoaded[i].lcTextures[j].tid, 1.f);
        if (auto *tex = D3dResManagerData::getBaseTex(landClassesLoaded[i].lcTextures[j].tid);
            landClassesLoaded[i].lcType == LC_CUSTOM && tex && tex->level_count() == 1)
        {
          lcSamplerInfo.mip_map_mode = d3d::MipMapMode::Disabled;
          lcSamplerInfo.filter_mode = d3d::FilterMode::Point;
        }
        landClassesLoaded[i].lcTextures[j].sampler = d3d::request_sampler(lcSamplerInfo);
      }
    }
    landClassesLoaded[i].textureDimensions = safediv(1.0f, landClasses[i].tile);
    landClassesLoaded[i].bumpScales = landClasses[i].bumpScales;
    landClassesLoaded[i].puddleScales = landClasses[i].puddleScales;
    landClassesLoaded[i].displacementMin = landClasses[i].displacementMin;
    landClassesLoaded[i].displacementMax = landClasses[i].displacementMax;
    landClassesLoaded[i].compatibilityDiffuseScales = landClasses[i].compatibilityDiffuseScales;
    landClassesLoaded[i].finalColorMul = landClasses[i].finalColorMul;
    landClassesLoaded[i].randomFlowmapParams = landClasses[i].randomFlowmapParams;
    landClassesLoaded[i].flowmapMask = landClasses[i].flowmapMask;
    landClassesLoaded[i].waterDecalBumpScale = landClasses[i].waterDecalBumpScale;

    landClassesLoaded[i].flowmapTex.tid = query_tex_loading(landClasses[i].flowmapTex);
    landClassesLoaded[i].flowmapTex.sampler =
      d3d::request_sampler(landmesh::get_texture_sampler_info(landClassesLoaded[i].flowmapTex.tid, 1.f));

    landClassesLoaded[i].physmatIDs = landClasses[i].physmatIds;

    for (int dtype = 0; dtype < landClasses[i].lcDetailTextures.size(); ++dtype)
    {
      landClassesLoaded[i].lcDetailTextures[dtype] = landClasses[i].lcDetailTextures[dtype];
      landClassesLoaded[i].lcDetailTextures[dtype].reserve((landClassesLoaded[i].lcDetailTextures[dtype].size() + 7) & ~7);
    }
    if (landClassesLoaded[i].lcType >= LC_MEGA_NO_NORMAL && landClassesLoaded[i].lcType < LC_CUSTOM)
      for (auto d : landClassesLoaded[i].lcDetailTextures[LandClassDetailTextures::ALBEDO])
        G_ASSERTF(d >= 0, "mega land class should have valid diffuse textures lc <%s>", landClasses[i].name);
    if (landClassesLoaded[i].lcType == LC_MEGA_DETAILED)
      for (auto d : landClassesLoaded[i].lcDetailTextures[LandClassDetailTextures::REFLECTANCE])
        G_ASSERTF(d >= 0, "detailed mega land class should have valid _r(2) textures lc<%s>", landClasses[i].name);

    if (landClassesLoaded[i].lcType == LC_CUSTOM)
    {
      landClassesLoaded[i].lcType = insert_land_shader(landClasses[i].shader_name, landClasses[i].name);
      G_ASSERT(landClassesLoaded[i].lcType >= LC_CUSTOM);
    }
  }
  for (int dtype = 0; dtype < NUM_TEXTURES_STACK; ++dtype)
  {
    clear_and_resize(megaDetails[dtype], provider.getMegaDetailsId()[dtype].size());
    for (int i = 0; i < megaDetails[dtype].size(); ++i)
    {
      megaDetails[dtype][i].tid = query_tex_loading(provider.getMegaDetailsId()[dtype][i]);
      megaDetails[dtype][i].sampler = d3d::INVALID_SAMPLER_HANDLE; // will set on updateCustomSamplers
    }
  }

  for (int i = 0; i < megaDetailsArray.size(); ++i)
  {
    // On DNG disabled anistropic use by using tex3Dlod.
    megaDetailsArray[i].first.tid = query_tex_loading(provider.getMegaDetailsArrayId(i));
    megaDetailsArray[i].first.sampler = d3d::INVALID_SAMPLER_HANDLE; // will set on updateCustomSamplers
    megaDetailsArray[i].second = D3dResManagerData::getD3dTex<D3DResourceType::ARRTEX>(megaDetailsArray[i].first.tid);
  }

  updateCustomSamplers(provider);
}

void LandMeshRenderer::updateCustomSamplers(LandMeshManager &provider)
{
  for (int dtype = 0; dtype < NUM_TEXTURES_STACK; ++dtype)
    for (int i = 0; i < megaDetails[dtype].size(); ++i)
      megaDetails[dtype][i].sampler = d3d::request_sampler(landmesh::get_texture_sampler_info(megaDetails[dtype][i].tid));

  for (int i = 0; i < megaDetailsArray.size(); ++i)
    megaDetailsArray[i].first.sampler = d3d::request_sampler(landmesh::get_texture_sampler_info((megaDetailsArray[i].first.tid)));

  ShaderGlobal::set_sampler(var::decals_detail_overrideSampler,
    d3d::request_sampler({.anisotropic_max = (float)::dgs_tex_anisotropy}));

  provider.updateOverrideSamplers();
}
void LandMeshRenderer::setCustomLcTextures()
{
  ShaderGlobal::set_texture(::get_shader_variable_id("biomeIndicesTex", true), BAD_TEXTUREID);
  if (biomeLandClassIdx < 0)
    return;

  int i = biomeLandClassIdx;
  const LandClassDetailTextures &land = landClasses[i];
  const LCTexturesLoaded &landLoaded = landClassesLoaded[i];
  static int use_flowmap_from_textureVarID = ::get_shader_glob_var_id("use_flowmap_from_texture", true);
  ShaderGlobal::set_int(use_flowmap_from_textureVarID, 0);

  ShaderGlobal::set_float(var::indicestexDimensions, landLoaded.textureDimensions);

  if (landLoaded.lcTextures.size())
  {
    for (int i = 0; i < landLoaded.lcTextures.size(); ++i)
    {
      bind_managed_tex_ps(lmesh_sampler__land_detail_tex1 + 1 + i, landLoaded.lcTextures[i].tid, landLoaded.lcTextures[i].sampler);
    }
  }

  if (lmesh_sampler__land_detail_array1 >= 0)
  {
    for (int i = 0; i < megaDetailsArray.size(); ++i)
    {
      d3d::set_tex(STAGE_PS, lmesh_sampler__land_detail_array1 + i, megaDetailsArray[i].second);
      d3d::set_sampler(STAGE_PS, lmesh_sampler__land_detail_array1 + i, megaDetailsArray[i].first.sampler);
    }
  }

  const ShaderInfo &shader = landclassShader[LC_CUSTOM];

  if (shader.lc_ps_details_cb_register >= 0 && landLoaded.detailsCB && *landLoaded.detailsCB)
  {
    d3d::set_const_buffer(STAGE_PS, shader.lc_ps_details_cb_register, *landLoaded.detailsCB);
  }

  if (lmesh_vs_weight_params > 0 && landLoaded.weighMapMulOffset.x > 0.0f)
  {
    G_ASSERT(lmesh_ps_weight_params > 0);
    d3d::set_vs_const(lmesh_vs_weight_params, &landLoaded.weighMapMulOffset, 1);
    Point4 temp = {landLoaded.weightMapNoiseScale, 0, 0, 0};
    d3d::set_ps_const(lmesh_ps_weight_params, &temp, 1);
  }

  BaseTexture *lcTex = ::acquire_managed_tex(landClasses[i].lcTextures[0]);
  TextureInfo lcTexInfo;
  lcTex->getinfo(lcTexInfo);
  ::release_managed_tex(landClasses[i].lcTextures[0]);

  float landSize = safediv(1.0f, landClasses[i].tile);
  float worldLcTexelSize = landSize / lcTexInfo.w;

  Point3 offset = Point3(0, 0, 0);

  ShaderGlobal::set_texture(::get_shader_variable_id("biomeIndicesTex", true), landClasses[i].lcTextures[0]);
  ShaderGlobal::set_float4(::get_shader_variable_id("biome_indices_tex_size", true), lcTexInfo.w, lcTexInfo.h, 1.0f / lcTexInfo.w,
    1.0f / lcTexInfo.h);
  ShaderGlobal::set_float4(::get_shader_variable_id("land_detail_mul_offset", true), landClasses[i].tile, -landClasses[i].tile,
    (landClasses[i].offset.x - offset.x + 0.5f * worldLcTexelSize) * landClasses[i].tile,
    (landClasses[i].offset.y - offset.z + 0.5f * worldLcTexelSize) * -landClasses[i].tile);
}

void LandMeshRenderer::renderLandclasses(CellState &curState, bool useFilter, LandClassType lcFilter)
{
  eastl::optional<LandClassType> currentLcType;
  Point4 weight[2] = {Point4(0, 0, 0, 0), Point4(0, 0, 0, 0)};
  d3d::set_ps_const(lmesh_ps_const__weight, &weight[0].x, 2);
  int last_ps_cb_register = -1;
  bool valid_shader = false;
  bool has_flowmap_tex = false;
  bool blendSet = false;

  shaders::OverrideStateId prevStateId = shaders::overrides::get_current();

  for (int detailI = 0; detailI < curState.numDetailTextures; ++detailI)
  {
    const LCTexturesLoaded &landLoaded = landClassesLoaded[curState.lcIds[detailI]];

    if (useFilter && (landLoaded.lcType != lcFilter))
      continue;
    // useFilter == render to grass, grass biome id doesnt blend
    // we could blend what is blendable and write using UAV grass types
    // that would require some changes in daNetGame and WT, but for time being keep as-is
    if (!useFilter && !blendSet)
    {
      prevStateId = setStateBlend();
      blendSet = true;
    }

    if ((!currentLcType || currentLcType != landLoaded.lcType) || ((landLoaded.flowmapTex.tid != BAD_TEXTUREID) != has_flowmap_tex))
    {
      static int use_flowmap_from_textureVarID = ::get_shader_glob_var_id("use_flowmap_from_texture", true);
      has_flowmap_tex = (landLoaded.flowmapTex.tid != BAD_TEXTUREID);
      // renderLandclasses always sets this interval when called so we don't need reset it to default value
      ShaderGlobal::set_int(use_flowmap_from_textureVarID, has_flowmap_tex ? 1 : 0);

      currentLcType = landLoaded.lcType;
      valid_shader = false;
      if (lmesh_sampler__land_detail_array1 >= 0 && currentLcType >= LC_CUSTOM)
      {
        Point4 texSizes = Point4::ZERO;
        for (int i = 0; i < megaDetailsArray.size(); ++i)
        {
          Texture *tex = megaDetailsArray[i].second;
          d3d::set_tex(STAGE_PS, lmesh_sampler__land_detail_array1 + i, tex);
          d3d::set_sampler(STAGE_PS, lmesh_sampler__land_detail_array1 + i, megaDetailsArray[i].first.sampler);
          store_tex_width(tex, i, texSizes);
        }

        set_ps_const_opt(lmesh_ps_const__detailarraytexsizes, texSizes);
      }
      G_ASSERT(landclassShader[*currentLcType].elem);
      if (!landclassShader[*currentLcType].elem->setStates(0, true)) // different land class types!
      {
        logerr("can not set land class = %d stateblocks 0x%X: %d:%d", *currentLcType, ShaderGlobal::getCurBlockStateWord(),
          ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME), ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE));
        continue;
      }
      else
        valid_shader = true;
    }
    if (!valid_shader)
      continue;

    G_ASSERT(landLoaded.colorMap.tid);

    bind_managed_tex_ps(lmesh_sampler__land_detail_tex1, landLoaded.colorMap.tid, landLoaded.colorMap.sampler);
    set_ps_const1_opt(lmesh_ps_const__invtexturesizes, curState.invTexSizes[detailI >> 2][detailI & 3]);

    const ShaderInfo &shader = landclassShader[*currentLcType];
    if (currentLcType != LC_SIMPLE)
    {
      if (currentLcType >= LC_CUSTOM)
      {
        ShaderGlobal::set_float(var::indicestexDimensions, landLoaded.textureDimensions);
      }
      if (shader.lc_textures_sampler >= 0 && landLoaded.lcTextures.size())
      {
        Point4 texSizes = Point4::ZERO;
        for (int i = 0; i < landLoaded.lcTextures.size(); ++i)
        {
          Texture *tex =
            bind_managed_tex_ps(shader.lc_textures_sampler + i, landLoaded.lcTextures[i].tid, landLoaded.lcTextures[i].sampler);
          store_tex_width(tex, i, texSizes);
        }

        set_ps_const_opt(lmesh_ps_const__lctexsizes, texSizes);
      }
      if (shader.ps_const_offset >= 0 && landLoaded.lcDetailParamsPS.size())
        d3d::set_ps_const(shader.ps_const_offset, &landLoaded.lcDetailParamsPS[0].x, landLoaded.lcDetailParamsPS.size());
      if (shader.lc_detail_const_offset >= 0 && landLoaded.lcDetailTextures[0].size())
      {
        G_ASSERT(((landLoaded.lcDetailTextures[0].size() + 7) & ~7) <= landLoaded.lcDetailTextures[0].capacity());
        // packed int16 fed as raw bytes into PS const registers: set_ps_const takes const void*, so no float*
        // reinterpret_cast (which would be a needless V1032 align-2 -> align-4 pointer cast).
        d3d::set_ps_const(shader.lc_detail_const_offset, landLoaded.lcDetailTextures[0].data(),
          (landLoaded.lcDetailTextures[0].size() + 7) / 8);
      }

      if (shader.vs_const_offset >= 0 && landLoaded.lcDetailParamsVS.size())
        d3d::set_vs_const(shader.vs_const_offset, &landLoaded.lcDetailParamsVS[0].x, landLoaded.lcDetailParamsVS.size());
      if (shader.lc_ps_details_cb_register >= 0 && landLoaded.detailsCB && *landLoaded.detailsCB)
      {
        if (last_ps_cb_register >= 0 && last_ps_cb_register != shader.lc_ps_details_cb_register)
          d3d::set_const_buffer(STAGE_PS, last_ps_cb_register, NULL);
        d3d::set_const_buffer(STAGE_PS, last_ps_cb_register = shader.lc_ps_details_cb_register, *landLoaded.detailsCB);
      }
      if (lmesh_vs_weight_params > 0 && landLoaded.weighMapMulOffset.x > 0.0f)
      {
        G_ASSERT(lmesh_ps_weight_params > 0);
        d3d::set_vs_const(lmesh_vs_weight_params, &landLoaded.weighMapMulOffset, 1);
        Point4 temp = {landLoaded.weightMapNoiseScale, 0, 0, 0};
        d3d::set_ps_const(lmesh_ps_weight_params, &temp, 1);
      }
    }

    if (currentLcType < LC_CUSTOM)
    {
      set_ps_const_opt(lmesh_ps_const__bumpscales, landLoaded.bumpScales);
      set_ps_const_opt(lmesh_ps_const__puddlescales, landLoaded.puddleScales);
      set_ps_const_opt(lmesh_ps_const__displacementmin, landLoaded.displacementMin);
      set_ps_const_opt(lmesh_ps_const__displacementmax, landLoaded.displacementMax);
      set_ps_const_opt(lmesh_ps_const__compdiffusescales, landLoaded.compatibilityDiffuseScales);
      set_ps_const_opt(lmesh_ps_const__finalcolormul, landLoaded.finalColorMul);

      if (lmesh_ps_const__randomFlowmapParams >= 0)
      {
        d3d::set_ps_const(lmesh_ps_const__randomFlowmapParams, &landLoaded.randomFlowmapParams.x, 1);
        d3d::set_ps_const(lmesh_ps_const__randomFlowmapParams + 1, &landLoaded.flowmapMask.x, 1);
      }

      if ((lmesh_sampler__flowmap_tex >= 0) && (has_flowmap_tex))
      {
        bind_managed_tex_ps(lmesh_sampler__flowmap_tex, landLoaded.flowmapTex.tid, landLoaded.flowmapTex.sampler);
      }
    }

    if (currentLcType >= LC_MEGA_NO_NORMAL && currentLcType < LC_CUSTOM)
    {
      Point4 texSizes = Point4::ZERO;
      for (int di = 0; di < landLoaded.lcDetailTextures[LandClassDetailTextures::ALBEDO].size(); ++di)
      {
        const int albedoId = landLoaded.lcDetailTextures[LandClassDetailTextures::ALBEDO][di];
        // debug("landLoaded.detailArrayNo[di] = %d", landLoaded.detailArrayNo[di]);
        // debug("megaDetails.size() = %d", megaDetails.size());
        // debug("megaDetails[%d] = 0x%d", landLoaded.detailArrayNo[di], megaDetails[landLoaded.detailArrayNo[di]]);
        int sampler_idx = lmesh_sampler__land_detail_tex1 + 3 + di;
        if (albedoId >= 0)
        {
          TidSamplerWithoutMipbiasPair tidSamplerPair = megaDetails[LandClassDetailTextures::ALBEDO][albedoId];
          Texture *tex = bind_managed_tex_ps(sampler_idx, tidSamplerPair.tid, tidSamplerPair.sampler);
          store_tex_width(tex, di, texSizes);
        }
        else
          d3d::set_tex(STAGE_PS, sampler_idx, nullptr);
        if (lmesh_sampler__land_detail_ntex1 >= 0 && currentLcType > LC_MEGA_NO_NORMAL)
        {
          const int reflectanceId = landLoaded.lcDetailTextures[LandClassDetailTextures::REFLECTANCE][di];
          sampler_idx = lmesh_sampler__land_detail_ntex1 + di;
          if (reflectanceId >= 0)
          {
            TidSamplerWithoutMipbiasPair tidSamplerPair = megaDetails[LandClassDetailTextures::REFLECTANCE][reflectanceId];
            bind_managed_tex_ps(sampler_idx, tidSamplerPair.tid, tidSamplerPair.sampler);
          }
          else
            d3d::set_tex(STAGE_PS, sampler_idx, nullptr);
        }
      }

      set_ps_const_opt(lmesh_ps_const__lcdetailstexsizes, texSizes);
    }

    int detailNo = detailI >> 1, detailSubNo = ((detailI & 1) << 1);
    Color4 mul_offset = Color4(curState.mul_offset[curState.mirrorState.x][curState.mirrorState.y][detailNo][detailSubNo],
      curState.mul_offset[curState.mirrorState.x][curState.mirrorState.y][detailNo][detailSubNo + 1],
      curState.mul_offset[curState.mirrorState.x][curState.mirrorState.y][detailNo + 4][detailSubNo],
      curState.mul_offset[curState.mirrorState.x][curState.mirrorState.y][detailNo + 4][detailSubNo + 1]);

    d3d::set_vs_const(lmesh_vs_const__mul_offset_base, &mul_offset.r, 1);
    if (detailI < 4)
      weight[0][detailI] = 1.f;
    else
      weight[1][detailI - 4] = 1.f;
    d3d::set_ps_const(lmesh_ps_const__weight, &weight[0].x, 2);
    d3d::setvsrc_ex(0, one_quad, 0, sizeof(short) * 4);
    d3d::draw(PRIM_TRISTRIP, 0, 2);
    if (detailI < 4)
      weight[0][detailI] = 0.f;
    else
      weight[1][detailI - 4] = 0.f;
  }

  resetOverride(prevStateId);

  if (last_ps_cb_register >= 0)
    d3d::set_const_buffer(STAGE_PS, last_ps_cb_register, NULL);
}
