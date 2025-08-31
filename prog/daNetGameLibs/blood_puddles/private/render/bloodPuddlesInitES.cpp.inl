// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fgNodes.h"

#include <3d/dag_quadIndexBuffer.h>
#include <ioSys/dag_dataBlock.h>

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/shaderVar.h>

#include <blood_puddles/public/render/bloodPuddles.h>

#include <effectManager/effectManager.h>
#include <render/renderEvent.h>

constexpr eastl::array group_name_hashes{
#define BLOOD_DECAL_NO_COUNT
#define BLOOD_DECAL_GROUP_DECL(_name, blk_name, _id) ECS_HASH(#blk_name),
#include <blood_puddles/public/shaders/decal_group_enum_decl.hlsli>
#undef BLOOD_DECAL_GROUP_DECL
#undef BLOOD_DECAL_NO_COUNT
};

template <typename Callable>
inline void get_blood_settings_ecs_query(Callable c);

template <typename Callable>
inline void get_blood_group_attributes_ecs_query(Callable c);

BloodPuddles::BloodPuddles() :
  matrixManager(BLOOD_PUDDLES_MAX_MATRICES_COUNT, "blood_puddle_matrices", ECS_HASH("decals__bloodPuddlesMatrixId"), MAX_PUDDLES),
  updateOffset(0),
  updateLength(0)
{
  index_buffer::init_box();

  const char *bloodQuality = dgs_get_settings()->getBlockByName("graphics")->getStr("bloodQuality", "high");
  const bool isDeferredBlood = !strcmp(bloodQuality, "high");

  bool hasSettings = false;
  get_blood_settings_ecs_query([this, &hasSettings](const ecs::Object &blood_puddles_settings__atlas,
                                 const ecs::Object &blood_puddles_settings__groupAttributes) {
    hasSettings = true;
    initAtlasResources(blood_puddles_settings__atlas);
    initGroupAttributes(blood_puddles_settings__groupAttributes);
  });

  if (!hasSettings)
  {
    logerr("BloodPuddles: failed to init, `blood_puddles_settings` is not present on the scene");
    return;
  }

  useDeferredMode(isDeferredBlood);
  initNodes();
}

BloodPuddles::~BloodPuddles()
{
  closeResources();
  closeNodes();
  index_buffer::release_box();
}

void BloodPuddles::reset()
{
  closeResources();
  closeNodes();

  get_blood_settings_ecs_query(
    [this](const ecs::Object &blood_puddles_settings__atlas, const ecs::Object &blood_puddles_settings__groupAttributes) {
      initAtlasResources(blood_puddles_settings__atlas);
      initGroupAttributes(blood_puddles_settings__groupAttributes);
    });

  initNodes();

  updateOffset = 0;
  updateLength = puddles.size();
  matrixManager.reset();
}

void BloodPuddles::initAtlasResources(const ecs::Object &atlas_settings)
{
  puddlesBuf =
    dag::create_sbuffer(sizeof(puddles[0]), MAX_PUDDLES, SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "blood_puddles_buf");
  perGroupParametersBuf = dag::buffers::create_persistent_cb(BLOOD_DECAL_GROUPS_COUNT, "blood_decal_params");

  const char *puddleTexName = atlas_settings.getMemberOr<const char *>("baseTex", nullptr);
  puddleTex = dag::get_tex_gameres(puddleTexName, "blood_puddle_tex");

  const char *flowmapTexName = atlas_settings.getMemberOr<const char *>("flowmapTex", nullptr);
  flowmapTex = dag::get_tex_gameres(flowmapTexName, "blood_puddle_flowmap_tex");

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(::get_shader_variable_id("blood_puddle_tex_samplerstate", true), d3d::request_sampler(smpInfo));
    ShaderGlobal::set_sampler(::get_shader_variable_id("blood_puddle_flowmap_tex_samplerstate", true), d3d::request_sampler(smpInfo));
  }

  const Point2 atlasSize = atlas_settings.getMemberOr("atlasSize", Point2(1, 1));
  ShaderGlobal::set_color4(::get_shader_variable_id("blood_decals_atlas_size", true), atlasSize.x, atlasSize.y, 0, 0);
  ShaderGlobal::set_real(::get_shader_variable_id("blood_puddle_depth", true), 0.03f);
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

