#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstCollision.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/hash_set.h>

#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/treeDestr.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <gamePhys/collision/cachedCollisionObject.h>
#include <math/random/dag_random.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_critSec.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <gamePhys/phys/rendinstDestrRefl.h>
#include <math/dag_math3d.h>
#include <util/dag_convar.h>
#include <generic/dag_sort.h>
#include <perfMon/dag_statDrv.h>

CONSOLE_BOOL_VAL("ridestr", ri_debug_collision, false);

static constexpr int INITIAL_REPLICATION = 1;
static const Point3 RI_DESTROY_BBOX_MARGIN_UP = Point3(0, 0.05, 0);

static bool apply_reflection = false;
static bool apply_cached_destr = false;
static bool apply_delayed_extra_destr = false;
static rendinstdestr::create_tree_rend_inst_destr_cb create_tree_cb = NULL;
static rendinstdestr::remove_tree_rendinst_destr_cb rem_tree_cb = NULL;
static rendinstdestr::remove_physx_collision_object_callback rem_physx_collision_obj_cb = NULL;
static rendinstdestr::create_apex_actors_callback create_apex_actors_at_point_cb = NULL;
static rendinstdestr::apex_force_remove_actor_callback apex_force_remove_actor_cb = NULL;
static rendinstdestr::on_destr_changed_callback on_changed_destr_cb = NULL;
static rendinstdestr::get_camera_pos get_current_camera_pos = NULL;
static eastl::fixed_vector<rendinstdestr::on_rendinst_destroyed_callback, 1> on_rendinst_destroyed_callbacks;
static Tab<rendinstdestr::restorable_rendinst_callback> restorable_rendinst_callbacks;
static ri_tree_sound_cb g_tree_sound_cb = nullptr;
static rendinstdestr::DestrSettings destrSettings;
static rendinst::ri_damage_effect_cb ri_effect_cb = nullptr;
static PhysWorld *phys_world = NULL;
static danet::BitStream cachedDestr;
static WinCritSec delayed_ri_extra_destr_mutex;
static dag::Vector<rendinst::RendInstDesc> delayed_ri_extra_destruction;
static Tab<RendInstPhys> riPhys(midmem);
static Tab<CachedCollisionObjectInfo *> cachedCollisionObjects(midmem);
static CollisionObject tree_collision;
static void do_delayed_ri_extra_destruction_impl();
#if ENABLE_APEX
template <>
struct eastl::hash<rendinst::RendInstDesc>
{
  size_t operator()(const rendinst::RendInstDesc &val) const
  {
    uint32_t result = 2166136261U;
    result = (result * 16777619) ^ val.cellIdx;
    result = (result * 16777619) ^ val.pool;
    result = (result * 16777619) ^ val.offs;
    return (size_t)result;
  }
};
static eastl::hash_set<rendinst::RendInstDesc> apex_destructed_list;
#endif

static void call_restorable_rendinst_cb(const rendinst::RendInstDesc &desc, rendinstdestr::RestorableRendinstState state)
{
  for (rendinstdestr::restorable_rendinst_callback cb : restorable_rendinst_callbacks)
    cb(desc, state);
}

namespace rendinst
{
extern bool enable_apex;
};

class RestorableRendinst
{
  // To restore
  rendinst::RendInstDesc riDesc;
  rendinst::RendInstBufferData riBuffer;

public:
  destructables::id_t destrId = destructables::INVALID_ID;

private:
  float ttl;
  float atTime;
  rendinst::CollisionInfo collInfo;
  bool confirmedDestruction;
  rendinst::riex_handle_t generatedHandle;
#if ENABLE_APEX
  int apexDestrId;
#endif

public:
  RestorableRendinst(const rendinst::RendInstDesc &desc, const rendinst::RendInstBufferData &buffer, destructables::id_t destr_id,
    float at_time, const rendinst::CollisionInfo &coll_info, rendinst::riex_handle_t generated_handle, int apex_destr_id) :
    riDesc(desc),
    riBuffer(buffer),
    destrId(destr_id),
    ttl(1.f),
    atTime(at_time),
    collInfo(coll_info),
    confirmedDestruction(false),
    generatedHandle(generated_handle)
#if ENABLE_APEX
    ,
    apexDestrId(apex_destr_id)
#endif
  {
    G_UNUSED(apex_destr_id);
  }

  bool isRestoreNeeded() const { return !confirmedDestruction; }

  rendinst::riex_handle_t restore()
  {
    rendinst::riex_handle_t h = rendinst::restoreRiGenDestr(riDesc, riBuffer);
    rendinst::delRIGenExtra(generatedHandle);
    destructables::removeDestructableById(destrId);
    if (rem_tree_cb)
      rem_tree_cb(riDesc);
#if ENABLE_APEX
    if (rendinst::enable_apex && apex_force_remove_actor_cb && apexDestrId >= 0)
      apex_force_remove_actor_cb(apexDestrId);
    apex_destructed_list.erase(riDesc);
#endif
    return h;
  }

  bool update(float dt) // return false if need to be destroyed
  {
    ttl -= dt;
    return !confirmedDestruction && ttl > 0.f;
  }

  void confirmDestruction() { confirmedDestruction = true; }

  void invalidateHandle(rendinst::riex_handle_t invalidate_handle)
  {
    if (generatedHandle == invalidate_handle)
      generatedHandle = rendinst::RIEX_HANDLE_NULL;
  }

  const rendinst::RendInstDesc &getRiDesc() const { return riDesc; }
  float getAtTime() const { return atTime; }
  const rendinst::CollisionInfo &getCollInfo() const { return collInfo; }
};
static eastl::vector<RestorableRendinst> restorables;

static void invalidate_handle_cb(rendinst::riex_handle_t invalidate_handle)
{
  for (RestorableRendinst &restr : restorables)
    restr.invalidateHandle(invalidate_handle);
}

rendinstdestr::DestrSettings &rendinstdestr::get_mutable_destr_settings() { return destrSettings; }

const rendinstdestr::DestrSettings &rendinstdestr::get_destr_settings() { return destrSettings; }

void rendinstdestr::init_ex(mpi::ObjectID oid, rendinstdestr::on_destr_changed_callback on_destr_cb,
  create_tree_rend_inst_destr_cb create_tree_destr_cb, remove_tree_rendinst_destr_cb rem_tree_destr_cb, ri_tree_sound_cb tree_sound_cb,
  get_camera_pos get_current_camera_pos_, remove_physx_collision_object_callback remove_physx_obj_cb,
  create_apex_actors_callback create_apex_actors_cb, apex_force_remove_actor_callback apex_remove_actor_cb)
{
  on_changed_destr_cb = on_destr_cb;
  init_refl_object(oid);
  apply_cached_destr = true;
  apply_delayed_extra_destr = true;
  rendinst::enable_apex = false;
  create_tree_cb = create_tree_destr_cb;
  rem_tree_cb = rem_tree_destr_cb;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = remove_physx_obj_cb;
  create_apex_actors_at_point_cb = create_apex_actors_cb;
  apex_force_remove_actor_cb = apex_remove_actor_cb;
  get_current_camera_pos = get_current_camera_pos_;
  rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb);
  rendinst::do_delayed_ri_extra_destruction = do_delayed_ri_extra_destruction_impl;
}

void rendinstdestr::init(rendinstdestr::on_destr_changed_callback on_destr_cb, bool apply_cache,
  create_tree_rend_inst_destr_cb create_tree_destr_cb, remove_tree_rendinst_destr_cb rem_tree_destr_cb, ri_tree_sound_cb tree_sound_cb)
{
  on_changed_destr_cb = on_destr_cb;
  rendinst::enable_apex = false;

  apply_cached_destr = apply_cache;

  create_tree_cb = create_tree_destr_cb;
  rem_tree_cb = rem_tree_destr_cb;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = nullptr;
  create_apex_actors_at_point_cb = nullptr;
  apex_force_remove_actor_cb = nullptr;
}


void rendinstdestr::clear()
{
  restorables.clear();
  restorables.shrink_to_fit();
  clear_tree_collision();
  clear_all_ptr_items(cachedCollisionObjects);
  clear_phys_objs();
}

void rendinstdestr::shutdown()
{
  clear();
  endSession();
  destroy_refl_object();
  ri_effect_cb = nullptr;
  decltype(on_rendinst_destroyed_callbacks)().swap(on_rendinst_destroyed_callbacks);
}

