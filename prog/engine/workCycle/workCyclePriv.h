// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

#if USE_X11
#include <X11/Xlib.h>
#endif

class DagorGameScene;
class IDagorGameSceneRenderer;

namespace workcycle_internal
{
extern DagorGameScene *game_scene;
extern DagorGameScene *secondary_game_scene;
extern IDagorGameSceneRenderer *game_scene_renderer;
extern volatile int64_t last_wc_time, last_draw_time;
extern bool window_initing;
extern bool enable_idle_priority;
extern bool is_window_in_thread;
extern float gametimeElapsed;
extern int minVariableRateActTimeUsec, lastFrameTime;
extern int fixedActPerfFrame, curFrameActs;

intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
void set_priority(bool foreground);
void default_on_swap_callback();
void idle_loop();

void set_title(const char *title, bool utf8);

#if _TARGET_PC_LINUX
void set_title_tooltip(const char *title, const char *tooltip, bool utf8);
#else
inline void set_title_tooltip(const char *title, const char *, bool utf8) { set_title(title, utf8); }
#endif //_TARGET_PC_LINUX

#if USE_X11
Display *get_root_display();
bool get_last_cursor_pos(int *cx, int *cy, Window w);
const XWindowAttributes &get_window_attrib(Window w, bool translated);
void update_cursor_position(int cx, int cy, Window w);
#endif
} // namespace workcycle_internal

namespace tql
{
extern void (*on_frame_finished)();
}
