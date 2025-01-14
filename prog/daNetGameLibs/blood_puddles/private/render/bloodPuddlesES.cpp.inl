// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fgNodes.h"

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_resetDevice.h>
#include <decalMatrices/decalsMatrices.h>
#include <ecs/core/entityManager.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/props/atmosphere.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/rendInst/riExtra.h>
#include <render/primitiveObjects.h>
#include <render/viewVecs.h>
#include <render/fx/effectManager.h>
#include <3d/dag_quadIndexBuffer.h>
#include <util/dag_console.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <startup/dag_globalSettings.h>
#include "camera/sceneCam.h"
#include <landMesh/biomeQuery.h>
#include <render/fx/fx.h>
#include "game/dasEvents.h"
#include <ecs/delayedAct/actInThread.h>
#include <daECS/core/coreEvents.h>
#include <render/renderSettings.h>
#include <render/renderEvent.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/updateStageRender.h>
#include "blood_puddles/public/render/bloodPuddles.h"

static const uint32_t MAX_PUDDLES = 4096;
static const uint32_t REMOVE_PUDDLES_THRESHOLD = MAX_PUDDLES / 4 * 3;

static eastl::unique_ptr<BloodPuddles> blood_puddles_mgr;

extern bool is_point_under_water(const Point3 &point);

BloodPuddles *get_blood_puddles_mgr() { return blood_puddles_mgr.get(); }
void init_blood_puddles_mgr() { blood_puddles_mgr = eastl::make_unique<BloodPuddles>(); }
void close_blood_puddles_mgr() { blood_puddles_mgr.reset(); }

template <typename Callable>
inline void is_bood_enabled_ecs_query(Callable c);

template <typename Callable>
inline void get_blood_color_ecs_query(Callable c);

bool is_blood_enabled()
{
  bool isEnabled = true;
  is_bood_enabled_ecs_query([&](const bool isBloodEnabled) { isEnabled = isBloodEnabled; });
  return isEnabled;
}

Color4 get_blood_color()
{
  Color4 bloodColor(1.f, 1.f, 1.f, 1.f);
  get_blood_color_ecs_query([&](const bool isBloodEnabled, const E3DCOLOR &disabledBloodColor) {
    if (!isBloodEnabled)
      bloodColor = color4(disabledBloodColor);
  });
  return bloodColor;
}

void add_hit_blood_effect(const Point3 &pos, const Point3 &dir)
{
  if (get_blood_puddles_mgr() && is_blood_enabled())
  {
    if (get_blood_puddles_mgr()->tryAllocateSplashEffect())
    {
      ecs::ComponentsInitializer attrs;
      ECS_INIT(attrs, blood_puddles__pos, pos);
      ECS_INIT(attrs, blood_puddles__dir, dir);
      g_entity_mgr->createEntityAsync("blood_puddles_creator", eastl::move(attrs));
    }
  }
}

BloodPuddles::BloodPuddles() :
  matrixManager(BLOOD_PUDDLES_MAX_MATRICES_COUNT, "blood_puddle_matrices", ECS_HASH("decals__bloodPuddlesMatrixId"), MAX_PUDDLES),
  updateOffset(0),
  updateLength(0)
{
  index_buffer::init_box();

  const char *bloodQuality = dgs_get_settings()->getBlockByName("graphics")->getStr("bloodQuality", "high");
  const bool isDeferredBlood = !strcmp(bloodQuality, "high");

  const DataBlock paramsBlk(BLOOD_PARAMS_DATA_BLOCK_NAME);
  initResources(paramsBlk);
  useDeferredMode(isDeferredBlood);
  initNodes();
  initTextureParams(paramsBlk);
  initShadingParams(paramsBlk);
  initGroupDists(paramsBlk);

  splashFxTemplName = paramsBlk.getStr("splashEffectTemplateName", nullptr);
  if (splashFxTemplName.empty())
  {
    splashFxId = acesfx::get_type_by_name(paramsBlk.getStr("splashEffectName", nullptr));
    splashFxScale = paramsBlk.getReal("splashEffectScale", 0.7);
  }
  maxSplashesPerFrame = paramsBlk.getInt("maxSplashesPerFrame", 2);
}

