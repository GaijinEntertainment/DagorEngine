// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void exit_game(const char *reason_static_str); // pointer should be statically allocated string (not null)
bool dng_is_app_terminating();                 //< returns true after entering post-shutdown handler

void set_window_title(const char *net_role);
const char *get_dir(const char *location);
extern bool has_in_game_editor();
bool is_initial_loading_complete();
// Called right before the game scripts load, on the initial loading thread (the main thread in a
// dedicated build): the mounts, vroms, game resources, world renderer, collision world and das init
// (gamescripts::init) are done by then.
extern void (*on_app_before_game_scripts)();

void set_fps_limit(int max_fps);
int get_fps_limit();
void set_corrected_fps_limit(int fps_limit = -1);
