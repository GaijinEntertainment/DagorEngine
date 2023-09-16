/*
initially, created for make correct wetness for 1 unit (for hero)
1. unit has bbox, create 'volume' texture
- instead of volume, create flat-3d-texture (volume texture, which slices stored in 2d atlas texture) (easy to update texture in one
dip)
2. for each volume texture point - calculate correct wave height at this point
- transform volume uv to worldPos
- sample water height at this point & store in flat-3d-texture
3. while rendering the model
- transfrorm pixel worldPos to unitBox uwv, using hero transform matrix (i.e. we have volume uwv)
- sample water height at this point & calculate wetness


NOTE: enable only when 1. render hero & hero is ship 2.when we make resolve deferred buffer
*/
#include <render/heroWetness.h>

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>

#include <3d/dag_texMgr.h>
#include <3d/dag_tex3d.h>
#include <startup/dag_globalSettings.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <EASTL/vector.h>


HeroWetness::HeroWetness() :
  heroWetnessVolumeSlices(32),
  heroWetnessVolumeSizeX(32),
  heroWetnessVolumeSizeY(32),
  heroWetnessDarkening(0.6f),
  heroWetnessAboveDist(0.5f),
  waterHeightRendererVb(NULL),
  waterHeightRendererShmat(NULL),
  waterHeightRendererShElem(NULL),
  waterHeightRendererVDecl(0),
  globalFrameBlockId(-1),
  heroWetnessTexVarId(-1),
  heroFoamScroll(Point2(0.f, 0.f)),
  heroFoamScrollSpeed(0.5f),
  heroFoamFadeUnderWater(Point2(2.f, 2.f)),
  numClearsToDo(heroWetnessTex.size()),
  heroWetnessRenderIter(0)
{
  mem_set_0(heroWetnessTex);
  for (int texNo = 0; texNo < heroWetnessTexId.size(); ++texNo)
    heroWetnessTexId[texNo] = BAD_TEXTUREID;

  init();
}

HeroWetness::~HeroWetness() { close(); }

void HeroWetness::init()
{
  const DataBlock *heroWetnessBlk = ::dgs_get_game_params()->getBlockByNameEx("hero_wetness");
  if (!heroWetnessBlk->getBool("useHeroWetness", true))
    return;

  uint32_t fmt = TEXFMT_A16B16G16R16F;
  unsigned int workingFlags = d3d::USAGE_FILTER | d3d::USAGE_RTARGET;
  if ((d3d::get_texformat_usage(fmt) & workingFlags) != workingFlags)
    return;

  // options & shader vars
  heroWetnessVolumeSlices = max(heroWetnessBlk->getInt("volumeSlices", 32), 1); // z upwards, i.e. horizontal slices
  heroWetnessVolumeSizeX = max(heroWetnessBlk->getInt("volumeSizeX", 32), 1);
  heroWetnessVolumeSizeY = max(heroWetnessBlk->getInt("volumeSizeY", 32), 1);

  heroWetnessDarkening = heroWetnessBlk->getReal("diffuseDarkening", 0.6f);
  heroWetnessAboveDist = heroWetnessBlk->getReal("aboveWaterDist", 0.5f);

  static int heroWetnessVolumeNumLayersVarId = get_shader_variable_id("hero_wetness_volume_num_layers", true);
  ShaderGlobal::set_real(heroWetnessVolumeNumLayersVarId, heroWetnessVolumeSlices);

  static int heroWetnessDarkeningVarId = get_shader_variable_id("hero_wetness_darkening", true);
  ShaderGlobal::set_real(heroWetnessDarkeningVarId, heroWetnessDarkening);

  static int heroWetnessAboveVarId = get_shader_variable_id("hero_wet_above", true);
  ShaderGlobal::set_real(heroWetnessAboveVarId, heroWetnessAboveDist);

  heroWetnessTexVarId = get_shader_variable_id("hero_wetness_tex", true);
  ShaderGlobal::set_texture(heroWetnessTexVarId, BAD_TEXTUREID);

  globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");

  // flat 3d texture
  for (int i = 0; i < heroWetnessTex.size(); i++)
  {
    String texName = String(100, "heroWetnessVolumeTex%d", i);
    heroWetnessTex[i] = d3d::create_tex(NULL, heroWetnessVolumeSizeX * heroWetnessVolumeSlices, heroWetnessVolumeSizeY,
      TEXCF_RTARGET | fmt, 1, texName.str());

    heroWetnessTexId[i] = register_managed_tex(texName.str(), heroWetnessTex[i]);
    heroWetnessTex[i]->texaddr(TEXADDR_CLAMP);
  }

  heroWetnessRenderIter = 0;

  // create optimized renderer
  const char *shader_name = "volume_water_height";
  waterHeightRendererShmat = new_shader_material_by_name_optional(shader_name, NULL);
  CompiledShaderChannelId chan[1] = {
    {SCTYPE_FLOAT4, SCUSAGE_POS, 0, 0},
  };

  waterHeightRendererVDecl = dynrender::addShaderVdecl(chan, countof(chan));

  if (!waterHeightRendererShmat.get())
    fatal("can't create ShaderMaterial for '%s'", shader_name);
  else
  {
    waterHeightRendererShElem = waterHeightRendererShmat->make_elem();
    if (!waterHeightRendererShElem)
      fatal("can't create ShaderElement for ShaderMaterial '%s'", shader_name);
    if (waterHeightRendererShElem)
      waterHeightRendererShElem->replaceVdecl(waterHeightRendererVDecl);
  }

  int vbSize = 4 * 6 * heroWetnessVolumeSlices;
  G_ASSERT(!waterHeightRendererVb);
  waterHeightRendererVb = d3d::create_vb(sizeof(float) * vbSize, 0, "wetnessCalculationPostfx");
  d3d_err(waterHeightRendererVb);
  fillVertexBuffer(vbSize);

  // init foam
  const DataBlock *heroFoamBlk = ::dgs_get_game_params()->getBlockByNameEx("heroFoam");
  initHeroWaterFoam(heroFoamBlk);

  clearHeroWetnessVolume();
}

