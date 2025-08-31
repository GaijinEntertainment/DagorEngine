// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/decals/dynamicDecals.h>
#include <memory/dag_framemem.h>
#include <render/atlasTexManager.h>
#include <drv/3d/dag_tex3d.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/vector_map.h>
#include <shaders/dag_shaders.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <3d/dag_texMgr.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_critSec.h>
#include <3d/dag_gpuConfig.h>

// for tweaking (needs DYNAMIC_DECALS_TWEAKING define in dynamic_decals_params.hlsli )
// needs shader/exe recompile
#include <math/dag_hlsl_floatx.h>
#include <render/decals/dynamic_decals_params.hlsli>

static ShaderVariableInfo planar_decal_countVarId("planar_decal_count", true);

namespace dyn_decals
{

struct ShaderParams
{
  float cuttingPlaneWingOrMode;
  float capsuleHolesCount;
  float sphereHolesCount;
  float burnMarksCount;
  Point4 cuttingPlane[11];
};

struct AtlasElem
{
  Point4 uv;
  bool pendingUpdate;
  int refCount;
};

class IdManager
{
  int32_t lastAllocatedId = -1;
  dag::Vector<int32_t> freeIds;

public:
  int32_t allocate()
  {
    int32_t id;
    if (freeIds.size())
    {
      id = freeIds.back();
      freeIds.pop_back();
    }
    else
    {
      id = ++lastAllocatedId;
    }
    return id;
  }

  void free(int32_t id)
  {
    G_ASSERT(id <= lastAllocatedId);
    if (id == lastAllocatedId)
      lastAllocatedId--;
    else
      freeIds.push_back(id);
  }

