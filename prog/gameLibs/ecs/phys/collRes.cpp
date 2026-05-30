// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/dataComponent.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <debug/dag_assert.h>
#include <EASTL/vector.h>

CollisionResource *get_collres_from_riextra(ecs::EntityManager &mgr, ecs::EntityId eid);
CollisionResource *create_collres_from_ecs_object(ecs::EntityManager &mgr, ecs::EntityId eid);
void add_collres_nodes_from_ecs_object(CollisionResource *collres, const ecs::Array &desc);


// Owned CollisionResource copies we deep-copied for per-entity mutation or created
// fresh from an ecs-object descriptor. All access goes through the four helpers below.
// Linear scan; fine for the expected N (<100). If N grows past a few hundred, switch to
// eastl::vector_set / eastl::hash_set - the four helpers below are the only touch-points.
static Tab<CollisionResource *> allocatedComps;

// Whether `coll_res` is one of our owned copies.
static bool is_owned_collres(CollisionResource *coll_res)
{
  return eastl::find(allocatedComps.begin(), allocatedComps.end(), coll_res) != allocatedComps.end();
}

// Start tracking `coll_res` as one of our owned copies. Caller must own coll_res.
static void track_owned_collres(CollisionResource *coll_res) { allocatedComps.push_back(coll_res); }

// Untrack `coll_res` from our owned set. Returns true iff it was owned (and was removed).
// Caller is responsible for the actual destruction.
static bool untrack_owned_collres(CollisionResource *coll_res) { return erase_item_by_value(allocatedComps, coll_res); }

// Destroy and forget every owned copy.
static void clear_owned_collres()
{
  for (CollisionResource *comp : allocatedComps)
  {
    comp->~CollisionResource();
    midmem->free(comp);
  }
  clear_and_shrink(allocatedComps);
}

static void apply_flag_rules_to_nodes(CollisionResource &coll_res, const ecs::Array &rules, ecs::EntityId eid, const char *templ_name)
{
  auto allNodes = coll_res.getAllNodes();
  const int totalNodes = int(allNodes.size());
  for (const auto &ruleIt : rules)
  {
    const ecs::Object &rule = ruleIt.get<ecs::Object>();
    const ecs::IntList *nodeIds = rule[ECS_HASH("nodeIds")].getNullable<ecs::IntList>();
    if (!nodeIds)
      continue;
    const bool exclude = rule[ECS_HASH("exclude")].getOr(false);
    const int *absFlags = rule[ECS_HASH("behaviorFlags")].getNullable<int>();
    const int *clrFlags = rule[ECS_HASH("clearFlags")].getNullable<int>();
    const int *setFlags = rule[ECS_HASH("setFlags")].getNullable<int>();
    int appliedCount = 0;
    auto applyToNode = [&](int nodeId) {
      if (nodeId < 0 || nodeId >= totalNodes)
        return;
      uint16_t &f = allNodes[nodeId].behaviorFlags;
      if (absFlags)
        f = uint16_t(*absFlags);
      if (clrFlags)
        f &= ~uint16_t(*clrFlags);
      if (setFlags)
        f |= uint16_t(*setFlags);
      appliedCount++;
    };
    if (!exclude)
    {
      for (int id : *nodeIds)
        applyToNode(id);
    }
    else
    {
      eastl::vector<bool> excluded(totalNodes, false);
      for (int id : *nodeIds)
        if (id >= 0 && id < totalNodes)
          excluded[id] = true;
      for (int id = 0; id < totalNodes; id++)
        if (!excluded[id])
          applyToNode(id);
    }
    if (appliedCount == 0)
      logwarn("apply_flag_rules_to_nodes: rule matched 0 nodes for entity %d<%s>", (ecs::entity_id_t)eid, templ_name);
  }
}

bool clone_collres(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::component_index_t cidx = mgr.getDataComponents().findComponentId(ECS_HASH("collres").hash);
  if (cidx == ecs::INVALID_COMPONENT_INDEX)
    return false;
  CollisionResource **storage = (CollisionResource **)mgr.getComponentRefRW(eid, cidx).getRawData();
  if (!storage || !*storage)
    return false;
  if (is_owned_collres(*storage))
    return false;
  CollisionResource *originalPtr = *storage;
  track_owned_collres(*storage = originalPtr->deepCopy());
  originalPtr->delRef();
  return true;
}

