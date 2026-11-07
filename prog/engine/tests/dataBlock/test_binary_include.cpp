// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include "test_binary_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_finally.h>

namespace
{
const char *const INC_FNAME = "_datablock_test_binary_include.blk";

void write_file(const void *data, int size)
{
  file_ptr_t fp = df_open(INC_FNAME, DF_WRITE | DF_CREATE);
  REQUIRE(fp);
  CHECK(df_write(fp, data, size) == size);
  df_close(fp);
}

void write_binary_include(int keep_bytes)
{
  DataBlock src;
  src.setInt("included", 42);
  src.addBlock("sub")->setStr("name", "inner");
  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 1024);
  REQUIRE(src.saveToStream(cwr));
  write_file(cwr.data(), keep_bytes < 0 ? cwr.size() : keep_bytes);
}

void write_bbf3_include_with_unknown_param_type()
{
  using namespace dblk_test_binary;
  Bytes body = {0x01, 0x40, 0x01, 0x01, 'a', 0, 0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x00};
  put_u32(body, 0x70000000);
  Bytes bbf3 = {0x00, 'B', 'B', 'F'};
  put_u32(bbf3, 3);
  put_u32(bbf3, (uint32_t)body.size());
  bbf3.insert(bbf3.end(), body.begin(), body.end());
  write_file(bbf3.data(), (int)bbf3.size());
}

bool load_parent(DataBlock &blk, dblk::ReadFlags flg = dblk::ReadFlags())
{
  String text(0, "before:i=1\ninclude \"%s\"\nafter:i=2\n", INC_FNAME);
  return dblk::load_text(blk, dag::ConstSpan<char>(text.str(), text.length()), flg, "parent.blk");
}

bool reported(const ErrorCollector &c, const char *needle)
{
  for (auto &e : c.errors)
    if (strstr(e.text.str(), needle))
      return true;
  return false;
}
} // namespace

TEST_CASE("DataBlock text include of a binary BLK loads its content", "[datablock][includes]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);
  FINALLY([] { dd_erase(INC_FNAME); });
  write_binary_include(-1);

  DataBlock blk;
  REQUIRE(load_parent(blk));
  CHECK(blk.getInt("before", 0) == 1);
  CHECK(blk.getInt("included", 0) == 42);
  REQUIRE(blk.getBlockByName("sub"));
  CHECK(strcmp(blk.getBlockByName("sub")->getStr("name", ""), "inner") == 0);
  CHECK(blk.getInt("after", 0) == 2);
  CHECK(collector.empty());
}

TEST_CASE("DataBlock text load fails when its binary include is truncated", "[datablock][includes]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);
  FINALLY([] { dd_erase(INC_FNAME); });
  write_binary_include(4);

  DataBlock blk;
  dblk::ReadFlags flg;
  SECTION("non-robust load") {}
  SECTION("robust load") { flg = dblk::ReadFlag::ROBUST; }
  SECTION("strict load") { guard.setAllFatal(); }
  SECTION("robust load with fatalOnLoadFailed")
  {
    flg = dblk::ReadFlag::ROBUST;
    DataBlock::fatalOnLoadFailed = true;
  }
  CHECK(!load_parent(blk, flg));
  CHECK(!blk.isValid());
  CHECK(reported(collector, "error loading include file"));
  CHECK(collector.hadSerious() == DataBlock::fatalOnLoadFailed);
}

TEST_CASE("DataBlock text load goes on after a failed binary include with only fatalOnLoadFailed", "[datablock][includes]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock::fatalOnLoadFailed = true;
  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);
  FINALLY([] { dd_erase(INC_FNAME); });
  SECTION("truncated include") { write_binary_include(4); }
  SECTION("BBF3 include with an unknown param type") { write_bbf3_include_with_unknown_param_type(); }

  DataBlock blk;
  CHECK(load_parent(blk));
  CHECK(blk.isValid());
  CHECK(blk.paramCount() == 2);
  CHECK(blk.getInt("after", 0) == 2);
  CHECK(reported(collector, "BLK read error"));
  CHECK(!reported(collector, "incompatible namemap params"));
  CHECK(!reported(collector, "bad strings table"));
  CHECK(!collector.hadSerious());
}

TEST_CASE("DataBlock text load fails when its binary include needs a vromfs", "[datablock][includes]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);
  FINALLY([] { dd_erase(INC_FNAME); });
  const char bbfBinaryWithSharedNm[] = {3, 0, 0, 0};
  write_file(bbfBinaryWithSharedNm, sizeof(bbfBinaryWithSharedNm));

  DataBlock blk;
  CHECK(!load_parent(blk));
  CHECK(!blk.isValid());
  CHECK(reported(collector, "may be read from vromfs only"));
  CHECK(reported(collector, "error loading include file"));
}
