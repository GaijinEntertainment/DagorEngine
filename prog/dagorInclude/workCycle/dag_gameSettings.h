//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//! if dgs_limit_fps=true, FPS is limited by game rate; default=false
extern bool dgs_limit_fps;

//! toggles FPS computation and on-screen rendering
extern bool dgs_draw_fps;

//! controls whether application window is locked or is movable
extern bool dgs_immovable_window;

//! controls whether application consume CPU when in background
extern bool dgs_dont_use_cpu_in_background;

//! controls whether application has higher priority CPU when active; default=false
extern bool dgs_higher_active_app_priority;

//! control flag to make single screenshot and autoreset false
extern bool dgctrl_need_screen_shot;

//! control flag to make single huge resolutionscreenshot and autoreset false
extern bool dgctrl_need_print_screen_shot;

//! control multipliert to make single huge resolution screenshot and autoreset false
extern int dgctrl_print_screen_shot_multiplier;

//! the mip number to start static scene lightmaps loading from.
extern unsigned int lightmap_quality;

//! Visibility range multiplier.
extern float visibility_range_multiplier;