void BloodPuddles::initGroupVariantsRanges(const ecs::Object &group_attributes)
{
  const ecs::Object *footprintsAttrs = group_attributes.getNullable<ecs::Object>(group_name_hashes[BLOOD_DECAL_GROUP_FOOTPRINT]);
  if (!footprintsAttrs)
  {
    logerr("BloodPuddles: missed footprint:object in blood_puddles_settings__groupAttributes");
    return;
  }

  const ecs::List<int> *footprintVariantsCounts = footprintsAttrs->getNullable<ecs::List<int>>(ECS_HASH("variantsCounts"));
  const int footprintVariantRangesCount = footprintVariantsCounts ? footprintVariantsCounts->size() : 0;

  int footprintsVariantsCount = 0;
  footprintsRanges.clear();
  footprintsRanges.reserve(footprintVariantRangesCount);
  for (int i = 0; i < footprintVariantRangesCount; i++)
  {
    const int footprintsGroupVariants = (*footprintVariantsCounts)[i];
    footprintsRanges.emplace_back(0, footprintsGroupVariants - 1);
    footprintsVariantsCount += footprintsGroupVariants;
  }

  for (int i = 0; i < groupNames.size(); ++i)
  {
    int decalGroupVariants = 0;
    if (i == BLOOD_DECAL_GROUP_FOOTPRINT)
      decalGroupVariants = footprintsVariantsCount;
    else
    {
      const ecs::Object *attrs = group_attributes.getNullable<ecs::Object>(group_name_hashes[i]);
      if (!attrs)
      {
        logerr("BloodPuddles: missed %s:object in blood_puddles_settings__groupAttributes", groupNames[i]);
        return;
      }

      decalGroupVariants = attrs->getMemberOr(ECS_HASH("variantsCount"), 4);
    }

    variantsRanges[i] = IPoint2(0, decalGroupVariants - 1);
  }
  for (int i = 1; i < groupNames.size(); ++i)
    variantsRanges[i] += IPoint2(variantsRanges[i - 1].y + 1, variantsRanges[i - 1].y + 1);

  if (footprintsRanges.size())
    footprintsRanges[0] += IPoint2(variantsRanges[BLOOD_DECAL_GROUP_FOOTPRINT].x, variantsRanges[BLOOD_DECAL_GROUP_FOOTPRINT].x);
  for (int i = 1; i < footprintsRanges.size(); i++)
    footprintsRanges[i] += IPoint2(footprintsRanges[i - 1].y + 1, footprintsRanges[i - 1].y + 1);
}

