// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_gameResources.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <daFx/dafx.h>
#include <daECS/core/event.h>

#include "fxErrorType.h"

class FXGunFire;
class AcesEffect;
class DataBlock;
class BaseTexture;
class Sbuffer;
struct Driver3dPerspective;

enum FxRenderGroup
{
  FX_RENDER_GROUP_LOWRES = 0,
  FX_RENDER_GROUP_HIGHRES = 1,
  FX_RENDER_GROUP_HAZE = 2,
  FX_RENDER_GROUP_WATER = 3,
  FX_RENDER_GROUP_WATER_PROJ = 4,
  FX_RENDER_GROUP_THERMAL = 5,
  FX_RENDER_GROUP_UNDERWATER = 6
};

enum FxRenderTargetOverride
{
  FX_RT_OVERRIDE_DEFAULT = 0,
  FX_RT_OVERRIDE_LOWRES = 1,
  FX_RT_OVERRIDE_HIGHRES = 2,
};

enum FxQuality
{
  FX_QUALITY_LOW = 1 << 0,
  FX_QUALITY_MEDIUM = 1 << 1,
  FX_QUALITY_HIGH = 1 << 2,
  FX_QUALITY_ALL = 0xFFFFFFFF,
};

enum FxResolutionSetting
{
  FX_LOW_RESOLUTION = 0,
  FX_MEDIUM_RESOLUTION = 1,
  FX_HIGH_RESOLUTION = 2,
};

enum
{
  HUID_ACES_DISTRIBUTE_MODE = 0xA671B023u
};
enum
{
  HUID_ACES_ALPHA_HEIGHT_RANGE = 0xBA2908F1u
};
enum
{
  HUID_ACES_RESET = 0xA781BF24u
};
enum
{
  HUID_ACES_IS_ACTIVE = 0xD6872FCEu
};

namespace acesfx
{
struct GravityZoneWithDistanceToCamera
{
  TMatrix transform;
  Point3 size;
  uint32_t shape;
  uint32_t type;
  float sqDistanceToCamera;
  bool isImportant;

  GravityZoneWithDistanceToCamera(
    const TMatrix &transform_, const Point3 &size_, uint32_t shape_, uint32_t type_, float sq_distance_to_camera, bool is_important) :
    transform(transform_),
    size(size_),
    shape(shape_),
    type(type_),
    sqDistanceToCamera(sq_distance_to_camera),
    isImportant(is_important)
  {}
};
using GravityZoneBuffer = dag::RelocatableFixedVector<GravityZoneWithDistanceToCamera, /*GRAVITY_ZONE_COUNT*/ 8, false>;
void push_gravity_zone(GravityZoneBuffer &buffer,
  const TMatrix &transform,
  const Point3 &size,
  uint32_t shape,
  uint32_t type,
  float sq_distance_to_camera,
  bool is_important);

extern int softness_depth_rcp_var_id;
static const unsigned do_not_render_fx_managers = -1;
extern int landCrashFireEffectTime;
extern int landCrashFireEffectTimeForTanks;

// clear only internally used slots
void use_relaxed_tex_slot_clears();
bool init_dafx(bool gpu_sim);
void term_dafx();
dafx::ContextId get_dafx_context();
dafx::CullingId get_cull_id();
dafx::CullingId get_cull_fom_id();
dafx::CullingId get_cull_bvh_id();
void setDepthTex(const BaseTexture *tex);
void setNormalsTex(const BaseTexture *tex);
void setSkyParams(const Point3 &sun_dir, const Color3 &sun_color, const Color3 &sky_color);
void setWindParams(float wind_strength, Point2 wind_dir);
void set_gravity_zones(GravityZoneBuffer &buffer);

void init(const DataBlock &fx);
void shutdown();
void reset();
void draw_debug_opt(const TMatrix4 &glob_tm);
FxResolutionSetting getFxResolutionSetting();
FxQuality getFxQualityMask();

void set_rt_override(FxRenderTargetOverride v);
FxRenderTargetOverride get_rt_override();

void set_quality_mask(FxQuality fx_quality);
void set_default_mip_bias(float mip_bias = 0.f);

struct StartEffectEvent : public ecs::Event
{
  Point3 pos;
  AcesEffect *fx;
  int fxType;

