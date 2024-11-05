// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <gameRes/dag_collisionResource.h>
#include <daECS/core/coreEvents.h>
#include "render/fx/effectManager.h"
#include "render/fx/fx.h"
#include "render/fx/effectEntity.h"
#include "game/gameEvents.h"
#include "phys/collRes.h"

ECS_TAG(render)
ECS_AFTER(auto_delete_entity_with_effect_component_es)
static inline void entity_binded_effect_es(const ecs::UpdateStageInfoAct &,
  const ecs::EntityId entity_binded_effect__entity,
  TheEffect &effect,
  const int entity_binded_effect__collNodeId,
  const TMatrix &entity_binded_effect__localEmitter)
{
  TMatrix tm;
  if (get_collres_body_tm(entity_binded_effect__entity, entity_binded_effect__collNodeId, tm, __FUNCTION__))
  {
    for (auto &fx : effect.getEffects())
      fx.fx->setEmitterTm(tm * entity_binded_effect__localEmitter);
  }
}

template <typename T>
inline bool get_animchar_collision_transform_ecs_query(ecs::EntityId eid, T cb);
bool get_animchar_collision_transform(const ecs::EntityId eid,
  const CollisionResource *&collision,
  const AnimV20::AnimcharBaseComponent *&eid_animchar,
  TMatrix &eid_transform)
{
  return get_animchar_collision_transform_ecs_query(eid,
    [&](const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar, const TMatrix &transform = TMatrix::IDENT) {
      eid_transform = transform;
      collision = &collres;
      eid_animchar = &animchar;
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag autodeleteEffectEntity, ecs::auto_type replication, ecs::auto_type effect)
static inline void validate_auto_delete_tag_on_client_only_entities_es(const ecs::Event &, ecs::EntityId eid)
{
  logerr("unable to autodelete effect entity, because only local (i.e. not networking) "
         "entities can be destroyed by client: eid=%d, template='%s'\n"
         "This logerr may be caused by adding the 'autodeleteEffectEntity' tag to entity that extends the 'replication' template",
    static_cast<ecs::entity_id_t>(eid), g_entity_mgr->getEntityTemplateName(eid));
}


ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(ecs::Tag autodeleteEffectEntity)
ECS_REQUIRE_NOT(ecs::auto_type replication)
static inline void auto_delete_client_entity_with_effect_component_es(
  const ecs::UpdateStageInfoAct &, ecs::EntityId eid, const TheEffect &effect)
{
  for (const auto &fx : effect.getEffects())
    if (!fx.fx->isUpdated() || fx.fx->queryIsActive()) // isAlive() doesn't work for locked effects
      return;

  g_entity_mgr->destroyEntity(eid);
}

ecs::EntityId spawn_fx(const TMatrix &fx_tm, const int fx_id)
{
  AcesEffect *fxPtr = acesfx::start_effect(fx_id, fx_tm, TMatrix::IDENT, false);
  if (fxPtr == nullptr)
    return ecs::INVALID_ENTITY_ID;
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, effect, TheEffect(eastl::move(*fxPtr)));
  return g_entity_mgr->createEntityAsync("effect_entity", eastl::move(attrs));
}

ecs::EntityId spawn_human_binded_fx(const TMatrix &fx_tm,
  const TMatrix &itm,
  const int fx_id,
  const ecs::EntityId eid,
  const int node_coll_id,
  const Color4 &color_mult,
  const char *tmpl)
{
  AcesEffect *fxPtr = acesfx::start_effect(fx_id, fx_tm, TMatrix::IDENT, false);
  if (fxPtr == nullptr || node_coll_id < 0)
    return ecs::INVALID_ENTITY_ID;
  fxPtr->setColorMult(color_mult);
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, entity_binded_effect__entity, eid);
  ECS_INIT(attrs, effect, TheEffect(eastl::move(*fxPtr)));
  ECS_INIT(attrs, entity_binded_effect__collNodeId, node_coll_id);
  ECS_INIT(attrs, entity_binded_effect__localEmitter, itm * fx_tm);
  return g_entity_mgr->createEntityAsync(tmpl, eastl::move(attrs));
}