void BloodPuddles::initGroupAttributes(const ecs::Object &group_attributes)
{
  initGroupVariantsRanges(group_attributes);

  for (int i = 0; i < BLOOD_DECAL_GROUPS_COUNT; ++i)
  {
    const ecs::Object *group = group_attributes.getNullable<ecs::Object>(group_name_hashes[i]);
    if (!group)
    {
      logerr("BloodPuddles: missed %s:object in blood_puddles_settings__groupAttributes", groupNames[i]);
      return;
    }

    const float appearTime = group->getMemberOr(ECS_HASH("appearTime"), 10.0f);

    if (i == BLOOD_DECAL_GROUP_FOOTPRINT)
    {
      footprintStrengthDecayPerStep = group->getMemberOr(ECS_HASH("strengthDecayPerStep"), 1.0f);
      footprintMinStrength = group->getMemberOr(ECS_HASH("minStrength"), 0.2f);
    }
    else if (i == BLOOD_DECAL_GROUP_SPLASH)
    {
      splashFxTemplName = group->getMemberOr<const char *>(ECS_HASH("effectTemplateName"), nullptr);
      if (splashFxTemplName.empty())
        logerr("effectTemplateName is expected to not be empty in blood_puddles_settings");
      maxSplashesPerFrame = group->getMemberOr("maxEffectsPerFrame", 2);
    }
    else if (i == BLOOD_DECAL_GROUP_PUDDLE)
    {
      const float stayFreshTime = group->getMemberOr(ECS_HASH("stayFreshTime"), 1.0f);
      puddleFreshnessTime = appearTime + stayFreshTime;
    }
    else if (i == BLOOD_DECAL_GROUP_EXPLOSION)
    {
      explosionMaxVerticalThreshold = group->getMemberOr(ECS_HASH("maxVerticalThreshold"), 0.7f);
    }

    groupDists[i] = group->getMemberOr(ECS_HASH("minDistance"), 0.25f);
    sizeRanges[i] = group->getMemberOr(ECS_HASH("sizeRange"), Point2(0.5f, 1.0f));

    float bloodNormalDominanceTerm = clamp(group->getMemberOr(ECS_HASH("bloodNormalDominanceTerm"), 0.0f), 0.0f, 1.0f);
    float edgeWidthScale = clamp(group->getMemberOr(ECS_HASH("edgeWidthScale"), 0.5f), 0.0f, 1.0f);
    perGroupPackedParams[i].invLifetime = 1.0f / appearTime;
    perGroupPackedParams[i].bloodNormalDominanceTerm_edgeWidthScale =
      (uint32_t(edgeWidthScale * 255.0f) << 8u) | (uint32_t(bloodNormalDominanceTerm * 255.0f));
  }

  perGroupParametersBuf.getBuf()->updateData(0, sizeof(perGroupPackedParams[0]) * perGroupPackedParams.size(),
    perGroupPackedParams.data(), VBLOCK_DISCARD);
}

static void reset_blood_es(const AfterDeviceReset &)
{
  if (get_blood_puddles_mgr())
    get_blood_puddles_mgr()->reset();
}

ECS_TAG(render)
ECS_TRACK(*)
static void update_blood_puddles_group_settings_es(const ecs::Event &, const ecs::Object &blood_puddles_settings__groupAttributes)
{
  if (BloodPuddles *mgr = get_blood_puddles_mgr())
    mgr->initGroupAttributes(blood_puddles_settings__groupAttributes);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void update_blood_puddles_shader_params_es(const ecs::Event &, const ecs::Object &blood_puddles_settings__visualAttributes)
{
  const float startSize = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("startSize"), 0.05f);
  const Point3 colorAtHighIntensity =
    blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("colorAtHighIntensity"), Point3(0.14f, 0.0f, 0.0f));
  const Point3 colorAtLowIntensity =
    blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("colorAtLowIntensity"), Point3(0.07f, 0.0f, 0.0f));
  const float reflectance = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("reflectance"), 0.15f);
  const float smoothness = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("smoothness"), 1.0f);
  const float smoothnessOnEdge = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("smoothnessOnEdge"), 1.0f);
  const float landscapeReflectance = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("landscapeReflectance"), 0.21f);
  const float landscapeSmoothnessEdge =
    blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("landscapeSmoothnessEdge"), 0.4f);
  const float centerAlbedoDarkening = blood_puddles_settings__visualAttributes.getMemberOr(ECS_HASH("centerAlbedoDarkening"), 0.5f);

  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_start_size"), startSize);
  ShaderGlobal::set_color4(get_shader_variable_id("blood_puddle_high_intensity_color"), colorAtHighIntensity, 1.0f);
  ShaderGlobal::set_color4(get_shader_variable_id("blood_puddle_low_intensity_color"), colorAtLowIntensity, 1.0f);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_reflectance"), reflectance);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_smoothness"), smoothness);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_smoothness_edge"), smoothnessOnEdge);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_landscape_reflectance"), landscapeReflectance);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_landscape_smoothness_edge"), landscapeSmoothnessEdge);
  ShaderGlobal::set_real(get_shader_variable_id("blood_puddle_center_albedo_darkening"), centerAlbedoDarkening);
}
