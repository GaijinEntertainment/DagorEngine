// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>

// returns zero resolution if world renderer doesn't exist
void get_display_resolution(int &w, int &h);
void get_rendering_resolution(int &w, int &h);
void get_postfx_resolution(int &w, int &h);
void get_final_render_target_resolution(int &w, int &h);

IPoint2 get_display_resolution();
IPoint2 get_rendering_resolution();
IPoint2 get_postfx_resolution();
IPoint2 get_final_render_target_resolution();
bool is_upsampling();
float get_default_static_resolution_scale();
