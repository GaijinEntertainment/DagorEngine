// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/globe.h>
#include <render/primitiveObjects.h>
#include <debug/dag_debug3d.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_materialData.h>
#include <3d/dag_render.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_mathUtils.h>
#include <math/dag_approachWindowed.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <daSkies2/daSkies.h>
#include <render/dynmodelRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <render/viewVecs.h>
#include <ioSys/dag_dataBlock.h>
#include <astronomy/astronomy.h>

GlobeRenderer::GlobeRenderer(Parameters &&parameters) : params(eastl::move(parameters))
{
  starsJulianDay = 2457388.5; // 2016-01-01:00

  textureSlices.resize(params.textureSlicesNames.size());
  G_ASSERT(textureSlices.size() == 2 || textureSlices.size() == sqr(int(sqrtf(textureSlices.size()) + 0.5f)));
  for (int sliceNo = 0; sliceNo < textureSlices.size(); ++sliceNo)
  {
    const Parameters::TextureSliceNames &sliceNames = params.textureSlicesNames[sliceNo];
    TextureSlice &slice = textureSlices[sliceNo];
    slice.colorTex = dag::get_tex_gameres(sliceNames.colorTexName.c_str());
    slice.normalsTex = d3d::check_texformat(TEXFMT_ATI2N) ? dag::get_tex_gameres(sliceNames.normalsTexName.c_str()) : SharedTex();
  }

  shadowsTex = d3d::check_texformat(TEXFMT_ATI1N) ? dag::get_tex_gameres(params.shadowsTexName.c_str()) : SharedTex();
  cloudsTex = d3d::check_texformat(TEXFMT_ATI2N) ? dag::get_tex_gameres(params.cloudsTexName.c_str()) : SharedTex();

  G_ASSERT(textureSlices.size() > 0 && textureSlices[0].colorTex);
  if (textureSlices.size() > 0 && textureSlices[0].colorTex)
  {
    SharedTex &sourceTex = textureSlices[0].colorTex;
    G_ASSERT(sourceTex);
    TextureInfo info;
    sourceTex->getinfo(info);
    texSize = Point2(info.w, info.h);
    texNumLods = sourceTex->level_count();
  }
  else
  {
    texSize = Point2(4096, 2048);
    texNumLods = 1;
  }

  drawProg.init("globe_model");
  applyLowProg.init("globe_model_apply_low");

  static int globeSliceTexSizeVarId = ::get_shader_glob_var_id("globe_slice_tex_size");
  static int globeSaturateColorVarId = ::get_shader_glob_var_id("globe_saturate_color");
  static int globeSkyLightMulVarId = ::get_shader_glob_var_id("globe_sky_light_mul");
  static int globeSunLightMulVarId = ::get_shader_glob_var_id("globe_sun_light_mul");
  static int globeCloudsColorVarId = ::get_shader_glob_var_id("globe_clouds_color");
  static int globeInitialAngleVarId = ::get_shader_glob_var_id("globe_initial_angle");
  static int globeRotateSpeedVarId = ::get_shader_glob_var_id("globe_rotate_speed");
  static int globeRotateModeVarId = ::get_shader_glob_var_id("globe_rotate_mode");

  ShaderGlobal::set_color4(globeSliceTexSizeVarId, Color4(texSize.x, texSize.y, texNumLods, 0));
  ShaderGlobal::set_real(globeSkyLightMulVarId, params.globeSkyLightMul);
  ShaderGlobal::set_real(globeSunLightMulVarId, params.globeSunLightMul);
  ShaderGlobal::set_color4(globeCloudsColorVarId, params.globeCloudsColor);
  ShaderGlobal::set_real(globeInitialAngleVarId, params.globeInitialAngle);
  ShaderGlobal::set_color4(globeSaturateColorVarId, params.globeSaturateColor);
  ShaderGlobal::set_real(globeRotateSpeedVarId, params.globeRotateSpeed);
  ShaderGlobal::set_int(globeRotateModeVarId, params.globeRotateClouds ? 1 : 0);
}