  ECS_BROADCAST_EVENT_DECL(StartEffectEvent)
  StartEffectEvent(int fx_type, AcesEffect *fx_, const Point3 &pos_) :
    ECS_EVENT_CONSTRUCTOR(StartEffectEvent), pos(pos_), fx(fx_), fxType(fx_type)
  {}
};

ECS_BROADCAST_EVENT_TYPE(StartEffectPosNormEvent, int /*type*/, Point3 /*pos*/, Point3 /*norm*/, bool /*is_player*/, float /*scale*/);

struct SoundDesc
{
  const char *parameter = nullptr;
  float value = 0.f;
  SoundDesc() = default;
  explicit SoundDesc(const char *parameter, float value) : parameter(parameter), value(value) {}
};
static const SoundDesc defSoundDesc;

AcesEffect *start_effect(int type,
  const TMatrix &emitter_tm,
  const TMatrix &fx_tm,
  bool is_player,
  const SoundDesc *snd_desc = &defSoundDesc,
  FxErrorType *perr = nullptr);

AcesEffect *start_effect_pos(
  int type, const Point3 &pos, bool is_player, float scale = 1.f, const SoundDesc *snd_desc = &defSoundDesc);

AcesEffect *start_effect_pos_norm(
  int type, const Point3 &pos, const Point3 &norm, bool is_player, float scale = 1.f, const SoundDesc *snd_desc = &defSoundDesc);

TMatrix create_matrix_from_pos_norm(const Point3 &pos, const Point3 &norm);

AcesEffect *start_effect_fxtm(int type, TMatrix tm, bool is_player, float scale = 1.f, const SoundDesc *snd_desc = &defSoundDesc);

bool prefetch_effect(int type);

void stop_effect(AcesEffect *&fx);
void kill_effect(AcesEffect *&fx);

bool can_start_effect(int type, bool is_player);
void start_all_effects(const Point3 &pos, float rad, int mul, float rad_step);

void start_update_prepare(float dt, const Driver3dPerspective &persp, int targetWidth, int targetHeight);
void start_update(float dt);

void finish_update(const TMatrix4 &tm);
void before_render();
void prepare_main_culling(const TMatrix4 &tm);
void prepare_fom_culling(const TMatrix4 &tm);
void prepare_bvh_culling(const TMatrix4 &tm);
void before_reset();
void after_reset();
void async_update_started();
void async_update_finished();

bool renderTransLowRes(dafx::CullingId cull_id = dafx::CullingId());
bool renderTransHighRes(dafx::CullingId cull_id = dafx::CullingId());
bool renderTransThermal(dafx::CullingId cull_id = dafx::CullingId());
bool renderUnderwater(dafx::CullingId cull_id = dafx::CullingId());
bool renderTransSpecial(uint8_t render_tag, dafx::CullingId cull_id = dafx::CullingId());
void renderTransHaze();
void renderTransWaterFoam();
void renderTransWaterProj(const TMatrix4 &view, const TMatrix4 &proj, const Point3 &pos, float mip_bias);
void renderToGbuffer(int global_frame_id);

void killEffectsInSphere(const BSphere3 &bsph);

bool hasVisibleHaze();


void create_ground_spot(TEXTUREID in_texture_1_id, TEXTUREID in_texture_2_id, int in_type, const Point2 &in_pos, float in_radius);

void dump_statistics();
void get_stats_by_fx_name(const char *name, eastl::string &o);

const char *get_fxres_name(const char *fx_name);
int get_type_by_name(const char *name, bool optional = false);
int get_and_init_type_by_name(const char *name);
const char *get_name_by_type(int type);
void set_dafx_globaldata(const TMatrix4 &tm, const TMatrix &view, const Point3 &view_pos);

float get_effect_life_time(int type);
FxQuality get_fx_target();

bool thermal_vision_on();

class UniqueCullingId
{
public:
  UniqueCullingId() = default;
  UniqueCullingId(dafx::CullingId refId_) : refId(refId_) {}
  UniqueCullingId(const UniqueCullingId &) = delete;
  UniqueCullingId &operator=(const UniqueCullingId &other) = delete;
  UniqueCullingId(UniqueCullingId &&other) { *this = eastl::move(other); }
  UniqueCullingId &operator=(UniqueCullingId &&other)
  {
    reset();
    refId = eastl::exchange(other.refId, dafx::CullingId());
    return *this;
  }
  ~UniqueCullingId() { reset(); }

  void reset()
  {
    if (refId)
    {
      dafx::destroy_culling_state(get_dafx_context(), refId);
      refId = dafx::CullingId();
    }
  }
  dafx::CullingId get() const { return refId; }

private:
  dafx::CullingId refId = dafx::CullingId();
};

}; // namespace acesfx
