#include <ecs/rendInst/riExtra.h>
#include <rendInst/rendinstHashMap.h>
#include <rendInst/moveRI.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/baseIo.h>
#include <ecs/core/entityManager.h>
#include <daECS/net/object.h>
#include <EASTL/intrusive_list.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResourceClassId.h>
#include <util/dag_delayedAction.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/algorithm.h>
#include <util/dag_convar.h>

#include <vecmath/dag_vecMath.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/phys/rendinstDestr.h>

ECS_REGISTER_EVENT(EventRendinstsLoaded);
ECS_REGISTER_EVENT(EventRendinstInitForLevel, rendinst::riex_handle_t /*riex_handle*/);
ECS_REGISTER_EVENT(CmdDestroyRendinst);

CONSOLE_BOOL_VAL("rendinst", debug_movement, false);
static const bbox3f RENDINST_WORLD_BBOX = {v_make_vec4f(-1e4f, -1e4f, -1e4f, 0.f), v_make_vec4f(1e4f, 1e4f, 1e4f, 0.f)};

struct RestorableId
{
  RestorableId() = default;
  explicit RestorableId(const rendinst::RendInstDesc &desc) : pool(desc.pool), cellIdx(desc.cellIdx), offs(desc.offs) {}

  bool operator==(const RestorableId &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }

  int pool;
  int cellIdx;
  int offs;
};

struct RestorableIdHasher
{
  size_t operator()(const RestorableId &rId) const { return mem_hash_fnv1((const char *)&rId, sizeof(rId)); }
};

typedef RiexHashMap<ecs::EntityId> HandlesMap;
static HandlesMap handles2eid; // >= 8+ K elements
static ska::flat_hash_map<RestorableId, ecs::EntityId, RestorableIdHasher> restorables2eid;

template <typename Callable>
void del_ri_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
void restore_ri_ecs_query(ecs::EntityId, Callable);

static void on_del_ri_extra_cb(rendinst::riex_handle_t handle)
{
  auto it = handles2eid.find(handle);
  if (it == handles2eid.end())
    return;

  del_ri_ecs_query(it->second, [&](ecs::EntityId eid, RiExtraComponent &ri_extra, const net::Object *replication) {
    dacoll::invalidate_ri_instance(rendinst::RendInstDesc(ri_extra.handle));
    ri_extra.handle = rendinst::RIEX_HANDLE_NULL;

    if (!replication || !replication->isReplica())
      g_entity_mgr->destroyEntity(eid);
  });

  handles2eid.erase(it);
}

static bool on_restorable_rendinst_cb(const rendinst::RendInstDesc &desc, rendinstdestr::RestorableRendinstState state)
{
  if (!desc.isRiExtra())
    return true;
  bool res = true;
  switch (state)
  {
    case rendinstdestr::RRS_CREATED:
    {
      rendinst::riex_handle_t handle = desc.getRiExtraHandle();
      // Restorable is about to be created for 'handle', search for
      // corresponding eid.
      auto it = handles2eid.find(handle);
      if (it == handles2eid.end())
        break;
      // Save it for later.
      restorables2eid[RestorableId(desc)] = it->second;
      break;
    }
    case rendinstdestr::RRS_RESTORED:
    {
      // Restorable is restored, get eid.
      auto it = restorables2eid.find(RestorableId(desc));
      if (it == restorables2eid.end())
        break;
      rendinst::riex_handle_t handle = desc.getRiExtraHandle();
      bool found = false;
      restore_ri_ecs_query(it->second, [&](ecs::EntityId eid, RiExtraComponent &ri_extra) {
        // Restore the link between entity and rendinst.
        G_ASSERT(ri_extra.handle == rendinst::RIEX_HANDLE_NULL);
        ri_extra.handle = handle;
        handles2eid[handle] = eid;
        found = true;
      });
      if (!found)
      {
        // Entity wasn't found, kill the rendinst.
        rendinst::delRIGenExtra(handle);
        // We've invalidated the handle, report it!
        res = false;
      }
      restorables2eid.erase(it);
      break;
    }
    case rendinstdestr::RRS_DESTROYED:
      // Restorable is not going to be restored, forget it.
      restorables2eid.erase(RestorableId(desc));
      break;
    default: G_ASSERT(false); break;
  }
  return res;
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag riExtraAuthority)
void check_extra_authority_ecs_query(ecs::EntityId, Callable);

