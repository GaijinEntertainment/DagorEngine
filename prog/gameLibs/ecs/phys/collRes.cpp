// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResources.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <debug/dag_assert.h>

CollisionResource *get_collres_from_riextra(ecs::EntityManager &mgr, ecs::EntityId eid);
CollisionResource *create_collres_from_ecs_object(ecs::EntityManager &mgr, ecs::EntityId eid);

class CollResCTM final : public ecs::ComponentTypeManager
{
public:
  typedef CollisionResource component_type;

  CollResCTM() { CollisionResource::registerFactory(); }

  ~CollResCTM()
  {
    for (CollisionResource *comp : allocatedComps)
    {
      comp->~CollisionResource();
      midmem->free(comp);
    }
    clear_and_shrink(allocatedComps);
  }

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
    GameResHandle crhandle = collResName ? GAMERES_HANDLE_FROM_STRING(collResName) : nullptr;
    if (DAGOR_UNLIKELY(collResName && !is_game_resource_loaded(crhandle, CollisionGameResClassId)))
      logerr("<%s> is expected to be loaded in %s while creating %d<%s>", collResName, __FUNCTION__, (ecs::entity_id_t)eid,
        mgr.getEntityTemplateName(eid)); // Note: `requestResources` shall ensure that
    auto collRes = crhandle ? (CollisionResource *)get_game_resource_ex(crhandle, CollisionGameResClassId) : nullptr; // automatically
                                                                                                                      // add ref
    if (!collRes)
    {
      CollisionResource *ecsCollRes = create_collres_from_ecs_object(mgr, eid);
      CollisionResource *riCollRes = get_collres_from_riextra(mgr, eid);
      if (ecsCollRes)
      {
        collRes = ecsCollRes;
        allocatedComps.push_back(collRes);
      }
      else if (riCollRes)
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
      if (erase_item_by_value(allocatedComps, (CollisionResource *)cres))
      {
        cres->~CollisionResource();
        midmem->free(cres);
      }
      else
        cres->delRef();
      cres = nullptr; // just in case
    }
  }

  Tab<CollisionResource *> allocatedComps;
};

G_STATIC_ASSERT(!ecs::ComponentTypeInfo<CollisionResource>::can_be_tracked);
// todo:replce to InplaceCreator (same as Animchar)
ECS_REGISTER_TYPE_BASE(CollisionResource, ecs::ComponentTypeInfo<CollisionResource>::type_name, nullptr,
  &ecs::CTMFactory<CollResCTM>::create, &ecs::CTMFactory<CollResCTM>::destroy, ecs::COMPONENT_TYPE_NEED_RESOURCES);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "collres__res", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(CollisionResource, "collres", nullptr, 0, "?animchar");

ECS_REGISTER_RELOCATABLE_TYPE(CollisionObject, nullptr);

ECS_DEF_PULL_VAR(collres);
