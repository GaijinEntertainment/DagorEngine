//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <util/dag_stdint.h>

class CfgReader;
class CollisionResource;

void init_collres_debug_draw(CfgReader &cfg);
void term_collres_debug_draw();

// The triangles of res inside box that pass behavior_filter (0 takes all), in physmat colours; wire draws their edges.
void draw_collres_debug(const CollisionResource &res, bbox3f_cref box, uint16_t behavior_filter, bool wire);

float get_collres_debug_draw_radius();
