// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <string.h>

namespace
{
struct LoadResult
{
  DataBlock blk;
  ErrorCollector errors;
  bool ok;
};

// Loads into r.blk with all error flags non-fatal, errors are collected in r.errors
void load_text(LoadResult &r, const char *text)
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock::InstallReporterRAII reporter(&r.errors);
  r.ok = r.blk.loadText(text, (int)strlen(text), "delete_test.blk");
}

int count_params(const DataBlock &blk, const char *name)
{
  int n = 0;
  for (int i = 0; i < blk.paramCount(); ++i)
    if (strcmp(blk.getParamName(i), name) == 0)
      ++n;
  return n;
}

struct SimpleStringGuard
{
  bool saved = DataBlock::allowSimpleString;
  SimpleStringGuard() { DataBlock::allowSimpleString = true; }
  ~SimpleStringGuard() { DataBlock::allowSimpleString = saved; }
};
} // namespace

TEST_CASE("BLK @delete removes the first param with this name", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part\":t=\"\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  REQUIRE(count_params(r.blk, "part") == 1);
  CHECK(strcmp(r.blk.getStr("part", ""), "b") == 0);
}

TEST_CASE("BLK @delete removes the param by 1-based ordinal", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\npart:t=\"c\"\n\"@delete:part[2]\":t=\"\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  REQUIRE(count_params(r.blk, "part") == 2);
  const int nid = r.blk.getNameId("part");
  const int first = r.blk.findParam(nid);
  REQUIRE(first >= 0);
  const int second = r.blk.findParam(nid, first);
  REQUIRE(second >= 0);
  CHECK(strcmp(r.blk.getStr(first), "a") == 0);
  CHECK(strcmp(r.blk.getStr(second), "c") == 0);
}

TEST_CASE("BLK @delete with matching expected string removes the param", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part\":t=\"a\"\n\"@delete:part\":t=\"b\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  CHECK(count_params(r.blk, "part") == 0);
}

TEST_CASE("BLK @delete with matching expected string and ordinal removes the param", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part[2]\":t=\"b\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  REQUIRE(count_params(r.blk, "part") == 1);
  CHECK(strcmp(r.blk.getStr("part", ""), "a") == 0);
}

TEST_CASE("BLK @delete with different expected string fails the load", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\n\"@delete:part\":t=\"other\"\n");

  CHECK(!r.ok);
  CHECK(!r.errors.empty());
}

TEST_CASE("BLK @delete with different expected string and ordinal fails the load", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part[2]\":t=\"a\"\n");

  CHECK(!r.ok);
  CHECK(!r.errors.empty());
}

TEST_CASE("BLK @delete expected string is compared in full and case-sensitive", "[datablock][delete]")
{
  {
    LoadResult r;
    load_text(r, "part:t=\"abc\"\n\"@delete:part\":t=\"ab\"\n");
    CHECK(!r.ok);
  }
  {
    LoadResult r;
    load_text(r, "part:t=\"abc\"\n\"@delete:part\":t=\"abcd\"\n");
    CHECK(!r.ok);
  }
  {
    LoadResult r;
    load_text(r, "part:t=\"abc\"\n\"@delete:part\":t=\"ABC\"\n");
    CHECK(!r.ok);
  }
}

TEST_CASE("BLK @delete with expected string fails when target is not a string", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "count:i=5\n\"@delete:count\":t=\"5\"\n");

  CHECK(!r.ok);
  CHECK(!r.errors.empty());
}

TEST_CASE("BLK @delete with empty expected string is a wildcard for any type", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "count:i=5\nname:t=\"x\"\n\"@delete:count\":t=\"\"\n\"@delete:name\":t=\"\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  CHECK(r.blk.paramCount() == 0);
}

TEST_CASE("BLK @delete with non-string directive value is not compared", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "vsync:b=yes\nlevel:i=3\n\"@delete:vsync\":b=no\n\"@delete:level\":i=100\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  CHECK(r.blk.paramCount() == 0);
}

TEST_CASE("BLK @delete of a missing param reports an error and keeps loading", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "a:i=1\n\"@delete:missing\":t=\"\"\nb:i=2\n");

  CHECK(r.ok);
  CHECK(!r.errors.empty());
  CHECK(r.blk.getInt("a", 0) == 1);
  CHECK(r.blk.getInt("b", 0) == 2);
  CHECK(r.blk.paramCount() == 2);
}

