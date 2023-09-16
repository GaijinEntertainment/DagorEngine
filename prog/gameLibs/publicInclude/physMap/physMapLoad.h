//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct PhysMap;
class IGenLoad;

PhysMap *load_phys_map(IGenLoad &crd, bool is_lmp2);
PhysMap *load_phys_map_with_decals(IGenLoad &crd, bool is_lmp2);

// Build grid for decals with the provided size (sz*sz cells)
void make_grid_decals(PhysMap &phys_map, int sz);
