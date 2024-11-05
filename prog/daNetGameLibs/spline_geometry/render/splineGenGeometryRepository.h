// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "splineGenGeometryIb.h"
#include "splineGenGeometryAsset.h"
#include "splineGenGeometryMananger.h"
#include <daECS/core/entityComponent.h>
#include <ska_hash_map/flat_hash_map2.hpp>

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
  SplineGenGeometryIb &getOrMakeIb(uint32_t slices, uint32_t stripes);
  SplineGenGeometryAsset &getOrMakeAsset(const eastl::string &asset_name);
  SplineGenGeometryManager &getOrMakeManager(const eastl::string &template_name);
  void reset();
  ska::flat_hash_map<uint32_t, eastl::unique_ptr<SplineGenGeometryIb>> &getIbs();
  ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryAsset>> &getAssets();
  ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryManager>> &getManagers();

private:
  ska::flat_hash_map<uint32_t, eastl::unique_ptr<SplineGenGeometryIb>> ibs;
  ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryAsset>> assets;
  ska::flat_hash_map<eastl::string, eastl::unique_ptr<SplineGenGeometryManager>> managers;
};

ECS_DECLARE_BOXED_TYPE(SplineGenGeometryRepository);

SplineGenGeometryRepository &get_spline_gen_repository();
