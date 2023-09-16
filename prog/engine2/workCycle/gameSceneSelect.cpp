#include <workCycle/dag_gameScene.h>
#include "workCyclePriv.h"
#include <util/dag_globDef.h>
#include <EASTL/algorithm.h>

DagorGameScene *workcycle_internal::game_scene = nullptr;
DagorGameScene *workcycle_internal::secondary_game_scene = nullptr;

void dagor_select_game_scene(DagorGameScene *scene)
{
  static DagorGameScene *tmp_next_game_scene = NULL;
  static bool tmp_game_scene_selecting = false;

  using workcycle_internal::game_scene;
  if (tmp_game_scene_selecting)
  {
    G_ASSERTF(scene == game_scene || scene == tmp_next_game_scene, "old_scene=%p tmp_sel_game_scene=%p new_scene=%p", game_scene,
      tmp_next_game_scene, scene);
    tmp_next_game_scene = scene;
    return;
  }
  if (game_scene == scene)
    return;

  tmp_game_scene_selecting = true;
  tmp_next_game_scene = scene;
  DagorGameScene *prev = game_scene;

  if (game_scene)
  {
    game_scene->sceneDeselected(scene);
    if (tmp_next_game_scene != scene)
    {
      debug("DagorGameScene %p rejected change to %p", game_scene, scene);
      goto end;
    }
  }

  game_scene = nullptr;

  if (scene)
  {
    scene->sceneSelected(prev);
    if (tmp_next_game_scene != scene)
    {
      debug("DagorGameScene %p rejected change from %p", scene, game_scene);
      if (game_scene)
        game_scene->sceneSelected(game_scene);
      goto end;
    }
  }
  game_scene = scene;

end:
  tmp_game_scene_selecting = false;
  tmp_next_game_scene = NULL;
}

void dagor_select_secondary_game_scene(DagorGameScene *scene)
{
  using workcycle_internal::secondary_game_scene;

  if (secondary_game_scene == scene)
    return;

  auto prevSecondary = secondary_game_scene;

  if (secondary_game_scene)
  {
    secondary_game_scene->sceneDeselected(scene);
    secondary_game_scene = nullptr;
  }

  if (scene)
    scene->sceneSelected(prevSecondary);

  secondary_game_scene = scene;
}

DagorGameScene *dagor_get_current_game_scene() { return workcycle_internal::game_scene; }

DagorGameScene *dagor_get_secondary_game_scene() { return workcycle_internal::secondary_game_scene; }

void dagor_swap_scenes() { eastl::swap(workcycle_internal::game_scene, workcycle_internal::secondary_game_scene); }
