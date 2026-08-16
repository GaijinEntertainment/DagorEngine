// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/phys/segHumanPhys.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>

ECS_REGISTER_SHARED_TYPE(SharedSegmentedHumanPhysics, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(ecs::SharedComponent<SharedSegmentedHumanPhysics>, "human_segmented_physics", nullptr, 0,
  "human_net_phys");

static bool initSegmentedHumanPhysics(ecs::SharedComponent<SharedSegmentedHumanPhysics> &human_segmented_physics, ecs::EntityId eid)
{
  bool success = false;
  auto &templateBlkName = (*g_entity_mgr).get<ecs::string>(eid, ECS_HASH("human_segments__template"));
  DataBlock segPhysBlk;
  if (segPhysBlk.load(templateBlkName.c_str()))
    if (human_segmented_physics->LoadFromTemplate(segPhysBlk))
      success = true;
  return success;
}

bool SharedSegmentedHumanPhysics::onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  auto &human_segmented_physics =
    mgr.getRW<ecs::SharedComponent<SharedSegmentedHumanPhysics>>(eid, ECS_HASH("human_segmented_physics"));
  return initSegmentedHumanPhysics(human_segmented_physics, eid);
}

ECS_DEF_PULL_VAR(human_segmented_physics);

template <typename Callable>
inline void process_all_segmented_human_physics_ecs_query(ecs::EntityManager &manager, Callable c);

void reloadSharedSegmentedHumanPhysics()
{
  process_all_segmented_human_physics_ecs_query(*g_entity_mgr,
    [&](ecs::EntityId eid, ecs::SharedComponent<SharedSegmentedHumanPhysics> &human_segmented_physics) {
      initSegmentedHumanPhysics(human_segmented_physics, eid);
    });
}

#include <util/dag_console.h>

static bool debug_segmented_human_physics_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("human", "reload_segmented_human_physics", 1, 1) { reloadSharedSegmentedHumanPhysics(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(debug_segmented_human_physics_console_handler);
