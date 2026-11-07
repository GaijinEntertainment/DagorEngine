// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <memory/dag_memBase.h>
#include <util/dag_globDef.h>
#include <EASTL/string.h>
#include <initializer_list>
#include <string.h>

static const char *PACK_PATH = "datablock_tests.vromfs.bin";

struct PackEntry
{
  const char *name;
  eastl::string data;
};

static void put32(eastl::vector<char> &buf, size_t ofs, size_t v)
{
  uint32_t v32 = (uint32_t)v;
  memcpy(buf.data() + ofs, &v32, sizeof(v32));
}

static size_t align16(size_t v) { return (v + 15) & ~size_t(15); }

// 64-bit pack:b=false layout, entries sorted by name
static void write_pack(std::initializer_list<PackEntry> entries)
{
  const size_t count = entries.size(), namePtrOfs = 48, namesOfs = namePtrOfs + count * 8;
  size_t ofs = namesOfs;
  for (const PackEntry &e : entries)
    ofs += strlen(e.name) + 1;
  const size_t dataTabOfs = align16(ofs), tablesEnd = dataTabOfs + count * 16;
  ofs = tablesEnd;
  for (const PackEntry &e : entries)
    ofs = align16(ofs) + e.data.size();

  eastl::vector<char> body(ofs, 0);
  put32(body, 0, namePtrOfs);
  put32(body, 4, count);
  put32(body, 16, dataTabOfs);
  put32(body, 20, count);
  put32(body, 32, tablesEnd);
  size_t i = 0, nameOfs = namesOfs, dataOfs = tablesEnd;
  for (const PackEntry &e : entries)
  {
    dataOfs = align16(dataOfs);
    put32(body, namePtrOfs + i * 8, nameOfs);
    strcpy(body.data() + nameOfs, e.name);
    put32(body, dataTabOfs + i * 16, dataOfs);
    put32(body, dataTabOfs + i * 16 + 4, e.data.size());
    memcpy(body.data() + dataOfs, e.data.data(), e.data.size());
    nameOfs += strlen(e.name) + 1;
    dataOfs += e.data.size();
    i++;
  }

  eastl::vector<char> file(16, 0);
  put32(file, 0, _MAKE4C('VRFs'));
  put32(file, 4, _MAKE4C('PC'));
  put32(file, 8, body.size());
  file.insert(file.end(), body.begin(), body.end());
  file_ptr_t fp = df_open(PACK_PATH, DF_WRITE | DF_CREATE);
  REQUIRE(fp);
  REQUIRE(df_write(fp, file.data(), (int)file.size()) == (int)file.size());
  df_close(fp);
}

TEST_CASE("DataBlock with shared namemap included from a vromfs pack fails to load", "[datablock][vromfs]")
{
  static char mnt[] = "blk2_pack/";
  write_pack(
    {{"a.blk", "include \"b.blk\"\n"}, {"b.blk", eastl::string(16, '\3')}, {dblk::SHARED_NAMEMAP_FNAME, eastl::string(64, '\0')}});
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem);
  REQUIRE(pack);
  add_vromfs(pack, true, mnt);

  ErrorCollector collector;
  DataBlock::InstallReporterRAII reporter(&collector);
  DataBlock blk;
  blk.load("blk2_pack/a.blk");
  REQUIRE(!collector.empty());
  CHECK(strstr(collector.errors.front().text.str(), "vromfs pack"));
  collector.clear();
  CHECK(!blk.load("blk2_pack/b.blk"));
  REQUIRE(!collector.empty());
  CHECK(strstr(collector.errors.back().text.str(), "vromfs pack"));

  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  CHECK(dd_erase(PACK_PATH));
}

TEST_CASE("DataBlock shared namemap and dictionaries of a vromfs pack are not read from file offsets", "[datablock][vromfs]")
{
  VirtualRomFsSingleFile *nmLink = VirtualRomFsSingleFile::make_file_link(tmpmem, PACK_PATH, 4096, 64, dblk::SHARED_NAMEMAP_FNAME);
  CHECK(dblk::get_vromfs_shared_name_map(nmLink) == nullptr);
  CHECK(dblk::get_vromfs_blk_ddict(nmLink) == nullptr);
  CHECK(dblk::create_vromfs_blk_cdict(nmLink, 1) == nullptr);
  nmLink->release();

  eastl::string nmData(64, '\0');
  memset(&nmData[8], 0x11, 32);
  VirtualRomFsSingleFile *nmMem =
    VirtualRomFsSingleFile::make_mem_data(tmpmem, nmData.data(), (int)nmData.size(), dblk::SHARED_NAMEMAP_FNAME);
  eastl::string dictName = eastl::string(64, '1') + ".dict";
  VirtualRomFsSingleFile *dictLink = VirtualRomFsSingleFile::make_file_link(tmpmem, PACK_PATH, 4096, 64, dictName.c_str());
  add_vromfs(dictLink, true);
  CHECK(dblk::get_vromfs_blk_ddict(nmMem) == nullptr);
  remove_vromfs(dictLink);
  dictLink->release();

  eastl::string dictData(64, 'd');
  VirtualRomFsSingleFile *dictMem =
    VirtualRomFsSingleFile::make_mem_data(tmpmem, dictData.data(), (int)dictData.size(), dictName.c_str());
  add_vromfs(dictMem, true);
  ZSTD_DDict_s *ddict = dblk::get_vromfs_blk_ddict(nmMem);
  CHECK(ddict != nullptr);
  dblk::release_vromfs_blk_ddict(ddict);
  remove_vromfs(dictMem);
  dictMem->release();
  nmMem->release();
}
