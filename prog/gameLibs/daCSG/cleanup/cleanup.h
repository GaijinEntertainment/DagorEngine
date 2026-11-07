// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <EASTL/string.h>

namespace dacsg
{
// an edge between two charts is a border, which the cleanup keeps in place (spec 8)
struct ChartTest
{
  virtual ~ChartTest() = default;
  virtual bool sameChart(const LeafRef &a, const LeafRef &b) const = 0;
};

// False for a vertex index out of range or a leafRef or optional array of the wrong size, with s as it came, or for an open result,
// with s cleaned; self-intersection is validation's: without p.wideCrossingCheck an edit can cross a far face of its tilted fan.
// stats.unfixed counts faces still defective and steps undone for volume or genus.
// A new face copies FaceInfo, leafRef and facePlaneKey of a face it replaces.
// s is rounded to float first; tolerance is resolved and positive, not auto at 0; the surface moves up to p.maxMoveFactor times it.
// s is closed and carries one LeafRef per face; its provenance and facePlaneKey are empty or full. No charts: no borders.
bool cleanup(Solid &s, const CleanupParams &p, double tolerance, CleanupStats &stats, eastl::string &err,
  const ChartTest *charts = nullptr);
} // namespace dacsg
