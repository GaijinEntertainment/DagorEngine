// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <ioSys/dag_memIo.h>
#include <string.h>

namespace dblk
{
void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);
}

namespace
{
constexpr int MAX_BLOCK_DEPTH = 500;

enum class Format
{
  Text,
  Binary,
  BinaryDedup,
  BBF3,
};

void make_chain(DataBlock &root, int depth)
{
  DataBlock *b = &root;
  for (int i = 0; i < depth; ++i)
    b = b->addNewBlock("a");
  b->setInt("x", 1);
}

int chain_depth(const DataBlock &root)
{
  int depth = 0;
  const DataBlock *b = &root;
  while (b->blockCount() == 1)
  {
    b = b->getBlock(0);
    ++depth;
  }
  return b->getInt("x", 0) == 1 ? depth : -1;
}

void save(const DataBlock &blk, Format fmt, DynamicMemGeneralSaveCB &cwr)
{
  switch (fmt)
  {
    case Format::Text: blk.saveToTextStream(cwr); break;
    case Format::Binary: blk.saveToStream(cwr); break;
    case Format::BinaryDedup: blk.saveToStreamDedup(cwr); break;
    case Format::BBF3: dblk::save_to_bbf3(blk, cwr); break;
  }
}

bool had_nesting_error(const ErrorCollector &rep)
{
  for (auto &e : rep.errors)
    if (strstr(e.text.str(), "nesting"))
      return true;
  return false;
}

void check_depth_limit(Format fmt)
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();

  for (int depth : {MAX_BLOCK_DEPTH, MAX_BLOCK_DEPTH + 1})
  {
    DataBlock src;
    make_chain(src, depth);
    DynamicMemGeneralSaveCB cwr(tmpmem, 0, 64 << 10);
    save(src, fmt, cwr);

    ErrorCollector rep;
    DataBlock::InstallReporterRAII raii(&rep);
    DataBlock blk;
    InPlaceMemLoadCB crd(cwr.data(), cwr.size());
    bool ok = blk.loadFromStream(crd, "deep.blk");

    if (depth <= MAX_BLOCK_DEPTH)
    {
      CHECK(ok);
      CHECK(chain_depth(blk) == depth);
    }
    else
    {
      CHECK_FALSE(ok);
      CHECK(had_nesting_error(rep));
    }
  }
}
} // namespace

TEST_CASE("DataBlock text parser limits block nesting depth", "[datablock][nesting]") { check_depth_limit(Format::Text); }

TEST_CASE("DataBlock binary dump limits block nesting depth", "[datablock][nesting][binary]") { check_depth_limit(Format::Binary); }

TEST_CASE("DataBlock dedup binary dump limits block nesting depth", "[datablock][nesting][binary]")
{
  check_depth_limit(Format::BinaryDedup);
}

TEST_CASE("DataBlock BBF3 limits block nesting depth", "[datablock][nesting][bbf3]") { check_depth_limit(Format::BBF3); }

TEST_CASE("DataBlock text parser rejects deep nesting without stack overflow", "[datablock][nesting]")
{
  FatalFlagsGuard guard;
  guard.setAllNonFatal();
  ErrorCollector rep;
  DataBlock::InstallReporterRAII raii(&rep);

  const int depth = 100000;
  String text;
  for (int i = 0; i < depth; ++i)
    text += "a{";
  for (int i = 0; i < depth; ++i)
    text += "}";

  DataBlock blk;
  InPlaceMemLoadCB crd(text.str(), text.length());
  CHECK_FALSE(blk.loadFromStream(crd, "deep.blk"));
  CHECK(had_nesting_error(rep));
}
