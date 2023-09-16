#include <daECS/core/entityManager.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include "ecsQueryInternal.h"

namespace ecs
{

void EntityManager::setConstrainedMTMode(bool on)
{
  const int current = interlocked_add(constrainedMode, on ? 1 : -1);
  const bool isOn = bool(current != 0), wasOn = bool(current - (on ? 1 : -1) != 0);
  if (isOn != wasOn)
  {
#if DAECS_EXTENSIVE_CHECKS // this should not happen
    if (on)
    {
      if (lastUpdatedCreationQueueGen == INVALID_CREATION_QUEUE_GEN)
      {
        logerr("Can't setConstrainedMTMode while tracking changes");
        // todo: check if we are within sync creation
      }
      else
      {
        if (allQueriesUpdatedToArch != archetypes.generation())
          logerr("queries are not up to date, which shouldn't happen");
      }
    }
#endif

    if (current < 0 && !on)
    {
      logerr("non paired constrained mode release!");
      interlocked_increment(constrainedMode); // increase by one to prevent numerous 'fix'
    }
  }
}

#if DAECS_EXTENSIVE_CHECKS
bool EntityManager::isQueryingArchetype(uint32_t arch) const
{
  G_ASSERT_RETURN(arch < archetypes.queryingArchetypeCount.size(), false);
  return interlocked_acquire_load(archetypes.queryingArchetypeCount[arch]) != 0;
}
void EntityManager::changeQueryingArchetype(uint32_t arch, int add)
{
  G_ASSERT_RETURN(arch < archetypes.queryingArchetypeCount.size(), );
  if (interlocked_add(archetypes.queryingArchetypeCount[arch], add) < 0)
  {
    logerr("archetype %d querying count became negative after %d add!", arch, add);
    archetypes.queryingArchetypeCount[arch] = 0; // attempt to restore
  }
}

EntityManager::ScopedQueryingArchetypesCheck::ScopedQueryingArchetypesCheck(uint32_t index_, EntityManager &mgr_) :
  index(index_), mgr(mgr_)
{
  validateCount = mgr.archetypeQueries[index].getQueriesCount();
  changeQueryingArchetypes(1);
}
void EntityManager::ScopedQueryingArchetypesCheck::changeQueryingArchetypes(int add)
{
  const auto *aqi = mgr.archetypeQueries[index].queriesBegin(), *aqEnd = aqi + validateCount;
  for (; aqi != aqEnd; ++aqi)
    mgr.changeQueryingArchetype(*aqi, add);
}

#endif

}; // namespace ecs
