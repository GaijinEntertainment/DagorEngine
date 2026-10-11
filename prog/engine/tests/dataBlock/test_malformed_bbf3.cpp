// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include "test_binary_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <string.h>

// Regression tests for DataBlock::doLoadFromStreamBBF3: a crafted legacy BBF3
// stream must be rejected instead of growing its name id table without bound.
// See prog/engine/ioSys/dataBlock/blk_readBBF3.cpp.

using dblk_test_binary::Bytes;
using dblk_test_binary::put_u32;

TEST_CASE("DataBlock BBF3: name ids past 24 bits are rejected", "[datablock][bbf3][malformed]")
{
  const uint32_t names = 400000;
  Bytes b = {0x00, 'B', 'B', 'F'};
  put_u32(b, 0x00100003);        // version 3; the high half (16) is unused
  put_u32(b, 2 + 4 + names);     // size of the data after this field
  b.push_back(0x00);             // namemap header, little endian 0xE000:
  b.push_back(0xE0);             //   count packing 3 (int32), 8192 buckets
  put_u32(b, names);             // name count
  b.resize(b.size() + names, 0); // empty names, all in one bucket

  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector rep;
  DataBlock::InstallReporterRAII raii(&rep);
  DataBlock blk;
  InPlaceMemLoadCB crd(b.data(), (int)b.size());
  CHECK_FALSE(blk.loadFromStream(crd, "bbf3_names.blk"));
  REQUIRE(rep.count() > 0);
  CHECK(strstr(rep.errors[0].text.str(), "namemap"));
}
