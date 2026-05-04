// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>

// Log the build timestamp, startup time, command line arguments and the current working directory.
void ec_log_startup_info();

// The startup scene renders the initial workspace creator/selector dialogs.
// bg_color: background color of the window (can be seen before ImGui starts drawing)
void startup_editor_core_select_startup_scene(E3DCOLOR bg_color = E3DCOLOR(229, 229, 229));

// Draw a new startup scene frame.
// This is needed to get rid of the open dialog from the screen once the modal loop ends.
void startup_editor_core_force_startup_scene_draw();
