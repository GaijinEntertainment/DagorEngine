// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/updateStage.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include "entityManagerEvent.h"
#include <perfMon/dag_statDrv.h>
#include "ecsPerformQueryInline.h"

namespace ecs
{
const char *const es_stage_names[US_COUNT] = {
#define S(x) #x,
  ECS_UPDATE_STAGES_LIST
#undef S
};
inline const char *get_stage_name(uint32_t stage) { return stage < countof(es_stage_names) ? es_stage_names[stage] : "US_UPDATE"; }

#if TIME_PROFILER_ENABLED
static uint32_t dap_stage_tokens[US_COUNT + 1];
#endif

void init_profiler_tokens()
{
#if TIME_PROFILER_ENABLED
  if (dap_stage_tokens[0])
    return;
  for (int i = 0; i < countof(dap_stage_tokens); ++i)
  {
    const char *sn = get_stage_name(i);
    dap_stage_tokens[i] = ::da_profiler::add_description(__FILE__, __LINE__, /*flags*/ 0, sn);
  }
#endif
}

void EntityManager::update(const ecs::UpdateStageInfo &info)
{
  G_ASSERTF(lastEsGen == EntitySystemDesc::generation, "setEsOrder was not called");
  if (info.stage >= esUpdates.size()) // unlikely to happen, just sanity change
    return;
    // reference implementation
    // for (const EntitySystemDesc *esd : esList)
    //   if (esd && esd->stageMask & info.stage)
    //     performQuery(*esd, [esd, &info](const QueryView &ec) mutable{esd->ops.onUpdate(info, ec);}, 0);
#if TIME_PROFILER_ENABLED
  DA_PROFILE_EVENT_DESC(dap_stage_tokens[eastl::min(info.stage, (int)US_COUNT)]);
#endif
  createQueuedEntities(); // if entities were scheduled for creation outside ES
  for (auto esIndex : esUpdates[info.stage])
  {
    const EntitySystemDesc &es = *esList[esIndex];
    {
#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
      DA_PROFILE_EVENT_DESC(es.dapToken);
#endif
      performQueryEmptyAllowed(esListQueries[esIndex], (ESFuncType)es.ops.onUpdate, (const ESPayLoad &)info, es.userData, es.quant);
    }
    if (!isConstrainedMTMode())
    {
      if (current_tick_events < average_tick_events_uint) // let's try to send events as early, as possible
        sendQueuedEvents(average_tick_events_uint - current_tick_events);
    }
  }
  if (hasQueuedEntitiesCreation())
    performDelayedCreation(false); // we try to destroy queued entities after each query, asap
}

}; // namespace ecs
