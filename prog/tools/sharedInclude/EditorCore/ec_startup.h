// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// void editor_core_load_common_bitmap();

// The startup scene renders the initial workspace creator/selector dialogs.
void startup_editor_core_select_startup_scene();

// Draw a new startup scene frame.
// This is needed to get rid of the open dialog from the screen once the modal loop ends.
void startup_editor_core_force_startup_scene_draw();

void startup_editor_core_world_elem(bool do_clear_on_render);