void rendinstdestr::startSession(void *phys_wld)
{
  phys_world = static_cast<PhysWorld *>(phys_wld);
  enable_reflection_refl_object(true);
  apply_reflection = true;

  if (cachedDestr.GetNumberOfUnreadBits() > 0 && apply_cached_destr)
  {
    deserialize_destr_data(cachedDestr, INITIAL_REPLICATION);
    cachedDestr.Reset();
  }
}

void rendinstdestr::endSession()
{
  clear();
  enable_reflection_refl_object(false);
  apply_reflection = false;
  cachedDestr.Reset();

  WinAutoLock lock(delayed_ri_extra_destr_mutex);
  clear_and_shrink(delayed_ri_extra_destruction);
}

mpi::IObject *rendinstdestr::getReflectionObject() { return get_refl_object(); }

void rendinstdestr::testObjToRestorablesIntersection(const BBox3 & /*obj_box*/, const TMatrix & /*obj_tm*/,
  rendinst::RendInstCollisionCB *coll_cb, float at_time)
{
  for (auto &restr : restorables)
  {
    if (restr.getAtTime() < at_time)
      continue;
    if (rendinst::isRIGenPosInst(restr.getRiDesc()))
      coll_cb->addTreeCheck(restr.getCollInfo());
    else
      coll_cb->addCollisionCheck(restr.getCollInfo());
  }
}

void rendinstdestr::remove_restorable_by_destructable_id(destructables::id_t id)
{
  if (id == destructables::INVALID_ID)
    return;
  auto it = eastl::find(restorables.begin(), restorables.end(), id,
    [](const RestorableRendinst &rri, destructables::id_t destr_id) { return rri.destrId == destr_id; });
  if (it != restorables.end())
  {
    call_restorable_rendinst_cb(it->getRiDesc(), rendinstdestr::RRS_DESTROYED);
    restorables.erase(it);
  }
}

void rendinstdestr::setApexEnabled(bool enabled) { rendinst::enable_apex = enabled; }
void rendinstdestr::resetApexDestructedRIList()
{
#if ENABLE_APEX
  if (apex_destructed_list.size())
    debug("clearing apex_destructed_list=%d", apex_destructed_list.size());
  apex_destructed_list.clear();
#endif
}


using DestrNeighborsSet =
  eastl::vector_map<rendinst::riex_handle_t, rendinst::CollisionInfo, eastl::less<rendinst::riex_handle_t>, framemem_allocator>;

static void findRendinstNeighbors(rendinst::RendInstDesc desc, int destroy_neighbour_recursive_depth,
  const rendinst::CollisionInfo *coll_info, DestrNeighborsSet &ri_to_destroy)
{
  if (destroy_neighbour_recursive_depth <= 0 || !coll_info || !coll_info->isParent || !phys_world || !rendinst::isRiGenDescValid(desc))
    return;

  struct RiExtraCollisionCB final : public rendinst::RendInstCollisionCB
  {
    const rendinst::RendInstDesc &initiatorDesc;
    DestrNeighborsSet &destrNeighborsData;
    const int destroyNeighbourDepth;

    RiExtraCollisionCB(const rendinst::RendInstDesc &id, DestrNeighborsSet &dnd, int depth) :
      initiatorDesc(id), destrNeighborsData(dnd), destroyNeighbourDepth(depth)
    {}
    virtual void addCollisionCheck(const rendinst::CollisionInfo &coll_info) override
    {
      rendinst::riex_handle_t riHandle = rendinst::make_handle(coll_info.desc.pool, coll_info.desc.idx);
      if (coll_info.desc != initiatorDesc && coll_info.isDestr && !coll_info.isImmortal && coll_info.destructibleByParent)
        if (destrNeighborsData.find(riHandle) == destrNeighborsData.end())
        {
          destrNeighborsData.insert(eastl::make_pair(riHandle, coll_info));
          findRendinstNeighbors(coll_info.desc, destroyNeighbourDepth - 1, &coll_info, destrNeighborsData);
        }
    }
    virtual void addTreeCheck(const rendinst::CollisionInfo & /*coll_info*/) override {}
  } rendinstCallback(desc, ri_to_destroy, destroy_neighbour_recursive_depth);

  auto localBBox = coll_info->localBBox;
  localBBox.lim[1] += RI_DESTROY_BBOX_MARGIN_UP;
  rendinst::testObjToRIGenIntersection(localBBox, coll_info->tm, rendinstCallback, rendinst::GatherRiTypeFlag::RiExtraOnly);
}

static rendinst::RendInstDesc destroyRendinstInternal(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos,
  const Point3 &impulse, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr,
  rendinst::ri_damage_effect_cb effect_cb, bool is_client, ApexDmgInfo *apex_dmg_info, rendinstdestr::on_destr_callback on_destr_cb,
  rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(ridestr__destroyRendinstInternal);
  // Note: 'desc' is copied by value in order to avoid situations where desc = &coll_info->desc and
  // 'on_destr_cb' invalidates it.
  if ((add_restorable && !rendinst::isRiGenDescValid(desc)) || !phys_world) // desc is valid only when we provide it in non-offseted
                                                                            // state. otherwise we process tons of destroyRendinsts
    return desc;

#if ENABLE_APEX
  if (rendinst::enable_apex && rem_physx_collision_obj_cb)
  {
    TIME_PROFILE(destroyRI_rem_physx_collision_obj_cb);
    rem_physx_collision_obj_cb(desc);
  }
#endif

  TMatrix mainTm = TMatrix::IDENT;
  if (add_restorable)
    mainTm = rendinst::getRIGenMatrix(desc);
  else
    mainTm = rendinst::getRIGenMatrixDestr(desc);

  const uint32_t instSeed = desc.isRiExtra() ? rendinst::get_riextra_instance_seed(desc.getRiExtraHandle()) : 0;
  rendinst::RendInstBufferData riBuffer;
  DynamicPhysObjectData *poData = NULL;
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::RendInstDesc restorableCbDesc;
  rendinst::riex_handle_t generatedHandle = rendinst::RIEX_HANDLE_NULL;
  dacoll::invalidate_ri_instance(desc); // do before destring, otherwise in riextra we'll lose unique data and will not be able to
                                        // compare them

  if (on_destr_cb)
    on_destr_cb(desc);

  int32_t userData = coll_info ? coll_info->userData : -1;
  bool riRemoved = false;
  if (desc.isRiExtra() && desc.isDynamicRiExtra() && coll_info)
    rendinst::onRiExtraDestruction(rendinst::make_handle(desc.pool, desc.idx), true, userData);
  else if (add_restorable)
  {
    if (desc.isRiExtra() && is_client && coll_info)
    {
      if (auto restorableDesc = rendinst::get_restorable_desc(desc); restorableDesc.isValid())
      {
        // Note that we call restorable callback here before 'doRIGenDestr', that's
        // in order to be called before handle invalidate callbacks. Calling restorable
        // callback after handle invalidate callback is generally useless, since the handle
        // is already invalid and there's nothing for user to do.
        restorableCbDesc = restorableDesc;
        call_restorable_rendinst_cb(restorableCbDesc, rendinstdestr::RRS_CREATED);
      }
    }
    const Point3 *collPoint = (pos == ZERO<Point3>()) ? nullptr : &pos;
    poData = rendinst::doRIGenDestr(desc, riBuffer, offsetedDesc, impulse.length(), effect_cb, &generatedHandle, true, userData,
      collPoint, &riRemoved, flags);
    if (poData && restorableCbDesc.isValid())
    {
      G_ASSERT(offsetedDesc.cellIdx == restorableCbDesc.cellIdx);
      G_ASSERT(offsetedDesc.offs == restorableCbDesc.offs);
    }
  }
  else
    poData = rendinst::doRIGenDestrEx(desc, impulse.length(), effect_cb, userData);

  BBox3 bbox = flags & rendinst::DestrOptionFlag::UseFullBbox ? rendinst::getRIGenFullBBox(desc)
               : coll_info                                    ? coll_info->localBBox
                                                              : rendinst::getRIGenBBox(desc);

  rendinstdestr::call_on_rendinst_destroyed_cb(desc.getRiExtraHandle(), mainTm, bbox);

  bool apex_asset_created = false;
  int apex_destructible_id = -1;
#if ENABLE_APEX
  if (rendinst::enable_apex && create_destr && create_apex_actors_at_point_cb)
  {
    TMatrix normalizedTm = mainTm;
    Point3 scale = Point3(mainTm.getcol(0).length(), mainTm.getcol(1).length(), mainTm.getcol(2).length());

    normalizedTm.orthonormalize();
    if (apex_destructed_list.find(desc) == apex_destructed_list.end()) // skip duplicates from duplicated server packets
    {
      TIME_PROFILE(destroyRI_create_apex_actors_at_point_cb);
      apex_destructible_id =
        create_apex_actors_at_point_cb(getRIGenDestrName(desc), normalizedTm, scale, pos, impulse, desc.pool, apex_dmg_info, desc.idx);
      apex_destructed_list.insert(desc);
    }
    else
      logwarn("skip duplicate apex destruction of RI=%d:%d~%d:%d (apex_destructed_list=%d)", desc.cellIdx, desc.idx, desc.pool,
        desc.offs, apex_destructed_list.size());
    apex_asset_created = apex_destructible_id != -1; // NOTE: check exactly != -1 (not >=0, ..._cb may return negative values,
                                                     // depending on its logic)
  }
#else
  G_UNUSED(apex_dmg_info);
#endif

  destructables::id_t destrId = destructables::INVALID_ID;
  if (!apex_asset_created && poData && create_destr) //-V560
  {
    gamephys::DestructableObject *destr = NULL;
    const Point3 &campos = get_current_camera_pos ? get_current_camera_pos() : mainTm.getcol(3);
    destrId = destructables::addDestructable(&destr, poData, mainTm, phys_world, campos, desc.pool, instSeed);
    if (destr) //-V1051
    {
      destr->ttl = desc.isRiExtra() ? rendinst::getTtl(desc) : 15.f;
      if (impulse != ZERO<Point3>())
        destr->addImpulse(*phys_world, pos, impulse);
    }
  }

  if (add_restorable && is_client && coll_info && riRemoved)
  {
    // At this point coll_info->desc might be invalid, since 'on_destr_cb' can
    // call 'reset' on it, but we need proper coll_info for restorable so that
    // vehicle collisions could be checked against it, so copy proper desc over.
    rendinst::CollisionInfo tmp = *coll_info;
    tmp.desc = desc;
    restorables.emplace_back(offsetedDesc, riBuffer, destrId, at_time, tmp, generatedHandle, apex_destructible_id);
  }
  else if (restorableCbDesc.isValid())
    call_restorable_rendinst_cb(restorableCbDesc, rendinstdestr::RRS_DESTROYED);

  if (on_changed_destr_cb)
    on_changed_destr_cb(desc);

  return offsetedDesc;
}

