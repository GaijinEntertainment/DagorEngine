// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasEvent.h>
#include <daECS/net/network.h>
#include "net/net.h"
#include "net/time.h"
#include "net/dedicated.h"
#include "main/app.h"
#include "main/appProfile.h"
#include <ecs/scripts/netsqevent.h>
#include "net/recipientFilters.h"
#include "game/gameScripts.h"
#include "phys/gridCollision.h"
#include "phys/netPhys.h"
#include "phys/lagCompensation.h"
#include "render/skies.h"
#include "render/fx/fx.h"
#include "render/fx/effectEntity.h"
#include <levelSplines/levelSplines.h>
#include "main/gameProjConfig.h"

const char *gameproj::game_telemetry_name() { return nullptr; }

float dagor_game_act_time = 0.f;

class StubTimeManager final : public ITimeManager
{
  double advance(float dt, float &) override { return 0.0; }
  double getSeconds() const override { return 0.0; }
  int getMillis() const override { return 0; }
  int getAsyncMillis() const override { return 0; }
  int getType4CC() const override { return 0; }
};

ITimeManager &get_time_mgr()
{
  G_ASSERT(0);
  static StubTimeManager stub;
  return stub;
}
float get_sync_time() { G_ASSERT_RETURN(false, 0.); }

std::uint32_t get_current_server_route_id() { G_ASSERT_RETURN(false, {}); }
const char *get_server_route_host(std::uint32_t) { G_ASSERT_RETURN(false, {}); }
std::uint32_t get_server_route_count() { G_ASSERT_RETURN(false, {}); }
void switch_server_route(std::uint32_t) { G_ASSERT(0); }
void send_echo_msg(uint32_t) { G_ASSERT(0); }

bool is_server() { G_ASSERT_RETURN(false, false); }
bool is_true_net_server() { G_ASSERT_RETURN(false, false); }
bool has_network() { G_ASSERT_RETURN(false, false); }
bool dedicated::is_dedicated() { G_ASSERT_RETURN(false, false); }
int get_build_number() { G_ASSERT_RETURN(false, 0); }
net::IConnection *get_server_conn() { G_ASSERT_RETURN(false, nullptr); }
net::IConnection *get_client_connection(int id) { G_ASSERT_RETURN(false, nullptr); }
dag::Span<net::IConnection *> get_client_connections() { G_ASSERT_RETURN(false, {}); }
void net_disconnect(net::IConnection &, DisconnectionCause) { G_ASSERT(0); }

BasePhysActor *get_phys_actor(ecs::EntityId eid) { G_ASSERT_RETURN(false, nullptr); }
const char *BasePhysActor::getPhysTypeStr() const { return nullptr; }

int send_net_msg(ecs::EntityManager &, ecs::EntityId, net::IMessage &&, const net::MessageNetDesc *) { G_ASSERT_RETURN(false, -1); }
int send_net_msg(ecs::EntityId, net::IMessage &&, const net::MessageNetDesc *) { G_ASSERT_RETURN(false, -1); }

void send_transform_snapshots_targeted_event(ecs::EntityId, danet::BitStream &) { G_ASSERT(0); }
void send_transform_snapshots_event(danet::BitStream &) { G_ASSERT(0); }
void send_reliable_transform_snapshots_event(danet::BitStream &) { G_ASSERT(0); }

float get_timespeed() { G_ASSERT_RETURN(false, 0.f); }
void set_timespeed(float) { G_ASSERT(0); }
void toggle_pause() { G_ASSERT(0); }

#include <gui/dag_visualLog.h>
namespace visuallog
{
void logmsg(const char *text, LogItemCBProc, void *, E3DCOLOR, int) { G_ASSERT(0); }
dag::ConstSpan<SimpleString> getHistory() { G_ASSERT_RETURN(false, {}); }
} // namespace visuallog

float phys_get_timestep() { G_ASSERT_RETURN(false, 0.f); }

#include <ecs/phys/netPhysResync.h>
void ECSCustomPhysStateSyncer::init(ecs::EntityId, int) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, float &) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, int &) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, bool &) {}

#if DAGOR_DBGLEVEL > 0
#include <gamePhys/common/fixed_dt.h>
float gamephys::PHYSICS_UPDATE_FIXED_DT = gamephys::DEFAULT_PHYSICS_UPDATE_FIXED_DT;
#endif
namespace rendinst
{
bool isRiExtraLoaded() { G_ASSERT_RETURN(false, false); }
} // namespace rendinst

#include "main/gameObjects.h"
void traceray_ladders(const Point3 &from, const Point3 &to, const GameObjInstCB &game_obj_inst_cb) { G_ASSERT(0); }

