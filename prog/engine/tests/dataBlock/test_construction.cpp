// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock default construction", "[datablock][construction]")
{
  DataBlock blk;
  CHECK(blk.isEmpty());
  CHECK(blk.paramCount() == 0);
  CHECK(blk.blockCount() == 0);
  CHECK(blk.topMost());
}

TEST_CASE("DataBlock copy constructor", "[datablock][construction]")
{
  DataBlock src;
  src.setInt("val", 42);
  src.setStr("name", "test");
  DataBlock *sub = src.addBlock("child");
  sub->setReal("x", 1.5f);

  DataBlock copy(src);
  CHECK(copy.getInt("val", 0) == 42);
  CHECK(strcmp(copy.getStr("name", ""), "test") == 0);
  CHECK(copy.blockCount() == 1);
  CHECK(copy.getBlockByNameEx("child")->getReal("x", 0.0f) == 1.5f);

  // independence: modifying copy does not affect source
  copy.setInt("val", 100);
  CHECK(src.getInt("val", 0) == 42);
}

TEST_CASE("DataBlock move constructor", "[datablock][construction]")
{
  DataBlock src;
  src.setInt("val", 42);
  src.addBlock("child")->setStr("key", "value");

  DataBlock moved(eastl::move(src));
  CHECK(moved.getInt("val", 0) == 42);
  CHECK(moved.blockCount() == 1);
  CHECK(strcmp(moved.getBlockByNameEx("child")->getStr("key", ""), "value") == 0);
}

TEST_CASE("DataBlock copy assignment", "[datablock][construction]")
{
  DataBlock src;
  src.setInt("a", 1);
  src.setReal("b", 2.0f);

  DataBlock dst;
  dst.setStr("old", "data");
  dst = src;

  CHECK(dst.getInt("a", 0) == 1);
  CHECK(dst.getReal("b", 0.0f) == 2.0f);
  CHECK(!dst.paramExists("old"));

  // self-assignment safety
  DataBlock &ref = dst;
  dst = ref;
  CHECK(dst.getInt("a", 0) == 1);
}

TEST_CASE("DataBlock move assignment", "[datablock][construction]")
{
  DataBlock src;
  src.setInt("val", 99);

  DataBlock dst;
  dst = eastl::move(src);
  CHECK(dst.getInt("val", 0) == 99);
}
