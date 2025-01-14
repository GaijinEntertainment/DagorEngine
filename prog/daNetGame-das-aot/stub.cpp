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
#include "render/skies.h"
#include "render/fx/fx.h"
#include "render/fx/effectEntity.h"
#include <landMesh/lmeshHoles.h>
#include <levelSplines/levelSplines.h>

const char *default_game_name = "aot"; // Referenced by `get_game_name`

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


int send_net_msg(ecs::EntityId, net::IMessage &&, const net::MessageNetDesc *) { G_ASSERT_RETURN(false, -1); }

void send_transform_snapshots_targeted_event(ecs::EntityId, danet::BitStream &) { G_ASSERT(0); }
void send_transform_snapshots_event(danet::BitStream &) { G_ASSERT(0); }
void send_reliable_transform_snapshots_event(danet::BitStream &) { G_ASSERT(0); }

float get_timespeed() { G_ASSERT_RETURN(false, 0.f); }
void set_timespeed(float) { G_ASSERT(0); }
void toggle_pause() { G_ASSERT(0); }

#include <gamePhys/props/deformMatProps.h>
namespace physmat
{
void init() { G_ASSERT(0); }
const DeformMatProps *DeformMatProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }
} // namespace physmat

#include <gui/dag_visualLog.h>
namespace visuallog
{
void logmsg(const char *text, LogItemCBProc, void *, E3DCOLOR, int) { G_ASSERT(0); }
} // namespace visuallog

float phys_get_timestep() { G_ASSERT_RETURN(false, 0.f); }

#include <gamePhys/props/atmosphere.h>

float gamephys::atmosphere::_g = 0.f;
float gamephys::atmosphere::_water_density = 0.f;

#include <gamePhys/phys/utils.h>
namespace gamephys
{
namespace atmosphere
{
float density(float h) { G_ASSERT_RETURN(false, 0.0f); }
Point3 get_wind() { G_ASSERT_RETURN(false, Point3()); }
float temperature(float) { G_ASSERT_RETURN(false, 0.0f); }
float sonicSpeed(float) { G_ASSERT_RETURN(false, 0.0f); }
} // namespace atmosphere
void Orient::setYP0(const Point3 &) { G_ASSERT(0); }
void Orient::setQuat(const Quat &) { G_ASSERT(0); }
void Orient::wrap() { G_ASSERT(0); }
void extrapolate_circular(const Point3 &, const Point3 &, const Point3 &, float, Point3 &, Point3 &) { G_ASSERT(0); }
} // namespace gamephys

#include <gamePhys/common/mass.h>
namespace gamephys
{
void Mass::setFuel(float, int, bool) { G_ASSERT(0); }
bool Mass::hasFuel(float, int) const { G_ASSERT_RETURN(false, false); }
float Mass::getFuelMassCurrent() const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassCurrent(int) const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassMax() const { G_ASSERT_RETURN(false, 0.f); };
float Mass::getFuelMassMax(int) const { G_ASSERT_RETURN(false, 0.f); };
} // namespace gamephys

#include <ecs/phys/netPhysResync.h>
void ECSCustomPhysStateSyncer::init(ecs::EntityId, int) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, float &) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, int &) {}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *, bool &) {}