void rendinstdestr::debug_draw_ri_phys()
{
  for (auto &phys : riPhys)
  {
    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    BBox3 resBox;
    resBox += as_point3(&bbox_transformed.bmin);
    resBox += as_point3(&bbox_transformed.bmax);
    draw_cached_debug_box(resBox, E3DCOLOR(0, 0, 255));

    end_draw_cached_debug_lines();
    set_cached_debug_lines_wtm(TMatrix::IDENT);
  }
}

bbox3f rendinstdestr::get_ri_phys_containing_bbox(const bbox3f &intersect_bbox, const Frustum &intersect_frustum, bool only_trees)
{
  bbox3f resultBbox;
  v_bbox3_init_empty(resultBbox);
  for (auto &phys : riPhys)
  {
    if (only_trees && phys.type != RendInstPhysType::TREE)
      continue;

    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    if (!v_bbox3_test_box_intersect(bbox_transformed, intersect_bbox))
      continue;
    if (!intersect_frustum.testBoxB(bbox_transformed.bmin, bbox_transformed.bmax))
      continue;

    v_bbox3_add_pt(resultBbox, bbox_transformed.bmin);
    v_bbox3_add_pt(resultBbox, bbox_transformed.bmax);
  }
  return resultBbox;
}

rendinst::RendInstDesc rendinstdestr::destroyRendinst(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos,
  const Point3 &impulse, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr,
  rendinst::ri_damage_effect_cb effect_cb, bool is_client, ApexDmgInfo *apex_dmg_info, int destroy_neighbour_recursive_depth,
  float impulse_mult_for_child, on_destr_callback on_destr_cb, rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(rendinstdestr__destroyRendinst);
  // Note: 'desc' is copied by value in order to avoid situations where desc = &coll_info->desc and
  // 'on_destr_cb' invalidates it.
  if ((add_restorable && !rendinst::isRiGenDescValid(desc)) || !phys_world) // desc is valid only when we provide it in non-offseted
                                                                            // state. otherwise we process tons of destroyRendinsts
    return desc;

  DestrNeighborsSet riToDestroy;
  findRendinstNeighbors(desc, destroy_neighbour_recursive_depth, coll_info, riToDestroy);

  // first lets try to destroy parent object first
  const auto ret = destroyRendinstInternal(desc, add_restorable, pos, impulse, at_time, coll_info, create_destr, effect_cb, is_client,
    apex_dmg_info, on_destr_cb, flags);

  for (auto &it : riToDestroy)
    destroyRendinstInternal(it.second.desc, add_restorable, pos, impulse * impulse_mult_for_child, at_time, &it.second, create_destr,
      effect_cb, is_client, apex_dmg_info, on_destr_cb, flags | rendinst::DestrOptionFlag::ForceDestroy);
  return ret;
}

void rendinstdestr::destroyRiExtra(rendinst::riex_handle_t riex_handle, const TMatrix &transform,
  rendinst::ri_damage_effect_cb effect_cb)
{
  uint32_t instSeed = rendinst::get_riextra_instance_seed(riex_handle);
  if (DynamicPhysObjectData *poData = rendinst::doRIExGenDestrEx(riex_handle, effect_cb))
  {
    uint32_t res_idx = rendinst::handle_to_ri_type(riex_handle);
    const Point3 &campos = get_current_camera_pos ? get_current_camera_pos() : transform.getcol(3);
    gamephys::DestructableObject *destr = NULL;
    destructables::addDestructable(&destr, poData, transform, phys_world, campos, res_idx, instSeed);
    if (destr)
      destr->ttl = rendinst::get_riextra_ttl(riex_handle);
  }
}