static void on_riex_destruction_cb(rendinst::riex_handle_t handle, bool is_dynamic, int32_t user_data)
{
  auto it = handles2eid.find(handle);
  const bool destroysEntity = is_dynamic; // Only dynamic RI entities should be destroyed, we have a separate system which destroys it
                                          // for non-dynamic ones.
  if (it != handles2eid.end())
  {
    check_extra_authority_ecs_query(it->second, [&](bool &ri_extra__destroyed, ecs::EntityManager &manager) {
      manager.sendEventImmediate(it->second, CmdDestroyRendinst(user_data, destroysEntity));
      ri_extra__destroyed = true;
    });
  }
}

/*static*/
void RiExtraComponent::requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
{
  if (res_cb.getOr<rendinst::riex_handle_t>(ECS_HASH("ri_extra__handle"), rendinst::RIEX_HANDLE_NULL) != rendinst::RIEX_HANDLE_NULL)
    return;
  auto riResName = res_cb.getNullable<ecs::string>(ECS_HASH("ri_extra__borrowedResName"));
  if (!riResName)
    riResName = res_cb.getNullable<ecs::string>(ECS_HASH("ri_extra__name"));
  request_ri_resources(res_cb, (riResName && !riResName->empty()) ? riResName->c_str() : "<missing_ri_res>");
}

RiExtraComponent::RiExtraComponent(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  handle = mgr.getOr<rendinst::riex_handle_t>(eid, ECS_HASH("ri_extra__handle"), rendinst::RIEX_HANDLE_NULL);
  if (handle != rendinst::RIEX_HANDLE_NULL)
    handles2eid.insert(HandlesMap::value_type(handle, eid)).first->second = eid;
}

static rendinst::riex_handle_t spawn_ri_extra(const ecs::EntityManager &mgr, ecs::EntityId eid, int resIdx, const char *name)
{
  auto riAddFail = [&]() {
    logerr("Failed to add ri_extra.name=<%s>(%d) while creating entity %d<%s>", name, resIdx, (ecs::entity_id_t)eid,
      mgr.getEntityTemplateName(eid));
    return rendinst::RIEX_HANDLE_NULL;
  };
  if (resIdx < 0)
  {
    resIdx = rendinst::addRIGenExtraResIdx(name, -1, -1, rendinst::AddRIFlag::UseShadow | rendinst::AddRIFlag::Dynamic);
    if (EASTL_UNLIKELY(resIdx < 0))
      return riAddFail();
    rendinst::addRiGenExtraDebris(resIdx, 0);
  }
  const TMatrix &transform = mgr.get<TMatrix>(eid, ECS_HASH("transform"));
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, transform.array);
  if (EASTL_UNLIKELY(!v_bbox3_test_pt_inside(RENDINST_WORLD_BBOX, tm.col3)))
  {
    logerr("Failed to add ri_extra.name=<%s>(%d) on invalid pos " FMT_P3 " while creating entity %d<%s>", name, resIdx,
      P3D(transform.getcol(3)), (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
    return rendinst::RIEX_HANDLE_NULL;
  }
  const bool hasCollision = mgr.getOr(eid, ECS_HASH("ri_extra__hasCollision"), true);
  rendinst::riex_handle_t handle = rendinst::addRIGenExtra44(resIdx, true, tm, hasCollision, -1, -1);
  if (handle != rendinst::RIEX_HANDLE_NULL)
  {
    handles2eid.insert(HandlesMap::value_type(handle, eid)).first->second = eid;
    if (mgr.has(eid, ECS_HASH("ri_extra__dynamic")))
      rendinst::setRIGenExtraResDynamic(name);
    if (const int *hp = mgr.getNullable<int>(eid, ECS_HASH("ri_extra__overrideHitPoints")))
      rendinst::setRiGenExtraHp(handle, (float)*hp);
    return handle;
  }
  return riAddFail();
}

struct DelayedSpawnRiExtra final : public DelayedAction, public eastl::intrusive_list_node
{
  ecs::EntityId eid;
  static inline eastl::intrusive_list<DelayedSpawnRiExtra> dyn_riex_creation_queue;
  DelayedSpawnRiExtra(ecs::EntityId e) : eid(e) { dyn_riex_creation_queue.push_back(*this); }
  ~DelayedSpawnRiExtra() { dyn_riex_creation_queue.remove(*this); }
  EA_NON_COPYABLE(DelayedSpawnRiExtra)
  bool precondition() override { return rendinst::isRiExtraLoaded(); }
  void performAction() override
  {
    auto &mgr = *g_entity_mgr;
    const char *name = mgr.get<ecs::string>(eid, ECS_HASH("ri_extra__name")).c_str();
    int resIdx = rendinst::getRIGenExtraResIdx(name);
    const rendinst::riex_handle_t handle = spawn_ri_extra(mgr, eid, resIdx, name);
    mgr.getRW<RiExtraComponent>(eid, ECS_HASH("ri_extra")).handle = handle;
    if (nullptr != mgr.getNullable<ecs::Tag>(eid, ECS_HASH("levelRiExtra")))
      g_entity_mgr->broadcastEventImmediate(EventRendinstInitForLevel(handle));
  }
};

bool RiExtraComponent::onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  if (mgr.getOr<rendinst::riex_handle_t>(eid, ECS_HASH("ri_extra__handle"), rendinst::RIEX_HANDLE_NULL) != rendinst::RIEX_HANDLE_NULL)
    return true;
  const char *name = mgr.get<ecs::string>(eid, ECS_HASH("ri_extra__name")).c_str();
  int resIdx = rendinst::getRIGenExtraResIdx(name);
  if (resIdx >= 0 || (rendinst::isRiExtraLoaded() && DelayedSpawnRiExtra::dyn_riex_creation_queue.empty()))
  {
    handle = spawn_ri_extra(mgr, eid, resIdx, name);
    if (nullptr != mgr.getNullable<ecs::Tag>(eid, ECS_HASH("levelRiExtra")))
      g_entity_mgr->broadcastEventImmediate(EventRendinstInitForLevel(handle));
  }
  else
  {
    // ri extra (i.e. level) isn't loaded yet -> add delayed action that wait's it's loading to ensure
    // that all dyn riex are added in order it was created after level loaded (required for correct sync with server)
    add_delayed_action(new DelayedSpawnRiExtra(eid));
    return true;
  }
  return handle != rendinst::RIEX_HANDLE_NULL;
}

