// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class LandMeshManager;
class BBox2;

namespace rendinst::gen
{
// The height range of the land under bbox_xz, widened for rendinst placement. The land mesh cells give it; a
// manager with no land mesh cells gives its heightmap's range; with neither (or a null lmesh) it is 0..8192.
void land_min_max(LandMeshManager *lmesh, const BBox2 &bbox_xz, float &out_min, float &out_max);
} // namespace rendinst::gen
