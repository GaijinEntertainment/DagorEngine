// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <render/dynmodelRenderer/animCharRenderAdditionalData.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dynamic_material_params.hlsli>
#include "shaders/metatex_const.hlsli"
#include <generic/dag_carray.h>
#include <render/world/animCharRenderUtil.h>
#include <rendInst/rendInstGen.h>
#include <game/gameEvents.h>
#include <EASTL/hash_set.h>


ECS_TAG(render)
static void additional_data_for_plane_cutting_es(const UpdateStageInfoBeforeRender &,
  int plane_cutting__setId,
  bool plane_cutting__cutting,
  bool plane_cutting__orMode,
  ecs::Point4List &additional_data)
{
  int cutting_pos = animchar_additional_data::request_space<AAD_PLANE_CUTTING>(additional_data, 1);
  additional_data[cutting_pos].x = float(max(plane_cutting__setId, 0));
  additional_data[cutting_pos].y = float(plane_cutting__cutting && plane_cutting__setId >= 0 ? 1.f : 0.f);
  additional_data[cutting_pos].z = float(plane_cutting__orMode ? 1.f : 0.f);
}

ECS_TAG(render)
static void additional_data_for_tracks_es(const UpdateStageInfoBeforeRender &,
  const Point2 &vehicle_tracks_visual_pos,
  const Point2 &vehicle_tracks_visual_pos_delta,
  ecs::Point4List &additional_data)
{
  int start_pos = animchar_additional_data::request_space<AAD_VEHICLE_TRACKS>(additional_data, 1);
  additional_data[start_pos] = Point4(vehicle_tracks_visual_pos.x, vehicle_tracks_visual_pos.y, vehicle_tracks_visual_pos_delta.x,
    vehicle_tracks_visual_pos_delta.y);
}

static Point2 get_camo_scale_and_rotation_vars(float scale, float rotation)
{
  float rotationSin, rotationCos;
  sincos(rotation, rotationSin, rotationCos);
  float camoScale = 2.f / (4096.f * scale);
  return Point2(camoScale * rotationSin, camoScale * rotationCos);
}

ECS_TAG(render)
static void additional_data_for_vehicle_camo_es(const UpdateStageInfoBeforeRender &,
  float vehicle_camo_condition,
  float vehicle_camo_scale,
  float vehicle_camo_rotation,
  ecs::Point4List &additional_data)
{
  int start_pos = animchar_additional_data::request_space<AAD_VEHICLE_CAMO>(additional_data, 1);
  Point2 camoVars = get_camo_scale_and_rotation_vars(vehicle_camo_scale, vehicle_camo_rotation);
  additional_data[start_pos] = Point4(1.0f / max(vehicle_camo_condition, 0.001f), camoVars.x, camoVars.y, 0);
}

ECS_TAG(render)
ECS_TRACK(vehicle_camo_condition, vehicle_camo_scale, vehicle_camo_rotation)
ECS_REQUIRE(float vehicle_camo_condition, float vehicle_camo_scale, float vehicle_camo_rotation)
static void on_vehicle_camo_changed_es(const ecs::Event &, ecs::EntityId eid) { mark_animchar_for_reactive_mask_pass(eid); }

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
ECS_REQUIRE(ecs::Tag needModelMatrix)
static void additional_data_copy_model_tm_es(
  const UpdateStageInfoBeforeRender &, const AnimV20::AnimcharRendComponent &animchar_render, ecs::Point4List &additional_data)
{
  int model_pos = animchar_additional_data::request_space<AAD_MODEL_TM>(additional_data, 3);
  const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  const TMatrix &tm = scene->getNodeWtm(0);
  additional_data[model_pos] = Point4(tm.getcol(0).x, tm.getcol(1).x, tm.getcol(2).x, -tm.getcol(3).x);
  additional_data[model_pos + 1] = Point4(tm.getcol(0).y, tm.getcol(1).y, tm.getcol(2).y, -tm.getcol(3).y);
  additional_data[model_pos + 2] = Point4(tm.getcol(0).z, tm.getcol(1).z, tm.getcol(2).z, -tm.getcol(3).z);
}