void rendinstdestr::update(float dt, const Frustum *frustum)
{
  TIME_PROFILE(rendinstdestr_update);

  {
    TIME_PROFILE_DEV(update_cached_collision_objects);
    for (int i = cachedCollisionObjects.size() - 1; i >= 0; --i)
      if (!cachedCollisionObjects[i]->update(dt))
        erase_ptr_items(cachedCollisionObjects, i, 1);
  }

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);

  {
    TIME_PROFILE_DEV(riPhys);

    struct RendInstUpdateTm
    {
      rendinst::riex_handle_t id;
      TMatrix tm;
      bool moved;
    };

    eastl::vector<RendInstUpdateTm, framemem_allocator> updateList;
    updateList.reserve(riPhys.size());
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];

      bool physObject = phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT;

      bool actived = phys.ttl >= 0.f;
      if (physObject)
      {
        if ((phys.physBody && phys.physBody->isActive() && phys.physBody->getInteractionLayer()) ||
            (phys.physBody && phys.additionalBody && phys.additionalBody->isActive() && phys.additionalBody->getInteractionLayer()))
        {
          if (lengthSq(phys.physBody->getVelocity()) > sqr(20.f) || phys.maxTtl < 0.f)
          {
            phys.physBody->setTm(phys.lastValidTm * phys.centerOfMassTm);
            phys.physBody->setVelocity(Point3(0.f, 0.f, 0.f));
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));
            phys.ttl = -1.f;
          }
          else
            phys.ttl = 5.f;
        }
        else
        {
          if (phys.physBody && !phys.physBody->getInteractionLayer())
            phys.physBody->activateBody(true);
          if (phys.additionalBody && !phys.additionalBody->getInteractionLayer())
            phys.additionalBody->activateBody(true);
          phys.ttl -= dt;
        }
      }
      else
      {
        if (phys.physModel->isActive())
          phys.ttl = 1.f;
        else
          phys.ttl -= dt;
      }
      if (actived && physObject && phys.maxLifeDist > 0.f)
      {
        const float distSq = lengthSq(phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3));
        if (distSq > sqr(phys.maxLifeDist))
        {
          phys.maxLifeDist = -1.f;
          phys.maxTtl = -1.f;
          phys.ttl = -1.f;
        }
      }
      if (actived && physObject && phys.distConstrainedPhys > 0.f && phys.physBody && phys.physBody->getInteractionLayer())
      {
        const float distSq = lengthSq(phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3));
        if (distSq > sqr(phys.distConstrainedPhys))
        {
          phys.distConstrainedPhys = -1.f;
          phys.physBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
          if (phys.additionalBody)
            phys.additionalBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
        }
      }
      phys.maxTtl -= dt;
      if (phys.ttl < 0.f)
      {
        if (!physObject)
        {
          if (rendinst::returnRIGenExternalControl(phys.desc, phys.ri))
          {
            phys.cleanup();
            erase_items(riPhys, i, 1);
            i--;
            continue;
          }
        }
        else if (actived && phys.physBody && phys.physBody->getInteractionLayer())
        {
          phys.physBody->setGroupAndLayerMask(0, 0);
          phys.physBody->setGravity(Point3(0.f, -0.1f, 0.f));
          phys.physBody->activateBody(true);
          if (phys.treeSound.inited())
            phys.treeSound.setFalled(phys.lastValidTm);
          clear_all_ptr_items_and_shrink(phys.joints);
          del_it(phys.additionalBody);
          phys.ttl = 15.f;
          phys.maxTtl = 15.f;
        }
        else if (phys.physBody)
        {
          if (rendinst::removeRIGenExternalControl(phys.desc, phys.ri))
          {
            phys.cleanup();
            erase_items(riPhys, i, 1);
            i--;
          }
          continue;
        }
      }

      phys.physModel->update(dt);

      DECL_ALIGN16(TMatrix, tm) = TMatrix::IDENT;
      if (physObject)
      {
        if (phys.physBody)
        {
          phys.physBody->getTm(tm);
          if (ri_debug_collision)
            dacoll::draw_phys_body(phys.physBody);
        }
        TMatrix additionalTm = TMatrix::IDENT;
        if (phys.additionalBody)
        {
          phys.additionalBody->getTm(additionalTm);
          if (ri_debug_collision)
            dacoll::draw_phys_body(phys.additionalBody);
        }
        tm *= inverse(phys.centerOfMassTm);
        phys.lastValidTm = tm;

        if (phys.additionalBody && phys.treeSound.inited() && !phys.treeSound.falled())
        {
          struct WrapperContactResultCallback
          {
            typedef ::CollisionObjectUserData obj_user_data_t;
            typedef ::gamephys::CollisionContactData contact_data_t;
            bool haveContact;
            WrapperContactResultCallback() : haveContact(false) {}
            float addSingleResult(const contact_data_t &, obj_user_data_t *, obj_user_data_t * /*objB*/, void *)
            {
              haveContact = true;
              return 0.f;
            }
            static void visualDebugForSingleResult(...) {}
            bool needsCollision(obj_user_data_t *, bool b_is_static) { return b_is_static; }
          } contactCallback;

          physWorld->contactTest(phys.additionalBody, contactCallback);
          if (contactCallback.haveContact)
            phys.treeSound.setFalled(additionalTm);
          else
            phys.treeSound.updateFalling(tm);
        }
      }
      else
      {
        phys.physModel->location.toTM(tm);
        phys.riColObj.body->setTmInstant(tm);
      }

      tm.setcol(0, tm.getcol(0) * phys.scale.x);
      tm.setcol(1, tm.getcol(1) * phys.scale.y);
      tm.setcol(2, tm.getcol(2) * phys.scale.z);

      BBox3 bbox = rendinst::getRIGenBBox(phys.desc);
      bbox = BBox3(bbox.center() * tm, bbox.width().length());
      if (frustum && !frustum->testBoxB(bbox))
        continue;
      RendInstUpdateTm &destrPos = updateList.push_back();
      destrPos.id = phys.ri->riHandle;
      destrPos.tm = tm;
      destrPos.moved = physObject;
    }
    if (!updateList.empty())
    {
      TIME_PROFILE(rendinstDestrTmUpdate);
      rendinst::ScopedRIExtraWriteLock wr;
      TMatrix4_vec4 m4;
      for (auto &l : updateList)
      {
        m4 = l.tm;
        rendinst::moveRIGenExtra44(l.id, (mat44f &)m4, l.moved, true);
      }
    }
  }

  TIME_PROFILE_DEV(restorables_update);
  for (auto it = restorables.begin(); it != restorables.end();)
  {
    if (it->update(dt))
      ++it;
    else
    {
      bool restoreNeeded = it->isRestoreNeeded();
      rifx::composite::setEntityApproved(it->getRiDesc().getRiExtraHandle(), !restoreNeeded);
      if (restoreNeeded)
      {
        rendinst::riex_handle_t h = it->restore();
        rendinst::RendInstDesc desc = it->getRiDesc();
        if (h == rendinst::RIEX_HANDLE_NULL)
        {
          // Failed to recreate rendinst, consider it's gone for good.
          call_restorable_rendinst_cb(desc, rendinstdestr::RRS_DESTROYED);
        }
        else
        {
          // Rendinst recreated, store new handle in desc and pass to cb.
          desc.idx = rendinst::handle_to_ri_inst(h);
          G_ASSERT(rendinst::handle_to_ri_type(h) == desc.pool);
          rendinstdestr::RestorableRendinstState state = rendinstdestr::RRS_RESTORED; // Start with 'restored' state.
          for (rendinstdestr::restorable_rendinst_callback cb : restorable_rendinst_callbacks)
          {
            if (!cb(desc, state))
            {
              // Restoration canceled, report 'destroyed' to the rest.
              state = rendinstdestr::RRS_DESTROYED;
              desc.idx = 0;
            }
          }
        }
      }
      else
      {
        call_restorable_rendinst_cb(it->getRiDesc(), rendinstdestr::RRS_DESTROYED);
      }
      it = restorables.erase(it);
    }
  }
}

static bool sort_by_offset(const rendinst::DestroyedInstanceRange &left, const rendinst::DestroyedInstanceRange &right)
{
  return left.startOffs < right.startOffs;
}

bool rendinstdestr::serialize_destr_data(danet::BitStream &bs)
{
  uint16_t cellCount = 0;
  const bool written = rendinst::getDestrCellData(0 /*primary layer*/, [&](const Tab<rendinst::DestroyedCellData> &destrCellData) {
    G_ASSERT(destrCellData.size() <= USHRT_MAX);
    cellCount = min<uint32_t>(destrCellData.size(), USHRT_MAX);

    bs.WriteCompressed(cellCount);
    for (int i = 0; i < cellCount; ++i)
    {
      const rendinst::DestroyedCellData &cell = destrCellData[i];
      G_ASSERT(cell.destroyedPoolInfo.size() <= USHRT_MAX);
      uint16_t poolCount = min<uint32_t>(cell.destroyedPoolInfo.size(), USHRT_MAX);

      bs.Write(int16_t(cell.cellId));
      bs.WriteCompressed(poolCount);
      uint8_t shift = cell.cellId < 0 ? 4 : 3;
      for (int j = 0; j < poolCount; ++j)
      {
        const rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
        G_ASSERT(pool.destroyedInstances.size() <= USHRT_MAX);
        G_ASSERT(eastl::is_sorted(pool.destroyedInstances.begin(), pool.destroyedInstances.end(), sort_by_offset));
        G_UNUSED(&sort_by_offset);
        uint16_t rangeCount = min<uint32_t>(pool.destroyedInstances.size(), USHRT_MAX);

        bs.WriteCompressed(uint16_t(pool.poolIdx));
        bs.WriteCompressed(rangeCount);
        uint32_t curRange = 0;
        for (int k = 0; k < rangeCount; ++k)
        {
          const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
          G_ASSERTF(!(range.startOffs % (1u << shift)) && !(range.endOffs % (1u << shift)),
            "startOffs=0x%X endOffs=0x%X cellId=%d poolIdx=%d div=%d", range.startOffs, range.endOffs, cell.cellId, pool.poolIdx,
            1u << shift);
          uint32_t startOffsDiv = range.startOffs >> shift;
          uint32_t endOffsDiv = range.endOffs >> shift;
          G_ASSERT(startOffsDiv >= curRange);
          bs.WriteCompressed(startOffsDiv - curRange);
          bs.WriteCompressed(endOffsDiv - startOffsDiv);
          curRange = endOffsDiv;
        }
      }
    }

    return true;
  });

  // if nothing was written, just write 0 cell count
  if (!written)
    bs.WriteCompressed(cellCount);
  return cellCount > 0;
}

static void do_delayed_ri_extra_destruction_impl()
{
  using namespace rendinstdestr;
  if (!apply_delayed_extra_destr)
    return;

  WinAutoLock lock(delayed_ri_extra_destr_mutex);
  for (auto it = delayed_ri_extra_destruction.begin(); it != delayed_ri_extra_destruction.end();)
  {
    auto desc = *it;
    if (const auto idx = rendinst::find_restorable_data_index(desc); idx >= 0)
    {
      desc.idx = idx;
      destroyRendinst(desc, false, ZERO<Point3>(), ZERO<Point3>(), 0.f, NULL, get_ri_damage_effect_cb() != nullptr,
        get_ri_damage_effect_cb(), get_destr_settings().isClient);
      it = delayed_ri_extra_destruction.erase_unsorted(it);
    }
    else
    {
      ++it;
    }
  }
}

