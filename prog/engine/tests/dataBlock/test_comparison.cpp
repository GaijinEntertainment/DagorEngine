// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock operator== identical blocks", "[datablock][comparison]")
{
  DataBlock a;
  a.setInt("val", 42);
  a.setStr("name", "test");
  a.addBlock("child")->setReal("x", 1.0f);

  DataBlock b;
  b.setFrom(&a);

  CHECK(a == b);
  CHECK(!(a != b));
}

TEST_CASE("DataBlock operator== different params", "[datablock][comparison]")
{
  DataBlock a;
  a.setInt("val", 42);

  DataBlock b;
  b.setInt("val", 99);

  CHECK(a != b);
  CHECK(!(a == b));
}

TEST_CASE("DataBlock operator== different block structure", "[datablock][comparison]")
{
  DataBlock a;
  a.addBlock("child");

  DataBlock b;
  // no blocks

  CHECK(a != b);
}

TEST_CASE("DataBlock operator== empty blocks", "[datablock][comparison]")
{
  DataBlock a;
  DataBlock b;
  CHECK(a == b);
}

TEST_CASE("DataBlock operator== different param names", "[datablock][comparison]")
{
  DataBlock a;
  a.setInt("x", 1);

  DataBlock b;
  b.setInt("y", 1);

  CHECK(a != b);
}

TEST_CASE("DataBlock operator== different param types same name", "[datablock][comparison]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  DataBlock a;
  a.addInt("val", 1);

  DataBlock b;
  b.addStr("val", "1");

  CHECK(a != b);
}

TEST_CASE("DataBlock are_approximately_equal", "[datablock][comparison]")
{
  DataBlock a;
  a.setReal("x", 1.0f);

  DataBlock b;
  b.setReal("x", 1.00001f);

  CHECK(dblk::are_approximately_equal(a, b, 1e-4f));
  CHECK(!dblk::are_approximately_equal(a, b, 1e-6f));
}

TEST_CASE("DataBlock are_approximately_equal with int params", "[datablock][comparison]")
{
  DataBlock a;
  a.setInt("val", 42);

  DataBlock b;
  b.setInt("val", 42);

  CHECK(dblk::are_approximately_equal(a, b));
}
