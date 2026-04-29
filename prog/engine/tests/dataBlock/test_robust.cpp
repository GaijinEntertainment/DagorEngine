// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DataBlock robust load missing file does not error", "[datablock][robust]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  dblk::load(blk, "nonexistent_file.blk", dblk::ReadFlag::ROBUST);

  // In robust mode, errors are suppressed (issue_error macro short-circuits
  // when robust_load is true). However, with tls_reporter installed,
  // errors still get reported through the reporter.
  // The key behavior: no fatal, load returns false gracefully.
  CHECK(blk.isEmpty());
}

TEST_CASE("DataBlock robust load invalid text does not error", "[datablock][robust]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  const char *bad = "broken {{{ syntax";
  dblk::load_text(blk, dag::ConstSpan<char>(bad, (int)strlen(bad)), dblk::ReadFlag::ROBUST, "bad.blk");

  // robust mode: errors reported to reporter if installed, but not fatal
  if (!collector.empty())
    CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock robust flag is sticky", "[datablock][robust]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  DataBlock blk;
  dblk::load_text(blk, dag::ConstSpan<char>("val:i=1", 7), dblk::ReadFlag::ROBUST, "test.blk");

  dblk::ReadFlags flags = dblk::get_flags(blk);
  CHECK((flags & dblk::ReadFlag::ROBUST) != dblk::ReadFlags());
}

TEST_CASE("DataBlock robust ops suppresses missing param error", "[datablock][robust]")
{
  FatalFlagsGuard guard;
  DataBlock::fatalOnMissingVar = true; // would be fatal without robust

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  DataBlock blk;
  dblk::load_text(blk, dag::ConstSpan<char>("", 0), dblk::ReadFlag::ROBUST, "empty.blk");
  // ROBUST sets F_ROBUST_LD; we also need F_ROBUST_OPS for param access
  dblk::set_flag(blk, dblk::ReadFlag::ROBUST);

  // access missing param -- in robust ops mode, error is suppressed
  blk.getInt("nonexistent");

  // with reporter installed, the error is still reported through it
  // but the key thing is it would NOT be fatal even with fatalOnMissingVar=true
  // because robust ops short-circuits the issue_error macro
}

TEST_CASE("DataBlock RESTORE_FLAGS restores state after load", "[datablock][robust]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  DataBlock blk;
  dblk::load_text(blk, dag::ConstSpan<char>("val:i=1", 7), dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS, "test.blk");

  // RESTORE_FLAGS should restore the flags after load, so ROBUST should not be sticky
  dblk::ReadFlags flags = dblk::get_flags(blk);
  CHECK((flags & dblk::ReadFlag::ROBUST) == dblk::ReadFlags());
}