struct DynMatInfo
{
  DynMatType type;
  uint32_t param_cnt;
};
// TODO: maybe use ECS enum instead
static ska::flat_hash_map<ecs::hash_str_t, DynMatInfo> dynmat_info_map;

static void init_dynmat_info_map()
{
  G_FAST_ASSERT(dynmat_info_map.empty());
  dynmat_info_map.insert(eastl::make_pair(ECS_HASH("emissive").hash, DynMatInfo{DynMatType::DMT_EMISSIVE, DynMatEmissive::DMEM_CNT}));
  dynmat_info_map.insert(eastl::make_pair(ECS_HASH("metatex").hash, DynMatInfo{DynMatType::DMT_METATEX, METATEX_PARAMS_COUNT}));
}

static void fill_dynmat_params_emissive(const ecs::Object &params_channel, int offset, ecs::Point4List &additional_data)
{
  additional_data[offset - DynMatEmissive::DMEM_EMISSIVE_COLOR] =
    params_channel[ECS_HASH("dynmat_param__emissive_color")].getOr<Point4>({1, 1, 1, 0});
  additional_data[offset - DynMatEmissive::DMEM_EMISSION_ALBEDO_MULT].x =
    params_channel[ECS_HASH("dynmat_param__emission_albedo_mult")].getOr<real>({1});
}
static void fill_dynmat_params_metatex(const ecs::Object &params_channel, int offset, ecs::Point4List &additional_data)
{
#define SET_P4(i, component)   additional_data[offset - i] = params_channel[ECS_HASH(component)].getOr<Point4>({0.0, 0.0, 0.0, 0.0});
#define SET_R(i, c, component) additional_data[offset - i].c = params_channel[ECS_HASH(component)].getOr<real>(0.0);

  // TODO: maybe separate types, for swarm too
  if (!params_channel.empty())
  {
    SET_P4(0, "dynmat_param__color_black_from");
    SET_P4(1, "dynmat_param__color_black_to");
    SET_P4(2, "dynmat_param__color_red_from");
    SET_P4(3, "dynmat_param__color_red_to");
    SET_P4(4, "dynmat_param__color_green_from");
    SET_P4(5, "dynmat_param__color_green_to");
    SET_P4(6, "dynmat_param__color_blue_from");
    SET_P4(7, "dynmat_param__color_blue_to");
    SET_P4(8, "dynmat_param__color_alpha_from");
    SET_P4(9, "dynmat_param__color_alpha_to");

    SET_R(10, x, "dynmat_param__emissive_black_from");
    SET_R(10, y, "dynmat_param__emissive_black_to");
    SET_R(10, z, "dynmat_param__emissive_red_from");
    SET_R(10, w, "dynmat_param__emissive_red_to");
    SET_R(11, x, "dynmat_param__emissive_green_from");
    SET_R(11, y, "dynmat_param__emissive_green_to");
    SET_R(11, z, "dynmat_param__emissive_blue_from");
    SET_R(11, w, "dynmat_param__emissive_blue_to");
    SET_R(12, x, "dynmat_param__emissive_alpha_from");
    SET_R(12, y, "dynmat_param__emissive_alpha_to");
    SET_P4(13, "dynmat_param__emissive_color");
    SET_R(14, x, "dynmat_param__albedo_blend_for_emission");

    SET_R(15, x, "dynmat_param__opacity_black_from");
    SET_R(15, y, "dynmat_param__opacity_black_to");
    SET_R(15, z, "dynmat_param__opacity_red_from");
    SET_R(15, w, "dynmat_param__opacity_red_to");
    SET_R(16, x, "dynmat_param__opacity_green_from");
    SET_R(16, y, "dynmat_param__opacity_green_to");
    SET_R(16, z, "dynmat_param__opacity_blue_from");
    SET_R(16, w, "dynmat_param__opacity_blue_to");
    SET_R(17, x, "dynmat_param__opacity_alpha_from");
    SET_R(17, y, "dynmat_param__opacity_alpha_to");
  }

#undef SET_P4
#undef SET_R
}


