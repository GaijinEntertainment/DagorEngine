// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>

// Tests for the deduplicated binary dump format (BBF \6, saveToStreamDedup):
// blocks with identical param runs share one range of the params array,
// so mutation of one block must never leak into another (copy-on-write).

static bool roundtrip(const DataBlock &src, DataBlock &out, bool dedup, int *dump_size = nullptr)
{
  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4096);
  if (!(dedup ? src.saveToStreamDedup(cwr) : src.saveToStream(cwr)))
    return false;
  if (dump_size)
    *dump_size = (int)cwr.size();
  InPlaceMemLoadCB crd(cwr.data(), cwr.size());
  return out.loadFromStream(crd, "dedup.blk");
}

static bool equal(const DataBlock &a, const DataBlock &b) { return dblk::are_approximately_equal(a, b, 0.f); }

// mimics one riDesc resource entry: own params + tex/mat/texScale_data sub-blocks
static void build_res_entry(DataBlock *b, int lods, const char *cls, const char *par)
{
  b->setInt("lods", lods);
  b->setInt("faces", 100 + lods);
  b->setPoint3("bbox0", Point3(-1, -2, -3) * float(lods));
  b->setPoint3("bbox1", Point3(1, 2, 3) * float(lods));
  b->setPoint4("bsph", Point4(0, 0, 0, 5));
  DataBlock *tex = b->addBlock("tex");
  tex->addStr("tex", "tex_diffuse");
  tex->addStr("tex", "tex_normal");
  for (int i = 0; i < 3; i++)
  {
    DataBlock *mat = b->addBlock("mat");
    mat->setStr("cls", cls);
    mat->setStr("par", par);
    mat->setInt("t0", 0);
    mat->setInt("t2", 1);
  }
  b->addBlock("texScale_data")->setReal("texScale0", 0.5f);
}

static void build_desc(DataBlock &src)
{
  build_res_entry(src.addBlock("res_a"), 3, "rendinst_simple", "script:t=param_string_payload");
  build_res_entry(src.addBlock("res_b"), 4, "rendinst_simple", "script:t=param_string_payload");
  build_res_entry(src.addBlock("res_c"), 3, "rendinst_layered", "script:t=other_payload");
}

TEST_CASE("DataBlock dedup dump round-trip of all param types", "[datablock][dedup]")
{
  DataBlock src;
  src.setStr("s_interned", "name_also_used_as_param");
  src.setStr("s_complex", "some long unique payload string that will not match any name");
  src.setInt("i", -123);
  src.setReal("r", 0.25f);
  src.setPoint2("p2", Point2(1, 2));
  src.setPoint3("p3", Point3(1, 2, 3));
  src.setPoint4("p4", Point4(1, 2, 3, 4));
  src.setIPoint2("ip2", IPoint2(5, 6));
  src.setIPoint3("ip3", IPoint3(7, 8, 9));
  src.setBool("b", true);
  src.setE3dcolor("c", E3DCOLOR(1, 2, 3, 4));
  src.setTm("tm", TMatrix::IDENT);
  src.setInt64("i64", 0x123456789abcdefLL);
  src.addInt("arr", 1);
  src.addInt("arr", 2);
  src.addBlock("name_also_used_as_param")->setInt("x", 1);
  src.addBlock("empty_block");

  DataBlock dst;
  REQUIRE(roundtrip(src, dst, /*dedup*/ true));
  CHECK(equal(src, dst));
}

TEST_CASE("DataBlock dedup dump shares identical param runs", "[datablock][dedup]")
{
  DataBlock src;
  build_desc(src);

  int plain_sz = 0, dd_sz = 0;
  DataBlock plain, dd;
  REQUIRE(roundtrip(src, plain, false, &plain_sz));
  REQUIRE(roundtrip(src, dd, true, &dd_sz));
  CHECK(equal(src, plain));
  CHECK(equal(src, dd));
  CHECK(dd_sz < plain_sz); // res_a/res_b mat runs are stored once
}

TEST_CASE("DataBlock dedup dump copy-on-write isolation", "[datablock][dedup]")
{
  DataBlock dd;
  {
    DataBlock src;
    build_desc(src);
    REQUIRE(roundtrip(src, dd, true));
  }

  DataBlock refHolder;
  const DataBlock *ref = refHolder.addNewBlock(dd.getBlockByName("res_b"), "res_b");

  SECTION("param mutation does not leak into block sharing the run")
  {
    dd.getBlockByName("res_a")->setStr("refPhysObj", "phys_a");
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
    CHECK(strcmp(dd.getBlockByName("res_a")->getStr("refPhysObj", ""), "phys_a") == 0);

    dd.getBlockByName("res_a")->getBlockByName("mat")->setInt("t0", 77); // mat run is shared with res_b's mat
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
    CHECK(dd.getBlockByName("res_a")->getBlockByName("mat")->getInt("t0", -1) == 77);
  }

  SECTION("changeParamName does not rename in shared run")
  {
    dd.getBlockByName("res_a")->getBlockByName("mat")->changeParamName(0, "cls2");
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
    CHECK(dd.getBlockByName("res_a")->getBlockByName("mat")->paramExists("cls2"));
  }

  SECTION("removeParam does not shift params in shared run")
  {
    CHECK(dd.getBlockByName("res_a")->getBlockByName("mat")->removeParam("t0"));
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
    CHECK(!dd.getBlockByName("res_a")->getBlockByName("mat")->paramExists("t0"));
  }

  SECTION("adding top-level block keeps existing content intact")
  {
    dd.addBlock("res_new")->setInt("lods", 1);
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
  }

  SECTION("removing a block does not corrupt the block sharing its param runs")
  {
    CHECK(dd.removeBlock("res_a"));
    CHECK(equal(*dd.getBlockByName("res_b"), *ref));
  }
}

