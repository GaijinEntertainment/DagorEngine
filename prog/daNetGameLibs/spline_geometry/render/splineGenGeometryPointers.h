// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/shared_ptr.h>

class SplineGenGeometryIb;
class SplineGenGeometryAsset;
class SplineGenGeometryManager;
class SplineGenGeometryShapeManager;

using SplineGenGeometryIbPtr = eastl::weak_ptr<SplineGenGeometryIb>;
using SplineGenGeometryAssetPtr = eastl::weak_ptr<SplineGenGeometryAsset>;
using SplineGenGeometryManagerPtr = eastl::weak_ptr<SplineGenGeometryManager>;
using SplineGenGeometryShapeManagerPtr = eastl::weak_ptr<SplineGenGeometryShapeManager>;