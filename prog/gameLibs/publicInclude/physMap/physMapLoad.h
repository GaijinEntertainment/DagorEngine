//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct PhysMap;
class IGenLoad;

PhysMap *load_phys_map(IGenLoad &crd, bool is_lmp2);
PhysMap *load_phys_map_with_decals(IGenLoad &crd, bool is_lmp2);
void load_phys_map_decals(PhysMap *physMap);

// Partition phys_map.decals into a compact grid (sz*sz cells) for fast
// region queries; replaces the source decals on success. Grid verts are
// quantized to u16 per chunk, moving decal boundaries by <= ~1 mm at the
// 128 m chunk extent cap (typical content packs into 64 m chunks and stays
// around ~0.5 mm). Source shapes the compact layout cannot encode, including
// tris wider than the cap, are rejected (with logerr): the decals then stay
// and render un-gridded, correct but without spatial culling.
// phys_map.compactDecals != nullptr afterwards indicates grid storage.
void make_grid_decals(PhysMap &phys_map, int sz);
