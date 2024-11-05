// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void exit_game(const char *reason_static_str); // pointer should be statically allocated string (not null)
void set_window_title(const char *net_role);
extern const char *default_game_name;
inline const char *get_game_name() { return default_game_name; }
const char *get_dir(const char *location);
extern bool has_in_game_editor();
bool is_initial_loading_complete();

void set_fps_limit(int max_fps);
int get_fps_limit();
void set_corrected_fps_limit(int fps_limit = -1);