void BloodPuddles::initResources(const DataBlock &blk)
{
  puddlesBuf =
    dag::create_sbuffer(sizeof(puddles[0]), MAX_PUDDLES, SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "blood_puddles_buf");
  perGroupParametersBuf = dag::buffers::create_persistent_cb(BLOOD_DECAL_GROUPS_COUNT, "blood_decal_params");

  const char *puddleTexName = blk.getStr("shapesAtlas", nullptr);
  puddleTex = dag::get_tex_gameres(puddleTexName, "blood_puddle_tex");
  if (puddleTex)
    puddleTex->disableSampler();

  const int separateFlowmapModeVarId = ::get_shader_variable_id("blood_puddles_has_separate_flowmap", true);
  const char *flowmapTexName = blk.getStr("flowmap", nullptr);
  if (flowmapTexName)
  {
    useSeparateFlowmapTex = true;
    ShaderGlobal::set_int(separateFlowmapModeVarId, 1);

    flowmapTex = dag::get_tex_gameres(flowmapTexName, "blood_puddle_flowmap_tex");
    if (flowmapTex)
      flowmapTex->disableSampler();
  }
  else
  {
    useSeparateFlowmapTex = false;
    ShaderGlobal::set_int(separateFlowmapModeVarId, 0);
  }

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(::get_shader_variable_id("blood_puddle_tex_samplerstate", true), d3d::request_sampler(smpInfo));
    ShaderGlobal::set_sampler(::get_shader_variable_id("blood_puddle_flowmap_tex_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  const Point2 atlasSize = blk.getPoint2("atlasSize", Point2(1, 1));
  ShaderGlobal::set_color4(::get_shader_variable_id("blood_decals_atlas_size", true), atlasSize.x, atlasSize.y, 0, 0);
  ShaderGlobal::set_real(::get_shader_variable_id("blood_puddle_depth", true), blk.getReal("depth", 0.03f));
}

void BloodPuddles::closeResources()
{
  puddlesBuf.close();
  perGroupParametersBuf.close();
  puddleTex.close();
  flowmapTex.close();
}

void BloodPuddles::initNodes()
{
  if (isDeferredMode)
  {
    prepassNode = make_blood_accumulation_prepass_node();
    resolveNode = make_blood_resolve_node();
  }
  else
    resolveNode = make_blood_forward_node();
}

void BloodPuddles::closeNodes()
{
  prepassNode = {};
  resolveNode = {};
}

void BloodPuddles::reinitShadingParams()
{
  const DataBlock paramsBlk(BLOOD_PARAMS_DATA_BLOCK_NAME);
  initShadingParams(paramsBlk);
}

void BloodPuddles::initGroupDists(const DataBlock &blk)
{
  const DataBlock *distBlk = blk.getBlockByNameEx("groupDists");
  for (int i = 0; i < BLOOD_DECAL_GROUPS_COUNT; ++i)
    groupDists[i] = distBlk->getReal(groupNames[i], 0.25);
}


void BloodPuddles::initTextureParams(const DataBlock &blk)
{
  int footprintsVariantsCount = 0;
  const DataBlock *footprintsVarsBlk = blk.getBlockByNameEx("footprints");
  footprintsRanges.clear();
  footprintsRanges.reserve(footprintsVarsBlk->paramCount());
  for (int i = 0; i < footprintsVarsBlk->paramCount(); i++)
  {
    int footprintsGroupVariants = footprintsVarsBlk->getInt(i);
    footprintsRanges.emplace_back(0, footprintsGroupVariants - 1);
    footprintsVariantsCount += footprintsGroupVariants;
  }

  for (int i = 0; i < groupNames.size(); ++i)
  {
    int decalGroupVariants = 0;
    if (i == BLOOD_DECAL_GROUP_FOOTPRINT)
      decalGroupVariants = footprintsVariantsCount;
    else
      decalGroupVariants = blk.getInt(String(0, "%sVariantsCount", groupNames[i]), 4);

    variantsRanges[i] = IPoint2(0, decalGroupVariants - 1);
  }
  for (int i = 1; i < groupNames.size(); ++i)
    variantsRanges[i] += IPoint2(variantsRanges[i - 1].y + 1, variantsRanges[i - 1].y + 1);

  if (footprintsRanges.size())
    footprintsRanges[0] += IPoint2(variantsRanges[BLOOD_DECAL_GROUP_FOOTPRINT].x, variantsRanges[BLOOD_DECAL_GROUP_FOOTPRINT].x);
  for (int i = 1; i < footprintsRanges.size(); i++)
    footprintsRanges[i] += IPoint2(footprintsRanges[i - 1].y + 1, footprintsRanges[i - 1].y + 1);
}

void BloodPuddles::initShadingParams(const DataBlock &blk)
{
  for (int i = 0; i < sizeRanges.size(); ++i)
    sizeRanges[i] = blk.getPoint2(String(0, "%sSizeRange", groupNames[i]), Point2(0.5, 1));

  additionalRandomInfl = blk.getReal("additionalRandomInfl", 0.5);
  footprintDecayPerStep = blk.getReal("footprintDecayPerStep", 1);

  struct UnpackedPerGroupParams
  {
    float lifeTime, stayFreshTime;
  };
  eastl::array<UnpackedPerGroupParams, BLOOD_DECAL_GROUPS_COUNT> unpackedParams;

  for (int i = 0; i < BLOOD_DECAL_GROUPS_COUNT; ++i)
  {
    unpackedParams[i].lifeTime = blk.getReal(String(0, "%sAppearTime", groupNames[i]), 10);
    unpackedParams[i].stayFreshTime = blk.getReal(String(0, "%sStayFreshTime", groupNames[i]), 1);
  }

  splashFxLifeTime = unpackedParams[BLOOD_DECAL_GROUP_SPLASH].lifeTime;

  puddleFreshnessTime = unpackedParams[BLOOD_DECAL_GROUP_PUDDLE].lifeTime + unpackedParams[BLOOD_DECAL_GROUP_PUDDLE].stayFreshTime;

  const DataBlock *distBlk = blk.getBlockByNameEx("groupBloodNormalDominanceTerm");
  const DataBlock *edgeBlk = blk.getBlockByNameEx("groupEdgeWidthScale");

  for (int i = 0; i < BLOOD_DECAL_GROUPS_COUNT; ++i)
  {
    perGroupPackedParams[i].invLifetime = 1.0f / unpackedParams[i].lifeTime;

    float bloodNormalDominanceTerm = clamp(distBlk->getReal(groupNames[i], 0.0f), 0.0f, 1.0f);
    float edgeWidthScale = clamp(edgeBlk->getReal(groupNames[i], 0.5f), 0.0f, 1.0f);

    perGroupPackedParams[i].bloodNormalDominanceTerm_edgeWidthScale =
      (uint32_t(edgeWidthScale * 255.0f) << 8u) | (uint32_t(bloodNormalDominanceTerm * 255.0f));
  }

  perGroupParametersBuf.getBuf()->updateData(0, sizeof(perGroupPackedParams[0]) * perGroupPackedParams.size(),
    perGroupPackedParams.data(), VBLOCK_DISCARD);
}

static uint32_t get_puddle_type(const PuddleInfo &puddle) { return puddle.matrix_frame_incident_type & ((1 << BITS_FOR_TYPE) - 1); }

static uint32_t get_puddle_mat_id(const PuddleInfo &puddle) { return (puddle.matrix_frame_incident_type >> 16) & MATRIX_MASK; }

bool BloodPuddles::tryPlacePuddle(PuddleCtx &ctx)
{
  G_ASSERT(puddles.size() <= MAX_PUDDLES);
  if (puddles.size() >= MAX_PUDDLES || !texturesAreReady())
    return false;
  ctx.pos += -normalize(ctx.dir) * 0.05f;
  rendinst::RendInstDesc riDesc;
  if (!dacoll::traceray_normalized(ctx.pos, ctx.dir, ctx.puddleDist, nullptr, &ctx.normal, dacoll::ETF_DEFAULT, &riDesc))
    return false;
  if (dot(ctx.normal, -ctx.dir) < sqrt(3) / 2) // Check cos between normal and horisontal plane (angle should be less than 30 degrees)
    return false;
  ctx.pos -= -normalize(ctx.dir) * ctx.puddleDist;
  ctx.riEx = riDesc.isValid() ? riDesc.getRiExtraHandle() : rendinst::RIEX_HANDLE_NULL;
  return true;
}

void BloodPuddles::addPuddleAt(const PuddleCtx &ctx, int group)
{
  G_ASSERT(puddles.size() <= MAX_PUDDLES);
  if (puddles.size() >= MAX_PUDDLES || !texturesAreReady())
    return;
  const float size = rnd_float(sizeRanges[group].x, sizeRanges[group].y);
  putDecal(group, ctx.pos, ctx.normal, size, ctx.riEx, ctx.pos, false);
}

float BloodPuddles::addRandomToSize(int group, float size) const
{
  size = clamp(size, sizeRanges[group].x, sizeRanges[group].y);
  return lerp(size, rnd_float(sizeRanges[group].x, sizeRanges[group].y), additionalRandomInfl);
}

// static void register_ri_matrix_data(rendinst::riex_handle_t riex_handle, uint32_t &matrix_id, TMatrix &itm)
// {
//   if (get_blood_puddles_mgr())
//   {
//     DecalsMatrices *matrixMgr = get_blood_puddles_mgr()->getMatrixManager();
//     ecs::EntityId riexEid = find_ri_extra_eid(riex_handle);
//     const TMatrix &riexTm = matrixMgr->getBasisMatrix(riexEid);
//     matrix_id = matrixMgr->registerMatrix(riexEid, riexTm);
//     matrixMgr->useMatrixId(matrix_id);
//     itm = inverse(riexTm);
//   }
//   else
//   {
//     matrix_id = 0;
//     itm = TMatrix::IDENT;
//   }
// }

static void register_ri_matrix_data(rendinst::riex_handle_t, uint32_t &matrix_id, TMatrix &itm)
{
  matrix_id = 0;
  itm = TMatrix::IDENT;
}

int BloodPuddles::getDecalVariant(const int group) const
{
  const int variantsCount = variantsRanges[group].y - variantsRanges[group].x + 1;
  const int variantId = puddles.size() % variantsCount;
  return variantsRanges[group].x + variantId;
}

void BloodPuddles::addDecal(const int group,
  const Point3 &pos,
  const Point3 &normal,
  const Point3 &dir,
  int matrix_id,
  float size,
  const Point3 &hit_pos,
  bool projective,
  bool is_landscape,
  int variant,
  float strength)
{
  G_ASSERT(group < 1 << BITS_FOR_TYPE);
  G_ASSERT(puddles.size() <= MAX_PUDDLES);
  if (puddles.size() >= MAX_PUDDLES || !texturesAreReady())
    return;

  if (variant != INVALID_VARIANT)
  {
    const int varId = abs(variant);
    const int fallbackId = variantsRanges[group].x;
    const int variantsRangeBegin = variantsRanges[group].x;
    const int variantsRangeEnd = variantsRanges[group].y;
    G_ASSERTF_AND_DO(varId <= variantsRangeEnd && varId >= variantsRangeBegin, variant = fallbackId,
      "BloodPuddles: oob variantId %d. should be in in range [%d:%d]. Please save full dump and report to render team", varId,
      variantsRangeBegin, variantsRangeEnd);
  }

  G_ASSERTF_AND_DO(matrix_id < BLOOD_PUDDLES_MAX_MATRICES_COUNT, matrix_id = 0,
    "BloodPuddles: oob matrix_id %d >= %d. Please save full dump and report to render team", matrix_id,
    BLOOD_PUDDLES_MAX_MATRICES_COUNT);

  float rotate;
  uint32_t incident = 7;
  if (projective)
  {
    Point3 direction = normalize(hit_pos - pos);
    float acos = dot(normal, direction);
    const int MAX_INCIDENT_ANGLE_COUNT = 8;
    incident = min((uint32_t)(abs(acos) * MAX_INCIDENT_ANGLE_COUNT), 7u);
    rotate = atan2(-direction.z, direction.x) - PI / 2.;
  }
  else
  {
    rotate = genRotate(group, dir);
  }
  if (variant == INVALID_VARIANT)
    variant = getDecalVariant(group);

  const uint32_t packedStrength = uint32_t(255.0f * strength);
  const uint32_t packedStartTime = float_to_half(get_shader_global_time_phase(0, 0));

  const uint32_t packedNormalX = float_to_half(normal.x);
  const uint32_t packedNormalY = float_to_half(normal.y);
  const uint32_t packedNormalZ = float_to_half(normal.z);
  const uint32_t packedFadeTime = 0;

  const uint32_t landscapeBit = is_landscape ? 1 : 0;

  const uint32_t packedFrameInfo = (uint32_t)((abs(variant) << 1) | (variant < 0));

  const uint32_t packedRotate = float_to_half(rotate);
  const uint32_t packedSize = float_to_half(size);

  PuddleInfo puddle;
  puddle.pos = pos;
  puddle.starttime__strength = (packedStartTime << 8u) | packedStrength;
  puddle.normal_fadetime.x = (packedNormalX << 16u) | packedNormalY;
  puddle.normal_fadetime.y = (packedNormalZ << 16u) | packedFadeTime;
  puddle.matrix_frame_incident_type = (landscapeBit << LANDSCAPE_SHIFT) | ((matrix_id & MATRIX_MASK) << 16u) |
                                      (packedFrameInfo << (BITS_FOR_TYPE + BITS_FOR_INCIDENT_ANGLE)) | (incident << BITS_FOR_TYPE) |
                                      group;
  puddle.rotate_size = (packedRotate << 16u) | packedSize;

  puddles.emplace_back(eastl::move(puddle));
  updateLength++;
}

void BloodPuddles::putDecal(int group,
  const Point3 &pos,
  const Point3 &normal,
  float size,
  rendinst::riex_handle_t riex_handle,
  const Point3 &hit_pos,
  bool projective)
{
  putDecal(group, pos, normal, Point3(0, 0, 0), size, riex_handle, hit_pos, projective);
}

void BloodPuddles::putDecal(int group,
  const Point3 &pos,
  const Point3 &normal,
  const Point3 &dir,
  float size,
  rendinst::riex_handle_t riex_handle,
  const Point3 &hit_pos,
  bool projective,
  int variant,
  float strength)
{
  if (is_point_under_water(pos))
    return;
  for (const PuddleInfo &puddle : puddles)
    // checks min square distance between existing puddles and new one
    if ((get_puddle_type(puddle) == group) && (lengthSq(puddle.pos - pos) < groupDists[group]))
      return;
  size = addRandomToSize(group, size);
  uint32_t matrixId = 0;
  TMatrix itm = TMatrix::IDENT;

  const rendinst::RendInstDesc riDesc{riex_handle};
  const bool isLandscape = !riDesc.isValid();

  if (riDesc.isValid() && riDesc.isDynamicRiExtra())
    register_ri_matrix_data(riex_handle, matrixId, itm);

  addDecal(group, itm * pos, itm % normal, dir, matrixId, size, hit_pos, projective, isLandscape, variant, strength);
}

void BloodPuddles::update()
{
  splashesThisFrame = 0;
  const float current_time = get_shader_global_time_phase(0, 0);

  if (matrixManager.numRemovedMatrices())
  {
    erasePuddles([this](const PuddleInfo &info) { return matrixManager.isMatrixRemoved(get_puddle_mat_id(info)); });
    matrixManager.clearDeletedMatrices();
  }

  if (puddlesToRemoveCount && current_time - FADE_TIME_TICK * static_cast<uint16_t>(puddles[0].normal_fadetime.y) > SECONDS_TO_FADE)
  {
    puddles.erase(puddles.begin());
    --puddlesToRemoveCount;
    updateOffset = 0;
    updateLength = puddles.size();
  }

  if (puddles.size() - puddlesToRemoveCount < REMOVE_PUDDLES_THRESHOLD)
    return;

  updateOffset = 0;
  updateLength = puddles.size();
  const int oldest = puddlesToRemoveCount;
  puddlesToRemoveCount++;

  uint32_t prevValue = puddles[oldest].normal_fadetime.y;
  uint16_t fadeTime = static_cast<uint16_t>(current_time * FADE_TIME_TICK_INV);
  puddles[oldest].normal_fadetime.y = (prevValue & 0xffff0000) | fadeTime;
}

void BloodPuddles::beforeRender()
{
  matrixManager.beforeRender();
  if (!updateLength)
    return;
  puddlesBuf.getBuf()->updateData(updateOffset * sizeof(puddles[0]), updateLength * sizeof(puddles[0]), puddles.data() + updateOffset,
    0);
  updateOffset = puddles.size();
  updateLength = 0;
}

void BloodPuddles::renderShElem(const TMatrix &view_tm, const TMatrix4 &proj_tm, const ShaderElement *shelem)
{
  if (puddles.empty() || !shelem || !texturesAreReady())
    return;
  TIME_D3D_PROFILE(blood_puddles);
  index_buffer::use_box();
  set_viewvecs_to_shader(view_tm, proj_tm);
  shelem->setStates();
  d3d::setvsrc(0, 0, 0);
  d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0, puddles.size());
}

BloodPuddles::~BloodPuddles()
{
  closeResources();
  closeNodes();
  index_buffer::release_box();
}

float BloodPuddles::getBloodFreshness(const Point3 &pos) const
{
  const float currentTime = get_shader_global_time_phase(0, 0);
  for (const PuddleInfo &puddle : puddles)
  {
    if (get_puddle_type(puddle) != BLOOD_DECAL_GROUP_PUDDLE)
      continue;
    const bool posInPuddle = lengthSq(puddle.pos - pos) < sqr(half_to_float(puddle.rotate_size));
    if (posInPuddle)
      return (1 - saturate((currentTime - half_to_float(puddle.starttime__strength >> 8)) / puddleFreshnessTime));
  }
  return 0;
}

void BloodPuddles::addSplashEmitter(const Point3 &start_pos,
  const Point3 &target_pos,
  const Point3 &normal,
  const Point3 &dir,
  float size,
  rendinst::riex_handle_t riex_handle,
  const Point3 &gravity)
{
  uint32_t matrixId = 0;
  TMatrix itm = TMatrix::IDENT;

  if (riex_handle != rendinst::RIEX_HANDLE_NULL)
    register_ri_matrix_data(riex_handle, matrixId, itm);

  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, blood_splash_emitter__pos, start_pos);
  ECS_INIT(attrs, blood_splash_emitter__targetPos, target_pos);
  ECS_INIT(attrs, blood_splash_emitter__gravity, gravity);
  ECS_INIT(attrs, blood_splash_emitter__velocity, dir * 0.25f);
  ECS_INIT(attrs, blood_splash_emitter__normal, normal);
  ECS_INIT(attrs, blood_splash_emitter__itm, itm);
  ECS_INIT(attrs, blood_splash_emitter__matrix_id, (int)matrixId);
  ECS_INIT(attrs, blood_splash_emitter__size, addRandomToSize(BLOOD_DECAL_GROUP_SPRAY, size));
  g_entity_mgr->createEntityAsync("blood_splash_emitter", eastl::move(attrs));
}

