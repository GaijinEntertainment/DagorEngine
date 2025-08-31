// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

class BaseTexture;
typedef BaseTexture Texture;

void animated_splash_screen_start(bool do_encode = true);
void animated_splash_screen_stop();
void animated_splash_screen_draw();
void debug_animated_splash_screen();
bool is_animated_splash_screen_started();
bool is_animated_splash_screen_encoding();

void start_animated_splash_screen_in_thread();
void stop_animated_splash_screen_in_thread();
bool is_animated_splash_screen_in_thread();

void animated_splash_screen_register_render(void (*render_func)(int w, int h, void *arg), void *arg, bool exclusive = false);
void animated_splash_screen_allow_watchdog_kick(bool allow);
