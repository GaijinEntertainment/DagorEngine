#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/slotAttach.h>
#include <ecs/anim/anim.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/utility/ecsRecreate.h>

#include <animChar/dag_animCharacter2.h>
#include <vecmath/dag_vecMath.h>

#include <ioSys/dag_dataBlock.h>

ECS_REGISTER_EVENT_NS(anim, EventItemAttached);
ECS_REGISTER_EVENT_NS(anim, EventItemDetached);
ECS_REGISTER_EVENT_NS(anim, CmdInitSkeletonAttach);
ECS_REGISTER_EVENT_NS(anim, CmdInitSlotAttach);

ECS_ON_EVENT(on_appear)
static __forceinline void slot_attach_on_load_es_event_handler(const ecs::Event &, const ecs::string &slot_attach__slotName,
  int &slot_attach__slotId)
{
  if (slot_attach__slotId < 0)
    slot_attach__slotId = AnimCharV20::addSlotId(slot_attach__slotName.c_str());
}

template <typename Callable>
inline bool get_attaches_list_ecs_query(ecs::EntityId eid, Callable c);

inline static bool attach_to_parent_impl(ecs::EntityId eid, ecs::EntityId slot_attach__attachedTo,
  ecs::EntityId &slot_attach__inAttachedList)
{
  return get_attaches_list_ecs_query(slot_attach__attachedTo, [&](ecs::EidList &attaches_list) {
    if (slot_attach__inAttachedList != slot_attach__attachedTo)
    {
      ecs::EidList *prevAttachedList =
        g_entity_mgr->getNullableRW<ecs::EidList>(slot_attach__inAttachedList, ECS_HASH("attaches_list"));
      if (prevAttachedList)
        remove_from_eid_list(eid, *prevAttachedList);
    }
    if (eastl::find(attaches_list.begin(), attaches_list.end(), eid) == attaches_list.end())
      attaches_list.push_back(eid);
    if (slot_attach__inAttachedList != slot_attach__attachedTo)
      ECS_SET(eid, slot_attach__inAttachedList, slot_attach__attachedTo);
    if (!g_entity_mgr->has(eid, ECS_HASH("attachedToParent")))
      add_sub_template_async(eid, "attached_to_parent");
  });
}

ECS_TAG(gameClient)
static void slot_attach_init_es_event_handler(const anim::CmdInitSlotAttach &, ecs::EntityId eid,
  ecs::EntityId slot_attach__attachedTo, ecs::EntityId slot_attach__inAttachedList)
{
  if (!attach_to_parent_impl(eid, slot_attach__attachedTo, slot_attach__inAttachedList))
    logerr("Could not slot attach %s to %s", g_entity_mgr->getEntityTemplateName(eid),
      g_entity_mgr->getEntityTemplateName(slot_attach__attachedTo));
}

ECS_NO_ORDER
ECS_TAG(gameClient)
ECS_REQUIRE_NOT(ecs::Tag attachedToParent)
static void attach_to_parent_es(const ecs::UpdateStageInfoAct &, ecs::EntityId eid, ecs::EntityId slot_attach__attachedTo,
  ecs::EntityId slot_attach__inAttachedList)
{
  attach_to_parent_impl(eid, slot_attach__attachedTo, slot_attach__inAttachedList);
}

static void attach_to_parent_on_create_es_event_handler(const ecs::EventEntityCreated &, ecs::EntityId eid,
  ecs::EntityId slot_attach__attachedTo, ecs::EntityId slot_attach__inAttachedList)
{
  attach_to_parent_impl(eid, slot_attach__attachedTo, slot_attach__inAttachedList);
}

template <typename Callable>
inline void get_all_attaches_ecs_query(Callable c);

ECS_REQUIRE(ecs::EidList attaches_list)
static void attach_on_parent_create_es_event_handler(const ecs::EventEntityCreated &, ecs::EntityId eid)
{
  ecs::EntityId attached_to = eid;
  get_all_attaches_ecs_query([&](ecs::EntityId eid, ecs::EntityId slot_attach__attachedTo, ecs::EntityId slot_attach__inAttachedList) {
    if (attached_to == slot_attach__attachedTo)
      attach_to_parent_impl(eid, slot_attach__attachedTo, slot_attach__inAttachedList);
  });
}

static void detach_from_parent_es_event_handler(const ecs::EventEntityDestroyed &, ecs::EntityId eid,
  ecs::EntityId slot_attach__attachedTo, ecs::EntityId &slot_attach__inAttachedList)
{
  ecs::EidList *attaches_list = g_entity_mgr->getNullableRW<ecs::EidList>(slot_attach__attachedTo, ECS_HASH("attaches_list"));
  if (attaches_list)
    remove_from_eid_list(eid, *attaches_list);
  slot_attach__inAttachedList = ecs::INVALID_ENTITY_ID;
}

static void slot_attached_to_disappear_es_event_handler(const ecs::EventComponentsDisappear &, ecs::EntityId eid,
  ecs::EntityId slot_attach__inAttachedList ECS_REQUIRE(ecs::EntityId slot_attach__attachedTo))
{
  ecs::EidList *prevAttachesList = g_entity_mgr->getNullableRW<ecs::EidList>(slot_attach__inAttachedList, ECS_HASH("attaches_list"));
  if (prevAttachesList)
  {
    remove_from_eid_list(eid, *prevAttachesList);
    remove_sub_template_async(eid, "attached_to_parent");
  }
}

ECS_TRACK(slot_attach__attachedTo)
static void track_attach_es_event_handler(const ecs::Event &, ecs::EntityId eid, ecs::EntityId slot_attach__attachedTo,
  ecs::EntityId slot_attach__inAttachedList)
{
  ecs::EidList *prevAttachesList = g_entity_mgr->getNullableRW<ecs::EidList>(slot_attach__inAttachedList, ECS_HASH("attaches_list"));
  if (prevAttachesList)
  {
    remove_from_eid_list(eid, *prevAttachesList);
    remove_sub_template_async(eid, "attached_to_parent");
  }
  attach_to_parent_impl(eid, slot_attach__attachedTo, slot_attach__inAttachedList);
}

void anim::attach(int &slot_attach__slotId, ecs::EntityId eid, int slot_id, ecs::EntityId attach_eid)
{
  g_entity_mgr->set(attach_eid, ECS_HASH("slot_attach__attachedTo"), eid);
  slot_attach__slotId = slot_id;
}

void anim::detach(int &slot_attach__slotId, ecs::EntityId attach_eid)
{
  ecs::EntityId &attachedTo = g_entity_mgr->getRW<ecs::EntityId>(attach_eid, ECS_HASH("slot_attach__attachedTo"));
  slot_attach__slotId = -1;
  attachedTo = ecs::INVALID_ENTITY_ID;
}

void anim::init_attachements(const DataBlock &blk)
{
  const DataBlock *slotsBlk = blk.getBlockByNameEx("attachmentSlots");
  for (int slotId = 0; slotId < slotsBlk->paramCount(); ++slotId)
  {
    const char *slotName = slotsBlk->getStr(slotId);
    const int curSlotId = AnimCharV20::addSlotId(slotName);
    if (slotId != curSlotId)
      logerr("Prefetch slot ids mismatch: %s - %d != %d", slotName, curSlotId, slotId);
  }
}
