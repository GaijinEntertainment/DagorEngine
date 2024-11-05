// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void render_grass_prepass();
void render_grass();
void init_grass(const DataBlock *grass_settings_from_level);
void erase_grass(const Point3 &world_pos, float radius);
void grass_invalidate();
void grass_invalidate(const dag::ConstSpan<BBox3> &boxes);
void grass_prepare(const TMatrix &itm);