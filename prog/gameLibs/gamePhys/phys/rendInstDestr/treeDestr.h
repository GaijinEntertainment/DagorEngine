// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

#include <gamePhys/phys/rendinstDestr.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <rendInst/rendInstDebris.h>

#include "riDestrInternal.h"

struct Frustum;

namespace rendinstdestr
{

#if RI_DESTR_DEBUG_ENABLED
extern int pull_tree_destr_imgui;
#endif

extern Tab<RendInstPhys> riPhys;
extern int currentCutTreeCount;
extern bool useDebugTreeInstData;
extern rendinst::TreeInstDebugData debugTreeInstData;
extern DestrSettings destrSettings;
extern rendinst::ri_damage_effect_cb ri_effect_cb;
extern on_ri_restored_callback on_ri_restored_cb;

bool restore_destroyed_tree(RendInstPhys &phys);
void init_tree_destr(ri_tree_sound_cb tree_sound_cb, bool enable_branch_destruction);
void update_ri_phys(float dt, float current_time, const Frustum *frustum);

} // namespace rendinstdestr
