// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiRender.h"
#include "userUi.h"
#include "ui/overlay.h"
#include "render/world/dargPanelAnchorResolve.h"
#include <gui/dag_stdGuiRender.h>

#include <startup/dag_globalSettings.h>
#include <util/dag_threadPool.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>

#include <ecs/core/entityManager.h>
#include <ecs/scripts/scripts.h>

#include <daRg/dag_guiScene.h>
#include <daRg/dag_panelRenderer.h>


namespace uirender
{


bool beforeRenderMultithreaded = true;
bool multithreaded = true;


all_darg_scenes_t get_all_scenes()
{
  all_darg_scenes_t scenes;
  if (auto gscn = user_ui::gui_scene.get())
    scenes.push_back(gscn);
  if (auto oscn = overlay_ui::gui_scene())
    scenes.push_back(oscn);
  return scenes;
}


bool has_scenes() { return overlay_ui::gui_scene() || user_ui::gui_scene.get(); }


static class UIRenderJob final : public cpujobs::IJob
{
  WinCritSec critSec;

public:
  enum RunState : uint8_t
  {
    INITIAL = 0x00,
    BEFORE_RENDER_DONE = 0x01,
    START_ALLOWED = 0x02,
    STARTED = 0x03,
  };
  uint8_t runState = INITIAL;

  void doJob() override
  {
    TIME_PROFILE(darg_scene_build_render);
    WinAutoLock lock(critSec);
    // Note: daRg is "owning" `StdGuiRender` until this job's completion so no much sense to do this reset in main thread
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();
    for (darg::IGuiScene *scn : get_all_scenes())
    {
      FRAMEMEM_REGION;
      HSQUIRRELVM vm = scn->getScriptVM();
      VMScopedCriticalSection vmLock(vm, &critSec);
      sq_limitthreadaccess(vm, ::get_current_thread_id());
      scn->renderThreadBeforeRender();
      scn->buildRender();
    }
  }

  void start(bool wake)
  {
    using namespace threadpool;
    auto wakef = (wake && get_current_worker_id() < 0) ? AddFlags::WakeOnAdd : AddFlags::None;
    uint32_t _1;
    add(this, PRIO_LOW, _1, wakef | AddFlags::IgnoreNotDone);
  }

  void tryStart(RunState value, RunState expected, bool wake = true)
  {
    // Start when both BEFORE_RENDER_DONE and START_ALLOWED are obtained
    if (interlocked_or(runState, value) == expected)
      start(wake);
  }
} ui_render_job;


void wait_ui_render_job_done()
{
  if (multithreaded)
  {
    if (!interlocked_acquire_load(ui_render_job.done))
    {
      TIME_PROFILE(wait_ui_render_job_done);
      threadpool::wait(&ui_render_job);
    }

    for (darg::IGuiScene *scn : get_all_scenes())
    {
      sq_limitthreadaccess(scn->getScriptVM(), ::get_main_thread_id());
    }
  }
}

static class UIBeforeRenderJob final : public cpujobs::IJob
{
  WinCritSec critSec;

public:
  float dt;
  void doJob() override
  {
    TIME_PROFILE(gui_before_render_update);
    WinAutoLock lock(critSec);

    for (darg::IGuiScene *scn : get_all_scenes())
    {
      FRAMEMEM_REGION;
      HSQUIRRELVM vm = scn->getScriptVM();
      VMScopedCriticalSection vmLock(vm, &critSec);
      sq_limitthreadaccess(vm, ::get_current_thread_id());
      scn->update(dt);
    }

    ui_render_job.tryStart(ui_render_job.BEFORE_RENDER_DONE, ui_render_job.START_ALLOWED);
  }
} ui_before_render_job;


void wait_ui_before_render_job_done()
{
  if (!interlocked_acquire_load(ui_before_render_job.done))
  {
    TIME_PROFILE(wait_ui_before_render_job_done);
    threadpool::wait(&ui_before_render_job);
  }
}


void start_ui_render_job(bool wake)
{
  if (beforeRenderMultithreaded)
    ui_render_job.tryStart(ui_render_job.START_ALLOWED, ui_render_job.BEFORE_RENDER_DONE, wake);
  else
    ui_render_job.start(wake);
}


void start_ui_before_render_job()
{
  if (beforeRenderMultithreaded)
    threadpool::add(&ui_before_render_job, threadpool::PRIO_LOW, true);
}

void skip_ui_render_job()
{
  if (beforeRenderMultithreaded)
    G_VERIFY(interlocked_exchange(ui_render_job.done, 1) == 0); // Since we resetted it on `ui_before_render_job` start
  StdGuiRender::reset_per_frame_dynamic_buffer_pos();
}

void update_all_gui_scenes_mainthread(float dt)
{
  for (darg::IGuiScene *scn : get_all_scenes())
  {
    scn->update(dt);
    scn->mainThreadBeforeRender();
  }
}

void before_render(float dt, const TMatrix &view_itm, const TMatrix &view_tm)
{
  TIME_PROFILE(ui_before_render);

  auto scenes = get_all_scenes();

  if (beforeRenderMultithreaded)
  {
    G_ASSERT(g_entity_mgr->isConstrainedMTMode());

    for (darg::IGuiScene *scn : scenes)
      scn->mainThreadBeforeRender();

    threadpool::wait(&ui_before_render_job);
    threadpool::wait(&ui_render_job);
    ui_before_render_job.dt = dt;
    interlocked_release_store(ui_render_job.done, 0); // Set to not done to be able to wait one `ui_render_job` insted of 2
    interlocked_release_store(ui_render_job.runState, ui_render_job.INITIAL);
  }
  else
    update_all_gui_scenes_mainthread(dt);

  for (auto &uiscn : scenes)
  {
    if (DAGOR_LIKELY(!uiscn->hasAnyPanels()))
      continue;
    TIME_PROFILE(ui_update_panels);
    ScopeRenderTarget scopeRt;
    d3d::set_render_target();
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    darg::IGuiScene::VrSceneData vrScene;
    vrScene.camera = view_itm;
    vrScene.cameraFrustum = Frustum(TMatrix4(view_tm) * projTm);
    vrScene.entityTmResolver = get_entitiy_node_transform;
    for (int i = &uiscn - scenes.data(); i < scenes.size(); ++i)
      if (scenes[i]->hasAnyPanels())
        scenes[i]->updateSpatialElements(vrScene);
    break;
  }
}

void init()
{
  multithreaded = ::dgs_get_settings()->getBlockByNameEx("ui")->getBool("multithreaded", true) && threadpool::get_num_workers() > 0;
  beforeRenderMultithreaded =
    ::dgs_get_settings()->getBlockByNameEx("ui")->getBool("beforeRenderMultithreaded", true) && threadpool::get_num_workers() > 0;
}


} // namespace uirender
