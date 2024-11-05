//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BigLightsShadows;
class DPoint3;
class Point4;

BigLightsShadows *create_big_lights_shadows(int w, int h, unsigned int maxCnt, const char *prefix);
void destroy_big_lights_shadows(BigLightsShadows *&r);
void render_big_lights_shadows(BigLightsShadows *r, const Point4 *pos_rad, uint32_t cnt, const DPoint3 *world_pos);
void set_big_lights_shadows(BigLightsShadows *r, int reg);