TEST_CASE("DataBlock dedup dump rejects out-of-range param run offset", "[datablock][dedup]")
{
  DataBlock src;
  src.setInt("x", 42);
  src.setInt("y", 43);

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4096);
  REQUIRE(src.saveToStreamDedup(cwr));
  char *data = (char *)cwr.data();
  const int sz = (int)cwr.size();
  REQUIRE(data[sz - 1] == 0); // trailing varint is the root block param run offset (root is written last, has no sub-blocks)

  DataBlock dst;
  data[sz - 1] = 0x7f; // run starts beyond the params pool
  {
    InPlaceMemLoadCB crd(data + 1, sz - 1); // +1: skip the stream format label
    CHECK(!dst.loadFromBinDump(crd, nullptr, /*dedup_ofs*/ true));
  }
  data[sz - 1] = 0x01; // run starts in range but its end exceeds the pool
  {
    InPlaceMemLoadCB crd(data + 1, sz - 1);
    CHECK(!dst.loadFromBinDump(crd, nullptr, /*dedup_ofs*/ true));
  }
  data[sz - 1] = 0x00; // control: restored dump loads again
  {
    InPlaceMemLoadCB crd(data + 1, sz - 1);
    CHECK(dst.loadFromBinDump(crd, nullptr, /*dedup_ofs*/ true));
    CHECK(dst.getInt("y", -1) == 43);
  }
}

namespace
{
struct Rng
{
  uint32_t s;
  uint32_t next() { return s = s * 1664525u + 1013904223u; }
  uint32_t range(uint32_t n) { return next() % n; }
};
} // namespace

static void add_random_params(DataBlock *b, Rng &rng)
{
  static const char *svals[] = {
    "a", "shader_class", "some unique long string payload num 1", "x", "shader_class", "some unique long string payload num 2"};
  for (int i = 0, n = rng.range(7); i < n; i++)
  {
    // param name is derived from type: same name with different types is not allowed within a block
    switch (rng.range(9))
    {
      case 0: b->addStr("cls", svals[rng.range(countof(svals))]); break;
      case 1: b->addInt("t0", rng.range(5)); break;
      case 2: b->addReal("val", rng.range(4) * 0.25f); break;
      case 3: b->addPoint3("pos", Point3(rng.range(3), rng.range(3), rng.range(3))); break;
      case 4: b->addPoint4("plane", Point4(rng.range(3), rng.range(3), rng.range(3), 1)); break;
      case 5: b->addBool("flag", rng.range(2) != 0); break;
      case 6: b->addE3dcolor("clr", E3DCOLOR(rng.range(255), rng.range(255), rng.range(255))); break;
      case 7: b->addInt64("id", int64_t(rng.next()) << 32 | rng.range(3)); break;
      case 8: b->addTm("tm", TMatrix::IDENT); break;
    }
  }
}

static void build_random_tree(DataBlock *b, Rng &rng, int depth, int &budget)
{
  static const char *bnames[] = {"mat", "tex", "node", "res", "lod", "data"};
  add_random_params(b, rng);
  if (depth >= 5 || budget <= 0)
    return;
  for (int i = 0, n = rng.range(depth == 0 ? 30 : 4); i < n && budget > 0; i++)
  {
    --budget;
    build_random_tree(b->addBlock(bnames[rng.range(countof(bnames))]), rng, depth + 1, budget);
  }
  // clone an existing child to produce duplicate subtrees
  if (b->blockCount() && rng.range(3) == 0)
  {
    const DataBlock *copyFrom = b->getBlock(rng.range(b->blockCount()));
    budget -= copyFrom->blockCount() + 1;
    b->addNewBlock(copyFrom, copyFrom->getBlockName());
  }
}

TEST_CASE("DataBlock dedup dump random trees round-trip", "[datablock][dedup]")
{
  for (int iter = 0; iter < 50; iter++)
  {
    Rng rng{0x9e3779b9u + uint32_t(iter) * 0x85ebca6bu};
    DataBlock src;
    int budget = 20 + rng.range(1500); // cover both stack-resident and heap block-count paths of the writer
    build_random_tree(&src, rng, 0, budget);

    DataBlock plain, dd;
    REQUIRE(roundtrip(src, plain, false));
    REQUIRE(roundtrip(src, dd, true));
    CHECK(equal(src, plain));
    CHECK(equal(src, dd));
  }
}