GlobeRenderer::GlobeRenderer(const DataBlock &blk) : GlobeRenderer(Parameters(blk)) {}

Point3 GlobeRenderer::geo_coord_to_cartesian(const Point2 &p)
{
  Point2 pRad = Point2(PI / 2 - DegToRad(p.x), DegToRad(p.y));
  return Point3(sinf(pRad.x) * cosf(pRad.y), cosf(pRad.x), sinf(pRad.x) * sinf(pRad.y)) * DaScattering::getEarthRadius();
}

GlobeRenderer::Parameters::Parameters(const DataBlock &blk)
{
  globeSkyLightMul = blk.getReal("globeLightMul", 1.0f);
  globeSunLightMul = blk.getReal("globeSunLightMul", 1.0f);
  globeCloudsColor.set_xyzw(blk.getPoint4("globeCloudsColor", Point4(1, 1, 1, 1)));
  globeInitialAngle = blk.getReal("globeInitialAngle", 0.0f);
  posOffset = blk.getPoint3("posOffset", Point3(0, 0, 0));
  useGmtTime = blk.getBool("useGmtTime", true);
  useKmRadiusScale = blk.getBool("useKmRadiusScale", true);
  starsIntensity = blk.getReal("starsIntensity", 1.0f);
  sunSize = blk.getReal("sunSize", 0.006f);
  sunIntensity = blk.getReal("sunIntensity", 1000.0f);
  moonSize = blk.getReal("moonSize", 0.006f);
  moonIntensity = blk.getReal("moonIntensity", 1.0f);
  globeSaturateColor.set_xyzw(blk.getPoint4("globeSaturateColor", Point4(1, 1, 1, 0)));
  globeRotateSpeed = blk.getReal("globeRotateSpeed", 0.0005);
  globeRotateClouds = blk.getBool("globeRotateClouds", false);

  const DataBlock *slices = blk.getBlockByNameEx("slices");
  textureSlicesNames.resize(slices->blockCount());
  for (int sliceNo = 0; sliceNo < slices->blockCount(); ++sliceNo)
  {
    const DataBlock *sliceBlock = slices->getBlock(sliceNo);
    TextureSliceNames &slice = textureSlicesNames[sliceNo];
    slice.colorTexName = sliceBlock->getStr("colorTex", "land_ocean_ice");
    slice.normalsTexName = sliceBlock->getStr("normalsTex", "earth_bump_n");
  }

  shadowsTexName = blk.getStr("shadowsTex", "globe_shadows");
  cloudsTexName = blk.getStr("cloudsTex", "globe_clouds_ocean_8k");
}

void GlobeRenderer::renderEnv(DaSkies &skies, RenderSun render_sun, RenderMoon render_moon, const Driver3dPerspective &persp,
  const Point3 &view_pos)
{
  static int skySpaceModeVarId = ::get_shader_glob_var_id("sky_space_mode", true);
  ShaderGlobal::set_int(skySpaceModeVarId, 1);

  skies.renderStars(persp, view_pos + params.posOffset, params.starsIntensity);

  if (render_sun == RenderSun::Yes)
    skies.renderCelestialObject(skies.getSunDir(), -1, params.sunIntensity, params.sunSize);
  if (render_moon == RenderMoon::Yes)
    skies.renderCelestialObject(skies.getMoonDir(), skies.getMoonAge() / 29.53f, params.moonIntensity, params.moonSize);

  ShaderGlobal::set_int(skySpaceModeVarId, 0);
}

