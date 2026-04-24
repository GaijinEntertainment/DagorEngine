// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "splineGenGeometryPointers.h"
#include "splineGenGeometryIb.h"
#include "splineGenGeometryAsset.h"
#include "splineGenGeometryManager.h"
#include "splineGenGeometryShapeManager.h"
#include <daECS/core/entityComponent.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <render/daFrameGraph/daFG.h>

// Repository that stores and creates shared resources among spline_gen_geometry entities.
// New objects are created on demand, whenever the requested key doesn't exist yet.
// It's stored in the singleton spline_gen_repository entity.
// It was added to the scenes, because it's existence is a prerequisite to other entities.
// In the destructor it frees up the resources.
// Having the buffers stored in global c++ variables meant that they got freed after the driver
// causing an access violation on exit, that's why it's an entity.
class SplineGenGeometryRepository
{
public:
  SplineGenGeometryRepository();
  SplineGenGeometryRepository(const SplineGenGeometryRepository &) = delete;
  SplineGenGeometryRepository &operator=(const SplineGenGeometryRepository &) = delete;
  SplineGenGeometryIbPtr getOrMakeIb(uint32_t slices, uint32_t stripes);
  SplineGenGeometryAssetPtr getOrMakeAsset(const eastl::string &asset_name);
  SplineGenGeometryManagerPtr getOrMakeManager(const eastl::string &template_name);
  SplineGenGeometryShapeManagerPtr getOrMakeShapeManager();
  void createTransparentSplineGenNode();
  void reset();
  ska::flat_hash_map<uint32_t, eastl::shared_ptr<SplineGenGeometryIb>> &getIbs();
  ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryAsset>> &getAssets();
  ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryManager>> &getManagers();

private:
  ska::flat_hash_map<uint32_t, eastl::shared_ptr<SplineGenGeometryIb>> ibs;
  ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryAsset>> assets;
  ska::flat_hash_map<eastl::string, eastl::shared_ptr<SplineGenGeometryManager>> managers;
  eastl::shared_ptr<SplineGenGeometryShapeManager> shapeManager = nullptr;
  dafg::NodeHandle transparentSplineGenNode;
};

ECS_DECLARE_BOXED_TYPE(SplineGenGeometryRepository);

SplineGenGeometryRepository &get_spline_gen_repository();
