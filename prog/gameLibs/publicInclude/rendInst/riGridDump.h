//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

// On-disk riExtra object set, written by the rigrid.dump console command. Deliberately holds no grid
// tuning: the set is the scene, so a reader is free to build a grid with any cell size and config
// from it.
//
// Layout: header, then poolCount name records, then instanceCount instance records.
// A name record is int32 length followed by that many bytes, no terminator. Pools keep riExtra
// order, so handle_to_ri_type of an instance handle indexes them directly.

namespace rigrid_dump
{

static constexpr int MAGIC = 0x47584952; // 'RIXG'
static constexpr int VERSION = 2;

// Provenance, not tuning: what the writer's grid was configured with, so a reader can reproduce its
// structure before it starts changing things. A reader is still free to ignore it and sweep.
struct GridConfig
{
  int32_t cellSize;
  float maxMainExtension;
  float maxSubExtension;
  float maxSubRadius;
  int32_t objectsToCreateSubGrid;
  int32_t maxLeafObjects;
  int32_t reserveObjectsOnGrow;
};

struct Header
{
  int32_t magic;
  int32_t version;
  int32_t poolCount;
  int32_t instanceCount;
  GridConfig config;
  int32_t reserved[5];
};

// The world sphere and box are stored as the grid itself sees them, not re-derived from the
// transform on load: the sphere is what an object is bucketed by and the box is what queries test,
// so a reader that recomputed them could disagree with the runtime it is meant to reproduce.
struct Instance
{
  uint64_t handle;  // riex_handle_t
  float tm[12];     // world transform, 3 basis vectors then position
  float bsphere[4]; // world sphere, center then radius
  float bbox[6];    // world box, bmin then bmax
};

static_assert(sizeof(GridConfig) == 28);
static_assert(sizeof(Header) == 64);
static_assert(sizeof(Instance) == 96);

} // namespace rigrid_dump
