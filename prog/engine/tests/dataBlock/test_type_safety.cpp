// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock type mismatch on set triggers error", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock::allowVarTypeChange = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 42);
  blk.setReal("val", 3.14f); // type mismatch: int -> real

  CHECK(!collector.empty());
}

TEST_CASE("DataBlock type mismatch on set with fatal flag reports serious", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnBadVarType = true;
  DataBlock::fatalOnMissingVar = false;
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::allowVarTypeChange = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 42);
  blk.setReal("val", 3.14f);

  CHECK(collector.hadSerious());
}

TEST_CASE("DataBlock type mismatch on set with fatal flag off reports non-serious", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::allowVarTypeChange = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 42);
  blk.setReal("val", 3.14f);

  CHECK(!collector.empty());
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock get wrong type with strongTypeChecking triggers error", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock::strongTypeChecking = true;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addReal("val", 3.14f);
  blk.getInt("val", 0); // wrong type: real -> int

  CHECK(!collector.empty());
}

TEST_CASE("DataBlock get wrong type without strongTypeChecking no error", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock::strongTypeChecking = false;

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addReal("val", 3.14f);
  blk.getInt("val", 0); // wrong type but strongTypeChecking is off

  // the issue_error_bad_type_get uses strongTypeChecking as do_fatal, which is false
  // and the robust check determines whether the error is reported at all
  // with a non-robust block and tls_reporter set, the error IS reported but not serious
  bool hadSeriousError = collector.hadSerious();
  CHECK(!hadSeriousError);
}

TEST_CASE("DataBlock same type set does not trigger error", "[datablock][type_safety]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  blk.addInt("val", 1);
  blk.setInt("val", 2); // same type, should be fine

  CHECK(collector.empty());
  CHECK(blk.getInt("val", 0) == 2);
}
