// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include "test_binary_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>

// Regression tests for DataBlock::loadFromBinDump: a crafted binary dump
// (BBF_full_binary_in_stream, tag \1) must be rejected instead of walking off
// the shared allocation. See prog/engine/ioSys/dataBlock/blk_serialize.cpp.

namespace
{
using dblk_test_binary::Bytes;
using dblk_test_binary::put_leb;
using dblk_test_binary::put_names;
using dblk_test_binary::put_node;
using dblk_test_binary::put_param;

// Loads a raw binary dump and additionally walks the whole tree to force every
// lazily-read byte to be touched. Returns loadFromStream's result.
bool load_and_walk(const Bytes &blob)
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector rep;
  DataBlock::InstallReporterRAII raii(&rep);

  DataBlock blk;
  InPlaceMemLoadCB crd(blob.data(), (int)blob.size());
  bool ok = blk.loadFromStream(crd, "malformed.blk");

  eastl::vector<const DataBlock *> stack;
  stack.push_back(&blk);
  while (!stack.empty())
  {
    const DataBlock *b = stack.back();
    stack.pop_back();
    for (uint32_t i = 0, e = b->paramCount(); i < e; ++i)
    {
      (void)b->getParamType(i);
      (void)b->getParamName(i);
    }
    for (uint32_t i = 0, e = b->blockCount(); i < e; ++i)
      stack.push_back(b->getBlock(i));
  }
  return ok;
}
} // namespace

TEST_CASE("DataBlock binary dump: valid baseline round-trips", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01); // BBF_full_binary_in_stream
  put_names(b, {"a"});
  put_leb(b, 1); // blocks_count
  put_leb(b, 1); // params_count
  put_leb(b, 0); // complex_data size
  put_param(b, 0, DataBlock::TYPE_INT, 7);
  put_node(b, 0, 1, 0); // root: unnamed, 1 param, 0 blocks

  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  DataBlock blk;
  InPlaceMemLoadCB crd(b.data(), (int)b.size());
  REQUIRE(blk.loadFromStream(crd, "ok.blk"));
  CHECK(blk.paramCount() == 1);
  CHECK(blk.getInt(0) == 7);
}

TEST_CASE("DataBlock binary dump: block count beyond node array is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {});
  put_leb(b, 1); // only one node in the dump
  put_leb(b, 0);
  put_leb(b, 0);
  put_node(b, 0, 0, 4096, /*f_block*/ 1); // root claims 4096 children
  CHECK(load_and_walk(b) == false);
}

TEST_CASE("DataBlock binary dump: param count desync is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {});
  put_leb(b, 1);
  put_leb(b, 0); // global params = 0
  put_leb(b, 0);
  put_node(b, 0, 4096, 0); // root claims 4096 params
  CHECK(load_and_walk(b) == false);
}

TEST_CASE("DataBlock binary dump: fBlock out of range is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {});
  put_leb(b, 1);
  put_leb(b, 0);
  put_leb(b, 0);
  put_node(b, 0, 0, 1, /*f_block*/ 0x10000);
  CHECK(load_and_walk(b) == false);
}

TEST_CASE("DataBlock binary dump: param nameId beyond namemap is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {"a"});
  put_leb(b, 1);
  put_leb(b, 1);
  put_leb(b, 0);
  put_param(b, 0x7FFFFF, DataBlock::TYPE_INT, 1); // nameId far past the 1-entry namemap
  put_node(b, 0, 1, 0);
  CHECK(load_and_walk(b) == false);
}

TEST_CASE("DataBlock binary dump: huge param count (size overflow) is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {});
  put_leb(b, 1);
  put_leb(b, 0x20000000); // *sizeof(Param) wraps a uint32; must be bounded
  put_leb(b, 0);
  put_node(b, 0, 0, 0);
  CHECK(load_and_walk(b) == false);
}

TEST_CASE("DataBlock binary dump: invalid param type is rejected", "[datablock][binary][malformed]")
{
  Bytes b;
  b.push_back(0x01);
  put_names(b, {"a"});
  put_leb(b, 1);
  put_leb(b, 1);
  put_leb(b, 0);
  put_param(b, 0, 0x7F, 0); // type 0x7F is not a valid ParamType
  put_node(b, 0, 1, 0);
  CHECK(load_and_walk(b) == false);
}
