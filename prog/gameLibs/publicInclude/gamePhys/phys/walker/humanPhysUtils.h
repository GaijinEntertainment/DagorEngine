//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>

// Utils for things such as acceleration, deceleration, rotation etc, which is human-phys specific. But possibly could
// be moved to more generic place if need will arise

namespace gamephys
{
// This functions represent the method used in classic shooters, like Q3: we accelerate along wish
// direction to the desired speed. Provides specific feel for character control.
Point3 accelerate_directional(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt);
// same as previous, but with clamp for resulting velocity vector
Point3 accelerate_directional_clamp(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt);

// This one is simple acceleration, where we try to accelerate in direction of the resulting delta between
// cur_vel and wish_dir * wish_spd. Almost identical method used in Enlisted from the start with different
// flavors/parameters.
// NOTE IMPORTANT DIFFERENCE from previously used method: this one will not accelerate in braking direction.
// So if your resulting speed is neutral, then it'll not try to slow you down, it'll slow you down only when
// you have an opposite direction in that case.
Point3 accelerate_geometrical(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt);

// Applies friction to the speed. This function uses current speed as a multiplier for the friction, so faster
// you move - more friction you have. Like aerodynamic friction of sorts.
float calc_friction_mult(float cur_spd, float thres_spd, float friction_k, float dt);
Point3 apply_friction(const Point3 &cur_vel, float thres_spd, float friction_k, float dt);
Point3 apply_omni_friction(const Point3 &cur_vel, float thres_spd, float friction_k, float dt); // friction in all directions,
                                                                                                // including vertical
}; // namespace gamephys
