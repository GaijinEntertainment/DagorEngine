#include <render/decals/dynamicDecals.h>
#include <memory/dag_framemem.h>
#include <render/atlasTexManager.h>
#include <3d/dag_tex3d.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/vector_map.h>
#include <shaders/dag_shaders.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <3d/dag_texMgr.h>
#include <util/dag_console.h>

// for tweaking (needs DYNAMIC_DECALS_TWEAKING define in dynamic_decals_params.hlsli )
// needs shader/exe recompile
#include <math/dag_hlsl_floatx.h>
#include <render/decals/dynamic_decals_params.hlsli>

namespace dyn_decals
{

struct ShaderParams
{
  float cuttingPlaneWingOrMode;
  float capsuleHolesCount;
  float sphereHolesCount;
  float burnMarksCount;
  float diffMarksCount;
  float splashMarksCount;
  float bulletMarkCount;
  float unused;
  Point4 cuttingPlane[11];
};

struct AtlasElem
{
  Point4 uv;
  bool pendingUpdate;
  int refCount;
};

void init_atlas();
void close_atlas();
void write_texture_to_atlas(const TextureIDPair &tex, Point4 &out_uv, bool &out_space_error, bool &out_load_error);

static atlas_tex_manager::TexRec atlas;
static eastl::vector_map<TEXTUREID, AtlasElem> atlas_elements;

const int decals_size_params = 4 * 2 * 3 + 4 + 1;

#define PARAM_COUNT                                                                                                    \
  ((sizeof(ShaderParams) + decals.capsules.size() * sizeof(CapsulePrim) + decals.spheres.size() * sizeof(SpherePrim) + \
     decals.burnMarks.size() * sizeof(BurnMarkPrim)) /                                                                 \
    sizeof(Point4))

static const int ATLAS_ELEM_MAX_NUM = 256;
static const carray<IPoint2, 4> ATLAS_SIZES = {IPoint2(1024, 1024), IPoint2(2048, 3072), IPoint2(4096, 4096), IPoint2(8192, 8192)};
static int current_atlas_size = 0;
static bool dynDecalsUseUncompressed = false;

#if DAGOR_DBGLEVEL > 0
static TEXTUREID loadErrorTexId = BAD_TEXTUREID;
#endif
static int dynDecalsNoiseTexVarId = -1;
static TEXTUREID dynDecalsNoiseTexId = BAD_TEXTUREID;

#ifdef DYNAMIC_DECALS_TWEAKING
static int dynDecalsSteelHoleNoiseVarId = -1;
static int dynDecalsWoodHoleNoiseVarId = -1;
static int dynDecalsFabricHoleNoiseVarId = -1;
static int dynDecalsSteelWreckageNoiseVarId = -1;
static int dynDecalsWoodWreckageNoiseVarId = -1;
static int dynDecalsFabricWreckageNoiseVarId = -1;
static int dynDecalsHoleBurnMarkNoiseVarId = -1;
static int dynDecalsBurnMarkColorVarId = -1;

static int dynDecalsBurnMarkParamsId = -1;
static int dynDecalsDiffMarkParamsId = -1;

static int dyn_decals_paramsId = -1;
static int dyn_decals_params2Id = -1;

static Point4 steelHoleNoise = dyn_decals_steel_hole_noise;
static Point4 woodHoleNoise = dyn_decals_wood_hole_noise;
static Point4 fabricHoleNoise = dyn_decals_fabrics_hole_noise;

static Point4 steelWreackageNoise = dyn_decals_steel_wreackage_noise;
static Point4 woodWreackageNoise = dyn_decals_wood_wreackage_noise;
static Point4 fabricWreackageNoise = dyn_decals_fabric_wreackage_noise;

static Point4 holeBurnMarkNoise = dyn_decals_holes_burn_mark_noise;
static Point4 burnMarkColor = dyn_decals_burn_color;

static Point4 burnMarkParams = dyn_decals_burn_mark_params;
static Point4 diffMarkParams = dyn_decals_diff_mark_params;

static float rimScale = dyn_decals_rimScale;
static float colorDistMul = dyn_decals_colorDistMul;
static float noiseUvMul = dyn_decals_noiseUvMul;
static float noiseScale = dyn_decals_noiseScale;

static float rimDistMul = dyn_decals_rimDistMul;
static float rimDistMul2 = dyn_decals_rimDistMul2;
#endif


DecalsData::DecalsData(const DecalsData &from)
{
  memcpy(this, &from, sizeof(DecalsData));
  for (TextureIDPair &tex : decalTextures)
    if (tex.getId() != BAD_TEXTUREID)
      acquire_managed_tex(tex.getId());
}

DecalsData::~DecalsData()
{
  for (TextureIDPair &tex : decalTextures)
    if (tex.getId() != BAD_TEXTUREID)
      release_managed_tex(tex.getId());
}

void DecalsData::setTexture(int slot_no, const TextureIDPair &tex)
{
  if (decalTextures[slot_no].getId() != BAD_TEXTUREID)
    release_managed_tex(decalTextures[slot_no].getId());
  decalTextures[slot_no] = tex;
  if (tex.getId() != BAD_TEXTUREID)
    acquire_managed_tex(tex.getId());
}


void init(const DataBlock &blk)
{
#if DAGOR_DBGLEVEL > 0
  const char *loadErrorTexName = blk.getStr("loadErrorTex", "load_error");
  loadErrorTexId = get_tex_gameres(loadErrorTexName);
  G_ASSERTF(loadErrorTexId != BAD_TEXTUREID, "missing tex <%s>", loadErrorTexName);
#endif

  dynDecalsNoiseTexVarId = get_shader_variable_id("dyn_decals_noise");
  const char *dynDecalsNoiseName = blk.getStr("noiseTex", "dyn_decals_noise");
  dynDecalsNoiseTexId = ::get_tex_gameres(dynDecalsNoiseName);
  G_ASSERTF(dynDecalsNoiseTexId != BAD_TEXTUREID, "missing tex <%s>", dynDecalsNoiseName);
  ShaderGlobal::set_texture(dynDecalsNoiseTexVarId, dynDecalsNoiseTexId);

  dynDecalsUseUncompressed = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("dynDecalsUseUncompressed", false);

#ifdef DYNAMIC_DECALS_TWEAKING
  rimScale = blk.getReal("rimScale", 1.0f);
  colorDistMul = blk.getReal("colorDistMul", 0.4f);
  noiseUvMul = blk.getReal("noiseUvMul", 20.0f);
  noiseScale = blk.getReal("noiseScale", 0.2f);
  dyn_decals_paramsId = get_shader_variable_id("dyn_decals_params");

  ShaderGlobal::set_color4(get_shader_variable_id("dyn_decals_cutting_color"),
    Color4::xyz1(blk.getPoint3("cutting_color", Point3(0.4f, 0.4f, 0.4f))));

  ShaderGlobal::set_real(get_shader_variable_id("dyn_decals_smoothness"), blk.getReal("smoothness", 0.3f));
  ShaderGlobal::set_real(get_shader_variable_id("dyn_decals_metallness"), blk.getReal("metallness", 0.6f));

  dynDecalsSteelHoleNoiseVarId = get_shader_variable_id("dyn_decals_steel_hole_noise", true);
  dynDecalsWoodHoleNoiseVarId = get_shader_variable_id("dyn_decals_wood_hole_noise", true);
  dynDecalsFabricHoleNoiseVarId = get_shader_variable_id("dyn_decals_fabric_hole_noise", true);
  dynDecalsSteelWreckageNoiseVarId = get_shader_variable_id("dyn_decals_steel_wreackage_noise", true);
  dynDecalsWoodWreckageNoiseVarId = get_shader_variable_id("dyn_decals_wood_wreackage_noise", true);
  dynDecalsFabricWreckageNoiseVarId = get_shader_variable_id("dyn_decals_fabric_wreackage_noise", true);
  dynDecalsHoleBurnMarkNoiseVarId = get_shader_variable_id("dyn_decals_holes_burn_mark_noise", true);
  dynDecalsBurnMarkColorVarId = get_shader_variable_id("dyn_decals_burn_color", true);
  dynDecalsBulletMarkNoiseVarId = get_shader_variable_id("dyn_decals_bullet_mark_noise", true);
  dynDecalsBurnMarkParamsId = get_shader_variable_id("dyn_decals_burn_mark_params", true);
  dynDecalsDiffMarkParamsId = get_shader_variable_id("dyn_decals_diff_mark_params", true);
  dynDecalsBulletDiffMarkParamsId = get_shader_variable_id("dyn_decals_bullet_diff_mark_params", true);

  dyn_decals_params2Id = get_shader_variable_id("dyn_decals_params2");
  rimDistMul = blk.getReal("rimDistMul", 3.0f);
  rimDistMul2 = blk.getReal("rimDistMul2", 10.0f);

  ShaderGlobal::set_color4(dynDecalsSteelHoleNoiseVarId, steelHoleNoise);
  ShaderGlobal::set_color4(dynDecalsWoodHoleNoiseVarId, woodHoleNoise);
  ShaderGlobal::set_color4(dynDecalsFabricHoleNoiseVarId, fabricHoleNoise);
  ShaderGlobal::set_color4(dynDecalsSteelWreckageNoiseVarId, steelWreackageNoise);
  ShaderGlobal::set_color4(dynDecalsWoodWreckageNoiseVarId, woodWreackageNoise);
  ShaderGlobal::set_color4(dynDecalsFabricWreckageNoiseVarId, fabricWreackageNoise);
  ShaderGlobal::set_color4(dynDecalsHoleBurnMarkNoiseVarId, holeBurnMarkNoise);
  ShaderGlobal::set_color4(dynDecalsBurnMarkColorVarId, burnMarkColor);
  ShaderGlobal::set_color4(dynDecalsBulletMarkNoiseVarId, bulletMarkNoise);

  ShaderGlobal::set_color4(dynDecalsBurnMarkParamsId, burnMarkParams);
  ShaderGlobal::set_color4(dynDecalsDiffMarkParamsId, diffMarkParams);
  ShaderGlobal::set_color4(dynDecalsBulletDiffMarkParamsId, bulletDiffMarkParams);

  ShaderGlobal::set_color4(dyn_decals_paramsId, Color4(rimScale, colorDistMul, noiseUvMul, noiseScale));
  ShaderGlobal::set_color4(dyn_decals_params2Id, Color4(rimDistMul, rimDistMul2, burnMarkParams.x, diffMarkParams.x));
#endif
}

void close()
{
#if DAGOR_DBGLEVEL > 0
  release_managed_tex(loadErrorTexId);
  loadErrorTexId = BAD_TEXTUREID;
#endif

  ShaderGlobal::set_texture(dynDecalsNoiseTexVarId, BAD_TEXTUREID);
  release_managed_tex(dynDecalsNoiseTexId);
  dynDecalsNoiseTexId = BAD_TEXTUREID;

  close_atlas();
}

void add_to_atlas(TEXTUREID texId)
{
  if (texId == BAD_TEXTUREID)
    return;
  auto iter = atlas_elements.find(texId);
  if (iter != atlas_elements.end())
  {
    ++iter->second.refCount;
    return;
  }
  AtlasElem &elem = atlas_elements[texId];
  elem.pendingUpdate = true;
  elem.uv = Point4(0, 0, 0, 0);
  elem.refCount = 1;
}

void add_to_atlas(dag::ConstSpan<TextureIDPair> textures)
{
  for (const auto &item : textures)
    add_to_atlas(item.getId());
}

void remove_from_atlas(TEXTUREID texId)
{
  if (texId == BAD_TEXTUREID)
    return;
  const auto iter = atlas_elements.find(texId);
  if (iter == atlas_elements.end())
    return;
  iter->second.refCount = max(iter->second.refCount - 1, 0);
}

void remove_from_atlas(dag::ConstSpan<TextureIDPair> textures)
{
  for (const auto &item : textures)
    remove_from_atlas(item.getId());
}

static bool shrink_atlas()
{
  auto iter = atlas_elements.begin();
  int numRemoved = 0;
  while (iter != atlas_elements.end())
  {
    if (iter->second.refCount == 0)
    {
      iter = atlas_elements.erase(iter);
      ++numRemoved;
    }
    else
      ++iter;
  }

  if (numRemoved == 0)
    return false;

  for (auto &elem : atlas_elements)
    elem.second.pendingUpdate = true;
  atlas.resetAtlas();
  return true;
}

static bool enlarge_atlas()
{
  if (current_atlas_size + 1 < ATLAS_SIZES.size())
  {
    ++current_atlas_size;
    auto iter = atlas_elements.begin();
    while (iter != atlas_elements.end())
    {
      if (iter->second.refCount == 0)
        iter = atlas_elements.erase(iter);
      else
      {
        iter->second.pendingUpdate = true;
        ++iter;
      }
    }
    atlas.resetAtlas();
    return true;
  }
  return false;
}

void clear_atlas() { close_atlas(); }

Point4 get_atlas_uv(TEXTUREID texId)
{
  auto iter = texId != BAD_TEXTUREID ? atlas_elements.find(texId) : atlas_elements.end();
  return iter != atlas_elements.end() ? iter->second.uv : Point4(1, 1, 0, 0);
}

bool is_pending_update(TEXTUREID texId)
{
  auto iter = texId != BAD_TEXTUREID ? atlas_elements.find(texId) : atlas_elements.end();
  return iter != atlas_elements.end() ? iter->second.pendingUpdate : false;
}

void add_decals_to_params(int start_params, dynrend::PerInstanceRenderData &data, const dyn_decals::DecalsData &decals)
{
  data.params.resize(start_params + decals_size_params);
  Point4 encodedWidhtAndMultipler(decals.widthBox, Point4::CTOR_FROM_PTR);
  for (int i = 0; i < dyn_decals::num_decals; ++i)
  {
    encodedWidhtAndMultipler[i] = ceil(encodedWidhtAndMultipler[i] * 1024);
    encodedWidhtAndMultipler[i] += 0.5f * decals.absDot[i];
    encodedWidhtAndMultipler[i] += 0.25f * decals.oppositeMirrored[i];
    encodedWidhtAndMultipler[i] += 0.125f * (decals.getTexture(i).getTex() != nullptr);
  }
  data.params[start_params] = encodedWidhtAndMultipler;
  for (int i = 0; i < dyn_decals::num_decals * 4; i++)
    data.params[start_params + 1 + i] = decals.decalLines[i];
  for (int i = 0; i < dyn_decals::num_decals * 2; i++)
    data.params[start_params + 1 + dyn_decals::num_decals * 4 + i] = decals.contactNormal[i];
  for (int i = 0; i < dyn_decals::num_decals; ++i)
  {
    data.params[start_params + 1 + dyn_decals::num_decals * 4 + dyn_decals::num_decals * 2 + i] =
      get_atlas_uv(decals.getTexture(i).getId());
  }
}

static unsigned int pack_value(float value, unsigned int precision, float min, float max)
{
  int endValue = roundf((value - min) / (max - min) * precision);
  return endValue;
}

void set_dyn_decals(const DynDecals &decals, Point4 *out_params)
{
  ShaderParams *shaderParams = reinterpret_cast<ShaderParams *>(&out_params[0]);
  shaderParams->cuttingPlaneWingOrMode = decals.cuttingPlaneWingOrMode + 0.5f;
  shaderParams->capsuleHolesCount = decals.capsules.size() + 0.5f;
  shaderParams->sphereHolesCount = decals.spheres.size() + 0.5f;
  shaderParams->burnMarksCount = decals.burnMarks.size() + 0.5f;

  int offset = 0;

  for (int i = 0; i < 7; ++i)
    shaderParams->cuttingPlane[i] = Point4(0.0f, 0.0f, 0.0f, 1.0f);
  for (int i = 7; i < 11; ++i)
    shaderParams->cuttingPlane[i] = Point4(0.0f, 0.0f, 0.0f, -1.0f);
  G_ASSERT(decals.planes.size() <= 11);
  for (int i = 0; i < min(decals.planes.size(), 11u); ++i)
    shaderParams->cuttingPlane[i] = decals.planes[i].plane;
  offset += sizeof(ShaderParams) / sizeof(Point4);

  float precision = (float)0x1fffffff;
  float maxValue = 100;
  float minValue = -100;
  for (int i = 0; i < min(decals.planes.size(), 11u); ++i)
  {
    float value = clamp((float)shaderParams->cuttingPlane[i].w, minValue, maxValue);
    *((uint32_t *)&shaderParams->cuttingPlane[i].w) = (decals.planes[i].material) << 29u;
    *((uint32_t *)&shaderParams->cuttingPlane[i].w) += pack_value(value, precision, minValue, maxValue);
  }

  CapsulePrim *capsuleHoles = decals.capsules.size() > 0 ? reinterpret_cast<CapsulePrim *>(&out_params[offset]) : NULL;
  if (capsuleHoles)
    memcpy(capsuleHoles, decals.capsules.data(), decals.capsules.size() * sizeof(CapsulePrim));
  offset += decals.capsules.size() * sizeof(CapsulePrim) / sizeof(Point4);

  SpherePrim *sphereHoles = decals.spheres.size() > 0 ? reinterpret_cast<SpherePrim *>(&out_params[offset]) : NULL;
  if (sphereHoles)
    memcpy(sphereHoles, decals.spheres.data(), decals.spheres.size() * sizeof(SpherePrim));
  offset += decals.spheres.size() * sizeof(SpherePrim) / sizeof(Point4);

  BurnMarkPrim *burnMarksParams = decals.burnMarks.size() > 0 ? reinterpret_cast<BurnMarkPrim *>(&out_params[offset]) : NULL;
  if (burnMarksParams)
    memcpy(burnMarksParams, decals.burnMarks.data(), decals.burnMarks.size() * sizeof(BurnMarkPrim));

  ShaderGlobal::set_texture(dynDecalsNoiseTexVarId, dynDecalsNoiseTexId);
}

void set_dyn_decals(const DynDecals &decals, Tab<Point4> &out_params)
{
  out_params.resize(PARAM_COUNT);
  set_dyn_decals(decals, out_params.data());
}

void set_dyn_decals(const DynDecals &decals, int start_params, dynrend::PerInstanceRenderData &render_data)
{
  render_data.params.resize(start_params + PARAM_COUNT);
  set_dyn_decals(decals, render_data.params.data() + start_params);
}

bool update()
{
  bool wasEnlarged = false;
  for (auto &elem : atlas_elements)
  {
    if (!elem.second.pendingUpdate || !prefetch_and_check_managed_texture_loaded(elem.first, true))
      continue;
    elem.second.pendingUpdate = false;
    bool spaceError = false, loadError = false;
    write_texture_to_atlas(TextureIDPair(NULL, elem.first), elem.second.uv, spaceError, loadError);
#if DAGOR_DBGLEVEL > 0
    if (loadError && loadErrorTexId != BAD_TEXTUREID)
    {
      if (prefetch_and_check_managed_texture_loaded(loadErrorTexId))
        write_texture_to_atlas(TextureIDPair(NULL, loadErrorTexId), elem.second.uv, spaceError, loadError);
      else
        elem.second.pendingUpdate = true;
    }
#endif
    if (spaceError)
    {
      wasEnlarged = true;
      if (shrink_atlas())
        break;
      else if (enlarge_atlas())
      {
        debug("Atlas is wasted. Reallocated a new one with sizes %dx%d", ATLAS_SIZES[current_atlas_size].x,
          ATLAS_SIZES[current_atlas_size].y);
        break;
      }
      else
        logwarn("Can't use a decal texture %s. Not enough space in the atlas", get_managed_texture_name(elem.first));
    }
  }
  return wasEnlarged;
}

unsigned int get_uncompressed_tex_fmt() { return TEXFMT_A8R8G8B8; }

void init_atlas()
{
  if (atlas.isInited())
    return;
  unsigned texfmt = TEXFMT_DXT5;
  if (dynDecalsUseUncompressed)
  {
    texfmt = get_uncompressed_tex_fmt();
  }
  texfmt |= TEXCF_UPDATE_DESTINATION | TEXCF_SRGBREAD;
  atlas.initAtlas(ATLAS_ELEM_MAX_NUM, dag::ConstSpan<unsigned>(&texfmt, 1), "dynamic_decals_atlas", ATLAS_SIZES[current_atlas_size],
    8);
  ShaderGlobal::set_texture(get_shader_variable_id("dynamic_decals_atlas", true), atlas.atlasTex[0].getTexId());
  atlas.atlasTex[0].getTex2D()->setAnisotropy(::dgs_tex_anisotropy);
}

void close_atlas()
{
  current_atlas_size = 0;
  atlas_elements.clear();
  atlas.resetAtlas();
}

void write_texture_to_atlas(const TextureIDPair &tex, Point4 &out_uv, bool &out_space_error, bool &out_load_error)
{
  out_space_error = false;
  out_load_error = false;
  out_uv = Point4(0, 0, 0, 0);

  Texture *texture = tex.getTex2D();
  if (!texture)
    texture = (Texture *)acquire_managed_tex(tex.getId());
  G_ASSERT(texture);
  if (!texture)
  {
    out_load_error = true;
    return;
  }

  TextureInfo info;
  texture->getinfo(info);
  unsigned int expectedAtlasTexFmt = TEXFMT_DXT5;
  if (dynDecalsUseUncompressed)
  {
    expectedAtlasTexFmt = get_uncompressed_tex_fmt();
  }
  if ((info.cflg & TEXFMT_MASK) == expectedAtlasTexFmt)
  {
    init_atlas();
    int atlasIndex = atlas.addTextureToAtlas(info, out_uv, false);
    if (atlasIndex != -1)
      out_load_error = !atlas.writeTextureToAtlas(texture, atlasIndex, 0);
    else
      out_space_error = true;
  }
  else
  {
    out_load_error = true;
    logerr("Unexpected texture format for decal %s (0x%08X->0x%08X)", texture->getTexName(), info.cflg & TEXFMT_MASK,
      expectedAtlasTexFmt);
  }

  if (out_load_error)
    logerr("Failed load decal %s", texture->getTexName());

  if (!tex.getTex2D())
    release_managed_tex(tex.getId());
}

#ifdef DYNAMIC_DECALS_TWEAKING
void set_dyn_decals_steel_hole_noise(const Point4 &noise)
{
  steelHoleNoise = noise;
  ShaderGlobal::set_color4(dynDecalsSteelHoleNoiseVarId, steelHoleNoise);
}

void set_dyn_decals_wood_hole_noise(const Point4 &noise)
{
  woodHoleNoise = noise;
  ShaderGlobal::set_color4(dynDecalsWoodHoleNoiseVarId, woodHoleNoise);
}

void set_dyn_decals_fabric_hole_noise(const Point4 &noise)
{
  fabricHoleNoise = noise;
  ShaderGlobal::set_color4(dynDecalsFabricHoleNoiseVarId, fabricHoleNoise);
}

void set_dyn_decals_steel_wrecking_noise(const Point4 &noise)
{
  steelWreackageNoise = noise;
  ShaderGlobal::set_color4(dynDecalsSteelWreckageNoiseVarId, steelWreackageNoise);
}

void set_dyn_decals_wood_wrecking_noise(const Point4 &noise)
{
  woodWreackageNoise = noise;
  ShaderGlobal::set_color4(dynDecalsWoodWreckageNoiseVarId, woodWreackageNoise);
}

void set_dyn_decals_fabric_wrecking_noise(const Point4 &noise)
{
  fabricWreackageNoise = noise;
  ShaderGlobal::set_color4(dynDecalsFabricWreckageNoiseVarId, fabricWreackageNoise);
}

void set_dyn_decals_hole_burn_mark_noise(const Point4 &noise)
{
  holeBurnMarkNoise = noise;
  ShaderGlobal::set_color4(dynDecalsHoleBurnMarkNoiseVarId, holeBurnMarkNoise);
}

void set_dyn_decals_burn_mark_color(const Point4 &color)
{
  burnMarkColor = color;
  ShaderGlobal::set_color4(dynDecalsBurnMarkColorVarId, burnMarkColor);
}

void set_dyn_decals_bullet_mark_noise(const Point4 &noise)
{
  bulletMarkNoise = noise;
  ShaderGlobal::set_color4(dynDecalsBulletMarkNoiseVarId, bulletMarkNoise);
}

void set_dyn_decals_burn_mark_params(const Point4 &params)
{
  burnMarkParams = params;
  ShaderGlobal::set_color4(dynDecalsBurnMarkParamsId, burnMarkParams);
}

void set_dyn_decals_diff_mark_params(const Point4 &params)
{
  diffMarkParams = params;
  ShaderGlobal::set_color4(dynDecalsDiffMarkParamsId, diffMarkParams);
}

void set_dyn_decals_bullet_diff_mark_params(const Point4 &params)
{
  bulletDiffMarkParams = params;
  ShaderGlobal::set_color4(dynDecalsBulletDiffMarkParamsId, bulletDiffMarkParams);
}

void set_dyn_decals_rim_scale(float scale)
{
  rimScale = scale;
  ShaderGlobal::set_color4(dyn_decals_paramsId, Color4(rimScale, colorDistMul, noiseUvMul, noiseScale));
}

void set_dyn_decals_color_dist_mult(float mult)
{
  colorDistMul = mult;
  ShaderGlobal::set_color4(dyn_decals_paramsId, Color4(rimScale, colorDistMul, noiseUvMul, noiseScale));
}

void set_dyn_decals_noise_uv_mul(float mult)
{
  noiseUvMul = mult;
  ShaderGlobal::set_color4(dyn_decals_paramsId, Color4(rimScale, colorDistMul, noiseUvMul, noiseScale));
}

void set_dyn_decals_noise_scale(float scale)
{
  noiseScale = scale;
  ShaderGlobal::set_color4(dyn_decals_paramsId, Color4(rimScale, colorDistMul, noiseUvMul, noiseScale));
}

void set_dyn_decals_rim_dist_mul(float mult)
{
  rimDistMul = mult;
  ShaderGlobal::set_color4(dyn_decals_params2Id, Color4(rimDistMul, rimDistMul2, burnMarkParams.x, diffMarkParams.x));
}

void set_dyn_decals_rim_dist_mul2(float mult)
{
  rimDistMul2 = mult;
  ShaderGlobal::set_color4(dyn_decals_params2Id, Color4(rimDistMul, rimDistMul2, burnMarkParams.x, diffMarkParams.x));
}

bool dyn_decals_console(int found, const char *argv[], int argc)
{
  CONSOLE_CHECK_NAME("fm", "steelHoleNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_steel_hole_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "woodHoleNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_wood_hole_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "fabricHoleNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_fabric_hole_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "steelWreackingNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_steel_wrecking_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "woodWreackingNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_wood_wrecking_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "fabricWreackingNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_fabric_wrecking_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "holeBurnMarkNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_hole_burn_mark_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "burnMarkColor", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 color = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_burn_mark_color(color);
    }
  }
  CONSOLE_CHECK_NAME("fm", "bulletMarkNoise", 5, 5)
  {
    if (argc < 5)
      console::print_d("need 4 parameters");
    else if (argc == 5)
    {
      Point4 noise = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));
      dyn_decals::set_dyn_decals_bullet_mark_noise(noise);
    }
  }
  CONSOLE_CHECK_NAME("fm", "diffMarkParams", 4, 4)
  {
    if (argc < 4)
      console::print_d("need 3 parameters [mult/min/max]");
    else if (argc == 4)
    {
      Point4 params = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), 0.0);
      dyn_decals::set_dyn_decals_diff_mark_params(params);
    }
  }
  CONSOLE_CHECK_NAME("fm", "burnMarkParams", 4, 4)
  {
    if (argc < 4)
      console::print_d("need 3 parameters [mult/min/max]");
    else if (argc == 4)
    {
      Point4 params = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), 0.0);
      dyn_decals::set_dyn_decals_burn_mark_params(params);
    }
  }
  CONSOLE_CHECK_NAME("fm", "bulletDiffMarkParams", 4, 4)
  {
    if (argc < 4)
      console::print_d("need 3 parameters [mult/min/max]");
    else if (argc == 4)
    {
      Point4 params = Point4(atof(argv[1]), atof(argv[2]), atof(argv[3]), 0.0);
      dyn_decals::set_dyn_decals_bullet_diff_mark_params(params);
    }
  }
  CONSOLE_CHECK_NAME("fm", "holeRimScale", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_rim_scale(atof(argv[1]));
  }
  CONSOLE_CHECK_NAME("fm", "holeNoiseUvMul", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_noise_uv_mul(atof(argv[1]));
  }
  CONSOLE_CHECK_NAME("fm", "holeNoiseScale", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_noise_scale(atof(argv[1]));
  }
  CONSOLE_CHECK_NAME("fm", "holeColorDistMul", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_color_dist_mult(atof(argv[1]));
  }
  CONSOLE_CHECK_NAME("fm", "holeRimDist", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_rim_dist_mul(atof(argv[1]));
  }
  CONSOLE_CHECK_NAME("fm", "holeRimDist2", 2, 2)
  {
    if (argc < 2)
      console::print_d("need 1 parameters");
    else if (argc == 2)
      dyn_decals::set_dyn_decals_rim_dist_mul2(atof(argv[1]));
  }
  return false;
}

#else
bool dyn_decals_console(int found, const char *argv[], int argc)
{
  G_UNUSED(argv);
  G_UNUSED(argc);
  G_UNUSED(found);
  return false;
}
#endif

} // namespace dyn_decals
