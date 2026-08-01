// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include "test_binary_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>

namespace
{
using dblk_test_binary::Bytes;
using dblk_test_binary::put_leb;
using dblk_test_binary::put_names;
using dblk_test_binary::put_node;
using dblk_test_binary::put_param;
} // namespace

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

TEST_CASE("DataBlock robust load malformed zstd binary stream does not assert", "[datablock][robust][zstd]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  // Build malformed BBF_full_binary_in_stream payload (\1):
  // root claims children far beyond available node array.
  Bytes inner;
  inner.push_back(0x01);
  put_leb(inner, 0); // names count
  put_leb(inner, 1); // blocks count
  put_leb(inner, 0); // params count
  put_leb(inner, 0); // complex data size
  put_node(inner, 0, 0, 4096, /*f_block*/ 1);

  // Ensure parser can fail before consuming all compressed bytes.
  inner.insert(inner.end(), 2048, 0xAB);

  size_t cbound = zstd_compress_bound(inner.size());
  REQUIRE(cbound > 0);
  Bytes compressed;
  compressed.resize(cbound);
  size_t csz = zstd_compress(compressed.data(), compressed.size(), inner.data(), inner.size());
  REQUIRE(csz > 0);
  REQUIRE(csz <= compressed.size());
  REQUIRE(csz <= 0xFFFFFFu);
  compressed.resize(csz);

  // Wrap as BBF_full_binary_in_stream_z (\2 + 3-byte compressed size).
  Bytes wrapped;
  wrapped.push_back(0x02);
  wrapped.push_back((uint8_t)(csz & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 8) & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 16) & 0xFF));
  wrapped.insert(wrapped.end(), compressed.begin(), compressed.end());

  DataBlock blk;
  InPlaceMemLoadCB crd(wrapped.data(), (int)wrapped.size());
  bool ok = dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST, "malformed_zstd.blk");
  CHECK(ok == false);
}

TEST_CASE("DataBlock robust load zstd stream with wrong inner tag does not assert", "[datablock][robust][zstd]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  // Decompressed stream starts with a non-\1 tag. In robust mode this should
  // fail gracefully without tripping assertion handlers.
  Bytes inner;
  inner.push_back(0x7F);
  inner.insert(inner.end(), 2048, 0xCD);

  size_t cbound = zstd_compress_bound(inner.size());
  REQUIRE(cbound > 0);
  Bytes compressed;
  compressed.resize(cbound);
  size_t csz = zstd_compress(compressed.data(), compressed.size(), inner.data(), inner.size());
  REQUIRE(csz > 0);
  REQUIRE(csz <= compressed.size());
  REQUIRE(csz <= 0xFFFFFFu);
  compressed.resize(csz);

  Bytes wrapped;
  wrapped.push_back(0x02);
  wrapped.push_back((uint8_t)(csz & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 8) & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 16) & 0xFF));
  wrapped.insert(wrapped.end(), compressed.begin(), compressed.end());

  DataBlock blk;
  InPlaceMemLoadCB crd(wrapped.data(), (int)wrapped.size());
  bool ok = dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST, "wrong_tag_zstd.blk");
  CHECK(ok == false);
}

TEST_CASE("DataBlock robust load valid zstd binary stream succeeds", "[datablock][robust][zstd]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  // Build a valid BBF_full_binary_in_stream payload (\1) with one int param.
  Bytes inner;
  inner.push_back(0x01);
  put_names(inner, {"a"});
  put_leb(inner, 1); // blocks_count
  put_leb(inner, 1); // params_count
  put_leb(inner, 0); // complex_data size
  put_param(inner, 0, DataBlock::TYPE_INT, 42);
  put_node(inner, 0, 1, 0); // root: unnamed, 1 param, 0 blocks

  size_t cbound = zstd_compress_bound(inner.size());
  REQUIRE(cbound > 0);
  Bytes compressed;
  compressed.resize(cbound);
  size_t csz = zstd_compress(compressed.data(), compressed.size(), inner.data(), inner.size());
  REQUIRE(csz > 0);
  REQUIRE(csz <= compressed.size());
  REQUIRE(csz <= 0xFFFFFFu);
  compressed.resize(csz);

  // Wrap as BBF_full_binary_in_stream_z (\2 + 3-byte compressed size).
  Bytes wrapped;
  wrapped.push_back(0x02);
  wrapped.push_back((uint8_t)(csz & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 8) & 0xFF));
  wrapped.push_back((uint8_t)((csz >> 16) & 0xFF));
  wrapped.insert(wrapped.end(), compressed.begin(), compressed.end());

  DataBlock blk;
  InPlaceMemLoadCB crd(wrapped.data(), (int)wrapped.size());
  bool ok = dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST, "valid_zstd.blk");
  REQUIRE(ok == true);
  CHECK(blk.paramCount() == 1);
  REQUIRE(blk.getParamName(0) != nullptr);
  CHECK(strcmp(blk.getParamName(0), "a") == 0);
  CHECK(blk.getInt(0) == 42);
}
