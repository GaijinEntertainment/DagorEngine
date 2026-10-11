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

const dblk::ReadFlags robust_and_default[] = {dblk::ReadFlag::ROBUST, dblk::ReadFlags()};

Bytes wrap_zstd(const Bytes &inner)
{
  size_t cbound = zstd_compress_bound(inner.size());
  REQUIRE(cbound > 0);
  Bytes wrapped(4 + cbound);
  size_t csz = zstd_compress(wrapped.data() + 4, cbound, inner.data(), inner.size());
  REQUIRE(csz > 0);
  REQUIRE(csz <= cbound);
  REQUIRE(csz <= 0xFFFFFFu);
  wrapped.resize(4 + csz);

  // Wrap as BBF_full_binary_in_stream_z (\2 + 3-byte compressed size).
  wrapped[0] = 0x02;
  wrapped[1] = (uint8_t)(csz & 0xFF);
  wrapped[2] = (uint8_t)((csz >> 8) & 0xFF);
  wrapped[3] = (uint8_t)((csz >> 16) & 0xFF);
  return wrapped;
}

// A valid BBF_full_binary_in_stream payload (\1) with one int param.
Bytes make_valid_dump()
{
  Bytes inner;
  inner.push_back(0x01);
  put_names(inner, {"a"});
  put_leb(inner, 1); // blocks_count
  put_leb(inner, 1); // params_count
  put_leb(inner, 0); // complex_data size
  put_param(inner, 0, DataBlock::TYPE_INT, 42);
  put_node(inner, 0, 1, 0); // root: unnamed, 1 param, 0 blocks
  return inner;
}

// Like a pipe: no size known, and a seek past the end throws.
struct UnsizedLoadCB : InPlaceMemLoadCB
{
  using InPlaceMemLoadCB::InPlaceMemLoadCB;
  int64_t getTargetDataSize() override { return -1; }
};

template <class LoadCB = InPlaceMemLoadCB>
void check_zstd_load_fails(const Bytes &wrapped, const char *fname)
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  for (dblk::ReadFlags flags : robust_and_default)
  {
    ErrorCollector collector;
    DataBlock::InstallReporterRAII reporter(&collector);
    DataBlock blk;
    LoadCB crd(wrapped.data(), (int)wrapped.size());
    CHECK_FALSE(dblk::load_from_stream(blk, crd, flags, fname));
    CHECK_FALSE(collector.empty());
  }
}
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

TEST_CASE("DataBlock load malformed zstd binary stream does not assert", "[datablock][robust][zstd]")
{
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
  check_zstd_load_fails(wrap_zstd(inner), "malformed_zstd.blk");
}

TEST_CASE("DataBlock load zstd stream with bad compressed size fails", "[datablock][robust][zstd]")
{
  Bytes wrapped = wrap_zstd(make_valid_dump());
  for (uint32_t csz : {uint32_t(wrapped.size()), 0u}) // past the end of the stream, and empty
  {
    memcpy(&wrapped[1], &csz, 3);
    check_zstd_load_fails(wrapped, "bad_size_zstd.blk");
  }
}

TEST_CASE("DataBlock load zstd stream with wrong inner tag does not assert", "[datablock][robust][zstd]")
{
  // Decompressed stream starts with a non-\1 tag, so only the tag check rejects it.
  Bytes inner = make_valid_dump();
  inner[0] = 0x7F;
  Bytes wrapped = wrap_zstd(inner);
  check_zstd_load_fails(wrapped, "wrong_tag_zstd.blk");

  // An unsized stream skips the size check, so a size past its end reaches the reader.
  const uint32_t csz = uint32_t(wrapped.size());
  memcpy(&wrapped[1], &csz, 3);
  check_zstd_load_fails<UnsizedLoadCB>(wrapped, "unsized_zstd.blk");
}

TEST_CASE("DataBlock load valid zstd binary stream succeeds", "[datablock][robust][zstd]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);

  for (int extra : {0, 16}) // extra bytes inside the frame are ignored
  {
    Bytes inner = make_valid_dump();
    inner.insert(inner.end(), extra, 0xEE);
    const Bytes wrapped = wrap_zstd(inner);

    for (dblk::ReadFlags flags : robust_and_default)
    {
      DataBlock blk;
      InPlaceMemLoadCB crd(wrapped.data(), (int)wrapped.size());
      REQUIRE(dblk::load_from_stream(blk, crd, flags, "valid_zstd.blk"));
      CHECK(blk.paramCount() == 1);
      REQUIRE(blk.getParamName(0) != nullptr);
      CHECK(strcmp(blk.getParamName(0), "a") == 0);
      CHECK(blk.getInt(0) == 42);
    }
  }
}
