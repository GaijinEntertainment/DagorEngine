// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_e3dColor.h>

static void build_test_blk(DataBlock &blk)
{
  blk.setStr("name", "test");
  blk.setInt("count", 42);
  blk.setReal("speed", 3.14f);
  blk.setBool("active", true);
  blk.setInt64("big", 1234567890123LL);
  blk.setPoint2("p2", Point2(1.0f, 2.0f));
  blk.setPoint3("p3", Point3(1.0f, 2.0f, 3.0f));
  blk.setPoint4("p4", Point4(1.0f, 2.0f, 3.0f, 4.0f));
  blk.setIPoint2("ip2", IPoint2(10, 20));
  blk.setIPoint3("ip3", IPoint3(10, 20, 30));
  blk.setIPoint4("ip4", IPoint4(1, 2, 3, 4));
  blk.setE3dcolor("color", E3DCOLOR(255, 128, 0, 200));

  TMatrix tm;
  tm.identity();
  tm.setcol(3, Point3(1.0f, 2.0f, 3.0f));
  blk.setTm("tm", tm);

  DataBlock *child = blk.addBlock("child");
  child->setInt("nested", 99);
}

TEST_CASE("DataBlock saveToTextStream produces output", "[datablock][saving]")
{
  DataBlock blk;
  build_test_blk(blk);

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 1024);
  CHECK(blk.saveToTextStream(cwr));
  CHECK(cwr.size() > 0);
}

TEST_CASE("DataBlock saveToTextStreamCompact is shorter", "[datablock][saving]")
{
  DataBlock blk;
  build_test_blk(blk);

  DynamicMemGeneralSaveCB full(tmpmem, 0, 1024);
  blk.saveToTextStream(full);

  DynamicMemGeneralSaveCB compact(tmpmem, 0, 1024);
  blk.saveToTextStreamCompact(compact);

  CHECK(compact.size() > 0);
  CHECK(compact.size() <= full.size());
}

TEST_CASE("DataBlock text round-trip", "[datablock][saving]")
{
  DataBlock src;
  build_test_blk(src);

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 1024);
  src.saveToTextStream(cwr);

  DataBlock dst;
  bool ok = dst.loadText((const char *)cwr.data(), cwr.size(), "roundtrip.blk");
  CHECK(ok);

  CHECK(strcmp(dst.getStr("name", ""), "test") == 0);
  CHECK(dst.getInt("count", 0) == 42);
  CHECK(dst.getBool("active", false) == true);
  CHECK(dst.getInt64("big", 0) == 1234567890123LL);
  CHECK(dst.getBlockByNameEx("child")->getInt("nested", 0) == 99);
}

TEST_CASE("DataBlock binary round-trip", "[datablock][saving]")
{
  DataBlock src;
  build_test_blk(src);

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 1024);
  CHECK(src.saveToStream(cwr));
  CHECK(cwr.size() > 0);

  InPlaceMemLoadCB crd(cwr.data(), cwr.size());
  DataBlock dst;
  bool ok = dst.loadFromStream(crd, "binary.blk");
  CHECK(ok);

  CHECK(strcmp(dst.getStr("name", ""), "test") == 0);
  CHECK(dst.getInt("count", 0) == 42);
  CHECK(dst.getReal("speed", 0.0f) == 3.14f);
  CHECK(dst.getBool("active", false) == true);
  CHECK(dst.getInt64("big", 0) == 1234567890123LL);

  Point2 p2 = dst.getPoint2("p2", Point2(0, 0));
  CHECK(p2.x == 1.0f);
  CHECK(p2.y == 2.0f);

  Point3 p3 = dst.getPoint3("p3", Point3(0, 0, 0));
  CHECK(p3.x == 1.0f);
  CHECK(p3.y == 2.0f);
  CHECK(p3.z == 3.0f);

  Point4 p4 = dst.getPoint4("p4", Point4(0, 0, 0, 0));
  CHECK(p4.x == 1.0f);
  CHECK(p4.y == 2.0f);
  CHECK(p4.z == 3.0f);
  CHECK(p4.w == 4.0f);

  IPoint2 ip2 = dst.getIPoint2("ip2", IPoint2(0, 0));
  CHECK(ip2.x == 10);
  CHECK(ip2.y == 20);

  IPoint3 ip3 = dst.getIPoint3("ip3", IPoint3(0, 0, 0));
  CHECK(ip3.x == 10);
  CHECK(ip3.y == 20);
  CHECK(ip3.z == 30);

  IPoint4 ip4 = dst.getIPoint4("ip4", IPoint4(0, 0, 0, 0));
  CHECK(ip4.x == 1);
  CHECK(ip4.y == 2);
  CHECK(ip4.z == 3);
  CHECK(ip4.w == 4);

  E3DCOLOR c = dst.getE3dcolor("color", E3DCOLOR(0, 0, 0, 0));
  CHECK(c.r == 255);
  CHECK(c.g == 128);
  CHECK(c.b == 0);
  CHECK(c.a == 200);

  TMatrix tm = dst.getTm("tm", TMatrix::IDENT);
  CHECK(tm.getcol(3).x == 1.0f);
  CHECK(tm.getcol(3).y == 2.0f);
  CHECK(tm.getcol(3).z == 3.0f);

  CHECK(dst.blockCount() == 1);
  CHECK(dst.getBlockByNameEx("child")->getInt("nested", 0) == 99);
}

TEST_CASE("DataBlock empty block round-trip", "[datablock][saving]")
{
  DataBlock src;

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 256);
  CHECK(src.saveToStream(cwr));

  InPlaceMemLoadCB crd(cwr.data(), cwr.size());
  DataBlock dst;
  CHECK(dst.loadFromStream(crd, "empty.blk"));
  CHECK(dst.isEmpty());
}
