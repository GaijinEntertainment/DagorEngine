//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/initializer_list.h>

class Point3;

bool p3_nonzero(const Point3 &p);
float distance_to_triangle(const Point3 &p, const Point3 &a, const Point3 &b, const Point3 &c, Point3 &out_contact,
  Point3 &out_normal);
void get_random_point_on_sphere(float min_x, float max_x, int &rand_seed, Point3 &out_pnt);

float min(std::initializer_list<float> val_list);
float max(std::initializer_list<float> val_list);