void HeroWetness::fillVertexBuffer(uint32_t size)
{
  // use flat 3d texture (2d atlas-texture to store z-layers)
  const float postfx_vert[] = {
    -1,
    -1,
    +1,
    -1,
    -1,
    +1,

    -1,
    +1,
    +1,
    -1,
    +1,
    +1,
  };

  eastl::vector<float> vbData(size);
  float uv_x;
  for (int j = 0; j < heroWetnessVolumeSlices; j++)
    for (int i = 0; i < 6; i++)
    {
      uv_x = postfx_vert[i * 2] * 0.5f + 0.5f;                                                      // pos -> uv
      vbData[(j * 6 + i) * 4 + 0] = (uv_x + (float)j) * 2.f / (float)heroWetnessVolumeSlices - 1.f; // uv * quad_scale + quad_offset -
                                                                                                    // > pos
      vbData[(j * 6 + i) * 4 + 1] = postfx_vert[i * 2 + 1];
      vbData[(j * 6 + i) * 4 + 2] = uv_x;
      vbData[(j * 6 + i) * 4 + 3] = (float)j / (float)heroWetnessVolumeSlices; // solume slice
    }

  float *verts = NULL;
  if (waterHeightRendererVb->lock(0, 0, (void **)&verts, VBLOCK_WRITEONLY) && verts)
  {
    memcpy(verts, vbData.data(), sizeof(float) * size);
    waterHeightRendererVb->unlock();
  }
}

void HeroWetness::close()
{
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(heroWetnessTexId[0], heroWetnessTex[0]);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(heroWetnessTexId[1], heroWetnessTex[1]);

  del_d3dres(waterHeightRendererVb);
  waterHeightRendererShmat = NULL;
  waterHeightRendererShElem = NULL;
  waterHeightRendererVDecl = 0;
}

void HeroWetness::clearHeroWetnessVolume()
{
  wetTime = -1.0f;
  numClearsToDo = heroWetnessTex.size();
}