static void delete_riextra(rendinst::riex_handle_t handle, bool destroy = false, bool add_restorable = false,
  bool create_destr_and_fx = true, int destroy_neighbour_recursive_depth = 1)
{
  rendinst::RendInstDesc desc(handle);
  if (destroy)
  {
    auto effectCb = create_destr_and_fx ? rendinstdestr::get_ri_damage_effect_cb() : nullptr;
    rendinstdestr::destroyRendinst(desc, add_restorable, Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f), 0.f, nullptr,
      create_destr_and_fx, effectCb, rendinstdestr::get_destr_settings().isClient, nullptr, destroy_neighbour_recursive_depth);
  }

  rendinst::delRIGenExtra(desc.getRiExtraHandle());
}

ECS_ON_EVENT(on_disappear)
void riextra_destroyed_es_event_handler(const ecs::Event &, const RiExtraComponent &ri_extra, bool ri_extra__destroyed)
{
  bool isRiHandleValid = rendinst::isRiGenExtraValid(ri_extra.handle);
  if (isRiHandleValid)
    delete_riextra(ri_extra.handle, !ri_extra__destroyed, false, true, 1);

  handles2eid.erase(ri_extra.handle);
}

typedef eastl::type_select<ecs::ComponentTypeInfo<RiExtraComponent>::is_boxed, ecs::BoxedCreator<RiExtraComponent>,
  ecs::InplaceCreator<RiExtraComponent>>::type RiExtraComponentCtm;
class RendInstCTM final : public RiExtraComponentCtm
{
public:
  void clear() override
  {
    handles2eid = {};
    restorables2eid = {};
    while (!DelayedSpawnRiExtra::dyn_riex_creation_queue.empty())
      G_VERIFY(remove_delayed_action(&DelayedSpawnRiExtra::dyn_riex_creation_queue.back()));
  }
  RendInstCTM()
  {
    rendinst::registerRIGenExtraInvalidateHandleCb(on_del_ri_extra_cb);
    rendinst::registerRiExtraDestructionCb(on_riex_destruction_cb);
    rendinstdestr::register_restorable_rendinst_cb(on_restorable_rendinst_cb);
  }
  ~RendInstCTM()
  {
    G_VERIFY(rendinst::unregisterRIGenExtraInvalidateHandleCb(on_del_ri_extra_cb));
    G_VERIFY(rendinst::unregisterRiExtraDestructionCb(on_riex_destruction_cb));
    G_VERIFY(rendinstdestr::unregister_restorable_rendinst_cb(on_restorable_rendinst_cb));
  }
};