#if DAGOR_DBGLEVEL > 0
#include <gamePhys/common/fixed_dt.h>
float gamephys::PHYSICS_UPDATE_FIXED_DT = gamephys::DEFAULT_PHYSICS_UPDATE_FIXED_DT;
#endif

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/collisionInstances.h>
namespace dacoll
{
bool traceray_normalized(
  const Point3 &, const Point3 &, real &, int *, Point3 *, int flags, rendinst::RendInstDesc *out_desc, int, const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool traceray_normalized_coll_type(const Point3 &p,
  const Point3 &dir,
  real &t,
  int *out_pmid,
  Point3 *out_norm,
  int flags,
  rendinst::RendInstDesc *out_desc,
  int *out_coll_type,
  int ray_mat_id,
  const TraceMeshFaces *handle)
{
  G_ASSERT_RETURN(false, false);
}
bool tracedown_normalized(const Point3 &, real &, int *, Point3 *, int, rendinst::RendInstDesc *, int, const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool traceray_normalized_frt(const Point3 &, const Point3 &, real &, int *, Point3 *) { G_ASSERT_RETURN(false, false); }
bool traceray_normalized_lmesh(const Point3 &, const Point3 &, real &, int *, Point3 *) { G_ASSERT_RETURN(false, false); }
bool traceray_normalized_ri(const Point3 &,
  const Point3 &,
  real &,
  int *,
  Point3 *,
  rendinst::TraceFlags,
  rendinst::RendInstDesc *,
  int,
  const TraceMeshFaces *,
  rendinst::riex_handle_t)
{
  G_ASSERT_RETURN(false, false);
}
void validate_trace_cache(const bbox3f &, const vec3f &, float, TraceMeshFaces *) { G_ASSERT(0); }
bool trace_game_objects(const Point3 &, const Point3 &, float &, Point3 &, int, int) { G_ASSERT_RETURN(false, false); }
bool traceht_water(const Point3 &, float &) { G_ASSERT_RETURN(false, false); }
float traceht_lmesh(const Point2 &) { G_ASSERT_RETURN(false, 0.f); }
float traceht_hmap(const Point2 &) { G_ASSERT_RETURN(false, 0.f); }
bool is_valid_heightmap_pos(const Point2 &) { G_ASSERT_RETURN(false, false); }
bool is_valid_water_height(float) { G_ASSERT_RETURN(false, false); }
float traceht_water_at_time(const Point3 &, float, float, bool &, float) { G_ASSERT_RETURN(false, false); }
bool traceray_water_at_time(const Point3 &, const Point3 &, float, float &) { G_ASSERT_RETURN(false, false); }
bool get_min_max_hmap_in_circle(const Point2 &, float, float &, float &) { G_ASSERT_RETURN(false, false); }
void get_min_max_hmap_list_in_circle(const Point2 &, float, Tab<Point2> &) { G_ASSERT(0); };
bool rayhit_normalized_lmesh(const Point3 &p, const Point3 &dir, real t) { G_ASSERT_RETURN(false, false); }
bool rayhit_normalized(const Point3 &p,
  const Point3 &dir,
  real t,
  int flags,
  int ray_mat_id,
  const TraceMeshFaces *handle,
  rendinst::riex_handle_t skip_riex_handle)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_cast(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int, const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_cast_land(const Point3 &, const Point3 &, float, ShapeQueryOutput &, int) { G_ASSERT_RETURN(false, false); }
bool sphere_cast_ex(
  const Point3 &, const Point3 &, float, ShapeQueryOutput &, int, dag::ConstSpan<CollisionObject>, const TraceMeshFaces *, int, int)
{
  G_ASSERT_RETURN(false, false);
}
bool sphere_query_ri(const Point3 &from,
  const Point3 &to,
  float rad,
  ShapeQueryOutput &out,
  int cast_mat_id,
  Tab<rendinst::RendInstDesc> *out_desc,
  const TraceMeshFaces *handle,
  RIFilterCB *)
{
  G_ASSERT_RETURN(false, false);
}
bool trace_sphere_cast_ex(const Point3 &from,
  const Point3 &to,
  float rad,
  int num_casts,
  dacoll::ShapeQueryOutput &out,
  int cast_mat_id,
  int ignore_game_obj_id,
  const TraceMeshFaces *handle,
  int flags)
{
  G_ASSERT_RETURN(false, false);
}
CollisionObject add_dynamic_collision_from_coll_resource(const DataBlock *props,
  const CollisionResource *coll_resource,
  void *user_ptr,
  int flags,
  int phys_layer,
  int mask,
  const TMatrix *wtm)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_collision_with_mask(
  const DataBlock &props, void *userPtr, bool is_player, bool add_to_world, bool auto_mask, int mask, const TMatrix *wtm)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_sphere_collision(const TMatrix &tm, float rad, void *user_ptr, bool add_to_world)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_capsule_collision(const TMatrix &tm, float rad, float height, void *user_ptr, bool add_to_world)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
CollisionObject add_dynamic_cylinder_collision(const TMatrix &tm, float rad, float ht, void *user_ptr, bool add_to_world)
{
  G_ASSERT_RETURN(false, CollisionObject());
}
void add_collision_hmap_custom(
  const Point3 &collision_pos, const BBox3 &collision_box, const Point2 &hmap_offset, float hmap_scale, int hmap_step)
{
  G_ASSERT(0);
}

void destroy_dynamic_collision(CollisionObject co) { G_ASSERT(0); }
bool test_pair_collision(dag::ConstSpan<CollisionObject> co_a,
  uint64_t cof_a,
  const TMatrix &tm_a,
  dag::ConstSpan<CollisionObject> co_b,
  uint64_t cof_b,
  const TMatrix &tm_b,
  Tab<gamephys::CollisionContactData> &out_contacts,
  TestPairFlags flags,
  bool set_co_tms)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_world(dag::ConstSpan<CollisionObject> collision,
  const TMatrix &tm,
  float bounding_rad,
  Tab<gamephys::CollisionContactData> &out_contacts,
  const TraceMeshFaces *)
{
  G_ASSERT_RETURN(false, false);
}
void update_ri_cache_in_volume_to_phys_world(const BBox3 &) {}
bool check_ri_collision_filtered(const rendinst::RendInstDesc &, const TMatrix &, const TMatrix &, int)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_ri(const CollisionObject &coll_obj,
  const BBox3 &box,
  Tab<gamephys::CollisionContactData> &out_contacts,
  const TraceMeshFaces *trace_cache,
  int mat_id)
{
  G_ASSERT_RETURN(false, false);
}
bool test_collision_frt(const CollisionObject &coll_obj, Tab<gamephys::CollisionContactData> &out_contacts, int mat_id)
{
  G_ASSERT_RETURN(false, false);
}
bool test_sphere_collision_world(
  const Point3 &pos, float radius, int mat_id, Tab<gamephys::CollisionContactData> &out_contacts, dacoll::PhysLayer group, int mask)
{
  G_ASSERT_RETURN(false, false);
}
bool test_box_collision_world(
  const TMatrix &tm, int mat_id, Tab<gamephys::CollisionContactData> &out_contacts, dacoll::PhysLayer group, int mask)
{
  G_ASSERT_RETURN(false, false);
}
PhysBody *get_convex_shape(struct CollisionObject) { G_ASSERT_RETURN(false, nullptr); }
void shape_query_frt(const PhysBody *, class TMatrix const &, class TMatrix const &, dacoll::ShapeQueryOutput &) { G_ASSERT(0); }
void shape_query_lmesh(const PhysBody *, class TMatrix const &, class TMatrix const &, dacoll::ShapeQueryOutput &) { G_ASSERT(0); }
void shape_query_ri(const PhysBody *,
  class TMatrix const &,
  class TMatrix const &,
  float,
  dacoll::ShapeQueryOutput &,
  int,
  Tab<struct rendinst::RendInstDesc> *,
  struct TraceMeshFaces const *,
  RIFilterCB *)
{
  G_ASSERT(0);
}
void fetch_sim_res(bool) { G_ASSERT(0); }
void set_vert_capsule_shape_size(const CollisionObject &, float, float) { G_ASSERT(0); }
void set_collision_sphere_rad(const CollisionObject &, float) { G_ASSERT(0); }
void set_collision_object_tm(const CollisionObject &, const TMatrix &) { G_ASSERT(0); }
void draw_collision_object(const CollisionObject &) { G_ASSERT(0); }

int set_hmap_step(int) { G_ASSERT_RETURN(false, -1); }
int get_hmap_step() { G_ASSERT_RETURN(false, -1); }
void set_obj_motion(CollisionObject, const TMatrix &, const Point3 &, const Point3 &) { G_ASSERT(0); }
PhysMat::MatID get_lmesh_mat_id_at_point(const Point2 &pos) { G_ASSERT_RETURN(false, PHYSMAT_INVALID); }
CollisionObject tmpObj;
CollisionObject &get_reusable_sphere_collision() { G_ASSERT_RETURN(false, tmpObj); }
CollisionObject &get_reusable_capsule_collision() { G_ASSERT_RETURN(false, tmpObj); }
CollisionObject &get_reusable_box_collision() { G_ASSERT_RETURN(false, tmpObj); }

void move_ri_instance(const rendinst::RendInstDesc &, const Point3 &, const Point3 &) { G_ASSERT(0); }
void enable_disable_ri_instance(const rendinst::RendInstDesc &, bool) { G_ASSERT(0); }
void flush_ri_instances() { G_ASSERT(0); }
bool is_ri_instance_enabled(const CollisionInstances *, const rendinst::RendInstDesc &) { G_ASSERT_RETURN(false, true); }
int get_link_name_id(const char *) { G_ASSERT_RETURN(false, -1); }

} // namespace dacoll
bool LandMeshHolesCell::check(const Point2 &, const HeightmapHandler *) const { return false; }

#include <gamePhys/collision/collisionResponse.h>
namespace daphys
{
void resolve_penetration(DPoint3 &pos,
  Quat &ori,
  dag::ConstSpan<gamephys::CollisionContactData> contacts,
  double inv_mass,
  const DPoint3 &inv_moi,
  double /*dt*/,
  bool use_future_contacts,
  int num_iter,
  float linear_slop,
  float erp)
{
  G_ASSERT(0);
};
void resolve_contacts(const DPoint3 &,
  const Quat &,
  DPoint3 &,
  DPoint3 &,
  dag::ConstSpan<gamephys::CollisionContactData>,
  double,
  const DPoint3 &,
  const ResolveContactParams &,
  int)
{
  G_ASSERT(0);
};
} // namespace daphys

#include <gamePhys/phys/physVars.h>
int PhysVars::registerVar(const char *, float) { G_ASSERT_RETURN(false, -1); }
int PhysVars::registerPullVar(const char *, float) { G_ASSERT_RETURN(false, -1); }
float PhysVars::getVar(int) const { G_ASSERT_RETURN(false, -1); }

#include <gamePhys/phys/animatedPhys.h>
void AnimatedPhys::init(const AnimV20::AnimcharBaseComponent &, const PhysVars &) { G_ASSERT(0); }
void AnimatedPhys::appendVar(const char *, const AnimV20::AnimcharBaseComponent &, const PhysVars &) { G_ASSERT(0); }
void AnimatedPhys::update(AnimV20::AnimcharBaseComponent &, PhysVars &) { G_ASSERT(0); }

#include <gamePhys/phys/rendinstDestr.h>
namespace rendinstdestr
{
bool apply_damage_to_riextra(rendinst::riex_handle_t, float, const Point3 &, const Point3 &, float) { G_ASSERT_RETURN(false, false); }
void remove_ri_without_collision_in_radius(const Point3 &, float) { G_ASSERT(0); }
void damage_ri_in_sphere(
  const Point3 &, float, const Point2 &, float, float, bool, on_riextra_destroyed_callback &&, riextra_should_damage &&)
{
  G_ASSERT(0);
}
void doRIExtraDamageInBox(const BBox3 &box,
  float at_time,
  bool create_destr,
  const Point3 &view_pos,
  calc_expl_damage_cb calc_expl_dmg_cb,
  const BSphere3 *check_sphere,
  const TMatrix *check_itm,
  rendinst::DestrOptionFlags destroy_flag)
{
  G_ASSERT(0);
}
rendinst::RendInstDesc destroyRendinst(rendinst::RendInstDesc desc,
  bool add_restorable,
  const Point3 &pos,
  const Point3 &impulse,
  float at_time,
  const rendinst::CollisionInfo *coll_info,
  bool create_destr,
  ApexDmgInfo *apex_dmg_info,
  int destroy_neighbour_recursive_depth,
  float impulse_mult_for_child,
  on_destr_callback on_destr_cb,
  rendinst::DestrOptionFlags destroy_flag)
{
  G_ASSERT_RETURN(false, rendinst::RendInstDesc());
}
rendinst::ri_damage_effect_cb get_ri_damage_effect_cb() { G_ASSERT_RETURN(false, nullptr); }
const DestrSettings &get_destr_settings()
{
  G_ASSERT(0);
  static DestrSettings s;
  return s;
}
CachedCollisionObjectInfo *get_or_add_cached_collision_object(rendinst::RendInstDesc const &, float)
{
  G_ASSERT_RETURN(false, nullptr);
}
} // namespace rendinstdestr

bool rendinst::isRiExtraLoaded() { G_ASSERT_RETURN(false, false); }

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
  void lock();
  void setFxScale(float);
  void setFxTm(const TMatrix &);
  void setEmitterTm(const TMatrix &, bool);
  void setSpawnRate(float);
  void setColorMult(struct Color4 const &);
  void setVelocity(const Point3 &vel);
  void setGravityTm(const Matrix3 &);
};
void AcesEffect::lock() { G_ASSERT(0); }
void AcesEffect::setFxScale(float) { G_ASSERT(0); }
void AcesEffect::setFxTm(const TMatrix &) { G_ASSERT(0); }
void AcesEffect::setEmitterTm(const TMatrix &, bool) { G_ASSERT(0); }
void AcesEffect::setSpawnRate(float) { G_ASSERT(0); }
void AcesEffect::setColorMult(struct Color4 const &) { G_ASSERT(0); }
void AcesEffect::setVelocity(const Point3 &vel) { G_ASSERT(0); }
void AcesEffect::setGravityTm(const Matrix3 &) { G_ASSERT(0); }
namespace acesfx
{
struct SoundDesc;
int get_type_by_name(const char *, bool) { G_ASSERT_RETURN(false, -1); }
FxQuality get_fx_target() { G_ASSERT_RETURN(false, FX_QUALITY_LOW); }
FxQuality getFxQualityMask() { G_ASSERT_RETURN(false, FX_QUALITY_LOW); }
AcesEffect *start_effect(int, const TMatrix &, const TMatrix &, bool, const SoundDesc *, FxErrorType *)
{
  G_ASSERT_RETURN(false, nullptr);
}
float get_effect_life_time(int) { G_ASSERT_RETURN(false, 0.0f); }
bool prefetch_effect(int) { G_ASSERT_RETURN(false, false); }
void start_update_prepare(float, const Driver3dPerspective &, int, int) { G_ASSERT(0); }
void start_update(float) { G_ASSERT(0); }
void finish_update(const TMatrix4 &) { G_ASSERT(0); }
void stop_effect(AcesEffect *&) { G_ASSERT(0); }
void wait_start_fx_job_done(bool) { G_ASSERT(0); }
void setNormalsTex(const BaseTexture *tex) { G_ASSERT(0); }
void setDepthTex(const BaseTexture *tex) { G_ASSERT(0); }
void push_gravity_zone(GravityZoneBuffer &, const TMatrix &, const Point3 &, uint32_t, uint32_t, float, bool) { G_ASSERT(0); }
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

#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlendCtrl.h>
#include <animChar/dag_animate2ndPass.h>
namespace AnimV20
{
int addEnumValue(const char *) { G_ASSERT_RETURN(false, 0); }
int getEnumValueByName(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getEnumName(int) { G_ASSERT_RETURN(false, ""); }
void AnimcharBaseComponent::act(float, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::recalcWtm() { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(mat44f &) const { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(TMatrix &) const { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const TMatrix &, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const Point3 &, const Point3 &, const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::setPostController(IAnimCharPostController *) { G_ASSERT(0); }
void AnimcharBaseComponent::calcAnimWtm(bool) { G_ASSERT(0); }
bool AnimcharBaseComponent::initAttachmentTmAndNodeWtm(int, mat44f &) const { G_ASSERT_RETURN(false, false); }
const mat44f *AnimcharBaseComponent::getSlotNodeWtm(int) const { G_ASSERT_RETURN(false, nullptr); }
const mat44f *AnimcharBaseComponent::getAttachmentTm(int) const { G_ASSERT_RETURN(false, nullptr); }
int AnimcharBaseComponent::getAttachmentSlotsCount() const { G_ASSERT_RETURN(false, 0); }
int AnimcharBaseComponent::getAttachmentSlotId(const int) const { G_ASSERT_RETURN(false, 0); }
void AnimcharBaseComponent::resetFastPhysWtmOfs(const vec3f wofs) { G_ASSERT(0); }
void AnimcharBaseComponent::setFastPhysSystemGravityDirection(const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::updateFastPhys(const float dt) { G_ASSERT(0); }
bool AnimV20::AnimcharRendComponent::calcWorldBox(bbox3f &, const AnimcharFinalMat44 &, bool) const { G_ASSERT_RETURN(false, false); }
vec4f AnimV20::AnimcharRendComponent::prepareSphere(const AnimcharFinalMat44 &) const { G_ASSERT_RETURN(false, vec4f()); }
const DataBlock *AnimcharBaseComponent::getDebugBlenderState(bool dump_tm) { G_ASSERT_RETURN(false, nullptr); }
AnimV20::AnimBlender::TlsContext &AnimV20::AnimBlender::selectCtx(intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *),
  void *irq_arg)
{
  G_ASSERT(0);
  static TlsContext dummy;
  return dummy;
}
void AnimcharBaseComponent::forcePostRecalcWtm(real) { G_ASSERT(0); }
void AnimcharBaseComponent::cloneTo(AnimcharBaseComponent *, bool) const { G_ASSERT(0); }

void AnimationGraph::enqueueState(IPureAnimStateHolder &, dag::ConstSpan<StateRec>, float, float) { G_ASSERT(0); }
void AnimationGraph::setStateSpeed(IPureAnimStateHolder &, dag::ConstSpan<StateRec>, float) { G_ASSERT(0); }
int AnimationGraph::getParamId(const char *, int) const { G_ASSERT_RETURN(false, 0); }

bool AnimBlendCtrl_Fifo3::isEnqueued(IPureAnimStateHolder &, IAnimBlendNode *) { G_ASSERT_RETURN(false, false); }
void AnimBlendCtrl_Fifo3::enqueueState(IPureAnimStateHolder &, IAnimBlendNode *, real, real) { G_ASSERT(0); }
int AnimBlendCtrl_ParametricSwitcher::getAnimForRange(real) { G_ASSERT_RETURN(false, 0); }

int AnimCommonStateHolder::getParamInt(int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setParamInt(int, int) { G_ASSERT(0); }
int AnimCommonStateHolder::getParamFlags(int, int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setParamFlags(int, int, int) { G_ASSERT(0); }
float AnimCommonStateHolder::getParamEffTimeScale(int) const { G_ASSERT_RETURN(false, 0.f); }
int AnimCommonStateHolder::getTimeScaleParamId(int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setTimeScaleParamId(int, int) { G_ASSERT(0); }
void AnimCommonStateHolder::advance(float) { G_ASSERT(0); }
void AnimCommonStateHolder::term() { G_ASSERT(0); }

void AnimBlender::buildNodeList() { G_ASSERT(0); }
} // namespace AnimV20

void Animate2ndPass::release() { G_ASSERT(0); }

namespace AnimCharV20
{
int getSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
int addSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getSlotName(const int) { G_ASSERT_RETURN(false, nullptr); }
} // namespace AnimCharV20

bool check_action_precondition(ecs::EntityId, int) { G_ASSERT_RETURN(false, false); }

#include <gamePhys/common/loc.h>
void gamephys::Loc::interpolate(const Loc &, const Loc &, const float) { G_ASSERT(0); }

#include <gamePhys/phys/commonPhysBase.h>
bool CommonPhysPartialState::deserialize(const danet::BitStream &, IPhysBase &) { G_ASSERT_RETURN(false, false); }

#include <statsd/statsd.h>
namespace statsd
{
void counter(const char *, long, std::initializer_list<MetricTag>) { G_ASSERT(0); }
void profile(const char *, long, const MetricTag &) { G_ASSERT(0); }
} // namespace statsd

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
bool sceneload::load_game_scene(const char *, int)
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

#include <landMesh/biomeQuery.h>
namespace biome_query
{
int query(const Point3 &, const float) { G_ASSERT_RETURN(false, {}); }
GpuReadbackResultState get_query_result(int, BiomeQueryResult &) { G_ASSERT_RETURN(false, {}); }
int get_biome_group_id(const char *) { G_ASSERT_RETURN(false, {}); }
const char *get_biome_group_name(int) { G_ASSERT_RETURN(false, {}); }
int get_num_biome_groups() { G_ASSERT_RETURN(false, 0); }
} // namespace biome_query

#include <gamePhys/props/physMatDamageModelProps.h>
/*static*/ const PhysMatDamageModelProps *PhysMatDamageModelProps::get_props(int) { G_ASSERT_RETURN(false, nullptr); }

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


#include "input/inputControls.h"
namespace controls
{
SensScale sens_scale;
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

#include <gui/dag_imgui.h>
ImGuiFunctionQueue *ImGuiFunctionQueue::windowHead = nullptr;
ImGuiFunctionQueue *ImGuiFunctionQueue::functionHead = nullptr;

ImGuiFunctionQueue::ImGuiFunctionQueue(
  const char *group_, const char *name_, const char *hotkey_, int priority_, int flags_, ImGuiFuncPtr func, bool is_window)
{}
ImGuiState imgui_get_state() { G_ASSERT_RETURN(false, ImGuiState::OFF); }
bool imgui_window_is_visible(const char *, const char *) { G_ASSERT_RETURN(false, false); }
int imgui_get_menu_bar_height() { G_ASSERT_RETURN(false, 0); }
void imgui_window_set_visible(const char *, const char *, const bool) { G_ASSERT(0); }
DataBlock *imgui_get_blk() { G_ASSERT_RETURN(false, nullptr); }

void imgui_set_default_font() { G_ASSERT(0); }
void imgui_set_bold_font() { G_ASSERT(0); }
void imgui_set_mono_font() { G_ASSERT(0); }
ImFont *imgui_get_bold_font() { G_ASSERT_RETURN(false, nullptr); }
ImFont *imgui_get_mono_font() { G_ASSERT_RETURN(false, nullptr); }
void imgui_apply_style_from_blk() { G_ASSERT(0); }

#include <daECS/scene/scene.h>
#include <daECS/io/blk.h>
namespace ecs
{
InitOnDemand<SceneManager> g_scenes;
}


#include <render/rendererFeatures.h>
class BaseTexture;
namespace bind_dascript
{
void toggleFeature(FeatureRenderFlags, bool) { G_ASSERT(0); }
bool worldRenderer_getWorldBBox3(BBox3 &bbox) { G_ASSERT_RETURN(false, false); }
void worldRenderer_shadowsInvalidate(const BBox3 &bbox) { G_ASSERT(0); }
void worldRenderer_invalidateAllShadows() { G_ASSERT(0); }
void worldRenderer_renderDebug() { G_ASSERT(0); }
int worldRenderer_getDynamicResolutionTargetFps() { G_ASSERT_RETURN(false, 0); }
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
void remove_puddles_in_crater(const Point3 &, float) { G_ASSERT(0); }

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

#include <render/debug3dSolidBuffered.h>
void flush_buffered_debug_meshes(bool) { G_ASSERT(0); }
void draw_debug_solid_mesh_buffered(const uint16_t *, int, const float *, int, int, const TMatrix &, Color4, size_t) { G_ASSERT(0); }

void draw_debug_solid_cube_buffered(const BBox3 &, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_triangle_buffered(Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_ball_buffered(const Point3 &, float, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_disk_buffered(const Point3, Point3, float, int, const Color4 &, size_t) { G_ASSERT(0); }
void clear_buffered_debug_solids() { G_ASSERT(0); }

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

#include <gamePhys/phys/destructableObject.h>
dag::ConstSpan<gamephys::DestructableObject *> destructables::getDestructableObjects()
{
  G_ASSERT_RETURN(false, dag::ConstSpan<gamephys::DestructableObject *>());
}

scene::TiledScene *create_ladders_scene(GameObjects &) { G_ASSERT_RETURN(false, nullptr); }

#include "phys/collRes.h"
bool get_collres_body_tm(const ecs::EntityId eid, const int coll_node_id, TMatrix &tm, const char *fnname)
{
  G_ASSERT_RETURN(false, false);
}

#include <landMesh/heightmapQuery.h>
int heightmap_query::query(const Point2 &) { G_ASSERT_RETURN(false, -1); }
GpuReadbackResultState heightmap_query::get_query_result(int, HeightmapQueryResult &)
{
  G_ASSERT_RETURN(false, GpuReadbackResultState::SYSTEM_NOT_INITIALIZED);
}

#include "net/dedicated/matching.h"
const Json::Value &dedicated_matching::get_mode_info()
{
  G_ASSERT(0);
  static Json::Value j0 = Json::nullValue;
  return j0;
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
namespace darg
{
E3DCOLOR Properties::getColor(const Sqrat::Object &, E3DCOLOR) const { G_ASSERT_RETURN(false, E3DCOLOR(0, 0, 0)); }
float Properties::getFloat(const Sqrat::Object &, float, bool) const { G_ASSERT_RETURN(false, 0.f); }
bool Properties::getBool(const Sqrat::Object &, bool, bool) const { G_ASSERT_RETURN(false, false); }
void Properties::trace_error(const char *, const Sqrat::Object &, const Sqrat::Object &) { G_ASSERT(0); }
int Properties::getFontId() const { G_ASSERT_RETURN(false, -1); }
float Properties::getFontSize() const { G_ASSERT_RETURN(false, 0.f); }
} // namespace darg

namespace char_random
{
int64_t generate_transaction_id() { G_ASSERT_RETURN(false, 0); };
} // namespace char_random