void BloodPuddles::startSplashEffect(const Point3 &pos, const Point3 &dir)
{
  if (!splashFxTemplName.empty())
  {
    TMatrix tm;
    tm.col[1] = normalize(dir);
    tm.col[2] = normalize(cross(Point3(1, 0, 0), tm.col[1]));
    tm.col[0] = normalize(cross(tm.col[1], tm.col[2]));
    tm.col[3] = pos;
    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, transform, tm);
    g_entity_mgr->createEntityAsync(splashFxTemplName, eastl::move(attrs));
  }
  else if (splashFxId >= 0)
  {
    AcesEffect *effect = acesfx::start_effect_pos_norm(splashFxId, pos, dir, false, splashFxScale);
    if (effect)
    {
      effect->setLifeDuration(splashFxLifeTime);
    }
  }
}

void BloodPuddles::addSplash(const Point3 &pos, const Point3 &normal, const TMatrix &itm, int matrix_id, float size)
{
  putDecal(BLOOD_DECAL_GROUP_SPRAY, itm * pos, itm % normal, matrix_id, size, pos, false);
  startSplashEffect(pos, normal);
}

void BloodPuddles::addFootprint(Point3 pos, const Point3 &dir, const Point3 &up_dir, float strength, bool is_left, int variant)
{
  G_ASSERT(puddles.size() <= MAX_PUDDLES);
  if (puddles.size() >= MAX_PUDDLES || !texturesAreReady())
    return;
  float puddleDist = 2;
  Point3 normal;
  pos += up_dir * 0.25f;
  rendinst::RendInstDesc riDesc;
  if (!dacoll::traceray_normalized(pos, -up_dir, puddleDist, nullptr, &normal, dacoll::ETF_DEFAULT, &riDesc))
    return;
  const Point3 puddleCenter = pos - up_dir * puddleDist;

  int decal_id = rnd_int(footprintsRanges[variant].x, footprintsRanges[variant].y);

  if (!is_left)
    decal_id = -decal_id;
  const float size = rnd_float(sizeRanges[BLOOD_DECAL_GROUP_FOOTPRINT].x, sizeRanges[BLOOD_DECAL_GROUP_FOOTPRINT].y);
  const rendinst::riex_handle_t riex_handle = riDesc.isValid() ? riDesc.getRiExtraHandle() : rendinst::RIEX_HANDLE_NULL;
  putDecal(BLOOD_DECAL_GROUP_FOOTPRINT, puddleCenter, normal, dir, size, riex_handle, pos, false, decal_id, strength);
}

