//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class BaseTexture;
typedef BaseTexture Texture;
class BBox2;
struct PhysMap;
class PhysMapTexData;

void destroy_phys_map_tex_data(PhysMapTexData *);
Texture *create_phys_map_ht_tex(PhysMapTexData *); // to be called from main thread
PhysMapTexData *render_phys_map_ht_data(const PhysMap &phys_map, const BBox2 &region, float &maxHt, float htscale,
  bool apply_decals = true); // to be called from loading thread