void rendinstdestr::deserialize_destr_data(const danet::BitStream &bs, int apply_flags, int max_simultaneous_destrs)
{
  // Temporary structure to keep it
  Tab<rendinst::DestroyedCellData> cellsNewDestrInfo(framemem_ptr());
  int newDestrs = 0;

  // always read cell count
  uint16_t cellCount = 0;
  if (!apply_reflection)
  {
    cachedDestr = bs;
    bs.ReadCompressed(cellCount);
    for (int i = 0; i < cellCount; ++i)
    {
      uint16_t poolCount = 0;

      bs.IgnoreBytes(sizeof(int16_t));
      bs.ReadCompressed(poolCount);
      for (int j = 0; j < poolCount; ++j)
      {
        uint16_t rangeCount = 0;
        uint16_t poolIdx = 0;

        bs.ReadCompressed(poolIdx);
        bs.ReadCompressed(rangeCount);

        bs.IgnoreBytes(2 * sizeof(uint16_t) * rangeCount);
      }
    }
    return;
  }
  bs.ReadCompressed(cellCount);

  rendinst::getDestrCellData(0 /*primary layer*/, [&](const Tab<rendinst::DestroyedCellData> &destrCellData) {
    cellsNewDestrInfo.resize(cellCount);

    for (int i = 0; i < cellCount; ++i)
    {
      int16_t cellId = 0;
      uint16_t poolCount = 0;

      bs.Read(cellId);
      bs.ReadCompressed(poolCount);

      // Search for cell
      const rendinst::DestroyedCellData *cell = NULL;
      for (int si = 0; si < destrCellData.size(); ++si)
      {
        if (destrCellData[si].cellId != cellId)
          continue;
        // Found this cell in our local list
        cell = &destrCellData[si];
        break;
      }
      cellsNewDestrInfo[i].cellId = cellId;
      dag::set_allocator(cellsNewDestrInfo[i].destroyedPoolInfo, framemem_ptr());
      cellsNewDestrInfo[i].destroyedPoolInfo.resize(poolCount);
      uint8_t shift = cellId < 0 ? 4 : 3;
      for (int j = 0; j < poolCount; ++j)
      {
        uint16_t poolIdx = 0;
        uint16_t rangeCount = 0;

        bs.ReadCompressed(poolIdx);
        bs.ReadCompressed(rangeCount);

        cellsNewDestrInfo[i].destroyedPoolInfo[j].poolIdx = poolIdx;
        dag::set_allocator(cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances, framemem_ptr());

        const rendinst::DestroyedPoolData *pool = NULL;
        if (cell)
        {
          // If we have this cell - search for pool in it
          for (int sj = 0; sj < cell->destroyedPoolInfo.size(); ++sj)
          {
            if (cell->destroyedPoolInfo[sj].poolIdx != poolIdx)
              continue;
            // Ok, pool found. Now we need to match ranges.
            pool = &cell->destroyedPoolInfo[sj];
            break;
          }
        }
        int lastIdx = 0;
        int stride = rendinst::getRIGenStride(0, cellId, poolIdx);
        unsigned curRange = 0;
        for (int k = 0; k < rangeCount; ++k)
        {
          unsigned start = 0;
          unsigned end = 0;

          bs.ReadCompressed(start);
          bs.ReadCompressed(end);
          start += curRange;
          end += start;
          curRange = end;

          int startOffs = start << shift;
          int endOffs = end << shift;

          if (pool)
          {
            rendinst::DestroyedInstanceRange *addedRange = NULL;
            bool finished = false;
            for (; lastIdx < pool->destroyedInstances.size();)
            {
              const rendinst::DestroyedInstanceRange &range = pool->destroyedInstances[lastIdx];
              if (addedRange)
              {
                if (endOffs > range.endOffs)
                {
                  // Finish prev one.
                  addedRange->endOffs = range.startOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;

                  // Start new one
                  addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                  addedRange->startOffs = range.endOffs;
                  addedRange->endOffs = endOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                  lastIdx++; // continue to next one
                }
                else if (endOffs >= range.startOffs)
                {
                  // Just finish prev one and break;
                  addedRange->endOffs = range.startOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                  addedRange = NULL;
                  finished = true;
                  break;
                }
                else
                {
                  // It's before start, so just finish it and break;
                  addedRange->endOffs = endOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                  addedRange = NULL;
                  finished = true;
                  break;
                }
              }
              else
              {
                if (startOffs < range.endOffs && startOffs >= range.startOffs)
                {
                  // It's starts inside range, so we need to place start marker at end of this range.
                  if (endOffs > range.endOffs)
                  {
                    addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                    addedRange->startOffs = range.endOffs;
                    addedRange->endOffs = endOffs;
                    newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                    lastIdx++;
                  }
                  else
                  {
                    finished = true;
                    break; // it's consumed by it
                  }
                }
                else if (startOffs < range.startOffs)
                {
                  // It starts before range (as it's sorted, so it should be good)
                  addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                  addedRange->startOffs = startOffs;
                  addedRange->endOffs = startOffs;
                  // Do not skip to next one, this one should be finished first.
                }
                else
                  lastIdx++;
              }
            }
            if (!finished && !addedRange)
            {
              rendinst::DestroyedInstanceRange &range = cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
              range.startOffs = startOffs;
              range.endOffs = endOffs;
              newDestrs += (endOffs - startOffs) / stride;
            }
            // Great that also means that there's point to search for restorables right here, to confirm it and delete it from tab
            for (auto &restr : restorables)
            {
              const rendinst::RendInstDesc &desc = restr.getRiDesc();
              if (desc.cellIdx != cellId || desc.pool != poolIdx || desc.offs < startOffs || desc.offs >= endOffs)
                continue;
              restr.confirmDestruction();
            }
          }
          else
          {
            // if no pool found previously - just simply add everything.
            rendinst::DestroyedInstanceRange &range = cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
            range.startOffs = startOffs;
            range.endOffs = endOffs;
            newDestrs += (endOffs - startOffs) / stride;
          }
        }
      }
    }
    return true;
  });

  newDestrs = ((apply_flags & INITIAL_REPLICATION) != 0) ? 0 : min(newDestrs, max_simultaneous_destrs);
  dag::Vector<rendinst::RendInstDesc, framemem_allocator> delayedRiExtraDestruction;

  for (int i = 0; i < cellsNewDestrInfo.size(); ++i)
  {
    const rendinst::DestroyedCellData &cell = cellsNewDestrInfo[i];
    bool shouldUpdateVb = false;
    for (int j = 0; j < cell.destroyedPoolInfo.size(); ++j)
    {
      const rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
      int stride = rendinst::getRIGenStride(0, cell.cellId, pool.poolIdx);
      for (int k = 0; k < pool.destroyedInstances.size(); ++k)
      {
        const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
        for (int offs = range.startOffs; offs < range.endOffs; offs += stride)
        {
          rendinst::RendInstDesc desc(cell.cellId, 0, pool.poolIdx, offs, 0);
          if (rendinst::isRIGenPosInst(desc))
          {
            int poolIdxBasedSeed = offs + (pool.poolIdx) ^ (cell.cellId < 16);
            float angle = _srnd(poolIdxBasedSeed) * PI;
            if (create_tree_cb)
              create_tree_cb(desc, false, Point3(cos(angle), 0.f, sin(angle)), true, true, 0.5f, 0.f, NULL, (newDestrs-- > 0));
          }
          else
          {
            G_ASSERT(desc.idx == 0);
            bool ok = true;
            if (desc.isRiExtra())
            {
              ok = false;

              if (const auto idx = rendinst::find_restorable_data_index(desc); idx >= 0)
              {
                desc.idx = idx;
                ok = true;
              }
              else if (apply_delayed_extra_destr)
              {
                // if ri extra was not found, add it to delayed destruction list to destroy it
                // in do_delayed_ri_extra_destruction, if it will appear later
                delayedRiExtraDestruction.push_back(desc);
              }
            }
            // Only call destroyRendinst if a handle (i.e. idx exists) is found, if it's not found,
            // then desc.idx will be 0 and will trigger all kinds of side effects with make_handle(pool, 0).
            // For the same reason we should NEVER call destroyRendinst without desc.idx being properly set,
            // because e.g. rendinst::doRIGenDestrEx will fail to delete RI due to this:
            //   uniqueData = riExtra[desc.pool].riUniqueData.at(desc.idx);
            //   if (riExtra[desc.pool].isDynamicRendinst || (uniqueData && uniqueData->cellId < 0))
            //     return NULL;
            if (ok)
              rendinstdestr::destroyRendinst(desc, false, ZERO<Point3>(), ZERO<Point3>(), 0.f, NULL,
                (newDestrs-- > 0) && get_ri_damage_effect_cb() != nullptr, get_ri_damage_effect_cb(), get_destr_settings().isClient);
          }
          shouldUpdateVb = true;
        }
      }
    }
    if (shouldUpdateVb && cell.cellId >= 0)
      rendinst::updateRiGenVbCell(0, cell.cellId);
  }

  if (!delayedRiExtraDestruction.empty())
  {
    WinAutoLock lock(delayed_ri_extra_destr_mutex);
    for (const auto &desc : delayedRiExtraDestruction)
    {
      if (eastl::find(delayed_ri_extra_destruction.begin(), delayed_ri_extra_destruction.end(), desc) ==
          delayed_ri_extra_destruction.end())
        delayed_ri_extra_destruction.push_back(desc);
    }
  }
}