ECS_REGISTER_MANAGED_TYPE(RiExtraComponent, nullptr, RendInstCTM);
ECS_AUTO_REGISTER_COMPONENT(RiExtraComponent, "ri_extra", nullptr, 0);

static struct RendInstSerializer final : public ecs::ComponentSerializer
{
  static constexpr uint32_t ri_type_bits = 12;
  static constexpr uint32_t ri_type_full_bits = 16;
  static constexpr uint32_t ri_inst_base_bits = 11,
                            ri_inst_other_bits = 12, // enough for storing 8M instances
    ri_inst_total_bits = ri_inst_base_bits + ri_inst_other_bits;
  static constexpr uint32_t short_bits = 1 + ri_inst_base_bits + ri_type_bits;
  static constexpr uint32_t long_bits = 1 + ri_inst_total_bits + ri_type_full_bits;
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<rendinst::riex_handle_t>::type);
    G_STATIC_ASSERT(ri_inst_total_bits <= rendinst::ri_instance_type_shift);
    G_STATIC_ASSERT(ri_type_bits <= ri_type_full_bits);
    G_STATIC_ASSERT(ri_type_full_bits + rendinst::ri_instance_type_shift <= 64);
    G_UNUSED(hint);

    rendinst::riex_handle_t handle = (*(const rendinst::riex_handle_t *)data) + 1; // so invalid became zero.
    uint32_t riType = rendinst::handle_to_ri_type(handle), riInst = rendinst::handle_to_ri_inst(handle);
    G_ASSERT(riInst < (1 << ri_inst_total_bits)); // we save 9 bits.
    if (riInst < (1 << ri_inst_base_bits) && riType < (1 << ri_type_bits))
    {
      // 24 bits (ri_inst_base_bits : ri_type_bits : 0)
      G_STATIC_ASSERT(short_bits % 8 == 0);
      uint32_t word24 = riInst | (riType << ri_inst_base_bits);
      cb.write(&word24, short_bits, 0);
    }
    else
    {
      // 40 bits (ri_inst_total_bits : 1 : ri_type_full_bits)
      G_STATIC_ASSERT(long_bits % 8 == 0);
      uint64_t word40 = riInst | (1 << ri_inst_total_bits) | (uint64_t(riType) << (ri_inst_total_bits + 1));
      cb.write(&word40, long_bits, 0);
    }
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<rendinst::riex_handle_t>::type);
    G_UNUSED(hint);
    rendinst::riex_handle_t &handle = *((rendinst::riex_handle_t *)data);
    uint32_t riType = 0, riInst = 0;
    uint32_t word24 = 0;
    bool isOk = cb.read(&word24, short_bits, 0);
    if (EASTL_UNLIKELY(word24 & (1 << ri_inst_total_bits)))
    {
      G_STATIC_ASSERT(short_bits + ri_type_full_bits == long_bits);
      riInst = word24 & ((1 << ri_inst_total_bits) - 1);
      isOk &= cb.read(&riType, ri_type_full_bits, 0);
    }
    else
    {
      riType = word24 >> ri_inst_base_bits;
      riInst = word24 & ((1 << ri_inst_base_bits) - 1);
    }
    if (!riInst && !riType)
      handle = rendinst::RIEX_HANDLE_NULL;
    else
      handle = rendinst::make_handle(riType, riInst) - 1;
    return isOk;
  }
} rendinst_serializer;

ECS_AUTO_REGISTER_COMPONENT(rendinst::riex_handle_t, "ri_extra__handle", &rendinst_serializer, 0);

ECS_TRACK(transform)
static __forceinline void rendinst_track_move_es_event_handler(const ecs::Event &, ecs::EntityId eid, RiExtraComponent &ri_extra,
  const TMatrix &transform, Point3 ri_extra__velocity = Point3(0, 0, 0), Point3 ri_extra__omega = Point3(0, 0, 0))
{
  if (!rendinst::isRiGenExtraValid(ri_extra.handle)) // to be removed
  {
    debug("%s riex.handle=%d eid=%d is not valid can't move", __FUNCTION__, ri_extra.handle, (ecs::entity_id_t)eid);
    return;
  }
  move_ri_extra_tm_with_motion(ri_extra.handle, transform, ri_extra__velocity, transform % ri_extra__omega);
#if DAECS_EXTENSIVE_CHECKS  // we can't enable console var in release anyway
  if (debug_movement.get()) // to be removed from here
    debug("rendinst_move moving riex.handle=%d eid=%d x =%@ pos = %@", ri_extra.handle, (ecs::entity_id_t)eid, transform.getcol(0),
      transform.getcol(3));
#endif
}

