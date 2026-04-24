// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_e3dColor.h>

TEST_CASE("DataBlock string params", "[datablock][params]")
{
  DataBlock blk;
  blk.addStr("name", "hello");
  CHECK(blk.paramCount() == 1);
  CHECK(strcmp(blk.getStr("name", ""), "hello") == 0);

  blk.setStr("name", "world");
  CHECK(blk.paramCount() == 1);
  CHECK(strcmp(blk.getStr("name", ""), "world") == 0);

  CHECK(strcmp(blk.getStr("missing", "default"), "default") == 0);

  CHECK(blk.paramExists("name"));
  CHECK(!blk.paramExists("missing"));
  CHECK(blk.findParam("name") == 0);
  CHECK(blk.findParam("missing") == -1);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_STRING);
  CHECK(strcmp(blk.getParamName(0), "name") == 0);
}

TEST_CASE("DataBlock int params", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("val", 42);
  CHECK(blk.getInt("val", 0) == 42);

  blk.setInt("val", -100);
  CHECK(blk.getInt("val", 0) == -100);
  CHECK(blk.getInt("missing", 7) == 7);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_INT);
}

TEST_CASE("DataBlock real params", "[datablock][params]")
{
  DataBlock blk;
  blk.addReal("speed", 3.14f);
  CHECK(blk.getReal("speed", 0.0f) == 3.14f);

  blk.setReal("speed", 2.71f);
  CHECK(blk.getReal("speed", 0.0f) == 2.71f);
  CHECK(blk.getReal("missing", 1.0f) == 1.0f);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_REAL);
}

TEST_CASE("DataBlock bool params", "[datablock][params]")
{
  DataBlock blk;
  blk.addBool("flag", true);
  CHECK(blk.getBool("flag", false) == true);

  blk.setBool("flag", false);
  CHECK(blk.getBool("flag", true) == false);
  CHECK(blk.getBool("missing", true) == true);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_BOOL);
}

TEST_CASE("DataBlock int64 params", "[datablock][params]")
{
  DataBlock blk;
  int64_t big = 0x123456789ABCDEF0LL;
  blk.addInt64("big", big);
  CHECK(blk.getInt64("big", 0) == big);

  blk.setInt64("big", -1LL);
  CHECK(blk.getInt64("big", 0) == -1LL);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_INT64);
}

TEST_CASE("DataBlock Point2 params", "[datablock][params]")
{
  DataBlock blk;
  Point2 p(1.0f, 2.0f);
  blk.addPoint2("pos", p);
  Point2 r = blk.getPoint2("pos", Point2(0, 0));
  CHECK(r.x == 1.0f);
  CHECK(r.y == 2.0f);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_POINT2);
}

TEST_CASE("DataBlock Point3 params", "[datablock][params]")
{
  DataBlock blk;
  Point3 p(1.0f, 2.0f, 3.0f);
  blk.addPoint3("pos", p);
  Point3 r = blk.getPoint3("pos", Point3(0, 0, 0));
  CHECK(r.x == 1.0f);
  CHECK(r.y == 2.0f);
  CHECK(r.z == 3.0f);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_POINT3);
}

TEST_CASE("DataBlock Point4 params", "[datablock][params]")
{
  DataBlock blk;
  Point4 p(1.0f, 2.0f, 3.0f, 4.0f);
  blk.addPoint4("pos", p);
  Point4 r = blk.getPoint4("pos", Point4(0, 0, 0, 0));
  CHECK(r.x == 1.0f);
  CHECK(r.y == 2.0f);
  CHECK(r.z == 3.0f);
  CHECK(r.w == 4.0f);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_POINT4);
}

TEST_CASE("DataBlock IPoint2 params", "[datablock][params]")
{
  DataBlock blk;
  IPoint2 p(10, 20);
  blk.addIPoint2("ip", p);
  IPoint2 r = blk.getIPoint2("ip", IPoint2(0, 0));
  CHECK(r.x == 10);
  CHECK(r.y == 20);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_IPOINT2);
}

TEST_CASE("DataBlock IPoint3 params", "[datablock][params]")
{
  DataBlock blk;
  IPoint3 p(10, 20, 30);
  blk.addIPoint3("ip", p);
  IPoint3 r = blk.getIPoint3("ip", IPoint3(0, 0, 0));
  CHECK(r.x == 10);
  CHECK(r.y == 20);
  CHECK(r.z == 30);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_IPOINT3);
}

