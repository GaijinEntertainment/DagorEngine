// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/ecsGameRes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <3d/dag_texPackMgr2.h>
#include <gameRes/dag_gameResSystem.h>
#include <EASTL/vector_set.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <ecs/gameres/commonLoadingJobMgr.h>

namespace ecs
{

bool load_gameres_list(const gameres_list_t &reslist)
{
  bool ret = set_required_res_list_restriction(&reslist.begin()->first, &reslist.end()->first, RRL_setAndPreloadAndReset,
    sizeof(gameres_list_t::value_type));
  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading();
  return ret;
}

bool filter_out_loaded_gameres(gameres_list_t &reslist, unsigned spins)
{
  WinCritSec &gameres_main_cs = get_gameres_main_cs();
  auto do_filter_out = [&]() {
    reslist.erase(eastl::remove_if(reslist.begin(), reslist.end(),
                    [](const gameres_list_t::value_type &kv) {
                      return is_game_resource_loaded_nolock(GAMERES_HANDLE_FROM_STRING(kv.first.c_str()), kv.second);
                    }),
      reslist.end());
  };
  if (!spins)
  {
    WinAutoLock lock(gameres_main_cs);
    do_filter_out();
  }
  else
    for (unsigned i = 0; i < spins; ++i)
    {
      if (gameres_main_cs.tryLock())
      {
        do_filter_out();
        gameres_main_cs.unlock();
        break;
      }
      // tryLock failed, yield and try again
      cpu_yield();
    }
  return !reslist.empty();
}

struct LoadGameResJob final : public cpujobs::IJob
{
  static constexpr unsigned JOB_TAG = _MAKE4C('LGRJ');
  gameres_list_t reslist;
  eastl::vector<EntityId> entities;
  bool failed = false;
  bool loaded = false;
  void doJob() override
  {
    failed = !load_gameres_list(reslist);
    loaded = true;
  }
  void releaseJob() override
  {
    if (g_entity_mgr && loaded) // in case if this callback is called after destruction of EntityManager
      g_entity_mgr->onEntitiesLoaded(entities, !failed);
    delete this;
  }
  unsigned getJobTag() override { return JOB_TAG; };
};

void place_gameres_request(eastl::vector<ecs::EntityId> &&eids, gameres_list_t &&requests) // called by EntityManager
{
  G_ASSERT(!eids.empty());
  G_ASSERT(requests.size());

  LoadGameResJob *job = new LoadGameResJob;
  job->entities = eastl::move(eids);
  job->reslist = eastl::move(requests);
  int loadingJobMgrId = get_common_loading_job_mgr();
#if DAECS_EXTENSIVE_CHECKS
  if (loadingJobMgrId == cpujobs::COREID_IMMEDIATE)
    logerr("ECS common loading job mgr id isn't set - resources will be loaded synchronously.\n"
           "ecs::set_common_loading_job_mgr() wasn't called?");
#endif
  G_VERIFY(cpujobs::add_job(loadingJobMgrId, job));
}

} // namespace ecs

ECS_NO_ORDER
static void cancel_loading_ecs_jobs_es(const ecs::EventEntityManagerBeforeClear &)
{
  cpujobs::remove_jobs_by_tag(ecs::get_common_loading_job_mgr(), ecs::LoadGameResJob::JOB_TAG);
  // Note: intentionally don't wait anything here to avoid causing stalls by it.
}