ECS_ON_EVENT(on_appear, EventRendinstsLoaded)
static __forceinline void rendinst_move_es_event_handler(const ecs::Event &, RiExtraComponent &ri_extra, const TMatrix &transform,
  Point3 *ri_extra__bboxMin, Point3 *ri_extra__bboxMax)
{
  if (rendinst::isRiGenExtraValid(ri_extra.handle))
  {
    move_ri_extra_tm(ri_extra.handle, transform);
    if (ri_extra__bboxMin && ri_extra__bboxMax)
    {
      const BBox3 bbox = rendinst::getRIGenBBox(rendinst::RendInstDesc(ri_extra.handle));
      *ri_extra__bboxMin = bbox.lim[0];
      *ri_extra__bboxMax = bbox.lim[1];
    }
  }
}

ECS_ON_EVENT(on_appear, EventRendinstsLoaded)
ECS_REQUIRE_NOT(RiExtraComponent &ri_extra)
static __forceinline void rendinst_with_handle_move_es_event_handler(const ecs::Event &,
  const rendinst::riex_handle_t ri_extra__handle, const TMatrix &transform, Point3 *ri_extra__bboxMin, Point3 *ri_extra__bboxMax)
{
  if (rendinst::isRiGenExtraValid(ri_extra__handle))
  {
    move_ri_extra_tm(ri_extra__handle, transform);
    if (ri_extra__bboxMin && ri_extra__bboxMax)
    {
      const BBox3 bbox = rendinst::getRIGenBBox(rendinst::RendInstDesc(ri_extra__handle));
      *ri_extra__bboxMin = bbox.lim[0];
      *ri_extra__bboxMax = bbox.lim[1];
    }
  }
}

ecs::EntityId find_ri_extra_eid(rendinst::riex_handle_t handle)
{
  auto it = handles2eid.find(handle);
  return it != handles2eid.end() ? it->second : ecs::INVALID_ENTITY_ID;
}

void request_ri_resources(const ecs::resource_request_cb_t &res_cb, const char *riResName)
{
  res_cb(riResName, RendInstGameResClassId);
  eastl::fixed_string<char, 64> riCollResName = riResName;
  riCollResName += RI_COLLISION_RES_SUFFIX;
  if (get_resource_type_id(riCollResName.c_str()) == CollisionGameResClassId) // same branch as in rendinst::addRIGenExtraResIdx()
    res_cb(riCollResName.c_str(), CollisionGameResClassId);
}

bool replace_ri_extra_res(ecs::EntityId eid, const char *res_name, bool destroy, bool add_restorable, bool create_destr_and_fx)
{
  auto &mgr = *g_entity_mgr;
  RiExtraComponent *riExtra = mgr.getNullableRW<RiExtraComponent>(eid, ECS_HASH("ri_extra"));
  if (riExtra == nullptr)
  {
    logerr("Component '%s'(0x%X) is not present in entity %d of template '%s'", "ri_extra", ECS_HASH("ri_extra").hash, eid,
      mgr.getEntityTemplateName(eid));
    return false;
  }

  if (riExtra->handle != rendinst::RIEX_HANDLE_NULL)
  {
    handles2eid.erase(riExtra->handle);

    bool isRiHandleValid = rendinst::isRiGenExtraValid(riExtra->handle);
    if (isRiHandleValid)
    {
      delete_riextra(riExtra->handle, destroy, add_restorable, create_destr_and_fx, 0);
      dacoll::invalidate_ri_instance(rendinst::RendInstDesc(riExtra->handle));
    }

    riExtra->handle = rendinst::RIEX_HANDLE_NULL;
  }

  if (res_name && res_name[0])
  {
    int resIdx = rendinst::getRIGenExtraResIdx(res_name);
    if (resIdx < 0)
    {
      logerr("%d<%s>: Undefined riextra resIdx for '%s'!", eid, mgr.getEntityTemplateName(eid), res_name);
      return false;
    }

    if (!rendinst::isRiExtraLoaded() || !DelayedSpawnRiExtra::dyn_riex_creation_queue.empty())
    {
      logerr("%d<%s>: Unable to replace riextra res '%s' - riextra are not loaded yet!", eid, mgr.getEntityTemplateName(eid),
        res_name);
      return false;
    }

    riExtra->handle = spawn_ri_extra(mgr, eid, resIdx, res_name);
    return riExtra->handle != rendinst::RIEX_HANDLE_NULL;
  }

  return true;
}