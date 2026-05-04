// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock iterate_child_blocks", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addNewBlock("a")->setInt("i", 1);
  blk.addNewBlock("b")->setInt("i", 2);
  blk.addNewBlock("c")->setInt("i", 3);

  int count = 0;
  int sum = 0;
  dblk::iterate_child_blocks(blk, [&](const DataBlock &child) {
    count++;
    sum += child.getInt("i", 0);
  });
  CHECK(count == 3);
  CHECK(sum == 6);
}

TEST_CASE("DataBlock iterate_child_blocks empty", "[datablock][iteration]")
{
  DataBlock blk;
  int count = 0;
  dblk::iterate_child_blocks(blk, [&](const DataBlock &) { count++; });
  CHECK(count == 0);
}

TEST_CASE("DataBlock iterate_blocks recursive", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addBlock("a")->addBlock("b")->addBlock("c");

  int count = 0;
  dblk::iterate_blocks(blk, [&](const DataBlock &) { count++; });
  // iterate_blocks visits root + a + b + c = 4
  CHECK(count == 4);
}

TEST_CASE("DataBlock iterate_blocks_lev", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addBlock("a")->addBlock("b");

  eastl::vector<int> levels;
  dblk::iterate_blocks_lev(blk, [&](const DataBlock &, int lev) { levels.push_back(lev); });
  // root=0, a=1, b=2
  REQUIRE(levels.size() == 3);
  CHECK(levels[0] == 0);
  CHECK(levels[1] == 1);
  CHECK(levels[2] == 2);
}

TEST_CASE("DataBlock iterate_params", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addInt("a", 1);
  blk.addStr("b", "text");
  blk.addReal("c", 3.14f);

  int count = 0;
  dblk::iterate_params(blk, [&](int idx, int nameId, int type) {
    (void)nameId;
    (void)type;
    (void)idx;
    count++;
  });
  CHECK(count == 3);
}

TEST_CASE("DataBlock iterate_child_blocks_by_name", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addNewBlock("target")->setInt("i", 1);
  blk.addNewBlock("other");
  blk.addNewBlock("target")->setInt("i", 2);

  int count = 0;
  int sum = 0;
  dblk::iterate_child_blocks_by_name(blk, "target", [&](const DataBlock &child) {
    count++;
    sum += child.getInt("i", 0);
  });
  CHECK(count == 2);
  CHECK(sum == 3);
}

TEST_CASE("DataBlock iterate_params_by_type", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addInt("a", 1);
  blk.addStr("b", "text");
  blk.addInt("c", 3);

  int count = 0;
  dblk::iterate_params_by_type(blk, DataBlock::TYPE_INT, [&](int idx, int nameId, int type) {
    (void)idx;
    (void)nameId;
    (void)type;
    count++;
  });
  CHECK(count == 2);
}

TEST_CASE("DataBlock iterate_params_by_name", "[datablock][iteration]")
{
  DataBlock blk;
  blk.addInt("x", 1);
  blk.addInt("y", 2);
  blk.addInt("x", 3);

  int count = 0;
  int sum = 0;
  dblk::iterate_params_by_name(blk, "x", [&](int idx, int nameId, int type) {
    (void)nameId;
    (void)type;
    sum += blk.getInt(idx);
    count++;
  });
  CHECK(count == 2);
  CHECK(sum == 4);
}

TEST_CASE("DataBlock iterate_params_by_name_and_type", "[datablock][iteration]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  DataBlock blk;
  blk.addInt("val", 1);
  blk.addStr("val", "text");
  blk.addInt("val", 3);

  int count = 0;
  dblk::iterate_params_by_name_and_type(blk, "val", DataBlock::TYPE_INT, [&](int idx) {
    (void)idx;
    count++;
  });
  CHECK(count == 2);
}
