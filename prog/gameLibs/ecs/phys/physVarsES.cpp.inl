#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <ecs/phys/physVars.h>
#include <math/random/dag_random.h>

ECS_REGISTER_RELOCATABLE_TYPE(PhysVars, nullptr);
ECS_AUTO_REGISTER_COMPONENT(PhysVars, "phys_vars", nullptr, 0);

ECS_ON_EVENT(on_appear)
ECS_BEFORE(anim_phys_init_es)
static __forceinline void phys_vars_fixed_init_es_event_handler(const ecs::Event &, PhysVars &phys_vars,
  const ecs::Object &phys_vars__fixedInit)
{
  for (auto &fixedInit : phys_vars__fixedInit)
    phys_vars.setupVar(fixedInit.first.c_str(), fixedInit.second.get<float>());
}

ECS_ON_EVENT(on_appear)
ECS_BEFORE(anim_phys_init_es)
static __forceinline void phys_vars_random_init_es_event_handler(const ecs::Event &, PhysVars &phys_vars,
  const ecs::Object &phys_vars__randomInit)
{
  for (auto &randomInit : phys_vars__randomInit)
  {
    const Point2 &randomLimits = randomInit.second.get<Point2>();
    phys_vars.setupVar(randomInit.first.c_str(), rnd_float(randomLimits[0], randomLimits[1]));
  }
}
