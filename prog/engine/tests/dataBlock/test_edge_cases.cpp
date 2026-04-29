// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>

TEST_CASE("DataBlock emptyBlock is valid and empty", "[datablock][edge_cases]")
{
  CHECK(DataBlock::emptyBlock.isEmpty());
  CHECK(DataBlock::emptyBlock.paramCount() == 0);
  CHECK(DataBlock::emptyBlock.blockCount() == 0);
}

TEST_CASE("DataBlock empty block operations", "[datablock][edge_cases]")
{
  DataBlock blk;
  CHECK(blk.isEmpty());
  CHECK(blk.paramCount() == 0);
  CHECK(blk.blockCount() == 0);

  CHECK(!blk.removeParam("nonexistent"));
  CHECK(!blk.removeBlock("nonexistent"));
  CHECK(blk.findParam("x") == -1);
  CHECK(blk.findBlock("x") == -1);
  CHECK(!blk.paramExists("x"));
  CHECK(!blk.blockExists("x"));
}

TEST_CASE("DataBlock empty string param", "[datablock][edge_cases]")
{
  DataBlock blk;
  blk.addStr("empty", "");
  CHECK(strcmp(blk.getStr("empty", "default"), "") == 0);
}

TEST_CASE("DataBlock long string param", "[datablock][edge_cases]")
{
  DataBlock blk;
  // create a string of 500 characters
  char longstr[501];
  memset(longstr, 'x', 500);
  longstr[500] = 0;

  blk.addStr("long", longstr);
  const char *result = blk.getStr("long", "");
  CHECK(strlen(result) == 500);
  CHECK(strcmp(result, longstr) == 0);
}

TEST_CASE("DataBlock many params", "[datablock][edge_cases]")
{
  DataBlock blk;
  for (int i = 0; i < 100; i++)
  {
    String name(0, "param_%d", i);
    blk.addInt(name.str(), i);
  }
  CHECK(blk.paramCount() == 100);

  for (int i = 0; i < 100; i++)
  {
    String name(0, "param_%d", i);
    CHECK(blk.getInt(name.str(), -1) == i);
  }
}

TEST_CASE("DataBlock many blocks", "[datablock][edge_cases]")
{
  DataBlock blk;
  for (int i = 0; i < 50; i++)
  {
    String name(0, "block_%d", i);
    blk.addNewBlock(name.str())->setInt("idx", i);
  }
  CHECK(blk.blockCount() == 50);

  for (int i = 0; i < 50; i++)
  {
    CHECK(blk.getBlock(i)->getInt("idx", -1) == i);
  }
}

TEST_CASE("DataBlock shrink preserves content", "[datablock][edge_cases]")
{
  DataBlock blk;
  blk.setInt("a", 1);
  blk.setStr("b", "text");
  blk.addBlock("child")->setReal("x", 1.0f);

  blk.compact();

  CHECK(blk.getInt("a", 0) == 1);
  CHECK(strcmp(blk.getStr("b", ""), "text") == 0);
  CHECK(blk.getBlockByNameEx("child")->getReal("x", 0.0f) == 1.0f);
}

TEST_CASE("DataBlock topMost", "[datablock][edge_cases]")
{
  DataBlock blk;
  CHECK(blk.topMost());

  DataBlock *child = blk.addBlock("child");
  CHECK(!child->topMost());
}

TEST_CASE("DataBlock memUsed", "[datablock][edge_cases]")
{
  DataBlock blk;
  size_t emptyMem = blk.memUsed();

  blk.setInt("a", 1);
  blk.setStr("b", "some text value");
  blk.addBlock("child")->setInt("val", 42);

  size_t usedMem = blk.memUsed();
  CHECK(usedMem > emptyMem);
}

TEST_CASE("DataBlock multiple operations sequence", "[datablock][edge_cases]")
{
  DataBlock blk;

  // add, modify, remove in sequence
  blk.addInt("x", 1);
  blk.addInt("y", 2);
  blk.addInt("z", 3);
  blk.setInt("y", 20);
  blk.removeParam("x");

  CHECK(blk.paramCount() == 2);
  CHECK(!blk.paramExists("x"));
  CHECK(blk.getInt("y", 0) == 20);
  CHECK(blk.getInt("z", 0) == 3);

  // blocks too
  blk.addBlock("a")->setInt("v", 1);
  blk.addNewBlock("a")->setInt("v", 2);
  blk.removeBlock((uint32_t)0);

  CHECK(blk.blockCount() == 1);
  CHECK(blk.getBlock(0)->getInt("v", 0) == 2);
}

TEST_CASE("DataBlock text round-trip with special characters in strings", "[datablock][edge_cases]")
{
  DataBlock src;
  src.addStr("path", "C:\\Users\\test\\file.txt");
  src.addStr("quote", "he said \"hello\"");

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 512);
  src.saveToTextStream(cwr);

  DataBlock dst;
  dst.loadText((const char *)cwr.data(), cwr.size(), "special.blk");

  CHECK(strcmp(dst.getStr("path", ""), "C:\\Users\\test\\file.txt") == 0);
  CHECK(strcmp(dst.getStr("quote", ""), "he said \"hello\"") == 0);
}

TEST_CASE("DataBlock deeply nested structure", "[datablock][edge_cases]")
{
  DataBlock blk;
  DataBlock *current = &blk;
  for (int i = 0; i < 20; i++)
  {
    String name(0, "level_%d", i);
    current = current->addBlock(name.str());
    current->setInt("depth", i);
  }

  // verify traversal
  current = &blk;
  for (int i = 0; i < 20; i++)
  {
    String name(0, "level_%d", i);
    current = current->getBlockByName(name.str());
    REQUIRE(current != nullptr);
    CHECK(current->getInt("depth", -1) == i);
  }
}
