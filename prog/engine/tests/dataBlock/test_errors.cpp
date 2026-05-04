// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock missing param error with fatalOnMissingVar=true is serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingVar = true;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.getInt("nonexistent"); // no default -> triggers issue_error_missing_param

  REQUIRE(!collector.empty());
  CHECK(collector.hadSerious());
}

TEST_CASE("DataBlock missing param error with fatalOnMissingVar=false is not serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingVar = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.getInt("nonexistent");

  REQUIRE(!collector.empty());
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock missing param with default does not trigger error", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingVar = true;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  int val = blk.getInt("nonexistent", 42);

  CHECK(collector.empty());
  CHECK(val == 42);
}

TEST_CASE("DataBlock missing file error with fatalOnMissingFile=true is serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingFile = true;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.load("this_file_definitely_does_not_exist.blk");

  REQUIRE(!collector.empty());
  CHECK(collector.hadSerious());
}

TEST_CASE("DataBlock missing file error with fatalOnMissingFile=false is not serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingFile = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.load("this_file_definitely_does_not_exist.blk");

  REQUIRE(!collector.empty());
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock parse error with fatalOnLoadFailed=true is serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnLoadFailed = true;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  const char *bad_text = "broken {{{ ::: syntax";
  blk.loadText(bad_text, (int)strlen(bad_text), "bad.blk");

  REQUIRE(!collector.empty());
  CHECK(collector.hadSerious());
}

TEST_CASE("DataBlock parse error with fatalOnLoadFailed=false is not serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnLoadFailed = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  const char *bad_text = "broken {{{ ::: syntax";
  blk.loadText(bad_text, (int)strlen(bad_text), "bad.blk");

  REQUIRE(!collector.empty());
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock type mismatch error with fatalOnBadVarType=true is serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnBadVarType = true;
  DataBlock::allowVarTypeChange = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 42);
  blk.addStr("val", "text"); // type mismatch via add triggers issue_error_bad_type

  REQUIRE(!collector.empty());
  CHECK(collector.hadSerious());
}

TEST_CASE("DataBlock type mismatch error with fatalOnBadVarType=false is not serious", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::allowVarTypeChange = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 42);
  blk.addStr("val", "text"); // type mismatch via add triggers issue_error_bad_type

  REQUIRE(!collector.empty());
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock all fatal flags produce serious errors simultaneously", "[datablock][errors]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  // set one flag at a time and verify corresponding error is non-serious
  {
    ErrorCollector collector;
    DataBlock::InstallReporterRAII reporter(&collector);

    DataBlock blk;
    blk.getStr("missing");

    CHECK(!collector.empty());
    CHECK(!collector.hadSerious());
  }

  // now with fatal
  DataBlock::fatalOnMissingVar = true;
  {
    ErrorCollector collector;
    DataBlock::InstallReporterRAII reporter(&collector);

    DataBlock blk;
    blk.getStr("missing");

    CHECK(!collector.empty());
    CHECK(collector.hadSerious());
  }
}
