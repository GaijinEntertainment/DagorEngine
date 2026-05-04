// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock addBlock get-or-create", "[datablock][blocks]")
{
  DataBlock blk;
  DataBlock *child = blk.addBlock("child");
  REQUIRE(child != nullptr);
  child->setInt("val", 1);

  DataBlock *same = blk.addBlock("child");
  CHECK(same == child);
  CHECK(same->getInt("val", 0) == 1);
  CHECK(blk.blockCount() == 1);
}

TEST_CASE("DataBlock addNewBlock always creates new", "[datablock][blocks]")
{
  DataBlock blk;
  DataBlock *a = blk.addNewBlock("child");
  DataBlock *b = blk.addNewBlock("child");
  CHECK(a != b);
  CHECK(blk.blockCount() == 2);
}

TEST_CASE("DataBlock getBlockByName returns null for missing", "[datablock][blocks]")
{
  DataBlock blk;
  CHECK(blk.getBlockByName("missing") == nullptr);
}

TEST_CASE("DataBlock getBlockByNameEx returns emptyBlock for missing", "[datablock][blocks]")
{
  DataBlock blk;
  const DataBlock *result = blk.getBlockByNameEx("missing");
  CHECK(result == &DataBlock::emptyBlock);
  CHECK(result->isEmpty());
}

TEST_CASE("DataBlock getBlock by index", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("first")->setInt("i", 1);
  blk.addNewBlock("second")->setInt("i", 2);

  CHECK(blk.blockCount() == 2);
  CHECK(strcmp(blk.getBlock(0)->getBlockName(), "first") == 0);
  CHECK(blk.getBlock(0)->getInt("i", 0) == 1);
  CHECK(strcmp(blk.getBlock(1)->getBlockName(), "second") == 0);
  CHECK(blk.getBlock(1)->getInt("i", 0) == 2);
}

TEST_CASE("DataBlock findBlock", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("a");
  blk.addNewBlock("b");
  blk.addNewBlock("a");

  CHECK(blk.findBlock("a") == 0);
  CHECK(blk.findBlock("a", 0) == 2);
  CHECK(blk.findBlock("a", 2) == -1);
  CHECK(blk.findBlock("b") == 1);
  CHECK(blk.findBlock("missing") == -1);
}

TEST_CASE("DataBlock removeBlock by name", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("a")->setInt("v", 1);
  blk.addNewBlock("b")->setInt("v", 2);
  blk.addNewBlock("a")->setInt("v", 3);

  CHECK(blk.removeBlock("a"));
  CHECK(blk.blockCount() == 1);
  CHECK(strcmp(blk.getBlock(0)->getBlockName(), "b") == 0);

  CHECK(!blk.removeBlock("nonexistent"));
}

TEST_CASE("DataBlock removeBlock by index", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("a");
  blk.addNewBlock("b");
  blk.addNewBlock("c");

  CHECK(blk.removeBlock((uint32_t)1));
  CHECK(blk.blockCount() == 2);
  CHECK(strcmp(blk.getBlock(0)->getBlockName(), "a") == 0);
  CHECK(strcmp(blk.getBlock(1)->getBlockName(), "c") == 0);
}

TEST_CASE("DataBlock blockExists and blockCountByName", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("x");
  blk.addNewBlock("x");
  blk.addNewBlock("y");

  CHECK(blk.blockExists("x"));
  CHECK(blk.blockExists("y"));
  CHECK(!blk.blockExists("z"));
  CHECK(blk.blockCountByName("x") == 2);
  CHECK(blk.blockCountByName("y") == 1);
  CHECK(blk.blockCountByName("z") == 0);
}

TEST_CASE("DataBlock nested blocks", "[datablock][blocks]")
{
  DataBlock blk;
  DataBlock *lvl1 = blk.addBlock("level1");
  DataBlock *lvl2 = lvl1->addBlock("level2");
  DataBlock *lvl3 = lvl2->addBlock("level3");
  lvl3->setInt("deep", 42);

  CHECK(blk.getBlockByNameEx("level1")->getBlockByNameEx("level2")->getBlockByNameEx("level3")->getInt("deep", 0) == 42);
}

TEST_CASE("DataBlock addNewBlock from copy", "[datablock][blocks]")
{
  DataBlock src;
  src.setInt("val", 10);
  src.addBlock("sub")->setStr("key", "text");

  DataBlock blk;
  DataBlock *copy = blk.addNewBlock(&src, "copied");
  REQUIRE(copy != nullptr);
  CHECK(strcmp(copy->getBlockName(), "copied") == 0);
  CHECK(copy->getInt("val", 0) == 10);
  CHECK(copy->blockCount() == 1);
  CHECK(strcmp(copy->getBlockByNameEx("sub")->getStr("key", ""), "text") == 0);
}

TEST_CASE("DataBlock swapBlocks", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addNewBlock("first")->setInt("i", 1);
  blk.addNewBlock("second")->setInt("i", 2);

  CHECK(blk.swapBlocks(0, 1));
  CHECK(strcmp(blk.getBlock(0)->getBlockName(), "second") == 0);
  CHECK(strcmp(blk.getBlock(1)->getBlockName(), "first") == 0);
  CHECK(blk.getBlock(0)->getInt("i", 0) == 2);
  CHECK(blk.getBlock(1)->getInt("i", 0) == 1);
}

TEST_CASE("DataBlock changeBlockName", "[datablock][blocks]")
{
  DataBlock blk;
  DataBlock *child = blk.addBlock("old_name");
  child->changeBlockName("new_name");
  CHECK(strcmp(child->getBlockName(), "new_name") == 0);
  CHECK(blk.getBlockByName("new_name") != nullptr);
  CHECK(blk.getBlockByName("old_name") == nullptr);
}

TEST_CASE("DataBlock setBlock replaces existing", "[datablock][blocks]")
{
  DataBlock blk;
  blk.addBlock("target")->setInt("old", 1);

  DataBlock replacement;
  replacement.setInt("new_val", 99);

  DataBlock *result = blk.setBlock(&replacement, "target");
  REQUIRE(result != nullptr);
  CHECK(blk.blockCountByName("target") == 1);
  CHECK(result->getInt("new_val", 0) == 99);
  CHECK(!result->paramExists("old"));
}