bool apply_collres_node_flag_rules(ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::Array &rules)
{
  if (rules.empty())
    return false;
  CollisionResource *coll_res = mgr.getNullableRW<CollisionResource>(eid, ECS_HASH("collres"));
  if (!coll_res)
    return false;
  if (!is_owned_collres(coll_res))
  {
    logerr("apply_collres_node_flag_rules: entity %d<%s> does not own its collres; add "
           "collres__performCopy:b=true to its template",
      (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
    return false;
  }
  apply_flag_rules_to_nodes(*coll_res, rules, eid, mgr.getEntityTemplateName(eid));
  return true;
}

class CollResCTM final : public ecs::ComponentTypeManager
{
public:
  typedef CollisionResource component_type;

  CollResCTM() { CollisionResource::registerFactory(); }

  ~CollResCTM() { clear_owned_collres(); }

  void requestResources(const char *, const ecs::resource_request_cb_t &res_cb) override
  {
    ecs::string emptyStr;
    ecs::Object emptyObj;
    if (auto &res = res_cb.getOr(ECS_HASH("collres__res"), emptyStr); !res.empty())
      return res_cb(res.c_str(), CollisionGameResClassId);
    else if (auto &desc = res_cb.getOr(ECS_HASH("collres__desc"), emptyObj); !desc.empty())
      return;
    res_cb("<missing_collres>", CollisionGameResClassId);
  }

  void create(void *d, ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    *(ecs::PtrComponentType<CollisionResource>::ptr_type(d)) = nullptr; // just in case
    const char *collResName = mgr.getOr(eid, ECS_HASH("collres__res"), ecs::nullstr);
    if (DAGOR_UNLIKELY(collResName && !is_game_resource_loaded(collResName, CollisionGameResClassId)))
      logerr("<%s> is expected to be loaded in %s while creating %d<%s>", collResName, __FUNCTION__, (ecs::entity_id_t)eid,
        mgr.getEntityTemplateName(eid)); // Note: `requestResources` shall ensure that
    auto collRes = collResName ? (CollisionResource *)get_game_resource_ex(collResName, CollisionGameResClassId) : nullptr; // adds ref
    bool owned = false;
    if (!collRes)
    {
      if (CollisionResource *ecsCollRes = create_collres_from_ecs_object(mgr, eid))
      {
        collRes = ecsCollRes;
        track_owned_collres(collRes);
        owned = true;
      }
      else if (CollisionResource *riCollRes = get_collres_from_riextra(mgr, eid))
      {
        collRes = riCollRes;
        collRes->addRef(); // add ref manually as we're borrowing this collRes from the riextra
      }
      else
      {
        logerr("Failed to load collres <%s> for entity %d<%s>", collResName, (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
        return;
      }
    }

    CollisionResource *originalPtr = collRes;
    if (!owned && mgr.getOr(eid, ECS_HASH("collres__performCopy"), false))
    {
      track_owned_collres(collRes = collRes->deepCopy());
      owned = true;
    }

    const ecs::Array *rules = mgr.getNullable<ecs::Array>(eid, ECS_HASH("collres__nodeFlagRules"));
    const ecs::Array *addDesc = mgr.getNullable<ecs::Array>(eid, ECS_HASH("collres__desc_add"));
    const bool modifyCollres = rules || addDesc;
    if (owned)
    {
      // Rules first, then desc_add. add_collres_nodes_from_ecs_object ends with
      // sortNodesList(), which can shift the node ids which the rules reference.
      if (rules && !rules->empty())
        apply_flag_rules_to_nodes(*collRes, *rules, eid, mgr.getEntityTemplateName(eid));
      if (addDesc)
        add_collres_nodes_from_ecs_object(collRes, *addDesc);
    }
    else if (modifyCollres)
    {
      logerr("entity %d<%s>: collres__nodeFlagRules / collres__desc_add require "
             "collres__performCopy:b=true on the template",
        (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
    }

    if (originalPtr != collRes)
      originalPtr->delRef();

    *(ecs::PtrComponentType<CollisionResource>::ptr_type(d)) = collRes;
    if (auto animChar = mgr.getNullable<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar")))
      collRes->initializeWithGeomNodeTree(animChar->getNodeTree());
  }

  void destroy(void *d) override
  {
    CollisionResource *__restrict *a = (ecs::PtrComponentType<CollisionResource>::ptr_type(d));
    G_ASSERT_RETURN(a, );
    auto &cres = *a;
    if (cres) // can be null if onLoaded wasn't called (entity destroyed during loading)
    {
      if (untrack_owned_collres(cres))
      {
        cres->~CollisionResource();
        midmem->free(cres);
      }
      else
        cres->delRef();
      cres = nullptr; // just in case
    }
  }
};

G_STATIC_ASSERT(!ecs::ComponentTypeInfo<CollisionResource>::can_be_tracked);
// todo:replce to InplaceCreator (same as Animchar)
ECS_REGISTER_TYPE_BASE(CollisionResource, ecs::ComponentTypeInfo<CollisionResource>::type_name, nullptr,
  &ecs::CTMFactory<CollResCTM>::create, &ecs::CTMFactory<CollResCTM>::destroy, ecs::COMPONENT_TYPE_NEED_RESOURCES);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "collres__res", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(CollisionResource, "collres", nullptr, 0, "?animchar");

ECS_REGISTER_RELOCATABLE_TYPE(CollisionObject, nullptr);

ECS_DEF_PULL_VAR(collres);