void BloodPuddles::reset()
{
  const DataBlock paramsBlk(BLOOD_PARAMS_DATA_BLOCK_NAME);
  closeResources();
  closeNodes();
  initResources(paramsBlk);
  initNodes();

  updateOffset = 0;
  updateLength = puddles.size();
  perGroupParametersBuf.getBuf()->updateData(0, sizeof(perGroupPackedParams[0]) * perGroupPackedParams.size(),
    perGroupPackedParams.data(), VBLOCK_DISCARD);
  matrixManager.reset();
}

static void reset_blood_es(const AfterDeviceReset &)
{
  if (get_blood_puddles_mgr())
    get_blood_puddles_mgr()->reset();
}

ECS_TAG(render)
ECS_TRACK(*)
static void update_blood_shader_params_es(const ecs::Event &,
  const ShaderVar &blood_begin_color,
  const Point4 &blood_begin_color_value,
  const ShaderVar &blood_end_color,
  const Point4 &blood_end_color_value,
  const ShaderVar &blood_puddle_high_intensity_color,
  const Point4 &blood_puddle_high_intensity_color_value,
  const ShaderVar &blood_puddle_low_intensity_color,
  const Point4 &blood_puddle_low_intensity_color_value,
  const ShaderVar &blood_puddle_start_size,
  const float blood_puddle_start_size_value,
  const ShaderVar &blood_puddle_reflectance,
  const float blood_puddle_reflectance_value,
  const ShaderVar &blood_puddle_smoothness,
  const float blood_puddle_smoothness_value,
  const ShaderVar &blood_puddle_smoothness_edge,
  const float blood_puddle_smoothness_edge_value,
  const ShaderVar &blood_puddle_landscape_reflectance,
  const float blood_puddle_landscape_reflectance_value,
  const ShaderVar &blood_puddle_landscape_smoothness_edge,
  const float blood_puddle_landscape_smoothness_edge_value,
  const ShaderVar &blood_puddle_landscape_albedo_darkening,
  const float blood_puddle_landscape_albedo_darkening_value)
{
  ShaderGlobal::set_color4(blood_begin_color, blood_begin_color_value);
  ShaderGlobal::set_color4(blood_end_color, blood_end_color_value);
  ShaderGlobal::set_color4(blood_puddle_high_intensity_color, blood_puddle_high_intensity_color_value);
  ShaderGlobal::set_color4(blood_puddle_low_intensity_color, blood_puddle_low_intensity_color_value);
  ShaderGlobal::set_real(blood_puddle_start_size, blood_puddle_start_size_value);
  ShaderGlobal::set_real(blood_puddle_reflectance, blood_puddle_reflectance_value);
  ShaderGlobal::set_real(blood_puddle_smoothness, blood_puddle_smoothness_value);
  ShaderGlobal::set_real(blood_puddle_smoothness_edge, blood_puddle_smoothness_edge_value);
  ShaderGlobal::set_real(blood_puddle_landscape_reflectance, blood_puddle_landscape_reflectance_value);
  ShaderGlobal::set_real(blood_puddle_landscape_smoothness_edge, blood_puddle_landscape_smoothness_edge_value);
  ShaderGlobal::set_real(blood_puddle_landscape_albedo_darkening, blood_puddle_landscape_albedo_darkening_value);
}

