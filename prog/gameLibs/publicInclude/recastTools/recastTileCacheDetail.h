//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <DetourNavMeshBuilder.h>
#include <DetourTileCacheBuilder.h>

class rcContext;
struct rcCompactHeightfield;
struct rcPolyMeshDetail;

namespace recastnavmesh
{

struct TileCacheDetailStorage
{
  TileCacheDetailStorage() = default;
  TileCacheDetailStorage(const TileCacheDetailStorage &) = delete;
  TileCacheDetailStorage &operator=(const TileCacheDetailStorage &) = delete;
  ~TileCacheDetailStorage();

  bool build(const dtTileCacheLayer &layer, const dtNavMeshCreateParams &params, float max_edge_error, float detail_sample_dist,
    float detail_sample_max_error);
  bool build(rcContext &ctx, const dtTileCacheLayer &layer, const dtNavMeshCreateParams &params, float max_edge_error,
    float detail_sample_dist, float detail_sample_max_error);
  void clear();
  void apply(dtNavMeshCreateParams &params) const;

private:
  rcCompactHeightfield *chf = nullptr;
  rcPolyMeshDetail *dmesh = nullptr;
  Tab<unsigned short> regs;
};

} // namespace recastnavmesh
