//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/c_common.h>

class Point3;

namespace PropPanel
{

namespace p2util
{
const char *get_icon_path();
void set_icon_path(const char *value);

void *get_main_parent_handle();
void set_main_parent_handle(void *phandle);

Point3 hsv2rgb(Point3 hsv);
Point3 rgb2hsv(Point3 rgb);

E3DCOLOR rgb_to_e3(Point3 rgb);
Point3 e3_to_rgb(E3DCOLOR e3);
} // namespace p2util

} // namespace PropPanel