void HeroWetness::calcHeroWetnessVolume(float dt, const TMatrix &hero_tm, const BBox3 &hero_box, bool is_ship, bool has_water_3d,
  float water_level)
{
  if ((hero_tm.getcol(3).y + hero_box.lim[0].y) < water_level)
    wetTime = 0.0f;

  const WetnessParams &wetPar = is_ship ? wetnessParams : wetnessParamsGroundVehicle;
  if (isInSleep())
    return;
  else if (wetTime > safediv(1.0f, wetPar.drySpeed.x))
    clearHeroWetnessVolume();
  else
    wetTime += dt;

  float clearInShader = 0.f;
  if (numClearsToDo > 0)
  {
    clearInShader = 1.f;
    numClearsToDo--;
  }

  // update foam
  updateHeroWaterFoam(dt, hero_tm, is_ship);

  // hero data
  Point3 boxSize = hero_box.width();
  Point3 boxOffset = hero_box.lim[0];

  // enlarge box on half texel to eliminate bounds bilinear blending artefacts
  Point3 enlarge = Point3(boxSize.x / (float)heroWetnessVolumeSizeX, boxSize.y / (float)heroWetnessVolumeSlices,
    boxSize.z / (float)heroWetnessVolumeSizeY);
  boxOffset -= enlarge * 0.5;
  boxSize += enlarge;

  // unit box uv -> model bbox -> world pos
  TMatrix toHeroBboxTM;
  toHeroBboxTM.setcol(0, Point3(boxSize.x, 0.f, 0.f));
  toHeroBboxTM.setcol(1, Point3(0.f, boxSize.y, 0.f));
  toHeroBboxTM.setcol(2, Point3(0.f, 0.f, boxSize.z));
  toHeroBboxTM.setcol(3, boxOffset);
  TMatrix unitBoxToWorldPos = hero_tm * toHeroBboxTM;

  Point3 toWorldPosTmX = unitBoxToWorldPos.getcol(0);
  Point3 toWorldPosTmY = unitBoxToWorldPos.getcol(1);
  Point3 toWorldPosTmZ = unitBoxToWorldPos.getcol(2);
  Point3 toWorldPosTmW = unitBoxToWorldPos.getcol(3);

  // world pos -> model bbox -> unit box uv
  TMatrix toUnitBboxTM;
  toUnitBboxTM.setcol(0, Point3(1.f / boxSize.x, 0.f, 0.f));
  toUnitBboxTM.setcol(1, Point3(0.f, 1.f / boxSize.y, 0.f));
  toUnitBboxTM.setcol(2, Point3(0.f, 0.f, 1.f / boxSize.z));
  toUnitBboxTM.setcol(3, -Point3(boxOffset.x / boxSize.x, boxOffset.y / boxSize.y, boxOffset.z / boxSize.z));

  TMatrix worldPosToUnitBox = toUnitBboxTM * orthonormalized_inverse(hero_tm);
  Point3 toUnitBoxTmX = worldPosToUnitBox.getcol(0);
  Point3 toUnitBoxTmY = worldPosToUnitBox.getcol(1);
  Point3 toUnitBoxTmZ = worldPosToUnitBox.getcol(2);
  Point3 toUnitBoxTmW = worldPosToUnitBox.getcol(3);


  // shader vars
  static int toHeroVolumeTmXVarId = get_shader_variable_id("to_hero_volume_tm_x", true);
  static int toHeroVolumeTmYVarId = get_shader_variable_id("to_hero_volume_tm_y", true);
  static int toHeroVolumeTmZVarId = get_shader_variable_id("to_hero_volume_tm_z", true);

  ShaderGlobal::set_color4(toHeroVolumeTmXVarId, toWorldPosTmX.x, toWorldPosTmY.x, toWorldPosTmZ.x, toWorldPosTmW.x);
  ShaderGlobal::set_color4(toHeroVolumeTmYVarId, toWorldPosTmX.y, toWorldPosTmY.y, toWorldPosTmZ.y, toWorldPosTmW.y);
  ShaderGlobal::set_color4(toHeroVolumeTmZVarId, toWorldPosTmX.z, toWorldPosTmY.z, toWorldPosTmZ.z, toWorldPosTmW.z);

  static int toUnitBoxTmXVarId = get_shader_variable_id("to_unit_box_tm_x", true);
  static int toUnitBoxTmYVarId = get_shader_variable_id("to_unit_box_tm_y", true);
  static int toUnitBoxTmZVarId = get_shader_variable_id("to_unit_box_tm_z", true);

  ShaderGlobal::set_color4(toUnitBoxTmXVarId, toUnitBoxTmX.x, toUnitBoxTmY.x, toUnitBoxTmZ.x, toUnitBoxTmW.x);
  ShaderGlobal::set_color4(toUnitBoxTmYVarId, toUnitBoxTmX.y, toUnitBoxTmY.y, toUnitBoxTmZ.y, toUnitBoxTmW.y);
  ShaderGlobal::set_color4(toUnitBoxTmZVarId, toUnitBoxTmX.z, toUnitBoxTmY.z, toUnitBoxTmZ.z, toUnitBoxTmW.z);

  static const int heroDrySpeedVarId = get_shader_variable_id("hero_dry_speed", true);
  static const int heroWetnessMaxDepthVarId = get_shader_variable_id("hero_wetness_max_depth", true);
  static const int heroWetnessDepthBiasVarId = get_shader_variable_id("hero_wetness_depth_bias", true);

  ShaderGlobal::set_color4(heroDrySpeedVarId, wetPar.drySpeed.x * dt, wetPar.drySpeed.y * dt, clearInShader,
    has_water_3d ? 1.0f : 0.0f);
  ShaderGlobal::set_real(heroWetnessMaxDepthVarId, wetPar.maxDepth);
  ShaderGlobal::set_real(heroWetnessDepthBiasVarId, wetPar.depthBias);

  // fill wetness volume (write water height at point)
  // save states
  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);


  // render flat-3d texture
  int curTex = heroWetnessRenderIter;
  heroWetnessTex[curTex]->texfilter(TEXFILTER_POINT);

  ShaderGlobal::set_texture(heroWetnessTexVarId, heroWetnessTexId[heroWetnessRenderIter]);
  heroWetnessRenderIter = heroWetnessRenderIter ^ 1; // 0  or 1
  d3d::set_render_target(heroWetnessTex[heroWetnessRenderIter], 0);

  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  if (!waterHeightRendererShElem)
    return;

  if (waterHeightRendererShElem->setStates(0, true))
  {
    d3d::setvsrc_ex(0, waterHeightRendererVb, 0, sizeof(float) * 4);
    d3d::draw(PRIM_TRILIST, 0, 2 * heroWetnessVolumeSlices);
  }

  d3d::resource_barrier({heroWetnessTex[heroWetnessRenderIter], RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  // restore states
  d3d::set_render_target(prevRt);
  heroWetnessTex[curTex]->texfilter(TEXFILTER_SMOOTH);
  ShaderGlobal::set_texture(heroWetnessTexVarId, heroWetnessTexId[heroWetnessRenderIter]);
}


void HeroWetness::initHeroWaterFoam(const DataBlock *options_blk)
{
  heroFoamScroll = Point2(0.f, 0.f);
  heroFoamScrollSpeed = options_blk->getReal("scrollSpeed", 0.5f);
  foamTexScale = options_blk->getPoint2("texScale", Point2(0.1, 0.55));
  foamTextureAlphaScaleOffset = options_blk->getPoint2("textureAlphaScaleOffset", Point2(1.5, -0.5));
  heroFoamFadeUnderWater = options_blk->getPoint2("fadeUnderWater", Point2(2.0, 1.0)); // level under water & fade speed
  heroFoamFadeUnderWater.x *= heroFoamFadeUnderWater.y;

  bool fEnabled = options_blk->getBool("enabled", true);
  if (!fEnabled)
    foamTextureAlphaScaleOffset = Point2(0.f, 0.f); // if disabled - just fade texture

  static int heroFoamTextureTransformVarId = get_shader_variable_id("hero_foam_texture_transform", true);
  ShaderGlobal::set_color4(heroFoamTextureTransformVarId, foamTexScale.x, foamTexScale.y, foamTextureAlphaScaleOffset.x,
    foamTextureAlphaScaleOffset.y);
}

void HeroWetness::updateHeroWaterFoam(float dt, const TMatrix &hero_tm, bool is_ship)
{
  Point3 forwardVector = hero_tm.getcol(0);
  Point3 sideVector = hero_tm.getcol(2);
  heroFoamScroll.x += forwardVector.y * heroFoamScrollSpeed * dt;
  heroFoamScroll.y += sideVector.y * heroFoamScrollSpeed * dt;

  static int shipFoamScrollFadeVarId = get_shader_variable_id("hero_foam_scroll_fade", true);
  if (is_ship)
    ShaderGlobal::set_color4(shipFoamScrollFadeVarId, heroFoamScroll.x, heroFoamScroll.y, heroFoamFadeUnderWater.y,
      heroFoamFadeUnderWater.x);
  else
    ShaderGlobal::set_color4(shipFoamScrollFadeVarId, 0, 0, 0, 0);
}

bool HeroWetness::isInSleep() const { return wetTime == -1.0f; }
