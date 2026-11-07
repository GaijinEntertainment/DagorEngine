// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameLibs/gamePhys/collision/customRendinstTrace/landMinMax_api.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <dag/dag_vector.h>
#include <heightmap/heightmapHandler.h>
#include <landMesh/lmeshManager.h>
#include <math/dag_bounds2.h>

using Catch::Matchers::WithinAbs;

namespace
{
const int SIDE = 16;
const float CELL = 1.f, HEIGHT_MIN = -48.f, HEIGHT_RANGE = 100.f;
const float BELOW = 20.f, ABOVE = 100.f, FALLBACK_TOP = 8192.f;

float column_height(int x) { return HEIGHT_MIN + 2.f * x; }

void add_heightmap(LandMeshManager &lmesh)
{
  dag::Vector<uint16_t> raw(SIDE * SIDE);
  for (int y = 0; y < SIDE; y++)
    for (int x = 0; x < SIDE; x++)
      raw[y * SIDE + x] = uint16_t((column_height(x) - HEIGHT_MIN) / HEIGHT_RANGE * 65535.f + 0.5f);
  auto *hmap = new HeightmapHandler;
  hmap->initRaw(raw.data(), CELL, SIDE, SIDE, HEIGHT_MIN, HEIGHT_RANGE, Point2(0, 0));
  lmesh.replaceHeightmapHandler(hmap, false);
}
} // namespace

TEST_CASE("a land with a heightmap only gives the heightmap's range under the box")
{
  LandMeshManager lmesh;
  add_heightmap(lmesh);
  REQUIRE(lmesh.getNumCellsX() == 0);
  float lo = 0, hi = 0;
  rendinst::gen::land_min_max(&lmesh, BBox2(Point2(0, 0), Point2(4, 4)), lo, hi);
  // The query takes the box's cells and one more on each axis
  CHECK_THAT(lo, WithinAbs(column_height(0) - BELOW, 0.01f));
  CHECK_THAT(hi, WithinAbs(column_height(5) + ABOVE, 0.01f));
}

TEST_CASE("a box with a corner off the heightmap gives the range of its part on it")
{
  LandMeshManager lmesh;
  add_heightmap(lmesh);
  float lo = 0, hi = 0;
  rendinst::gen::land_min_max(&lmesh, BBox2(Point2(-2, -2), Point2(4, 4)), lo, hi);
  CHECK_THAT(lo, WithinAbs(column_height(0) - BELOW, 0.01f));
  CHECK_THAT(hi, WithinAbs(column_height(5) + ABOVE, 0.01f));
}

TEST_CASE("a box off the heightmap gives the fallback range")
{
  LandMeshManager lmesh;
  add_heightmap(lmesh);
  float lo = -1, hi = -1;
  rendinst::gen::land_min_max(&lmesh, BBox2(Point2(1000, 1000), Point2(1004, 1004)), lo, hi);
  CHECK(lo == 0.f);
  CHECK(hi == FALLBACK_TOP);
}

TEST_CASE("no land gives the fallback range")
{
  float lo = -1, hi = -1;
  rendinst::gen::land_min_max(nullptr, BBox2(Point2(0, 0), Point2(4, 4)), lo, hi);
  CHECK(lo == 0.f);
  CHECK(hi == FALLBACK_TOP);
}

TEST_CASE("a non-square box takes the chunk of its longer side")
{
  LandMeshManager lmesh;
  add_heightmap(lmesh);
  float lo = 0, hi = 0;
  rendinst::gen::land_min_max(&lmesh, BBox2(Point2(0, 0), Point2(2, 8)), lo, hi);
  CHECK_THAT(lo, WithinAbs(column_height(0) - BELOW, 0.01f));
  // block-compressed heights quantize this top
  CHECK_THAT(hi, WithinAbs(column_height(9) + ABOVE, 0.05f));
}

TEST_CASE("a manager with no land mesh cells and no heightmap gives the fallback range")
{
  LandMeshManager lmesh;
  REQUIRE(lmesh.getNumCellsX() == 0);
  REQUIRE(lmesh.getHmapHandler() == nullptr);
  float lo = -1, hi = -1;
  rendinst::gen::land_min_max(&lmesh, BBox2(Point2(0, 0), Point2(4, 4)), lo, hi);
  CHECK(lo == 0.f);
  CHECK(hi == FALLBACK_TOP);
}

#include <unittest/mainCatch2.inc.cpp>
