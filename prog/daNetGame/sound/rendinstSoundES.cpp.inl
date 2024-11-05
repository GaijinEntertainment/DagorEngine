// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/sharedComponent.h>
#include <soundSystem/events.h>
#include <soundSystem/vars.h>
#include "sound/rendinstSound.h"


template <typename Callable>
static void rendinst_tree_sound_ecs_query(Callable c);

namespace rendinstsound
{
bool rendinst_tree_sound_cb(
  TreeSoundCbType call_type, const TreeSoundDesc &desc, ri_tree_sound_cb_data_t & /*sound_data*/, const TMatrix &tm)
{
  if (TREE_SOUND_CB_INIT == call_type)
  {
    bool isInited = false;
    rendinst_tree_sound_ecs_query(
      [&](const ecs::string &rendinst_tree_sounds__fallingPath, const ecs::string &rendinst_tree_sounds__falledPath) {
        G_UNREFERENCED(rendinst_tree_sounds__falledPath);
        if (sndsys::should_play(tm.getcol(3)))
        {
          sndsys::EventHandle handle = sndsys::init_event(rendinst_tree_sounds__fallingPath.c_str(), tm.getcol(3));
          sndsys::set_var(handle, "tree_height", desc.treeHeight);
          sndsys::set_var(handle, "is_bush", desc.isBush ? 1.f : 0.f);
          sndsys::start(handle);
          sndsys::abandon(handle);
          isInited = true;
        }
      });
    return isInited;
  }

  if (TREE_SOUND_CB_ON_FALLED == call_type)
  {
    rendinst_tree_sound_ecs_query(
      [&](const ecs::string &rendinst_tree_sounds__fallingPath, const ecs::string &rendinst_tree_sounds__falledPath) {
        G_UNREFERENCED(rendinst_tree_sounds__fallingPath);
        if (!rendinst_tree_sounds__falledPath.empty())
        {
          if (sndsys::should_play(tm.getcol(3)))
          {
            sndsys::EventHandle falledSoundEventHandle = sndsys::init_event(rendinst_tree_sounds__falledPath.c_str(), tm.getcol(3));
            sndsys::set_var(falledSoundEventHandle, "tree_height", desc.treeHeight);
            sndsys::set_var(falledSoundEventHandle, "is_bush", desc.isBush ? 1.f : 0.f);
            sndsys::start(falledSoundEventHandle);
            sndsys::abandon(falledSoundEventHandle);
          }
        }
      });
  }

  return true;
}

bool have_sound() { return true; }
} // namespace rendinstsound
