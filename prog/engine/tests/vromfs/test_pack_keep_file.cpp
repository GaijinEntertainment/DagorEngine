// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_vromfs.h>
#include <memory/dag_memBase.h>
#include <util/dag_globDef.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <atomic>
#include <string.h>
#include <thread>

static const char *PACK_PATH = "vromfs_tests.vromfs.bin";
static char mount_path[] = "vromfs_tests/";
static const long ENTRY_FILE_OFS = 16 + 80; // header + dataOfs of write_pack()

static void put32(eastl::vector<char> &buf, int ofs, uint32_t v) { memcpy(buf.data() + ofs, &v, sizeof(v)); }

// 64-bit pack:b=false layout, one entry
static void write_pack(const char *text)
{
  const int namePtrOfs = 48, nameOfs = 56, dataTabOfs = 64, dataOfs = 80, len = (int)strlen(text);
  eastl::vector<char> body(dataOfs + len, 0);
  put32(body, 0, namePtrOfs);
  put32(body, 4, 1);
  put32(body, 16, dataTabOfs);
  put32(body, 20, 1);
  put32(body, 32, dataOfs);
  put32(body, namePtrOfs, nameOfs);
  strcpy(body.data() + nameOfs, "a.txt");
  put32(body, dataTabOfs, dataOfs);
  put32(body, dataTabOfs + 4, len);
  memcpy(body.data() + dataOfs, text, len);

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

static eastl::string read_entry()
{
  file_ptr_t fp = df_open("vromfs_tests/a.txt", DF_READ);
  if (!fp)
    return "<not found>";
  eastl::string text(df_length(fp), ' ');
  df_read(fp, text.data(), (int)text.size());
  df_close(fp);
  return text;
}

static eastl::string read_entry_bytewise()
{
  file_ptr_t fp = df_open("vromfs_tests/a.txt", DF_READ);
  if (!fp)
    return "<not found>";
  eastl::string text;
  char c;
  while (df_read(fp, &c, 1) == 1)
    text += c;
  df_close(fp);
  return text;
}

static eastl::string map_entry()
{
  file_ptr_t fp = df_open("vromfs_tests/a.txt", DF_READ);
  if (!fp)
    return "<not found>";
  int len = 0;
  const char *view = (const char *)df_mmap(fp, &len);
  eastl::string text = view ? eastl::string(view, len) : "<not mapped>";
  df_unmap(view, len);
  df_close(fp);
  return text;
}

TEST_CASE("vromfs pack without keep_file keeps no file open and reads its entry", "[vromfs]")
{
  write_pack("old text");
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem);
  REQUIRE(pack);
  CHECK(pack->getFile() == nullptr);
  REQUIRE(pack->getFilePath());
  CHECK(strstr(pack->getFilePath(), PACK_PATH));
  add_vromfs(pack, true, mount_path);
  CHECK(read_entry() == "old text");
  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  CHECK(dd_erase(PACK_PATH));
}

TEST_CASE("vromfs pack with keep_file reuses its kept file for each entry read", "[vromfs]")
{
  write_pack("old text");
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem, 0, true);
  REQUIRE(pack);
  REQUIRE(pack->getFile());
  REQUIRE(pack->getFilePath());
  CHECK(strstr(pack->getFilePath(), PACK_PATH));
  add_vromfs(pack, true, mount_path);
  CHECK(vromfs_check_file_exists("vromfs_tests/a.txt"));

  file_ptr_t fp = df_open("vromfs_tests/a.txt", DF_READ);
  REQUIRE(fp);
  char head[4] = {}, tail[6] = {};
  REQUIRE(df_read(fp, head, 3) == 3);
  CHECK(ftell((FILE *)pack->getFile()) == ENTRY_FILE_OFS + 3);
  int viewLen = 0;
  const char *view = (const char *)df_mmap(fp, &viewLen);
  REQUIRE(view);
  REQUIRE(df_read(fp, tail, 5) == 5);
  CHECK(eastl::string(head) + tail == "old text");
  CHECK(eastl::string(view, viewLen) == "old text");
  df_unmap(view, viewLen);
  df_close(fp);
  CHECK(read_entry() == "old text");
  CHECK(map_entry() == "old text");

  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  CHECK(dd_erase(PACK_PATH));
}

TEST_CASE("a second reader of a keep_file pack opens the file by its name while the first holds the kept file", "[vromfs]")
{
  write_pack("old text");
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem, 0, true);
  REQUIRE(pack);
  REQUIRE(pack->getFile());
  add_vromfs(pack, true, mount_path);

  file_ptr_t reused = df_open("vromfs_tests/a.txt", DF_READ);
  REQUIRE(reused);
  char first[5] = {};
  REQUIRE(df_read(reused, first, 4) == 4);
  CHECK(read_entry() == "old text");
  CHECK(ftell((FILE *)pack->getFile()) == ENTRY_FILE_OFS + 4);
  CHECK(map_entry() == "old text");
  char rest[5] = {};
  REQUIRE(df_read(reused, rest, 4) == 4);
  CHECK(eastl::string(first) + rest == "old text");
  df_close(reused);
  CHECK(read_entry() == "old text");

  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  CHECK(dd_erase(PACK_PATH));
}

TEST_CASE("an entry that holds the kept file still reads after the pack is closed", "[vromfs]")
{
  write_pack("old text");
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem, 0, true);
  REQUIRE(pack);
  REQUIRE(pack->getFile());
  add_vromfs(pack, true, mount_path);
  file_ptr_t reused = df_open("vromfs_tests/a.txt", DF_READ);
  REQUIRE(reused);
  char first[5] = {}, rest[5] = {};
  REQUIRE(df_read(reused, first, 4) == 4);
  CHECK(ftell((FILE *)pack->getFile()) == ENTRY_FILE_OFS + 4);
  file_ptr_t kept = (file_ptr_t)pack->getFile();
  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  REQUIRE(df_read(reused, rest, 4) == 4);
  CHECK(eastl::string(first) + rest == "old text");
  df_close(reused);
  df_close(kept); // the close left it open
  CHECK(dd_erase(PACK_PATH));
}

TEST_CASE("readers of a keep_file pack on several threads each read their own bytes", "[vromfs]")
{
  write_pack("old text");
  VirtualRomFsPack *pack = open_vromfs_pack(PACK_PATH, tmpmem, 0, true);
  REQUIRE(pack);
  add_vromfs(pack, true, mount_path);
  std::atomic<int> wrongReads{0}, wrongMaps{0};
  auto readLoop = [&] {
    for (int i = 0; i < 2000; i++)
      if (read_entry_bytewise() != "old text")
        wrongReads++;
  };
  auto mapLoop = [&] {
    for (int i = 0; i < 2000; i++)
      if (map_entry() != "old text")
        wrongMaps++;
  };
  std::thread reader1(readLoop), reader2(readLoop), mapper(mapLoop);
  reader1.join();
  reader2.join();
  mapper.join();
  CHECK(wrongReads == 0);
  CHECK(wrongMaps == 0);
  remove_vromfs(pack);
  close_vromfs_pack(pack, tmpmem);
  CHECK(dd_erase(PACK_PATH));
}
