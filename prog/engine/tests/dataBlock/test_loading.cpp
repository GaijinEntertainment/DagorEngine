// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>

static const char *VALID_BLK_TEXT = "name:t=\"hello\"\n"
                                    "count:i=42\n"
                                    "speed:r=3.14\n"
                                    "active:b=yes\n"
                                    "pos:p3=1.0, 2.0, 3.0\n"
                                    "color:c=255, 128, 0, 200\n"
                                    "big:i64=1234567890123\n"
                                    "\n"
                                    "child{\n"
                                    "  nested_val:i=99\n"
                                    "  inner{\n"
                                    "    deep:t=\"deep_value\"\n"
                                    "  }\n"
                                    "}\n";

TEST_CASE("DataBlock loadText valid BLK", "[datablock][loading]")
{
  DataBlock blk;
  bool ok = blk.loadText(VALID_BLK_TEXT, (int)strlen(VALID_BLK_TEXT), "test.blk");
  CHECK(ok);

  CHECK(strcmp(blk.getStr("name", ""), "hello") == 0);
  CHECK(blk.getInt("count", 0) == 42);
  CHECK(blk.getReal("speed", 0.0f) == 3.14f);
  CHECK(blk.getBool("active", false) == true);

  Point3 pos = blk.getPoint3("pos", Point3(0, 0, 0));
  CHECK(pos.x == 1.0f);
  CHECK(pos.y == 2.0f);
  CHECK(pos.z == 3.0f);

  E3DCOLOR c = blk.getE3dcolor("color", E3DCOLOR(0, 0, 0, 0));
  CHECK(c.r == 255);
  CHECK(c.g == 128);
  CHECK(c.b == 0);
  CHECK(c.a == 200);

  CHECK(blk.getInt64("big", 0) == 1234567890123LL);

  const DataBlock *child = blk.getBlockByNameEx("child");
  CHECK(child != &DataBlock::emptyBlock);
  CHECK(child->getInt("nested_val", 0) == 99);

  const DataBlock *inner = child->getBlockByNameEx("inner");
  CHECK(inner != &DataBlock::emptyBlock);
  CHECK(strcmp(inner->getStr("deep", ""), "deep_value") == 0);
}

TEST_CASE("DataBlock loadText invalid BLK non-fatal", "[datablock][loading]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  const char *bad_text = "invalid { {{ broken ::: syntax";
  bool ok = blk.loadText(bad_text, (int)strlen(bad_text), "bad.blk");
  CHECK(!ok);
  CHECK(!collector.empty());
}

TEST_CASE("DataBlock loadText empty text", "[datablock][loading]")
{
  DataBlock blk;
  bool ok = blk.loadText("", 0, "empty.blk");
  CHECK(ok);
  CHECK(blk.isEmpty());
}

TEST_CASE("DataBlock load missing file non-fatal", "[datablock][loading]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  bool ok = blk.load("this_file_does_not_exist_at_all.blk");
  CHECK(!ok);
  CHECK(!collector.empty());
}

TEST_CASE("DataBlock loadFromStream", "[datablock][loading]")
{
  const char *text = "val:i=123\n";
  int len = (int)strlen(text);

  InPlaceMemLoadCB crd(text, len);
  DataBlock blk;
  bool ok = blk.loadFromStream(crd, "mem.blk");
  CHECK(ok);
  CHECK(blk.getInt("val", 0) == 123);
}

TEST_CASE("DataBlock loadText with multiple same-name params", "[datablock][loading]")
{
  const char *text = "item:t=\"first\"\n"
                     "item:t=\"second\"\n"
                     "item:t=\"third\"\n";

  DataBlock blk;
  bool ok = blk.loadText(text, (int)strlen(text), "multi.blk");
  CHECK(ok);
  CHECK(blk.paramCount() == 3);
  CHECK(blk.paramCountByName("item") == 3);
}

TEST_CASE("DataBlock loadText with multiple same-name blocks", "[datablock][loading]")
{
  const char *text = "section{\n"
                     "  a:i=1\n"
                     "}\n"
                     "section{\n"
                     "  a:i=2\n"
                     "}\n";

  DataBlock blk;
  bool ok = blk.loadText(text, (int)strlen(text), "multi_blocks.blk");
  CHECK(ok);
  CHECK(blk.blockCount() == 2);
  CHECK(blk.blockCountByName("section") == 2);
}