void GlobeRenderer::render(DaSkies &skies, RenderSun render_sun, RenderMoon render_moon, const Point3 &view_pos,
  const TextureIDPair &low_res_tex)
{
  TIME_D3D_PROFILE(applyGlobeSkies);

  static int globeColorTexVarId = ::get_shader_variable_id("globe_color_tex");
  static int globeNormalsTexVarId = ::get_shader_variable_id("globe_normals_tex");
  static int globeShadowsTexVarId = ::get_shader_glob_var_id("globe_shadows_tex");
  static int globeCloudsOceanTexVarId = ::get_shader_variable_id("globe_clouds_ocean_tex");
  static int globeUVWindowVarId = ::get_shader_variable_id("globe_uv_window");
  static int globe_tsize_wk_hkVarId = ::get_shader_glob_var_id("globe_tsize_wk_hk");
  static int sourceTexVarId = ::get_shader_glob_var_id("source_tex");

  Driver3dPerspective persp;
  d3d::getpersp(persp);

  {
    SCOPE_RENDER_TARGET;
    if (low_res_tex.getTex())
    {
      d3d::set_render_target(low_res_tex.getTex(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 1, 0);
    }

    int targetWidthForShader, targetHeightForShader;
    d3d::get_target_size(targetWidthForShader, targetHeightForShader);

#if _TARGET_XBOX || _TARGET_C1 || _TARGET_C2
    if (low_res_tex.getTex())
    {
      targetWidthForShader *= 2; // Don't degrade sampling quality on low-res.
      targetHeightForShader *= 2;
    }
#endif

    ShaderGlobal::set_color4(globe_tsize_wk_hkVarId, targetWidthForShader, targetHeightForShader, persp.wk, persp.hk);
    SCOPED_SET_TEXTURE(globeShadowsTexVarId, shadowsTex);
    SCOPED_SET_TEXTURE(globeCloudsOceanTexVarId, cloudsTex);

    float radiusScale = params.useKmRadiusScale ? 1.0f : 1000.0f;
    Point3 lightDir;

    if (params.useGmtTime)
    {
      double lst = calc_lst_hours(starsJulianDay, 0.0f);
      star_catalog::StarDesc starDesc;
      get_sun(starsJulianDay, starDesc);
      Point3 sunLightDir = Point3(1.0f, 0.0f, 0.0f);
      radec_2_xyz(starDesc.rightAscension, starDesc.declination, lst, 0.0f, sunLightDir.z, sunLightDir.x, sunLightDir.y);
      lightDir = sunLightDir;
    }
    else
    {
      lightDir = skies.getSunDir();
    }
    Point3 lightPos = view_pos / radiusScale + params.posOffset;
    ShaderGlobal::set_color4(get_shader_variable_id("globe_skies_light_pos"), Color4::xyz1(lightPos));
    ShaderGlobal::set_color4(get_shader_variable_id("globe_skies_light_dir"), lightDir.x, lightDir.z, lightDir.y, 0);
    ShaderGlobal::set_color4(get_shader_variable_id("globe_skies_light_color"), color4(DaScattering::getBaseSunColor(), 1.0f));

    set_viewvecs_to_shader();

    if (!low_res_tex.getTex())
      renderEnv(skies, render_sun, render_moon, persp, view_pos);

    int numTiles = floor(sqrtf(textureSlices.size()) + 0.5f);
    for (int sliceNo = 0; sliceNo < textureSlices.size(); ++sliceNo)
    {
      const TextureSlice &slice = textureSlices[sliceNo];
      if (textureSlices.size() == 2)
      {
        ShaderGlobal::set_color4(globeUVWindowVarId, 2, 1, -sliceNo, 0);
      }
      else
      {
        int tileX = sliceNo % numTiles;
        int tileY = sliceNo / numTiles;
        ShaderGlobal::set_color4(globeUVWindowVarId, numTiles, numTiles, -tileX, -tileY);
      }
      SCOPED_SET_TEXTURE(globeColorTexVarId, slice.colorTex);
      SCOPED_SET_TEXTURE(globeNormalsTexVarId, slice.normalsTex);
      drawProg.render();
    }
  }

  if (low_res_tex.getTex())
  {
    renderEnv(skies, render_sun, render_moon, persp, view_pos);
    ShaderGlobal::set_texture(sourceTexVarId, low_res_tex.getId());
    applyLowProg.render();
  }
}