static bool blood_puddle_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("blood_puddles", "reinitShadingParams", 1, 1)
  {
    BloodPuddles *mgr = get_blood_puddles_mgr();
    if (mgr && is_blood_enabled())
      mgr->reinitShadingParams();
  }
  CONSOLE_CHECK_NAME("blood_puddles", "mergeBlood", 2, 2)
  {
    BloodPuddles *mgr = get_blood_puddles_mgr();
    if (mgr && is_blood_enabled())
    {
      mgr->useDeferredMode(console::to_bool(argv[1]));
      mgr->reset();
    }
  }
  CONSOLE_CHECK_NAME("blood_puddles", "addPuddle", 1, 1)
  {
    BloodPuddles *mgr = get_blood_puddles_mgr();
    if (mgr && is_blood_enabled())
    {
      const TMatrix &cameraMatrix = get_cam_itm();
      const Point3 eye = cameraMatrix.getcol(3);
      const Point3 forward = cameraMatrix.getcol(2);
      const float puddleDist = 10000.0f;

      Point3 normal;
      float t = puddleDist;
      if (dacoll::traceray_normalized(eye, forward, t, nullptr, &normal, dacoll::ETF_DEFAULT, nullptr))
      {
        BloodPuddles::PuddleCtx ctx;
        ctx.pos = eye;
        ctx.normal = normal;
        ctx.puddleDist = puddleDist;
        ctx.dir = forward;

        if (mgr->tryPlacePuddle(ctx))
          mgr->addPuddleAt(ctx);
      }
    }
  }
  CONSOLE_CHECK_NAME("blood_puddles", "addHit", 1, 1)
  {
    const TMatrix &cameraMatrix = get_cam_itm();
    Point3 eye = cameraMatrix.getcol(3);
    Point3 forward = cameraMatrix.getcol(2);
    add_hit_blood_effect(eye, normalize(forward));
  }
  CONSOLE_CHECK_NAME("blood_puddles", "addHitAround", 1, 2)
  {
    int count = argc == 2 ? atoi(argv[1]) : 512;
    const TMatrix &cameraMatrix = get_cam_itm();
    Point3 eye = cameraMatrix.getcol(3);
    Point3 forward = cameraMatrix.getcol(2);
    Point3 v(-1., 1., 1.);
    const int c = sqrt(count);
    float d = 2. / c;
    int maxSplashesPerFrame = get_blood_puddles_mgr()->getMaxSplashesPerFrame();
    get_blood_puddles_mgr()->setMaxSplashesPerFrame(count);
    for (int i = 0; i < count; ++i)
    {
      Point3 v2 = v;
      v2 += Point3(d * float(i % c), -d * float(i) / c, 0.);
      v2.normalize();
      Point3 up(0., 1., 0.);
      Point3 right = cross(forward, up);
      Matrix3 mat;
      mat.setcol(0, right);
      mat.setcol(1, up);
      mat.setcol(2, forward);
      v2 = mat * v2;
      add_hit_blood_effect(eye, normalize(v2));
    }
    get_blood_puddles_mgr()->setMaxSplashesPerFrame(maxSplashesPerFrame);
  }
  CONSOLE_CHECK_NAME("blood_puddles", "clear", 1, 1) { get_blood_puddles_mgr()->erasePuddles(); }
  CONSOLE_CHECK_NAME("blood_puddles", "count", 1, 1) { console::print_d("numPuddles: %d", get_blood_puddles_mgr()->getCount()); }
  return found;
}

REGISTER_CONSOLE_HANDLER(blood_puddle_console_handler);