  void clear()
  {
    lastAllocatedId = -1;
    freeIds.clear();
  }
};

static void init_atlas(unsigned tex_fmt);
static void close_atlas();

enum class WriteTextureToAtlasResult
{
  Ok,
  NotEnoughSpace,
  LoadError
};

static WriteTextureToAtlasResult write_texture_to_atlas(TEXTUREID tex_id, Point4 &out_uv);

static atlas_tex_manager::TexRec atlas;
static eastl::vector_map<TEXTUREID, AtlasElem> atlas_elements;
static WinCritSec atlasMutex;

#define PARAM_COUNT                                                                                                    \
  ((sizeof(ShaderParams) + decals.capsules.size() * sizeof(CapsulePrim) + decals.spheres.size() * sizeof(SpherePrim) + \
     decals.burnMarks.size() * sizeof(BurnMarkPrim)) /                                                                 \
    sizeof(Point4))

static const int ATLAS_ELEM_MAX_NUM = 256;
static const carray<IPoint2, 4> ATLAS_SIZES = {IPoint2(1024, 1024), IPoint2(2048, 3072), IPoint2(4096, 4096), IPoint2(8192, 8192)};
static int current_atlas_size = 0;
static int initial_atlas_size_index = 0;

#if DAGOR_DBGLEVEL > 0
static TEXTUREID loadErrorTexId = BAD_TEXTUREID;
#endif
static int dynDecalsNoiseTexVarId = -1;
static TEXTUREID dynDecalsNoiseTexId = BAD_TEXTUREID;

// We can also include planar_decals_params.hlsli into dynamicDecals.h and use MAX_PLANAR_DECALS everywhere.
static_assert(num_decals == MAX_PLANAR_DECALS);
static dag::Vector<UniqueBuf> planarDecalsShaderParamsBufs; // We allocate multiple small cbuffers.
static dag::Vector<bool> planarDecalsShaderParamsNeedUpdate;
static dag::Vector<int> planarDecalCounts;
static bool useDynamicConstBuf = false;
static IdManager planarDecalsShaderParamsSetIds;
static WinCritSec planarDecalsMutex;

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

bool DecalsData::operator==(const DecalsData &other) const
{
  bool equal = true;
  equal &= memcmp(decalLines, other.decalLines, sizeof(decalLines)) == 0;          //-V1014
  equal &= memcmp(contactNormal, other.contactNormal, sizeof(contactNormal)) == 0; //-V1014
  equal &= memcmp(widthBox, other.widthBox, sizeof(widthBox)) == 0;
  equal &= memcmp(wrap, other.wrap, sizeof(wrap)) == 0;
  equal &= memcmp(absDot, other.absDot, sizeof(absDot)) == 0;
  equal &= memcmp(oppositeMirrored, other.oppositeMirrored, sizeof(oppositeMirrored)) == 0;
  for (int i = 0; i < num_decals; ++i)
  {
    equal &= decalTextures[i].getId() == other.decalTextures[i].getId();
    equal &= decalTextures[i].getTex() == other.decalTextures[i].getTex();
  }
  return equal;
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
  ShaderGlobal::set_sampler(get_shader_variable_id("dyn_decals_noise_samplerstate", true),
    get_texture_separate_sampler(dynDecalsNoiseTexId));

  initial_atlas_size_index = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("dynDecalsInitialAtlasSizeIndex", 0);
  G_ASSERT(initial_atlas_size_index < ATLAS_SIZES.size());
  current_atlas_size = initial_atlas_size_index;

  useDynamicConstBuf = d3d_get_gpu_cfg().forceDx10 || d3d_get_gpu_cfg().hardwareDx10;

#ifdef DYNAMIC_DECALS_TWEAKING
  rimScale = blk.getReal("rimScale", 1.0f);
  colorDistMul = blk.getReal("colorDistMul", 0.4f);
  noiseUvMul = blk.getReal("noiseUvMul", 20.0f);
  noiseScale = blk.getReal("noiseScale", 0.2f);
  dyn_decals_paramsId = get_shader_variable_id("dyn_decals_params");

  ShaderGlobal::set_color4(get_shader_variable_id("dyn_decals_cutting_color"),
    Color4::xyz1(blk.getPoint3("cutting_color", Point3(0.4f, 0.4f, 0.4f))));

  ShaderGlobal::set_real(get_shader_variable_id("dyn_decals_smoothness"), blk.getReal("smoothness", 0.3f));
  ShaderGlobal::set_real(get_shader_variable_id("dyn_decals_metalness"), blk.getReal("metallness", 0.6f));

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

  clear_atlas();

  planarDecalsShaderParamsBufs.clear();
  planarDecalsShaderParamsNeedUpdate.clear();
  planarDecalCounts.clear();
  planarDecalsShaderParamsSetIds.clear();
}

void add_to_atlas(TEXTUREID texId)
{
  if (texId == BAD_TEXTUREID)
    return;
  WinAutoLock lock(atlasMutex);
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
  WinAutoLock lock(atlasMutex);
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

PlanarDecalsParamsSet construct_decal_params(const DecalsData &decals);

int allocate_buffer()
{
  WinAutoLock lock(planarDecalsMutex);

  int setId = planarDecalsShaderParamsSetIds.allocate();
  if (setId >= planarDecalsShaderParamsBufs.size())
  {
    // SBCF_CPU_ACCESS_WRITE | SBCF_BIND_CONSTANT combo doesn't seem to work on dx10 hardware like old Intel igpus
    planarDecalsShaderParamsBufs.push_back(dag::create_sbuffer(sizeof(PlanarDecalsParamsSet), 1,
      useDynamicConstBuf ? SBCF_CB_PERSISTENT : SBCF_CPU_ACCESS_WRITE | SBCF_BIND_CONSTANT, 0,
      String(0, "planar_decals_params_cbuf_%d", setId)));
    planarDecalsShaderParamsNeedUpdate.push_back(true);
    planarDecalCounts.push_back(0);
  }
  if (!planarDecalsShaderParamsBufs[setId])
    return -1;

  planarDecalCounts[setId] = 0;

  return setId;
}

void update_buffer(int params_id, const DecalsData &decals)
{
  if (params_id < 0 || !are_decals_ready(decals))
    return;

  WinAutoLock lock(planarDecalsMutex);

  if (planarDecalsShaderParamsNeedUpdate[params_id])
  {
    int decalCount = get_valid_decal_count(decals);
    if (decalCount > 0)
    {
      PlanarDecalsParamsSet encodedDecals = construct_decal_params(decals);
      planarDecalsShaderParamsBufs[params_id]->updateData(0, sizeof(encodedDecals), &encodedDecals,
        (useDynamicConstBuf ? VBLOCK_DISCARD : 0) | VBLOCK_WRITEONLY);
      planarDecalsShaderParamsNeedUpdate[params_id] = false;
      planarDecalCounts[params_id] = decalCount;
    }
  }
}

void remove_from_buffer(int decals_param_set_id)
{
  if (decals_param_set_id < 0)
    return;

  WinAutoLock lock(planarDecalsMutex);

  planarDecalsShaderParamsSetIds.free(decals_param_set_id);
  // 'else' shouldn't happen. But still check for the case if something goes wrong.
  if (decals_param_set_id < planarDecalsShaderParamsNeedUpdate.size())
    planarDecalsShaderParamsNeedUpdate[decals_param_set_id] = true;
  else
    G_ASSERTF(0, "Attempt to remove non-allocated set of dynmodel decal parameters %d", decals_param_set_id);
}

void after_reset_device() { planarDecalsShaderParamsNeedUpdate.assign(planarDecalsShaderParamsNeedUpdate.size(), true); }

static void reset_atlas()
{
  atlas.resetAtlas();
  planarDecalsShaderParamsNeedUpdate.assign(planarDecalsShaderParamsNeedUpdate.size(), true);
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
  reset_atlas();
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
    reset_atlas();
    return true;
  }
  return false;
}

void clear_atlas()
{
  WinAutoLock lock(atlasMutex);
  close_atlas();
}

Point4 get_atlas_uv(TEXTUREID texId)
{
  WinAutoLock lock(atlasMutex);
  auto iter = texId != BAD_TEXTUREID ? atlas_elements.find(texId) : atlas_elements.end();
  return iter != atlas_elements.end() ? iter->second.uv : Point4(1, 1, 0, 0);
}

bool is_pending_update(TEXTUREID texId)
{
  WinAutoLock lock(atlasMutex);
  auto iter = texId != BAD_TEXTUREID ? atlas_elements.find(texId) : atlas_elements.end();
  return iter != atlas_elements.end() ? iter->second.pendingUpdate : false;
}

bool are_decals_ready(const DecalsData &decals)
{
  for (auto texId : decals.getTextures())
  {
    if (is_pending_update(texId.getId()))
      return false;
  }
  return true;
}

PlanarDecalsParamsSet construct_decal_params(const DecalsData &decals)
{
  PlanarDecalsParamsSet result = {};

  int decalCount = 0;
  for (int i = 0; i < dyn_decals::num_decals; ++i)
  {
    float4 atlasUv = get_atlas_uv(decals.getTexture(i).getId());

    bool valid = decals.getTexture(i).getTex() != nullptr && decals.getTexture(i).getId() != BAD_TEXTUREID &&
                 abs(atlasUv.z - atlasUv.x) > 1e-6f && abs(atlasUv.w - atlasUv.y) > 1e-6f;
    if (!valid)
      continue;

    bool twoSided = decals.absDot[i];
    bool mirrored = twoSided && decals.oppositeMirrored[i];

    result.decalWidths[decalCount] = decals.widthBox[i];

    result.tmAndUv[decalCount].tmRow0[0] = decals.decalLines[i * 4 + 0];
    result.tmAndUv[decalCount].tmRow1[0] = decals.decalLines[i * 4 + 1];
    result.tmAndUv[decalCount].tmRow2[0] = decals.contactNormal[i * 2 + 0];

    result.tmAndUv[decalCount].tmRow0[1] = twoSided ? decals.decalLines[i * 4 + 2] : result.tmAndUv[i].tmRow0[0];
    result.tmAndUv[decalCount].tmRow1[1] = twoSided ? decals.decalLines[i * 4 + 3] : result.tmAndUv[i].tmRow1[0];
    result.tmAndUv[decalCount].tmRow2[1] = twoSided ? decals.contactNormal[i * 2 + 1] : result.tmAndUv[i].tmRow2[0];
    if (mirrored)
      result.tmAndUv[decalCount].tmRow0[1] = float4(0, 0, 0, 1) - result.tmAndUv[decalCount].tmRow0[1];

    result.tmAndUv[decalCount].atlasUvOriginScale = float4(atlasUv.x, atlasUv.y, atlasUv.z - atlasUv.x, atlasUv.w - atlasUv.y);
    decalCount++;
  }

  return result;
}

int get_valid_decal_count(const DecalsData &decals)
{
  int decalCount = 0;
  for (int i = 0; i < dyn_decals::num_decals; ++i)
  {
    float4 atlasUv = get_atlas_uv(decals.getTexture(i).getId());
    bool valid = decals.getTexture(i).getTex() != nullptr && decals.getTexture(i).getId() != BAD_TEXTUREID &&
                 abs(atlasUv.z - atlasUv.x) > 1e-6f && abs(atlasUv.w - atlasUv.y) > 1e-6f;
    if (valid)
      decalCount++;
  }
  return decalCount;
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
}

void set_planar_decals(int decals_param_set_id, dynrend::PerInstanceRenderData &render_data)
{
  if (decals_param_set_id < 0)
    return;

  WinAutoLock lock(planarDecalsMutex);

  int setId = decals_param_set_id;
  G_ASSERT_RETURN(setId < planarDecalsShaderParamsBufs.size(), );

  if (planarDecalCounts[setId] == 0)
    return;

  dynrend::Interval &interval = render_data.intervals.push_back();
  interval.varId = planar_decal_countVarId.get_var_id();
  interval.setValue = planarDecalCounts[setId];

  render_data.constDataBuf = planarDecalsShaderParamsBufs[setId].getBufId();
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
  WinAutoLock lock(atlasMutex);
  bool wasEnlarged = false;
  for (auto &elem : atlas_elements)
  {
    TEXTUREID texId = elem.first;
    if (!elem.second.pendingUpdate || !prefetch_and_check_managed_texture_loaded(texId, true))
      continue;
    elem.second.pendingUpdate = false;
    WriteTextureToAtlasResult result = write_texture_to_atlas(texId, elem.second.uv);
    if (result == WriteTextureToAtlasResult::NotEnoughSpace)
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
    else if (result == WriteTextureToAtlasResult::LoadError)
    {
      logerr("Failed to load decal %s", get_managed_res_name(texId));
#if DAGOR_DBGLEVEL > 0
      if (loadErrorTexId != BAD_TEXTUREID)
      {
        if (prefetch_and_check_managed_texture_loaded(loadErrorTexId))
          write_texture_to_atlas(texId, elem.second.uv);
        else
          elem.second.pendingUpdate = true;
      }
#endif
    }
  }
  return wasEnlarged;
}

static void init_atlas(unsigned tex_fmt)
{
  if (atlas.isInited())
    return;
  tex_fmt |= TEXCF_UPDATE_DESTINATION | TEXCF_SRGBREAD;
  atlas.initAtlas(ATLAS_ELEM_MAX_NUM, dag::ConstSpan<unsigned>(&tex_fmt, 1), "dynamic_decals_atlas", ATLAS_SIZES[current_atlas_size],
    8);
  ShaderGlobal::set_texture(get_shader_variable_id("dynamic_decals_atlas", true), atlas.atlasTex[0].getTexId());
  d3d::SamplerInfo smpInfo;
  smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
  ShaderGlobal::set_sampler(::get_shader_variable_id("dynamic_decals_atlas_samplerstate", true), d3d::request_sampler(smpInfo));
}

static void close_atlas()
{
  current_atlas_size = initial_atlas_size_index;
  atlas_elements.clear();
  reset_atlas();
}

static WriteTextureToAtlasResult write_texture_to_atlas(TEXTUREID tex_id, Point4 &out_uv)
{
  out_uv = Point4(0, 0, 0, 0);

  Texture *texture = (Texture *)acquire_managed_tex(tex_id);
  G_ASSERT(texture);
  if (!texture)
    return WriteTextureToAtlasResult::LoadError;

  TextureInfo info;
  texture->getinfo(info);
  unsigned texFmt = info.cflg & TEXFMT_MASK;
  init_atlas(texFmt);
  G_ASSERT_RETURN(atlas.atlasTexFmt.size() == 1, WriteTextureToAtlasResult::LoadError);

  unsigned atlasTexFmt = atlas.atlasTexFmt[0] & TEXFMT_MASK;
  if (texFmt != atlasTexFmt)
  {
    const char *texFmtStr = get_tex_format_name(texFmt);
    const char *atlasTexFmtStr = get_tex_format_name(atlasTexFmt);
    logerr("Can't add a decal texture %s of format %s to the atlas of format %s which is taken from another texture. "
           "All decal textures must have the same format",
      get_managed_res_name(tex_id), texFmtStr ? texFmtStr : "?", atlasTexFmtStr ? atlasTexFmtStr : "?");
    return WriteTextureToAtlasResult::LoadError;
  }

  WriteTextureToAtlasResult result = WriteTextureToAtlasResult::Ok;

  int atlasIndex = atlas.addTextureToAtlas(info, out_uv, false);
  if (atlasIndex >= 0)
    result = atlas.writeTextureToAtlas(texture, atlasIndex, 0) ? WriteTextureToAtlasResult::Ok : WriteTextureToAtlasResult::LoadError;
  else
    result = WriteTextureToAtlasResult::NotEnoughSpace;

  release_managed_tex(tex_id);

  return result;
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
