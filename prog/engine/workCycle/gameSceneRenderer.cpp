#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSceneRenderer.h>
#include "workCyclePriv.h"

class DefaultDagorGameSceneRenderer : public IDagorGameSceneRenderer
{
public:
  virtual void render(DagorGameScene &scene) { scene.drawScene(); }
};

static DefaultDagorGameSceneRenderer _def_rend;
IDagorGameSceneRenderer *workcycle_internal::game_scene_renderer = &_def_rend;

void dagor_set_game_scene_renderer(IDagorGameSceneRenderer *renderer)
{
  if (!renderer)
    renderer = &_def_rend;
  workcycle_internal::game_scene_renderer = renderer;
}

IDagorGameSceneRenderer *dagor_get_current_game_scene_renderer() { return workcycle_internal::game_scene_renderer; }