bool rendinstdestr::apply_damage_to_riextra(rendinst::riex_handle_t handle, float dmg, const Point3 &pos, const Point3 &impulse,
  float at_time)
{
  if (!rendinst::isRiGenExtraValid(handle))
    return false;
  bool isDestr = false;
  apply_damage_to_ri(rendinst::RendInstDesc(handle), dmg, 0.f, pos, impulse, at_time, destrSettings.createDestr, ri_effect_cb,
    destrSettings.isClient, 1.f, &isDestr);
  return isDestr;
}

void rendinstdestr::apply_damage_to_ri(const rendinst::RendInstDesc &desc, float dmg, float impulse_to_hp, const Point3 &pos,
  const Point3 &impulse, float at_time, bool create_destr, rendinst::ri_damage_effect_cb effect_cb, bool is_client,
  float impulse_mult_for_child, bool *isDestroyed)
{
  rendinst::CollisionInfo collInfo(desc);
  collInfo = rendinst::getRiGenDestrInfo(desc);
  float hp = collInfo.hp > 0.f ? collInfo.hp : collInfo.destrImpulse * impulse_to_hp;
  if (desc.isRiExtra() && collInfo.hp > 0.f)
  {
    if (rendinst::applyDamageRIGenExtra(desc, dmg))
      dmg = hp + 1.f;
    else
    {
      rendinst::play_riextra_dmg_fx(desc.getRiExtraHandle(), pos, effect_cb);
      dmg = hp - 1.f;
    }
  }

  if (isDestroyed)
    *isDestroyed = collInfo.initialHp > 0.f && dmg >= hp;
  if (collInfo.isDestr && dmg >= hp)
    rendinstdestr::destroyRendinst(desc, true, pos, impulse, at_time, &collInfo, create_destr, effect_cb, is_client, NULL,
      collInfo.destroyNeighbourDepth, impulse_mult_for_child);
}

struct RiDmgCallback : public rendinst::RendInstCollisionCB
{
  Point3 pos;
  float rad;
  Point2 dmgNearFar;
  float atTime;
  bool createDestr;
  bool isClient;
  rendinst::ri_damage_effect_cb effectCb;
  Tab<rendinst::CollisionInfo> damageToProcess;

  RiDmgCallback(const Point3 &position, float radius, const Point2 &dmg_near_far, float at_time, bool create_destr,
    rendinst::ri_damage_effect_cb effect_cb, bool is_client) :
    pos(position),
    rad(radius),
    dmgNearFar(dmg_near_far),
    atTime(at_time),
    createDestr(create_destr),
    isClient(is_client),
    effectCb(effect_cb)
  {}

  void processDamage(const rendinst::CollisionInfo &coll_info)
  {
    if (!coll_info.isImmortal)
      damageToProcess.push_back(coll_info);
  }

  void processAllDamage(rendinstdestr::on_riextra_destroyed_callback &&riex_destr_cb,
    rendinstdestr::riextra_should_damage &&riex_should_damage)
  {
    for (const rendinst::CollisionInfo &info : damageToProcess)
      processOneRendinst(info, eastl::move(riex_destr_cb), eastl::move(riex_should_damage));
  }

  void processOneRendinst(const rendinst::CollisionInfo &coll_info, const rendinstdestr::on_riextra_destroyed_callback &&riex_destr_cb,
    const rendinstdestr::riextra_should_damage &&riex_should_damage)
  {
    if (!coll_info.desc.isValid())
      return;

    if (coll_info.desc.isRiExtra() && riex_should_damage &&
        !riex_should_damage(rendinst::make_handle(coll_info.desc.pool, coll_info.desc.idx)))
      return;

    if (coll_info.hp <= 0.f)
      return;

    Point3 centerPos = coll_info.tm * coll_info.localBBox.center();
    Point3 dir = centerPos - pos;
    float dist = length(dir);
    if (dist > rad)
      return;

    dir *= safeinv(dist);
    if (dacoll::traceray_normalized(pos, dir, dist, nullptr, nullptr, dacoll::ETF_DEFAULT | dacoll::ETF_RI_PHYS))
    {
      Point3 resPos = pos + dir * dist;
      if (!((coll_info.tm * coll_info.localBBox) & resPos))
        return;
    }

    float hp = coll_info.hp;
    float dmg = cvt(dist, 0.f, rad, dmgNearFar.x, dmgNearFar.y);
    float localDmg = dmg;
    if (coll_info.desc.isRiExtra())
    {
      if (rendinst::applyDamageRIGenExtra(coll_info.desc, localDmg))
        localDmg = hp + 1.f;
      else
        localDmg = hp - 1.f;
    }
    if (localDmg >= hp)
    {
      auto onDestroyCb = [this, riex_destr_cb](const rendinst::RendInstDesc &desc) {
        rendinst::riex_handle_t id = rendinst::make_handle(desc.pool, desc.idx);
        riex_destr_cb(id);

        for (rendinst::CollisionInfo &info : damageToProcess)
          if (rendinst::make_handle(info.desc.pool, info.desc.idx) == id)
            info.desc.reset();
      };

      if (coll_info.isDestr)
        rendinstdestr::destroyRendinst(coll_info.desc, true, pos, dir * dmg, atTime, &coll_info, createDestr, effectCb, isClient,
          nullptr, coll_info.destroyNeighbourDepth, 1.f, eastl::move(onDestroyCb));
      else
        riex_destr_cb(rendinst::make_handle(coll_info.desc.pool, coll_info.desc.idx));
    }
  }

  void addCollisionCheck(const rendinst::CollisionInfo &coll_info) override { processDamage(coll_info); }
  void addTreeCheck(const rendinst::CollisionInfo &coll_info) override { processDamage(coll_info); }
};

void rendinstdestr::damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float at_time, bool create_destr,
  rendinst::ri_damage_effect_cb effect_cb, on_riextra_destroyed_callback &&riex_destr_cb, bool is_client,
  riextra_should_damage &&should_damage)
{
  BBox3 objBox(Point3(0.f, 0.f, 0.f), rad);
  TMatrix objTm = TMatrix::IDENT;
  objTm.setcol(3, pos);
  RiDmgCallback cb(pos, rad, dmg_near_far, at_time, create_destr, effect_cb, is_client);
  rendinst::testObjToRIGenIntersection(objBox, objTm, cb, rendinst::GatherRiTypeFlag::RiGenAndExtra);
  cb.processAllDamage(eastl::move(riex_destr_cb), eastl::move(should_damage));
}

void rendinstdestr::set_on_rendinst_destroyed_cb(on_rendinst_destroyed_callback cb)
{
  auto &cblist = on_rendinst_destroyed_callbacks;
  if (cb && eastl::find(cblist.begin(), cblist.end(), cb) == cblist.end())
    cblist.push_back(cb);
}

void rendinstdestr::call_on_rendinst_destroyed_cb(rendinst::riex_handle_t handle, const TMatrix &tm, const BBox3 &box)
{
  for (on_rendinst_destroyed_callback cb : on_rendinst_destroyed_callbacks)
    cb(handle, tm, box);
}

CollisionObject &rendinstdestr::get_tree_collision() { return tree_collision; }

void rendinstdestr::clear_tree_collision()
{
  if (tree_collision)
    dacoll::destroy_dynamic_collision(tree_collision);
  tree_collision.clear_ptrs();
}

void rendinstdestr::set_ri_damage_effect_cb(rendinst::ri_damage_effect_cb cb) { ri_effect_cb = cb; }

void rendinstdestr::register_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  if (find_value_idx(restorable_rendinst_callbacks, cb) == -1)
    restorable_rendinst_callbacks.push_back(cb);
}