TEST_CASE("DataBlock IPoint4 params", "[datablock][params]")
{
  DataBlock blk;
  IPoint4 p(1, 2, 3, 4);
  blk.addIPoint4("ip", p);
  IPoint4 r = blk.getIPoint4("ip", IPoint4(0, 0, 0, 0));
  CHECK(r.x == 1);
  CHECK(r.y == 2);
  CHECK(r.z == 3);
  CHECK(r.w == 4);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_IPOINT4);
}

TEST_CASE("DataBlock E3DCOLOR params", "[datablock][params]")
{
  DataBlock blk;
  E3DCOLOR c(255, 128, 0, 200);
  blk.addE3dcolor("color", c);
  E3DCOLOR r = blk.getE3dcolor("color", E3DCOLOR(0, 0, 0, 0));
  CHECK(r.r == 255);
  CHECK(r.g == 128);
  CHECK(r.b == 0);
  CHECK(r.a == 200);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_E3DCOLOR);
}

TEST_CASE("DataBlock TMatrix params", "[datablock][params]")
{
  DataBlock blk;
  TMatrix tm;
  tm.identity();
  tm.setcol(3, Point3(1.0f, 2.0f, 3.0f));
  blk.addTm("tm", tm);

  TMatrix def;
  def.identity();
  TMatrix r = blk.getTm("tm", def);
  CHECK(r.getcol(3).x == 1.0f);
  CHECK(r.getcol(3).y == 2.0f);
  CHECK(r.getcol(3).z == 3.0f);
  CHECK(blk.getParamType(0) == DataBlock::TYPE_MATRIX);
}

TEST_CASE("DataBlock param remove by name", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("a", 1);
  blk.addInt("b", 2);
  blk.addInt("c", 3);
  CHECK(blk.paramCount() == 3);

  CHECK(blk.removeParam("b"));
  CHECK(blk.paramCount() == 2);
  CHECK(!blk.paramExists("b"));
  CHECK(blk.getInt("a", 0) == 1);
  CHECK(blk.getInt("c", 0) == 3);

  CHECK(!blk.removeParam("nonexistent"));
}

TEST_CASE("DataBlock param remove by index", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("a", 1);
  blk.addInt("b", 2);
  CHECK(blk.removeParam((uint32_t)0));
  CHECK(blk.paramCount() == 1);
  CHECK(blk.getInt("b", 0) == 2);
}

TEST_CASE("DataBlock param count by name", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("x", 1);
  blk.addInt("x", 2);
  blk.addInt("x", 3);
  blk.addInt("y", 4);
  CHECK(blk.paramCountByName("x") == 3);
  CHECK(blk.paramCountByName("y") == 1);
  CHECK(blk.paramCountByName("z") == 0);
}

TEST_CASE("DataBlock param get by index", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("a", 10);
  blk.addStr("b", "text");
  blk.addReal("c", 1.5f);

  CHECK(blk.getInt(0) == 10);
  CHECK(strcmp(blk.getStr(1), "text") == 0);
  CHECK(blk.getReal(2) == 1.5f);
}

TEST_CASE("DataBlock param set by index", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("val", 1);
  CHECK(blk.setInt(0, 99));
  CHECK(blk.getInt(0) == 99);
}

TEST_CASE("DataBlock get param by nameId", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("score", 100);
  int nid = blk.getNameId("score");
  CHECK(nid >= 0);
  CHECK(blk.getIntByNameId(nid, 0) == 100);
}

TEST_CASE("DataBlock duplicate param names with add", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("x", 1);
  blk.addInt("x", 2);
  blk.addInt("x", 3);
  CHECK(blk.paramCount() == 3);
  CHECK(blk.getInt(0) == 1);
  CHECK(blk.getInt(1) == 2);
  CHECK(blk.getInt(2) == 3);

  // findParam with start_after
  int first = blk.findParam("x");
  CHECK(first == 0);
  int second = blk.findParam("x", first);
  CHECK(second == 1);
  int third = blk.findParam("x", second);
  CHECK(third == 2);
  int none = blk.findParam("x", third);
  CHECK(none == -1);
}

TEST_CASE("DataBlock swapParams", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("a", 1);
  blk.addInt("b", 2);
  CHECK(blk.swapParams(0, 1));
  CHECK(blk.getInt(0) == 2);
  CHECK(blk.getInt(1) == 1);
  CHECK(strcmp(blk.getParamName(0), "b") == 0);
  CHECK(strcmp(blk.getParamName(1), "a") == 0);
}

TEST_CASE("DataBlock changeParamName", "[datablock][params]")
{
  DataBlock blk;
  blk.addInt("old_name", 42);
  blk.changeParamName(0, "new_name");
  CHECK(strcmp(blk.getParamName(0), "new_name") == 0);
  CHECK(blk.getInt("new_name", 0) == 42);
  CHECK(!blk.paramExists("old_name"));
}