TEST_CASE("BLK @delete of a missing param does not check the expected string", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "a:i=1\n\"@delete:missing\":t=\"whatever\"\nb:i=2\n");

  CHECK(r.ok);
  CHECK(!r.errors.empty());
  CHECK(r.blk.paramCount() == 2);
}

TEST_CASE("BLK @delete with ordinal over the param count reports an error and keeps the params", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part[3]\":t=\"\"\nafter:i=1\n");

  CHECK(r.ok);
  CHECK(!r.errors.empty());
  CHECK(count_params(r.blk, "part") == 2);
  CHECK(r.blk.getInt("after", 0) == 1);
}

TEST_CASE("BLK @delete of a missing param is a serious error only with fatalOnLoadFailed", "[datablock][delete]")
{
  const char *text = "a:i=1\n\"@delete:missing\":t=\"\"\n";
  {
    FatalFlagsGuard guard;
    DataBlock::fatalOnLoadFailed = false;
    ErrorCollector errors;
    DataBlock::InstallReporterRAII reporter(&errors);
    DataBlock blk;
    CHECK(blk.loadText(text, (int)strlen(text), "delete_test.blk"));
    REQUIRE(!errors.empty());
    CHECK(!errors.hadSerious());
  }
  {
    FatalFlagsGuard guard;
    DataBlock::fatalOnLoadFailed = true;
    ErrorCollector errors;
    DataBlock::InstallReporterRAII reporter(&errors);
    DataBlock blk;
    CHECK(blk.loadText(text, (int)strlen(text), "delete_test.blk"));
    REQUIRE(!errors.empty());
    CHECK(errors.hadSerious());
  }
}

TEST_CASE("BLK @delete only removes params of the current block", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "v:t=\"root\"\nchild{\n  v:t=\"inner\"\n  \"@delete:v\":t=\"inner\"\n}\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  CHECK(strcmp(r.blk.getStr("v", ""), "root") == 0);
  const DataBlock *child = r.blk.getBlockByName("child");
  REQUIRE(child != nullptr);
  CHECK(child->paramCount() == 0);
}

TEST_CASE("BLK @delete in a nested block with wrong expected string fails the load", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "child{\n  v:t=\"inner\"\n  \"@delete:v\":t=\"outer\"\n}\n");

  CHECK(!r.ok);
  CHECK(!r.errors.empty());
}

TEST_CASE("BLK @delete after a failed check is not applied", "[datablock][delete]")
{
  LoadResult r;
  load_text(r, "part:t=\"a\"\n\"@delete:part\":t=\"other\"\nlater:i=1\n");

  CHECK(!r.ok);
  CHECK(count_params(r.blk, "part") == 1);
  CHECK(strcmp(r.blk.getStr("part", ""), "a") == 0);
  CHECK(r.blk.getInt("later", 0) == 0);
}

TEST_CASE("BLK @delete simple string with matching expected removes the param", "[datablock][delete][simplestring]")
{
  SimpleStringGuard ss;
  LoadResult r;
  load_text(r, "part:t=\"a\"\npart:t=\"b\"\n\"@delete:part[2]\"=\"b\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  REQUIRE(count_params(r.blk, "part") == 1);
  CHECK(strcmp(r.blk.getStr("part", ""), "a") == 0);
}

TEST_CASE("BLK @delete simple string with empty expected is a wildcard", "[datablock][delete][simplestring]")
{
  SimpleStringGuard ss;
  LoadResult r;
  load_text(r, "part:t=\"a\"\n\"@delete:part\"=\"\"\n");

  CHECK(r.ok);
  CHECK(r.errors.empty());
  CHECK(count_params(r.blk, "part") == 0);
}

TEST_CASE("BLK @delete simple string with incorrect expected fails the load", "[datablock][delete][simplestring]")
{
  SimpleStringGuard ss;
  LoadResult r;
  load_text(r, "part:t=\"a\"\n\"@delete:part\"=\"other\"\n");

  CHECK(!r.ok);
  CHECK(!r.errors.empty());
  CHECK(count_params(r.blk, "part") == 1);
  CHECK(strcmp(r.blk.getStr("part", ""), "a") == 0);
}
