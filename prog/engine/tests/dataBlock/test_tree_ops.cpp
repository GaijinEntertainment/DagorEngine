// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock setFrom deep copy", "[datablock][tree_ops]")
{
  DataBlock src;
  src.setInt("val", 42);
  src.addNewBlock("child")->setStr("key", "value");
  src.addNewBlock("child")->addBlock("deep")->setReal("x", 1.0f);

  DataBlock dst;
  dst.setInt("old", 1);
  dst.setFrom(&src);

  CHECK(dst.getInt("val", 0) == 42);
  CHECK(!dst.paramExists("old"));
  CHECK(dst.blockCount() == 2);

  // verify independence
  dst.setInt("val", 100);
  CHECK(src.getInt("val", 0) == 42);
}

TEST_CASE("DataBlock setParamsFrom copies params only", "[datablock][tree_ops]")
{
  DataBlock src;
  src.setInt("a", 1);
  src.setStr("b", "text");
  src.addBlock("block");

  DataBlock dst;
  dst.addBlock("existing_block");
  dst.setParamsFrom(&src);

  CHECK(dst.getInt("a", 0) == 1);
  CHECK(strcmp(dst.getStr("b", ""), "text") == 0);
  // blocks from src are NOT copied, but dst keeps its own blocks
  CHECK(dst.blockCount() == 1);
  CHECK(strcmp(dst.getBlock(0)->getBlockName(), "existing_block") == 0);
}

TEST_CASE("DataBlock appendParamsFrom merges params", "[datablock][tree_ops]")
{
  DataBlock src;
  src.setInt("new_val", 42);

  DataBlock dst;
  dst.setStr("existing", "hello");
  dst.appendParamsFrom(&src);

  CHECK(dst.paramCount() == 2);
  CHECK(strcmp(dst.getStr("existing", ""), "hello") == 0);
  CHECK(dst.getInt("new_val", 0) == 42);
}

TEST_CASE("DataBlock clearData removes all", "[datablock][tree_ops]")
{
  DataBlock blk;
  blk.setInt("a", 1);
  blk.setStr("b", "text");
  blk.addBlock("child")->setReal("x", 1.0f);

  blk.clearData();

  CHECK(blk.paramCount() == 0);
  CHECK(blk.blockCount() == 0);
  CHECK(blk.isEmpty());
}

TEST_CASE("DataBlock reset clears everything", "[datablock][tree_ops]")
{
  DataBlock blk;
  blk.setInt("a", 1);
  blk.addBlock("child");

  blk.reset();

  CHECK(blk.isEmpty());
  CHECK(blk.paramCount() == 0);
  CHECK(blk.blockCount() == 0);
}
