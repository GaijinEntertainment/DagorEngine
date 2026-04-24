// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;

void add_billboard_decal(const Point3 &position, const Point3 &normal, int texture_idx, float size);
void erase_billboard_decals(const Point3 &world_pos, float radius);
void erase_all_billboard_decals();