static DynMatInfo get_dynmat_info_from_string(const char *str)
{
  if (DAGOR_UNLIKELY(dynmat_info_map.empty()))
    init_dynmat_info_map(); // lazy init

  auto it = dynmat_info_map.find(ECS_HASH_SLOW(str).hash);
  return it != dynmat_info_map.end() ? it->second : DynMatInfo{DynMatType::DMT_INVALID, 0U};
}

static float pack_4_ubyte_to_float(const uint32_t data[4])
{
  uint32_t resultI = 0;
  for (int i = 0; i < 4; ++i)
    resultI |= (data[i] & 0xFF) << (i * 8);
  return bitwise_cast<float>(resultI);
}

static constexpr int DYNMAT_MAX_CHANNEL_CNT = 8; // float4 -> 32*4 bits -> 8 bits for each {8 type + 8 offset}
static constexpr int DYNMAT_OFFSET_BITS = 8;
static constexpr int DYNMAT_PARAMS_START = 1; // first entry is metadata
static constexpr int DYNMAT_MAX_PARAM_CNT = (1 << DYNMAT_OFFSET_BITS) - DYNMAT_PARAMS_START;

static Point4 pack_dynamic_material_param_metadata(int channel_cnt,
  const carray<uint32_t, DYNMAT_MAX_CHANNEL_CNT> &param_offset_per_channel,
  const carray<uint32_t, DYNMAT_MAX_CHANNEL_CNT> &material_type_per_channel)
{
  Point4 result = Point4::ZERO;
  result.x = pack_4_ubyte_to_float(param_offset_per_channel.data());
  result.y = pack_4_ubyte_to_float(material_type_per_channel.data());
  if (channel_cnt > 4)
  {
    result.z = pack_4_ubyte_to_float(param_offset_per_channel.data() + 4);
    result.w = pack_4_ubyte_to_float(material_type_per_channel.data() + 4);
  }
  return result;
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void validate_dynamic_material_params_es_event_handler(const ecs::Event &, const ecs::Array &dynamic_material_channels_arr)
{
  int channelCnt = dynamic_material_channels_arr.size();

  if (channelCnt > DYNMAT_MAX_CHANNEL_CNT)
  {
    logerr("dynamic material channel count is too high: %d > %d", channelCnt, DYNMAT_MAX_CHANNEL_CNT);
    channelCnt = DYNMAT_MAX_CHANNEL_CNT;
  }

  int paramCountSum = DYNMAT_PARAMS_START;
  const ecs::Object defObj;
  for (int i = 0; i < channelCnt; ++i)
  {
    const ecs::Object &paramsChannel = dynamic_material_channels_arr[i].getOr<ecs::Object>(defObj);
    const char *dynmatTypeStr = paramsChannel[ECS_HASH("dynamic_material_type")].getOr("");

    DynMatInfo dynMatInfo = get_dynmat_info_from_string(dynmatTypeStr);
    if (DAGOR_UNLIKELY(dynMatInfo.type == DynMatType::DMT_INVALID))
      logerr("Invalid dynamic material type: %s", *dynmatTypeStr ? dynmatTypeStr : "<not set>");

    // can't really check it since metatex has varying param cnt // TODO: separate metatex types first, for swarm too
    // int paramCnt = paramsChannel.size() - 1; // material type is first
    // if (paramCnt != dynMatInfo.param_cnt)
    //   logerr("dynamic material parameter count mismatch: %d != %d", paramCnt, dynMatInfo.param_cnt);

    paramCountSum += dynMatInfo.param_cnt;
  }

  if (paramCountSum > DYNMAT_MAX_PARAM_CNT)
    logerr("dynamic material parameter count is too high: %d > %d", paramCountSum, DYNMAT_MAX_PARAM_CNT);
}


ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void additional_data_for_dynamic_material_params_es(
  const UpdateStageInfoBeforeRender &, const ecs::Array &dynamic_material_channels_arr, ecs::Point4List &additional_data)
{
  int channelCnt = min(dynamic_material_channels_arr.size(), (uint32_t)DYNMAT_MAX_CHANNEL_CNT);

  int paramCountSum = DYNMAT_PARAMS_START;
  carray<uint32_t, DYNMAT_MAX_CHANNEL_CNT> paramOffsetPerChannel;
  carray<uint32_t, DYNMAT_MAX_CHANNEL_CNT> materialTypePerChannel;
  carray<uint32_t, DYNMAT_MAX_CHANNEL_CNT> materialParamCountPerChannel;
  const ecs::Object defObj;
  for (int i = 0; i < channelCnt; ++i)
  {
    const ecs::Object &paramsChannel = dynamic_material_channels_arr[i].getOr<ecs::Object>(defObj);
    const char *dynmatTypeStr = paramsChannel[ECS_HASH("dynamic_material_type")].getOr("");

    DynMatInfo dynMatInfo = get_dynmat_info_from_string(dynmatTypeStr);
    materialTypePerChannel[i] = dynMatInfo.type;
    materialParamCountPerChannel[i] = dynMatInfo.param_cnt;
    paramOffsetPerChannel[i] = paramCountSum;
    paramCountSum += dynMatInfo.param_cnt;
  }
  for (int i = channelCnt; i < DYNMAT_MAX_CHANNEL_CNT; ++i)
  {
    materialTypePerChannel[i] = static_cast<uint32_t>(DynMatType::DMT_INVALID); // for validation
    paramOffsetPerChannel[i] = 0;
  }

  if (paramCountSum > DYNMAT_MAX_PARAM_CNT)
    return; // no reasonable fallback, but it can only happen in extreme cases

  int offset = animchar_additional_data::request_space<AAD_MATERIAL_PARAMS>(additional_data, paramCountSum);
  offset += paramCountSum - 1; // we fill additional_data in reverse order

  // first float4 is metadata
  additional_data[offset--] = pack_dynamic_material_param_metadata(channelCnt, paramOffsetPerChannel, materialTypePerChannel);

  for (size_t i = 0; i < channelCnt; ++i)
  {
    const ecs::Object &paramsChannel = dynamic_material_channels_arr[i].getOr<ecs::Object>(defObj);
    DynMatType dynMatType = static_cast<DynMatType>(materialTypePerChannel[i]);
    switch (dynMatType)
    {
      case DynMatType::DMT_EMISSIVE: fill_dynmat_params_emissive(paramsChannel, offset, additional_data); break;
      case DynMatType::DMT_METATEX: fill_dynmat_params_metatex(paramsChannel, offset, additional_data); break;
      default: break; // to keep the compiler happy
    }
    offset -= materialParamCountPerChannel[i];
  }
}

#if DAGOR_DBGLEVEL > 0

static int get_character_micro_details_count()
{
  static int cachedCount = 0;
  if (cachedCount > 0) // currently, the microdetail tex doesn't change, so it is safe to cache the count
    return cachedCount;

  static const ShaderVariableInfo microDetailsTexVarId("character_micro_details", true);
  const TEXTUREID texId = microDetailsTexVarId.get_var_id() >= 0 ? microDetailsTexVarId.get_texture() : BAD_TEXTUREID;
  if (texId == BAD_TEXTUREID)
    return 0;
  if (BaseTexture *tex = acquire_managed_tex(texId))
  {
    TextureInfo ti;
    tex->getinfo(ti);
    cachedCount = ti.a;
  }
  release_managed_tex(texId);
  return cachedCount;
}

static void validate_material_micro_detail_layer(const ShaderMaterial *mat, int micro_detail_count, const char *res_name)
{
  if (micro_detail_count <= 0)
    return;

  static const ShaderVariableInfo layer0VarId("micro_detail_layer", true);
  static const ShaderVariableInfo layer1VarId("micro_detail_layer1", true);
  static const ShaderVariableInfo layer2VarId("micro_detail_layer2", true);
  const ShaderVariableInfo *const layerVars[] = {&layer0VarId, &layer1VarId, &layer2VarId};

  for (const ShaderVariableInfo *layerVar : layerVars)
  {
    int layer = -1;
    if (layerVar->get_var_id() >= 0 && mat->getIntVariable(layerVar->get_var_id(), layer) && layer >= micro_detail_count)
      logerr("micro_detail_layer %d is out of range [0..%d) in shader '%s' of asset '%s'; "
             "fix the asset material or add the missing entry to character_micro_details.blk",
        layer, micro_detail_count, mat->getShaderClassName(), res_name);
  }
}

static bool material_uses_cloth_wind(const ShaderMaterial *mat)
{
  const char *shaderName = mat->getShaderClassName();
  if (strcmp(shaderName, "dynamic_cloth") == 0)
    return true;
  if (strcmp(shaderName, "dynamic_sheen_camo") == 0)
  {
    static const ShaderVariableInfo clothWindEnabledVarId("cloth_wind_enabled", true);
    int clothWindEnabled = 0;
    if (clothWindEnabledVarId.get_var_id() >= 0 && mat->getIntVariable(clothWindEnabledVarId.get_var_id(), clothWindEnabled) &&
        clothWindEnabled != 0)
      return true;
  }
  return false;
}

static void validate_animchar_render_data(const DynamicRenderableSceneInstance *scene,
  const char *res_name,
  const ecs::Point4List *additional_data,
  bool has_need_model_velocity)
{
  if (!scene)
    return;
  const DynamicRenderableSceneLodsResource *lodsRes = scene->getLodsResource();
  if (!lodsRes)
    return;

  static eastl::hash_set<uintptr_t> validated; // cheap test, good enough for validation when the shader asserts
  if (!validated.insert((uintptr_t)lodsRes).second)
    return;

  const int microDetailCount = get_character_micro_details_count();

  bool usesClothWind = false;
  Tab<ShaderMaterial *> matList(framemem_ptr());
  lodsRes->gatherUsedMat(matList);
  for (const ShaderMaterial *mat : matList)
  {
    validate_material_micro_detail_layer(mat, microDetailCount, res_name);
    usesClothWind = usesClothWind || material_uses_cloth_wind(mat);
  }

  if (usesClothWind && !(additional_data && has_need_model_velocity))
    logerr("animchar '%s' renders a cloth wind shader but is missing %s; AAD_CLOTH_WIND_PARAMS stays empty and the "
           "shader asserts. Add _use:t=\"cloth_wind\" (and animchar_additional_data) to its template.",
      res_name, !additional_data ? "the additional_data component" : "the needModelVelocity tag");
}

static void validate_rendinst_material(ShaderMaterial *mat, const char *res_name)
{
  if (!mat)
    return;
  static eastl::hash_set<uintptr_t> validated;
  if (!validated.insert((uintptr_t)mat).second) // cheap test, good enough for validation when the shader asserts
    return;

  static const ShaderVariableInfo paletteIndexVarId("palette_index", true);
  int paletteIndex = 0;
  // palette_index selects one of two merged paint palettes, so only 0 and 1 are valid (the shader asserts palette_index <= 1)
  if (paletteIndexVarId.get_var_id() >= 0 && mat->getIntVariable(paletteIndexVarId.get_var_id(), paletteIndex) && paletteIndex > 1)
    logerr("palette_index %d is out of range [0..1] in shader '%s' of rendinst '%s'; fix the asset paint material", paletteIndex,
      mat->getShaderClassName(), res_name);

  validate_material_micro_detail_layer(mat, get_character_micro_details_count(), res_name);
}
#endif

ECS_TAG(render, dev)
ECS_ON_EVENT(on_appear)
static void animchar_validate_render_data_es(const ecs::Event &,
  const AnimV20::AnimcharRendComponent &animchar_render,
  const AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Point4List *additional_data = nullptr,
  const ecs::Tag *needModelVelocity = nullptr)
{
#if DAGOR_DBGLEVEL > 0
  validate_animchar_render_data(animchar_render.getSceneInstance(), animchar.getCreateInfo()->resName.c_str(), additional_data,
    needModelVelocity != nullptr);
#else
  G_UNUSED(animchar_render);
  G_UNUSED(animchar);
  G_UNUSED(additional_data);
  G_UNUSED(needModelVelocity);
#endif
}

ECS_ON_EVENT(EventOnGameInit)
static void rendinst_palette_validation_init_es(const ecs::Event &)
{
#if DAGOR_DBGLEVEL > 0
  rendinst::shader_material_validation_cb = &validate_rendinst_material;
#endif
}