HSQUIRRELVM gamescripts::get_vm() { G_ASSERT_RETURN(false, nullptr); }
void server_send_net_sqevent(ecs::EntityId, ecs::SchemelessEvent &&, dag::Span<net::IConnection *> *) { G_ASSERT(0); }
void server_broadcast_net_sqevent(ecs::SchemelessEvent &&, dag::Span<net::IConnection *> *) { G_ASSERT(0); }
void client_send_net_sqevent(ecs::EntityId to_eid, const ecs::SchemelessEvent &evt, bool bcastevt) { G_ASSERT(0); }
net::IConnection *rcptf::get_entity_ctrl_conn(ecs::EntityId) { G_ASSERT_RETURN(false, nullptr); }
namespace net
{
void send_dasevent(ecs::EntityManager *,
  bool,
  ecs::EntityId,
  bind_dascript::DasEvent *,
  const char *,
  eastl::optional<dag::ConstSpan<net::IConnection *>>,
  eastl::fixed_function<sizeof(void *), eastl::string()>)
{
  G_ASSERT(0);
}
} // namespace net
net::CNetwork *get_net_internal() { G_ASSERT_RETURN(false, nullptr); }

#include <game/player.h>
namespace game
{
ecs::EntityId get_controlled_hero() { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
ecs::EntityId get_local_player_eid() { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
void set_local_player_eid(ecs::EntityId) { G_ASSERT(0); }

Player *find_player_for_connection(const net::IConnection *) { G_ASSERT_RETURN(false, nullptr); }
ecs::EntityId find_player_eid_for_connection(const net::IConnection *) { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }

const Point3 *player_get_looking_at(ecs::EntityId, const Point3 **) { G_ASSERT_RETURN(false, {}); }

void Player::resetSyncData() { G_ASSERT(0); }
int Player::calcControlsTickDelta(PhysTickRateType, bool) { G_ASSERT_RETURN(false, 0); }
} // namespace game

#include "jsonUtils/jsonUtils.h"
#include "jsonUtils/decodeJwt.h"
namespace jsonutils
{
bool decode_jwt_block(const char *, int, rapidjson::Document &, JwtCompressionType) { G_ASSERT_RETURN(false, false); }
DecodeJwtResult decode_jwt(const char *, const char *, rapidjson::Document &, rapidjson::Document &)
{
  G_ASSERT_RETURN(false, DecodeJwtResult::OK);
}
} // namespace jsonutils

const rapidjson::Document &get_currently_playing_replay_meta_info()
{
  G_ASSERT(0);
  static const rapidjson::Document dummy;
  return dummy;
}
bool load_replay_meta_info(const char *, const eastl::function<void(const rapidjson::Document &)> &) { G_ASSERT_RETURN(false, false); }

void exit_game(const char *) { G_ASSERT(0); }
char const *get_exe_version_str() { G_ASSERT_RETURN(false, nullptr); }
int app_profile_get_app_id() { G_ASSERT_RETURN(false, 0); }
const app_profile::ProfileSettings &app_profile::get()
{
  G_ASSERT(0);
  static app_profile::ProfileSettings dummy;
  return dummy;
}
app_profile::ProfileSettings &app_profile::getRW()
{
  G_ASSERT(0);
  static app_profile::ProfileSettings dummy;
  return dummy;
}
const char *app_profile_get_session_id() { G_ASSERT_RETURN(false, nullptr); }
const char *get_gun_stat_type_by_props_id(int gun_props_id) { G_ASSERT_RETURN(false, nullptr); }
const char *get_shell_template_by_shell_id(int) { G_ASSERT_RETURN(false, nullptr); }
const char *get_gun_template_by_props_id(int) { G_ASSERT_RETURN(false, nullptr); }
Tab<const char *> ecs_get_global_tags_context() { return {}; }

void TheEffect::reset() { G_ASSERT(0); }

class AcesEffect
{
public:
  void unlock();
  void setFxScale(float);
  void setFxTm(const TMatrix &);
  void setEmitterTm(const TMatrix &);
  void setSpawnRate(float);
  void setColorMult(struct Color4 const &);
  void setVelocity(const Point3 &vel);
  void setGravityTm(const Matrix3 &);
  void hide(bool);
};
void AcesEffect::unlock() { G_ASSERT(0); }
void AcesEffect::setFxScale(float) { G_ASSERT(0); }
void AcesEffect::setFxTm(const TMatrix &) { G_ASSERT(0); }
void AcesEffect::setEmitterTm(const TMatrix &) { G_ASSERT(0); }
void AcesEffect::setSpawnRate(float) { G_ASSERT(0); }
void AcesEffect::setColorMult(struct Color4 const &) { G_ASSERT(0); }
void AcesEffect::setVelocity(const Point3 &vel) { G_ASSERT(0); }
void AcesEffect::setGravityTm(const Matrix3 &) { G_ASSERT(0); }
void AcesEffect::hide(bool) { G_ASSERT(0); }
namespace acesfx
{
int get_type_by_name(const char *) { G_ASSERT_RETURN(false, -1); }
FxQuality get_fx_target() { G_ASSERT_RETURN(false, FX_QUALITY_LOW); }
FxQuality getFxQualityMask() { G_ASSERT_RETURN(false, FX_QUALITY_LOW); }
void start_effect(int, const TMatrix &, const TMatrix &, bool, float, AcesEffect **, FxErrorType *) { G_ASSERT(false); }
float get_effect_life_time(int) { G_ASSERT_RETURN(false, 0.0f); }
bool prefetch_effect(int) { G_ASSERT_RETURN(false, false); }
void setup_camera_and_debug(float, const TMatrix &, const Driver3dPerspective &, int, int) { G_ASSERT(0); }
void update_fx_managers(float) { G_ASSERT(0); }
void flush_dafx_commands() { G_ASSERT(0); }
void start_dafx_update(float) { G_ASSERT(0); }
void finish_update_main_camera(const TMatrix4 &tm) { G_ASSERT(0); }
void finish_update(const TMatrix4 &, Occlusion *) { G_ASSERT(0); }
void stop_effect(AcesEffect *&) { G_ASSERT(0); }
void setNormalsTex(const BaseTexture *tex) { G_ASSERT(0); }
void setDepthTex(const BaseTexture *tex) { G_ASSERT(0); }
void push_gravity_zone(GravityZoneBuffer &, const TMatrix &, const Point3 &, uint32_t, uint32_t, float, float, bool) { G_ASSERT(0); }
void set_gravity_zones(GravityZoneBuffer &) { G_ASSERT(0); }
} // namespace acesfx

namespace skies_utils
{
void load_daSkies(DaSkies &, const DataBlock &, int, const DataBlock &) { G_ASSERT(0); }
} // namespace skies_utils
DngSkies *get_daskies() { G_ASSERT_RETURN(false, nullptr); }
bool DaSkies::currentGroundSunSkyColor(float &, float &, Color3 &, Color3 &, Color3 &, Color3 &) const
{
  G_ASSERT_RETURN(false, false);
}
float DaSkies::getCloudsStartAltSettings() const { G_ASSERT_RETURN(false, 0.f); }
float DaSkies::getCloudsTopAltSettings() const { G_ASSERT_RETURN(false, 0.f); }

float get_daskies_time() { G_ASSERT_RETURN(false, 0.f); }

void set_daskies_time(float) { G_ASSERT(0); }

void move_cumulus_clouds(const Point2 &) { G_ASSERT(0); }

void move_strata_clouds(const Point2 &) { G_ASSERT(0); }

void select_weather_preset_delayed(const char *) { G_ASSERT(0); }

Point3 calculate_server_sun_dir() { G_ASSERT_RETURN(false, Point3()); }

#include "phys/gridCollision.h"

bool trace_entities_in_grid(uint32_t, const Point3 &, const Point3 &, float &, ecs::EntityId, IntersectedEntities &, SortIntersections)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_entities_in_grid(uint32_t, const Point3 &, const Point3 &, float &, ecs::EntityId) { G_ASSERT_RETURN(false, false); }
bool trace_entities_in_grid_by_capsule(
  uint32_t, const Point3 &, const Point3 &, float &, float, ecs::EntityId, IntersectedEntities &, SortIntersections)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_entities_in_grid_by_capsule(uint32_t,
  const Point3 &,
  const Point3 &,
  float &,
  float,
  ecs::EntityId,
  IntersectedEntities &,
  SortIntersections,
  const get_capsule_radius_cb_t &)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_entities_in_grid_by_capsule(uint32_t, const Point3 &, const Point3 &, float &, float, ecs::EntityId)
{
  G_ASSERT_RETURN(false, false);
}
bool rayhit_entities_in_grid(uint32_t, const Point3 &, const Point3 &, float, ecs::EntityId) { G_ASSERT_RETURN(false, false); }
bool query_entities_intersections_in_grid(
  uint32_t, dag::ConstSpan<plane3f>, const TMatrix &, float, bool, IntersectedEntities &, SortIntersections)
{
  G_ASSERT_RETURN(false, false);
}

bool trace_to_collision_nodes(const Point3 &, ecs::EntityId, IntersectedEntities &, SortIntersections, float)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_to_capsule_approximation(const Point3 &, ecs::EntityId, IntersectedEntities &, SortIntersections, float)
{
  G_ASSERT_RETURN(false, false);
}
bool rayhit_to_collision_nodes(const Point3 &, ecs::EntityId, float) { G_ASSERT_RETURN(false, false); }
bool rayhit_to_capsule_approximation(const Point3 &, ecs::EntityId, float) { G_ASSERT_RETURN(false, false); }

bool cvt_debug_text_pos(const Point3 &wp, float &out_cx, float &out_cy) { G_ASSERT_RETURN(false, false); }
void add_debug_text_mark(
  float scr_cx, float scr_cy, const char *str, int length = -1, float line_ofs = 0, E3DCOLOR frame_color = E3DCOLOR_MAKE(0, 0, 0, 160))
{
  G_ASSERT(0);
}

void add_debug_text_mark(const Point3 &wp, const char *str, int length, float line_ofs, E3DCOLOR frame_color) { G_ASSERT(0); }

bool check_action_precondition(ecs::EntityId, int) { G_ASSERT_RETURN(false, false); }

#include <gamePhys/common/loc.h>
void gamephys::Loc::interpolate(const Loc &, const Loc &, const float) { G_ASSERT(0); }
void gamephys::Loc::substract(const Loc &lhs, const Loc &rhs) { G_ASSERT(0); }
void gamephys::Loc::add(const Loc &lhs, const Loc &rhs) { G_ASSERT(0); }

namespace circuit
{
eastl::string_view get_name() { G_ASSERT_RETURN(false, eastl::string_view("")); }
const DataBlock *get_conf() { G_ASSERT_RETURN(false, nullptr); }
} // namespace circuit

namespace net
{
uint64_t get_session_id() { G_ASSERT_RETURN(false, 0); }
IConnection *get_replay_connection() { G_ASSERT_RETURN(false, nullptr); }
} // namespace net

namespace splineregions
{
struct SplineRegion
{
  bool checkPoint(const Point2 &) const;
  Point3 getAnyBorderPoint() const;
  Point3 getRandomBorderPoint() const;
  Point3 getRandomPoint() const;
  const char *getNameStr() const;
};
typedef Tab<splineregions::SplineRegion> LevelRegions;
bool SplineRegion::checkPoint(const Point2 &) const { G_ASSERT_RETURN(false, false); }
Point3 SplineRegion::getAnyBorderPoint() const { G_ASSERT_RETURN(false, Point3()); }
Point3 SplineRegion::getRandomBorderPoint() const { G_ASSERT_RETURN(false, Point3()); }
Point3 SplineRegion::getRandomPoint() const { G_ASSERT_RETURN(false, Point3()); }
const char *SplineRegion::getNameStr() const { G_ASSERT_RETURN(false, ""); }
void construct_region_from_spline(SplineRegion &region, const levelsplines::Spline &spline, float expand_dist) { G_ASSERT(0); }
void load_regions_from_splines(LevelRegions &, dag::ConstSpan<levelsplines::Spline>, const DataBlock &) {}
}; // namespace splineregions

bool rayhit_smoke_occluders(const Point3 &, const Point3 &) { G_ASSERT_RETURN(false, false); }

#include <main/gameLoad.h>
bool sceneload::load_game_scene(const char *, int, uint32_t)
{
  G_ASSERT(0);
  return false;
}
void sceneload::switch_scene(eastl::string_view, eastl::vector<eastl::string> &&, UserGameModeContext &&) { G_ASSERT(0); }
void sceneload::switch_scene_and_update(eastl::string_view) { G_ASSERT(0); }
void sceneload::connect_to_session(net::ConnectParams &&, UserGameModeContext &&) { G_ASSERT(0); }

void human_gun_custom_props_cleanup(ecs::Object &custom_props) { G_ASSERT(0); }

ecs::EntityId get_cur_cam_entity() { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
ecs::EntityId set_scene_camera_entity(ecs::EntityId) { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
ecs::EntityId enable_spectator_camera(const TMatrix &, int, ecs::EntityId) { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
void set_cam_itm(const TMatrix &) { G_ASSERT(0); }
void toggle_free_camera() { G_ASSERT(0); }
void enable_free_camera() { G_ASSERT(0); }
void disable_free_camera() { G_ASSERT(0); }
void force_free_camera() { G_ASSERT(0); }
Point3 get_free_cam_speeds() { G_ASSERT_RETURN(false, Point3::ZERO); }

namespace camera_in_camera
{
bool is_lens_only_zoom_enabled() { G_ASSERT_RETURN(false, false); }
} // namespace camera_in_camera

float get_scope_lens_magnification_limit_term() { return 1.0f; }

bool is_looking_into_optics_scope() { return false; }

#include "main/gameLoad.h"
namespace sceneload
{
const GamePackage &get_current_game()
{
  G_ASSERT(0);
  static GamePackage dummy;
  return dummy;
}
} // namespace sceneload

void BasePhysActor::resizeSyncStates(int) { G_ASSERT(0); }
float BasePhysActor::minTimeStep = 0.0f;
float BasePhysActor::maxTimeStep = 0.0f;
void teleport_phys_actor(ecs::EntityId, TMatrix const &) { G_ASSERT(0); }
int calc_phys_update_to_tick(float, float, int, int, int) { G_ASSERT_RETURN(false, -1); }

#include <daInput/input_api.h>
namespace dainput
{
int get_double_click_time() { G_ASSERT_RETURN(false, 0); }
bool get_actions_binding_column_active(unsigned) { G_ASSERT_RETURN(false, false); }
action_handle_t get_action_handle(const char *, uint16_t) { G_ASSERT_RETURN(false, BAD_ACTION_HANDLE); }
const char *get_action_name(dainput::action_handle_t) { G_ASSERT_RETURN(false, nullptr); }
unsigned get_last_used_device_mask(unsigned) { G_ASSERT_RETURN(false, 0); }
bool is_action_active(action_handle_t) { G_ASSERT_RETURN(false, false); }
bool is_action_enabled(action_handle_t) { G_ASSERT_RETURN(false, false); }
void set_action_enabled(action_handle_t, bool) { G_ASSERT(0); }
void set_action_mask_immediate(action_handle_t, bool) { G_ASSERT(0); }
bool is_action_mask_immediate(action_handle_t) { G_ASSERT_RETURN(false, false); }
DigitalActionBinding *get_digital_action_binding(action_handle_t, int) { G_ASSERT_RETURN(false, nullptr); }
AnalogAxisActionBinding *get_analog_axis_action_binding(action_handle_t, int) { G_ASSERT_RETURN(false, nullptr); }
AnalogStickActionBinding *get_analog_stick_action_binding(action_handle_t, int) { G_ASSERT_RETURN(false, nullptr); }
float get_analog_stick_action_smooth_value(action_handle_t) { G_ASSERT_RETURN(false, 0.0f); }
void set_analog_stick_action_smooth_value(action_handle_t, float) { G_ASSERT(0); }
int get_actions_count() { G_ASSERT_RETURN(false, 0); }
action_handle_t get_action_handle_by_ord(int ord_idx) { G_ASSERT_RETURN(false, BAD_ACTION_HANDLE); }
bool check_bindings_conflicts_one(action_handle_t, int, action_handle_t, int) { G_ASSERT_RETURN(false, false); }
const DigitalAction &get_digital_action_state(action_handle_t)
{
  static DigitalAction defaultAction;
  G_ASSERT_RETURN(false, defaultAction);
}
const AnalogStickAction &get_analog_stick_action_state(action_handle_t)
{
  static AnalogStickAction defaultAction;
  G_ASSERT_RETURN(false, defaultAction);
}
const AnalogAxisAction &get_analog_axis_action_state(action_handle_t)
{
  static AnalogAxisAction defaultAction;
  G_ASSERT_RETURN(false, defaultAction);
}
bool set_analog_axis_action_state(action_handle_t, float) { G_ASSERT_RETURN(false, false); }

action_handle_t get_action_set_handle(const char *action_set_name) { G_ASSERT_RETURN(false, BAD_ACTION_SET_HANDLE); }
void activate_action_set(action_set_handle_t set, bool activate) { G_ASSERT(0); }
bool reset_digital_action_sticky_toggle(action_handle_t) { G_ASSERT_RETURN(false, false); }
void send_action_event(action_handle_t action) { G_ASSERT(0); }

void dump_action_sets() { G_ASSERT(0); }
void dump_action_bindings() { G_ASSERT(0); }
void dump_action_sets_stack() { G_ASSERT(0); }

} // namespace dainput

int get_interp_delay_ticks(PhysTickRateType) { G_ASSERT_RETURN(false, 0); }

struct TouchInput
{
public:
  void pressButton(dainput::action_handle_t);
  void releaseButton(dainput::action_handle_t);
  bool isButtonPressed(dainput::action_handle_t);
  void setStickValue(dainput::action_handle_t, const Point2 &);
  Point2 getStickValue(dainput::action_handle_t);
  void setAxisValue(dainput::action_handle_t, const float &);
  float getAxisValue(dainput::action_handle_t);
};

void TouchInput::pressButton(dainput::action_handle_t) { G_ASSERT(0); }
void TouchInput::releaseButton(dainput::action_handle_t) { G_ASSERT(0); }
bool TouchInput::isButtonPressed(dainput::action_handle_t) { G_ASSERT_RETURN(false, false); }
void TouchInput::setStickValue(dainput::action_handle_t, const Point2 &) { G_ASSERT(0); }
Point2 TouchInput::getStickValue(dainput::action_handle_t) { G_ASSERT_RETURN(false, {}); }
void TouchInput::setAxisValue(dainput::action_handle_t, const float &) { G_ASSERT(0); }
float TouchInput::getAxisValue(dainput::action_handle_t) { G_ASSERT_RETURN(false, 0.f); }

TouchInput touch_input;

#include <camera/cameraShaker.h>
void CameraShaker::update(float, float) { G_ASSERT(0); }
void CameraShaker::shakeMatrix(TMatrix &) { G_ASSERT(0); }
void CameraShaker::setShake(float) { G_ASSERT(0); }
void CameraShaker::setShakeHiFreq(float) { G_ASSERT(0); }
void CameraShaker::setSmoothShakeHiFreq(float) { G_ASSERT(0); }

#include "sound_net/shellSoundNetProps.h"
/*static*/ const ShellSoundNetProps *ShellSoundNetProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
/*static*/ const ShellSoundNetProps *ShellSoundNetProps::try_get_props(int) { G_ASSERT_RETURN(false, nullptr); }
/*static*/ bool ShellSoundNetProps::can_load(const DataBlock *) { G_ASSERT_RETURN(false, false); }

#include "sound/actionSoundProps.h"
namespace sound
{
/*static*/ const ActionProps *ActionProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
/*static*/ const ActionProps *ActionProps::try_get_props(int) { G_ASSERT_RETURN(false, nullptr); }
} // namespace sound

bool is_level_loading() { G_ASSERT_RETURN(false, false); }
bool is_level_loaded() { G_ASSERT_RETURN(false, false); }

bool is_level_loaded_not_empty() { G_ASSERT_RETURN(false, false); }

bool is_level_unloading() { G_ASSERT_RETURN(false, false); }

ecs::EntityId get_current_level_eid() { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }

void select_weather_preset(const char *) { G_ASSERT(0); }

#include "net/netPropsRegistry.h"
namespace propsreg
{
int register_net_props(const char *blk_name, const char *class_name) { G_ASSERT_RETURN(false, 0); }
} // namespace propsreg

#include "net/netStat.h"
namespace netstat
{
dag::ConstSpan<Sample> get_aggregations() { G_ASSERT_RETURN(false, {}); }
} // namespace netstat

Tab<TMatrix> get_points_on_road_route(
  const Point3 &start_route_pos, const Point3 &end_route_pos, int points_count, float points_dist, float roads_search_rad)
{
  G_ASSERT_RETURN(false, {});
}


#include <forceFeedback/forceFeedback.h>
namespace force_feedback
{
namespace rumble
{
void add_event(const EventParams &) { G_ASSERT(0); }
} // namespace rumble
}; // namespace force_feedback

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  struct mg_context
  {
    int num_req_conn;
  };
  struct mg_connection
  {
    int request_len;
  };
  struct mg_request_info
  {
    int request_len;
  };

  struct mg_context *mg_start(const struct mg_callbacks *callbacks, void *user_data, const char **configuration_options)
  {
    G_ASSERT(0);
    static mg_context dummy;
    return &dummy;
  }

  void mg_stop(struct mg_context *) { G_ASSERT(0); }
  int mg_read(struct mg_connection *, void *buf, size_t len) { G_ASSERT_RETURN(false, 0); }
  struct mg_request_info *mg_get_request_info(struct mg_connection *)
  {
    G_ASSERT(0);
    static mg_request_info dummy;
    return &dummy;
  }
  int mg_websocket_write(struct mg_connection *conn, int opcode, const char *data, size_t data_len) { G_ASSERT_RETURN(false, 0); }
  void mg_process_requests(struct mg_context *ctx) { G_ASSERT(0); }

#ifdef __cplusplus
}
#endif // __cplusplus

#include <ui/uiShared.h>
namespace uishared
{
TMatrix view_tm, view_itm;
}

#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>
namespace ecs
{
InitOnDemand<SceneManager> g_scenes;
}


#include <render/rendererFeatures.h>
class BaseTexture;
struct CameraParams;
class CameraViewVisibilityMgr;
namespace bind_dascript
{
void toggleFeature(FeatureRenderFlags, bool) { G_ASSERT(0); }
bool worldRenderer_getWorldBBox3(BBox3 &bbox) { G_ASSERT_RETURN(false, false); }
void worldRenderer_shadowsInvalidate(const BBox3 &bbox) { G_ASSERT(0); }
void worldRenderer_invalidateAllShadows() { G_ASSERT(0); }
int worldRenderer_getDynamicResolutionTargetFps() { G_ASSERT_RETURN(false, 0); }
void worldRenderer_setDaGdpRangeScale(float) { G_ASSERT(0); }
bool does_world_renderer_exist() { G_ASSERT_RETURN(false, 0); }
} // namespace bind_dascript

#include <render/resolution.h>
void get_display_resolution(int &w, int &h) { G_ASSERT(0); }
void get_rendering_resolution(int &w, int &h) { G_ASSERT(0); }
void get_postfx_resolution(int &w, int &h) { G_ASSERT(0); }

#include <render/fx/uiPostFxManager.h>
void UiPostFxManager::setBlurUpdateEnabled(bool) { G_ASSERT(0); }

void UiPostFxManager::updateFinalBlurred(const IPoint2 &, const IPoint2 &, int) { G_ASSERT(0); }

#include <render/clipmapDecals.h>
int clipmap_decals_mgr::getDecalTypeIdByName(const char *) { G_ASSERT_RETURN(false, -1); }
void clipmap_decals_mgr::createDecal(int, const Point2 &, float, const Point2 &, int, bool, uint64_t, int) { G_ASSERT(0); }

void erase_grass(const Point3 &, float) { G_ASSERT(0); }
void invalidate_after_heightmap_change(const BBox3 &box) { G_ASSERT(0); }
void invalidate_ssr_history(int) { G_ASSERT(0); }
void remove_puddles_in_crater(const Point3 &, float) { G_ASSERT(0); }

struct PortalParams;

namespace portal_renderer_mgr
{
void update_portal(int, const TMatrix &, const TMatrix &, const PortalParams &) { G_ASSERT(0); }
int allocate_portal() { G_ASSERT_RETURN(false, -1); }
void free_portal(int) { G_ASSERT(0); }
} // namespace portal_renderer_mgr

PhysUpdateCtx PhysUpdateCtx::ctx;

namespace embeddedupdater
{
void set_accept_user_react() { G_ASSERT(0); };
int get_progress_percent(void) { G_ASSERT_RETURN(false, -1); }
int get_download_speed(void) { G_ASSERT_RETURN(false, -1); }
int get_total_download_mb(void) { G_ASSERT_RETURN(false, 0); }
int get_downloaded_mb(void) { G_ASSERT_RETURN(false, 0); }
int get_eta(void) { G_ASSERT_RETURN(false, -1); }
bool is_error_state(void) { G_ASSERT_RETURN(false, false); }
eastl::string get_state_text(void) { G_ASSERT_RETURN(false, {}); }
eastl::string get_error_text(void) { G_ASSERT_RETURN(false, {}); }
eastl::string get_fail_text(void) { G_ASSERT_RETURN(false, {}); }
eastl::string get_loc_text(const char *) { G_ASSERT_RETURN(false, {}); }

eastl::string get_lang() { G_ASSERT_RETURN(false, {}); }
} // namespace embeddedupdater


namespace blood_decals
{
void add_decal(Point3, Point3)
{
  G_ASSERT(0);
  return;
}
} // namespace blood_decals

#include <animChar/dag_animCharacter2.h>

bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &, eastl::function<void(ShaderMaterial *)> &&)
{
  G_ASSERT_RETURN(false, false);
}
bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &, const char *, eastl::function<void(ShaderMaterial *)> &&)
{
  G_ASSERT_RETURN(false, false);
}
bool recreate_material_with_new_params(AnimV20::AnimcharRendComponent &,
  const eastl::vector<const char *, framemem_allocator> &,
  eastl::function<void(ShaderMaterial *)> &&)
{
  G_ASSERT_RETURN(false, false);
}
void screen_to_world(const Point2 &, Point3 &, Point3 &) { G_ASSERT(0); }

#include <ecs/phys/collRes.h>
Point2 get_collres_slice_mean_and_dispersion(const CollisionResource &collres, float min_height, float max_height)
{
  G_ASSERT_RETURN(false, {});
}

namespace systeminfo
{
bool get_cpu_info(String &, String &, String &, String &, int &) { G_ASSERT_RETURN(false, false); }
bool get_video_info(String &, String &, String &) { G_ASSERT_RETURN(false, false); }
} // namespace systeminfo


namespace lagcatcher
{
void stop() { G_ASSERT(0); }
} // namespace lagcatcher


#include <projectiveDecals/projectiveDecals.h>

ProjectiveDecals::~ProjectiveDecals() { G_ASSERT(0); }
void ProjectiveDecals::updateDecal(
  unsigned int, TMatrix const &, float, unsigned short, unsigned short, Point4 const &, unsigned short)
{
  G_ASSERT(0);
}
bool ProjectiveDecals::init(const char *, const char *, const char *, const char *) { G_ASSERT_RETURN(false, false); }
void ProjectiveDecals::prepareRender(const Frustum &) { G_ASSERT(0); }
void ProjectiveDecals::render() { G_ASSERT(0); }
void ProjectiveDecals::afterReset() { G_ASSERT(0); }
int RingBufferDecals::allocate_decal_id() { G_ASSERT_RETURN(false, -1); }
void RingBufferDecals::init_buffer(uint32_t) { G_ASSERT(0); }
void RingBufferDecals::clear() { G_ASSERT(0); }


#include <projectiveDecals/projectiveDecals.h>

void ProjectiveDecals::updateParams(uint32_t, const Point4 &, uint16_t) { G_ASSERT(0); }


#include <daECS/utility/createInstantiated.h>
namespace ecs
{
EntityId createInstantiatedEntitySync(EntityManager &, const char *, ComponentsInitializer &&) { G_ASSERT_RETURN(false, {}); }
} // namespace ecs

#include <contentUpdater/fsUtils.h>
eastl::string updater::fs::join_path(std::initializer_list<const char *> parts) { G_ASSERT_RETURN(false, eastl::string{}); }

#include <videoPlayer/dag_videoPlayer.h>
class IGenVideoPlayer *IGenVideoPlayer::create_ogg_video_player(char const *, int) { G_ASSERT_RETURN(false, nullptr); }
class IGenVideoPlayer *IGenVideoPlayer::create_av1_video_player(char const *, int) { G_ASSERT_RETURN(false, nullptr); }

class GlobalVisibleCoversMap;
GlobalVisibleCoversMap *get_global_visible_covers_map() { G_ASSERT_RETURN(false, nullptr); }

void replay_play(char const *, float, sceneload::UserGameModeContext &&ugmCtx, const char *) { G_ASSERT(0); }

#include <util/dag_console.h>
static bool any_console_command_fallback(const char *[], int argc) { return argc > 0; }
REGISTER_CONSOLE_HANDLER(any_console_command_fallback);

scene::TiledScene *create_ladders_scene(GameObjects &) { G_ASSERT_RETURN(false, nullptr); }

#include "phys/collRes.h"
bool get_collres_body_tm(const ecs::EntityId eid, const int coll_node_id, TMatrix &tm, const char *fnname)
{
  G_ASSERT_RETURN(false, false);
}

#include "net/dedicated/matching.h"
const Json::Value &dedicated_matching::get_mode_info()
{
  G_ASSERT(0);
  static Json::Value j0 = Json::nullValue;
  return j0;
}
const eastl::unordered_map<matching::UserId, Json::Value> &dedicated_matching::get_session_players()
{
  G_ASSERT(0);
  static eastl::unordered_map<matching::UserId, Json::Value> r;
  return r;
}
void dedicated_matching::on_level_loaded() { G_ASSERT(0); }
void dedicated_matching::on_player_team_changed(matching::UserId, int) { G_ASSERT(0); }
void dedicated_matching::player_kick_from_room(matching::UserId) { G_ASSERT(0); }
void dedicated_matching::ban_player_in_room(matching::UserId) { G_ASSERT(0); }
int dedicated_matching::get_room_members_count() { G_ASSERT_RETURN(false, 0); }
int dedicated_matching::get_player_req_teams_num(matching::UserId) { G_ASSERT_RETURN(false, 0); }
const char *dedicated_matching::get_player_custom_info(matching::UserId) { G_ASSERT_RETURN(false, ""); }

#include <render/priorityManagedShadervar.h>
namespace PriorityShadervar
{
void set_real(int, int, float) { G_ASSERT(0); }
void set_int(int, int, int) { G_ASSERT(0); }
void set_color4(int, int, Point4) { G_ASSERT(0); }
void set_int4(int, int, IPoint4) { G_ASSERT(0); }
void clear(int, int) { G_ASSERT(0); }
} // namespace PriorityShadervar

#include <daRg/dag_properties.h>
#include <gui/dag_stdGuiRender.h>
namespace darg
{
E3DCOLOR Properties::getColor(const Sqrat::Object &, E3DCOLOR) const { G_ASSERT_RETURN(false, E3DCOLOR(0, 0, 0)); }
float Properties::getFloat(const Sqrat::Object &, float, bool) const { G_ASSERT_RETURN(false, 0.f); }
bool Properties::getBool(const Sqrat::Object &, bool, bool) const { G_ASSERT_RETURN(false, false); }
void Properties::trace_error(const char *, const Sqrat::Object &, const Sqrat::Object &) { G_ASSERT(0); }
int Properties::getFontId() const { G_ASSERT_RETURN(false, -1); }
float Properties::getFontSize() const { G_ASSERT_RETURN(false, 0.f); }
Picture *Properties::getPicture(const Sqrat::Object &) const
{
  G_ASSERT(0);
  return NULL;
}

DataBlock *Properties::getBlk(const Sqrat::Object &) const
{
  G_ASSERT(0);
  return NULL;
}

void render_picture(StdGuiRender::GuiContext &, Picture *, Point2, Point2, E3DCOLOR, float, float, bool, bool, float) { G_ASSERT(0); }
void render_picture(StdGuiRender::GuiContext &, Picture *, Point2, Point2, E3DCOLOR) { G_ASSERT(0); }
} // namespace darg

#include <compressionUtils/compression.h>

float lag_compensation_time(ecs::EntityId avatar_eid,
  ecs::EntityId lc_eid,
  float at_time,
  int interp_delay_ticks_packed,
  float additional_interp_delay,
  BasePhysActor **out_lc_eid_phys_actor)
{
  G_ASSERT_RETURN(false, 0.f);
}

class StubLagCompensationMgr final : public ILagCompensationMgr
{
  void startLagCompensation(float, ecs::EntityId) override {}
  LCError backtrackEntity(ecs::EntityId, float) override { return LCError::UnknownEntity; }
  void finishLagCompensation() override {}
};

ILagCompensationMgr &get_lag_compensation()
{
  G_ASSERT(0);
  static StubLagCompensationMgr stub;
  return stub;
}