bool rendinstdestr::unregister_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  return erase_item_by_value(restorable_rendinst_callbacks, cb);
}

rendinst::ri_damage_effect_cb rendinstdestr::get_ri_damage_effect_cb() { return ri_effect_cb; }

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time)
{
  return get_or_add_cached_collision_object(ri_desc, at_time, rendinst::getRiGenDestrInfo(ri_desc));
}

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info)
{
  CachedCollisionObjectInfo *objInfo = rendinstdestr::get_cached_collision_object(ri_desc);
  if (!objInfo)
  {
    if (!coll_info.isDestr)
      return nullptr;
    const float hitPointsToDestrImpulseMult = rendinstdestr::get_destr_settings().hitPointsToDestrImpulseMult;
    float destrImpulse = coll_info.destrImpulse > 0.f ? coll_info.destrImpulse : coll_info.hp * hitPointsToDestrImpulseMult;
    objInfo = new RendinstCollisionUserInfo::RendinstImpulseThresholdData(destrImpulse, coll_info.desc, at_time, coll_info);
    rendinstdestr::add_cached_collision_object(objInfo);
  }
  return objInfo;
}

CachedCollisionObjectInfo *rendinstdestr::get_cached_collision_object(const rendinst::RendInstDesc &ri_desc)
{
  for (int i = 0; i < cachedCollisionObjects.size(); ++i)
    if (cachedCollisionObjects[i]->riDesc == ri_desc)
      return cachedCollisionObjects[i];
  return nullptr;
}

void rendinstdestr::add_cached_collision_object(CachedCollisionObjectInfo *object) { cachedCollisionObjects.push_back(object); }

void rendinstdestr::remove_tree_rendinst_destr(const rendinst::RendInstDesc &desc)
{
  rendinst::delRIGenExtraFromCell(desc);

  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (riPhys[i].descForDestr != desc)
      continue;

    rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    riPhys[i].cleanup();
    erase_items(riPhys, i, 1);
    return;
  }
}

void rendinstdestr::clear_phys_objs()
{
  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (riPhys[i].physBody)
      rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    riPhys[i].cleanup();
  }
  clear_and_shrink(riPhys);
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, const TMatrix &tmA, float max_rad)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  int countCollision = 0;
  coA.body->setTmInstant(tmA);
  physWorld->fetchSimRes(true);
  for (int i = 0; i < riPhys.size(); ++i)
  {
    RendInstPhys &phys = riPhys[i];

    bool physObject = phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT;
    if (physObject)
      continue;

    const float maxRiPhysRad = 5.f;
    TMatrix physTm = TMatrix::IDENT;
    dacoll::get_coll_obj_tm(phys.riColObj, physTm);
    if (lengthSq(physTm.getcol(3) - tmA.getcol(3)) > sqr(maxRiPhysRad + max_rad))
      continue;

    int prevCountContacts = phys.physModel->contacts.size();
    WrapperContactResultCB physCollCb(phys.physModel->contacts);
    physWorld->contactTestPair(phys.riColObj.body, coA.body, physCollCb);
    countCollision += phys.physModel->contacts.size() - prevCountContacts;
  }
  return countCollision;
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, float max_rad)
{
  TMatrix tmA = TMatrix::IDENT;

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  coA.body->getTmInstant(tmA);

  return test_dynobj_to_ri_phys_collision(coA, tmA, max_rad);
}

