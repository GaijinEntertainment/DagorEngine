//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point2;
extern void init_clouds(const char *clouds_tex_name, const Point2 &size, const Point2 &speed);
extern void close_clouds();
// tex_id - new texture to be used, BAD_TEXTUREID to use default
extern void change_clouds_tex(TEXTUREID tex_id);
