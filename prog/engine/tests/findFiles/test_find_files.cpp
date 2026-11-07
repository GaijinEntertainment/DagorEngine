// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include <ioSys/dag_findFiles.h>
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

#include <stdio.h>
#include <string.h>

static const char *PROBE_FILE = "findfiles_probe.blk";
static const char *PROBE_DIR = "findfiles_probe_dir";
static const char *PROBE_NESTED_FILE = "findfiles_probe_dir/nested_probe.blk";

static void write_empty_file(const char *path)
{
  FILE *fp = fopen(path, "wb");
  REQUIRE(fp);
  fclose(fp);
}

struct ProbeTree
{
  ProbeTree()
  {
    dd_mkdir(PROBE_DIR);
    write_empty_file(PROBE_FILE);
    write_empty_file(PROBE_NESTED_FILE);
  }
  ~ProbeTree()
  {
    dd_erase(PROBE_NESTED_FILE);
    dd_erase(PROBE_FILE);
    dd_rmdir(PROBE_DIR);
  }
};

static bool find_first_file(const char *mask)
{
  for (const alefind_t &ff : dd_find_iterator(mask, DA_FILE))
    if (strcmp(ff.name, PROBE_FILE) == 0)
      return true;
  return false;
}

static bool contains(const Tab<SimpleString> &list, const char *path)
{
  for (const SimpleString &s : list)
    if (strcmp(s, path) == 0)
      return true;
  return false;
}

TEST_CASE("dd_find_first finds a file in the current folder from the empty base path", "[findFiles]")
{
  ProbeTree probe;
  CHECK(find_first_file("findfiles_probe*.blk"));
  CHECK(find_first_file("./findfiles_probe*.blk"));
}

TEST_CASE("find_files_in_folder lists the current folder from the empty base path", "[findFiles]")
{
  ProbeTree probe;
  Tab<SimpleString> files;
  find_files_in_folder(files, ".", ".blk", false, true, false);
  CHECK(files.size() == 1);
  CHECK(contains(files, "./findfiles_probe.blk"));
}

TEST_CASE("find_files_in_folder recurses into subfolders of the current folder", "[findFiles]")
{
  ProbeTree probe;
  Tab<SimpleString> files;
  find_files_in_folder(files, ".", ".blk", false, true, true);
  CHECK(files.size() == 2);
  CHECK(contains(files, "./findfiles_probe.blk"));
  CHECK(contains(files, "./findfiles_probe_dir/nested_probe.blk"));
}

TEST_CASE("find_files_in_folder lists a named subfolder", "[findFiles]")
{
  ProbeTree probe;
  Tab<SimpleString> files;
  find_files_in_folder(files, PROBE_DIR, ".blk", false, true, false);
  CHECK(files.size() == 1);
  CHECK(contains(files, "findfiles_probe_dir/nested_probe.blk"));
}
