//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class Point3;
extern void term_tex_noises();

// we can support arbitrary noise formats and resolutions with easy map, but it doesn't make much sense
extern const SharedTexHolder &init_and_get_argb8_64_noise(); // increment counter and init if not inited
extern const SharedTexHolder &init_and_get_l8_64_noise();    // increment counter and init if not inited
// increment counter and init if not inited
extern const SharedTexHolder &init_and_get_perlin_noise_3d(Point3 &min_r, Point3 &max_r);

// noise LUT texture by @Dave_Hoskins, www.shadertoy.com/view/4sfGzS
// size is 128x128
extern const SharedTexHolder &init_and_get_hash_128_noise(); // increment counter and init if not inited

// 128x128 rg8 blue noise texture
extern const SharedTexHolder &init_and_get_blue_noise();

extern void release_l8_64_noise();      // decrement counter and call close if counter is 0
extern void release_64_noise();         // decrement counter and call close if counter is 0
extern void release_perline_noise_3d(); // decrement counter and call close if counter is 0
extern void release_blue_noise();       // decrement counter and call close if counter is 0

// noise LUT texture by @Dave_Hoskins, www.shadertoy.com/view/4sfGzS
extern void release_hash_128_noise(); // decrement counter and call close if counter is 0