void rendinstdestr::create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable, const Point3 &impulse,
  bool create_phys, bool constrained_phys, float wanted_omega, float at_time, const rendinst::CollisionInfo *coll_info,
  bool create_destr)
{
  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenExternalControl` and
  // `rendinstdestr::destroyRendinst`. But it is not needed if we have special callback with invalidation for only
  // instance bbox
  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;

  rendinst::DestroyedRi *ri = create_destr ? rendinst::doRIGenExternalControl(desc, !create_phys) : NULL;
  Point2 impulseDir = normalize(Point2::xz(impulse));
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr && ri_effect_cb, ri_effect_cb, rendinstdestr::get_destr_settings().isClient, nullptr, 1, 1.0f, nullptr, flags);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (!ri)
    return;

  const float treeHeightSpringThreshold = 3.f;
  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

  TMatrix4_vec4 m4;
  rendinst::getRIGenExtra44(ri->riHandle, (mat44f &)m4);
  TMatrix tm = tmatrix(m4);
  Point3 scale = Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length());
  if (scale.lengthSq() < 1e-18)
    return;
  BBox3 bbox = rendinst::getRIGenBBox(desc);

  const auto riDestrData = rendinst::gather_ri_destr_data(desc);
  const float height = bbox.lim[1].y * scale.y * riDestrData.collisionHeightScale;
  if (height <= 0)
  {
    logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g)", desc.isRiExtra() ? "riExtra" : "riGen",
      desc.pool, bbox, height, scale, riDestrData.collisionHeightScale);
    return;
  }
  const float radius = bbox.width().x * 0.5f;
  const float mass = PI * sqr(radius) * height * treeDestr.treeDensity;
  const Point3 momentOfInertia = Point3(mass * (3.f * sqr(radius) + sqr(height)) / 12.f, mass * sqr(radius) * 0.5f,
    mass * (3.f * sqr(radius) + sqr(height)) / 12.f);
  tm.orthonormalize();
  const bool allowSpring = height < treeHeightSpringThreshold && desc.layer == 1;
  gamephys::DynamicPhysModel::PhysType physModelType =
    create_phys ? gamephys::DynamicPhysModel::E_PHYS_OBJECT
                : (allowSpring ? gamephys::DynamicPhysModel::E_ANGULAR_SPRING : gamephys::DynamicPhysModel::E_FORCED_POS_BY_COLLISION);

  RendInstPhys &phys =
    riPhys.emplace_back(RendInstPhysType::TREE, desc, tm, mass, momentOfInertia, scale, Point3(0.f, height * 0.5f, 0.f), physModelType,
      tm, rendinstdestr::get_destr_settings().rendInstMaxLifeTime, treeDestr.maxLifeDistance, ri);

  if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
    phys.treeSound.init(g_tree_sound_cb, tm, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);

  phys.descForDestr = offsetedDesc;

  if (phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT)
  {
    float canopyStartHt = height * 0.6f;
    float trunkStartHt = (constrained_phys ? height * 0.035f : 0.f) + radius;
    float trunkHt = max(canopyStartHt * 0.9f - trunkStartHt, 2.f * radius);
    float trunkHalfHt = trunkHt * 0.5f;
    TMatrix centerOfMassTm = TMatrix::IDENT;
    centerOfMassTm.setcol(3, Point3(0.f, trunkHalfHt + trunkStartHt, 0.f));
    phys.centerOfMassTm = centerOfMassTm;
    PhysWorld *physWorld = dacoll::get_phys_world();
    // trunk
    {
      // shape
      PhysCapsuleCollision capsuleShape(radius, trunkHt, 1 /*Y*/);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = momentOfInertia;
      if (constrained_phys)
      {
        phys.distConstrainedPhys = 1.0f;
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048
      }

      // body
      phys.physBody = new PhysBody(physWorld, mass, &capsuleShape, tm * centerOfMassTm, pbcd);
    }

    // canopy
    float sphRad = (height - canopyStartHt) * 0.5f;
    TMatrix sphTm = TMatrix::IDENT;
    float impulseDirMult = constrained_phys ? 0.1f * sphRad : 0.f;
    sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, height - sphRad, impulseDir.y * impulseDirMult));
    float canopyMass = sqr(sphRad) * sphRad * PI * 4.f / 3.f * treeDestr.canopyDensity;
    Point3 canopyInertia = treeDestr.canopyInertia * 0.4f * canopyMass * sqr(sphRad);
    {
      // shape
      const float canopyMaxRadius = treeDestr.canopyMaxRaduis;
      PhysSphereCollision sphCollision(clamp(sphRad, 0.05f, canopyMaxRadius));

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = canopyInertia;
      pbcd.rollingFriction = treeDestr.canopyRollingFriction;
      pbcd.restitution = treeDestr.canopyRestitution;
      pbcd.linearDamping = treeDestr.canopyLinearDamping;
      pbcd.angularDamping = treeDestr.canopyAngularDamping;
      if (constrained_phys)
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048

      // body
      phys.additionalBody = new PhysBody(physWorld, canopyMass, &sphCollision, tm * sphTm, pbcd);
    }

    // joint
    {
      TMatrix halfSphTm = sphTm;
      halfSphTm.setcol(3, sphTm.getcol(3) * 0.5f);
      PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, halfSphTm * inverse(phys.centerOfMassTm),
        inverse(halfSphTm));
      phys.joints.push_back() = joint;
      Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
      if (dofSpring)
      {
        const float cmLen = length(sphTm.getcol(3));
        const float stiffness = (mass + canopyMass * cmLen * 3.f) * treeDestr.canopyStiffnessMult;
        const float angularLimit = treeDestr.canopyAngularLimit;
        dofSpring->setLimit(0, Point2(0.f, 0.f));
        dofSpring->setLimit(1, Point2(0.f, 0.f));
        dofSpring->setLimit(2, Point2(0.f, 0.f));
        dofSpring->setSpring(3, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        dofSpring->setLimit(4, Point2(0.f, 0.f));
        dofSpring->setSpring(5, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        for (int i = 0; i < 6; ++i)
        {
#if defined(USE_BULLET_PHYSICS)
          dofSpring->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
          dofSpring->setAxisDamping(i, 0.5f);
        }
        dofSpring->setEquilibriumPoint();
      }
      phys.lastValidTm = tm;
    }

    if (constrained_phys)
    {
      PhysJoint *joint = physWorld->create6DofJoint(phys.physBody, NULL, inverse(phys.centerOfMassTm), TMatrix::IDENT);
      phys.joints.push_back() = joint;
      Phys6DofJoint *dofJoint = Phys6DofJoint::cast(joint);
      if (dofJoint)
      {
        dofJoint->setLimit(0, Point2(0.f, 0.f));
        dofJoint->setLimit(1, treeDestr.constraintLimitY);
        dofJoint->setLimit(2, Point2(0.f, 0.f));
#if defined(USE_BULLET_PHYSICS)
        for (int i = 0; i < 6; ++i)
          dofJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
      }

      // Impulse is calculated in such way it doesn't violate constraint
      // spdbot = omega * arm + vel
      // vel = omega * arm
      // omega = vel / arm
      // omega = imp * arm2 / inertia
      // vel = imp / mass
      // arm2 = omega * inertia / imp
      // arm2 = (vel / arm) * inertia / imp
      // arm2 = ((imp / mass) / arm) * inertia / imp
      // arm2 = (imp * inertia) / (mass * arm * imp)
      // arm2 = inertia / (mass * arm)

      // imp = omega * inertia / arm2
      float omega = clamp(wanted_omega, 0.3f, 1.f);
      float arm = safediv(momentOfInertia.x, mass * (trunkHalfHt + trunkStartHt));
      float impulseVal = safediv(omega * momentOfInertia.x, arm);
      phys.physBody->addImpulse(tm * (phys.centerOfMassTm.getcol(3) + Point3(0.f, arm, 0.f)), Point3::x0y(impulseDir) * impulseVal);
      // omega = vel / arm
      // vel = omega * arm
      // imp = vel * mass
      // imp = omega * arm * mass
      phys.additionalBody->addImpulse((tm * sphTm).getcol(3), omega * (height - sphRad) * canopyMass * Point3::x0y(impulseDir));
    }
    phys.lastValidTm = tm;

    if (!constrained_phys && (coll_info->destrFxId >= 0 || !coll_info->destrFxTemplate.empty()) && ri_effect_cb)
      ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tm, coll_info->desc.pool, false, nullptr, coll_info->destrFxTemplate.c_str());
  }
  else
  {
    float hh = height * 0.5f;
    TMatrix cotm = TMatrix::IDENT;
    cotm.m[3][1] = hh;
    phys.riColObj = dacoll::add_dynamic_cylinder_collision(cotm, radius, height, /*uptr*/ nullptr, /*add_to_world*/ false);
    dacoll::obj_apply_impulse(phys.riColObj, impulse * mass, Point3(0.f, hh, 0.f));
  }

  if (create_destr && !create_phys && ri_effect_cb)
  {
    if (riDestrData.fxType != -1 || *riDestrData.fxTemplate != 0)
    {
      TMatrix fxTm = tm;
      fxTm *= riDestrData.fxScale;
      fxTm.setcol(3, tm.getcol(3));
      ri_effect_cb(riDestrData.fxType, TMatrix::IDENT, fxTm, desc.pool, false, nullptr, riDestrData.fxTemplate);
    }
  }
}

void rendinstdestr::doRIExtraDamageInBox(const BBox3 &box, rendinst::ri_damage_effect_cb effect_cb, float at_time, bool is_client,
  bool create_destr, const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, const BSphere3 *check_sphere,
  const TMatrix *check_itm, rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(rendinstdestr__doRIExtraDamageInBox);

  rendinst::riex_collidable_t ri_h;
  rendinst::gatherRIGenExtraCollidable(ri_h, box, true /*read_lock*/);
  bool tooManyDestructables = ri_h.size() >= destructables::maxNumberOfDestructableBodies;

  if (create_destr && tooManyDestructables)
  {
    float riDamageRadiusSq = (box.center() - box.lim[0]).lengthSq();
    if (riDamageRadiusSq >= destructables::minDestrRadiusSq)
    {
      Point3 dirToExplosion = box.center() - view_pos;
      if (dirToExplosion.lengthSq() > riDamageRadiusSq + destructables::minDestrRadiusSq)
        create_destr = false;
    }
  }

  for (int j = 0; j < ri_h.size(); ++j)
  {
    rendinst::RendInstDesc riDesc(-1, rendinst::handle_to_ri_inst(ri_h[j]), rendinst::handle_to_ri_type(ri_h[j]), 0, 0);
    if (riDesc.pool >= 0 && rendinst::get_riextra_immortality(ri_h[j]) && !(flags & rendinst::DestrOptionFlag::ForceDestroy))
      continue;
    rendinst::CollisionInfo collInfo(riDesc);
    collInfo = rendinst::getRiGenDestrInfo(riDesc);

    Point3 riPos = collInfo.tm.getcol(3);
    if (check_sphere)
    {
      const BBox3 riBBox = collInfo.tm * collInfo.localBBox;
      if (!(*check_sphere & riBBox))
        continue;
    }
    if (check_itm)
    {
      BBox3 checkBox(Point3(-0.5f, 0.f, -0.5f), Point3(0.5f, 1.f, 0.5f));
      Point3 p = *check_itm * riPos;
      p.y = 0.5f;
      if (!(checkBox & p))
        continue;
    }

    if (calc_expl_dmg_cb)
    {
      TIME_PROFILE(rendinstdestr__doRIExtraDamageInBox_calc_expl_dmg);

      if (!collInfo.isDestr || collInfo.riPoolRef < 0 || collInfo.tm == TMatrix::ZERO)
        continue;

      float distance = sqrt(point_to_box_distance_sq(inverse(collInfo.tm) * box.center(), collInfo.localBBox));
      float damage = calc_expl_dmg_cb(collInfo, distance);
      if (damage == 0.f)
        continue;
      if (collInfo.hp > 0.f && !rendinst::applyDamageRIGenExtra(collInfo.desc, damage))
        continue;
    }
    bool local_create_destr = false;
    if (create_destr && (!tooManyDestructables || ((view_pos - riPos).lengthSq() < destructables::minDestrRadiusSq)))
      local_create_destr = true;
    if (collInfo.isDestr)
      flags |= rendinst::DestrOptionFlag::ForceDestroy;
    rendinstdestr::destroyRendinst(riDesc, true, box.center(), ZERO<Point3>(), at_time, &collInfo, local_create_destr, effect_cb,
      is_client, NULL, collInfo.destroyNeighbourDepth, 1.f, nullptr, flags);
  }
}

void rendinstdestr::remove_ri_without_collision_in_radius(const Point3 &pos, float radius)
{
  BBox3 box(pos, radius * 2);
  rendinst::hideRIGenExtraNotCollidable(box, false);

  struct ForeachCB final : public rendinst::ForeachCB
  {
    eastl::fixed_vector<rendinst::RendInstDesc, 32, true /* bEnableOverflow */> riDescToRemove;

    void executeForTm(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &) override
    {
      if (!rendinst::ri_gen_has_collision(ri_desc))
        riDescToRemove.push_back(ri_desc);
    }

    void executeForPos(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &) override
    {
      if (!rendinst::ri_gen_has_collision(ri_desc))
        riDescToRemove.push_back(ri_desc);
    }
  } cb;

  rendinst::foreachRIGenInBox(box, rendinst::GatherRiTypeFlag::RiGenOnly, cb);

  for (const rendinst::RendInstDesc &desc : cb.riDescToRemove)
  {
    rendinst::DestroyedRi *destroyedRi = rendinst::doRIGenExternalControl(desc, true);
    rendinst::removeRIGenExternalControl(desc, destroyedRi);
  }
}
