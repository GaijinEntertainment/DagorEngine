// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_assert.h>
#include <phys/dag_fastPhys.h>
#include <gameRes/dag_stdGameRes.h>
#include <gamePhys/props/atmosphere.h>
#include <ecs/delayedAct/actInThread.h>
#include "fastPhysTag.h"


using namespace AnimV20;

class FastPhysCTM final : public ecs::ComponentTypeManager
{
public:
  typedef FastPhysTag component_type;

  void requestResources(const char *, const ecs::resource_request_cb_t &res_cb) override
  {
    auto &res = res_cb.get<ecs::string>(ECS_HASH("animchar_fast_phys__res"));
    res_cb(!res.empty() ? res.c_str() : "<missing_animchar_fast_phys>", FastPhysDataGameResClassId);
  }

  void create(void *, ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    const char *res = mgr.get<ecs::string>(eid, ECS_HASH("animchar_fast_phys__res")).c_str();
    FastPhysSystem *fpsys = create_fast_phys_from_gameres(res);
    if (!fpsys)
    {
      logerr("Cannot create fastphys by res '%s' for entity %d<%s>", res, (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
      return;
    }
    AnimcharBaseComponent *animChar = mgr.getNullableRW<AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
    animChar->setFastPhysSystem(fpsys); // Note: fpsys get owned by animchar
  }
};

ECS_REGISTER_TYPE_BASE(FastPhysTag, ecs::ComponentTypeInfo<FastPhysTag>::type_name, nullptr, &ecs::CTMFactory<FastPhysCTM>::create,
  &ecs::CTMFactory<FastPhysCTM>::destroy, ecs::COMPONENT_TYPE_NEED_RESOURCES);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "animchar_fast_phys__res", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(FastPhysTag, "animchar_fast_phys", nullptr, 0, "animchar", "animchar_fast_phys__res");

ECS_NO_ORDER
ECS_REQUIRE(FastPhysTag animchar_fast_phys)
static inline void animchar_fast_phys_es(const ParallelUpdateFrameDelayed &, AnimV20::AnimcharBaseComponent &animchar,
  const bool animchar__visible = true)
{
  if (!animchar.getFastPhysSystem() || !animchar__visible)
    return;
  Point3 wind = gamephys::atmosphere::get_wind();
  float windLen = length(wind); // can be optimized
  animchar.getFastPhysSystem()->windVel = wind;
  animchar.getFastPhysSystem()->windPower = windLen;
  animchar.getFastPhysSystem()->windTurb = windLen;
}


ECS_REQUIRE(FastPhysTag animchar_fast_phys)
ECS_ON_EVENT(on_disappear)
static void animchar_fast_phys_destroy_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar)
{
  if (FastPhysSystem *fastPhys = animchar.getFastPhysSystem())
  {
    animchar.setFastPhysSystem(nullptr);
    delete fastPhys;
  }
}
