// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include <util/dag_localization.h>
#include <ioSys/dag_memIo.h>
#include <generic/dag_tab.h>
#include <EASTL/string.h>
#include <string.h>

// Cross-file duplicate detection: startup_localization_V2 calls loadCsv per
// file and each call builds a fresh LocTableParserCB, so the parser-local
// hashToIdx is empty at the start of every file. Without the persistent-map
// lookup in getKey(), a key first seen in file A and repeated in file B would
// silently overwrite the file-A value without firing dag_on_duplicate_loc_key
// -- and csvUtil relies on that callback to fail bad localization sets
// (csvUtil.cpp:24-29 increments dup_key_err_count).

namespace
{
int g_dup_count = 0;
eastl::string g_last_dup_key;
bool g_dup_overwrite = true;

bool test_on_duplicate(const char *key)
{
  g_dup_count++;
  g_last_dup_key = key;
  return g_dup_overwrite;
}

struct DuplicateCounterScope
{
  bool (*prev)(const char *) = nullptr;
  DuplicateCounterScope(bool overwrite = true)
  {
    prev = dag_on_duplicate_loc_key;
    dag_on_duplicate_loc_key = &test_on_duplicate;
    g_dup_count = 0;
    g_last_dup_key.clear();
    g_dup_overwrite = overwrite;
  }
  ~DuplicateCounterScope()
  {
    dag_on_duplicate_loc_key = prev;
    shutdown_localization();
  }
};

// Wraps text in an InPlaceMemLoadCB. parseCsvV2 tolerates an unterminated last
// row, but the file-backed loader pads with '\n' -- mirror that here so test
// inputs match the production path.
bool load_csv_text(Tab<char> &storage, const char *text)
{
  const int n = (int)strlen(text);
  storage.resize(n + 1);
  memcpy(storage.data(), text, n);
  storage[n] = '\n';
  InPlaceMemLoadCB cb(storage.data(), storage.size());
  return load_localization_table_from_csv(&cb, /*lang_col=*/0);
}
} // namespace

TEST_CASE("Cross-file duplicate key fires callback", "[localization][duplicates]")
{
  DuplicateCounterScope scope(/*overwrite=*/true);

  Tab<char> bufA, bufB;
  REQUIRE(load_csv_text(bufA, "foo;val_a"));
  CHECK(g_dup_count == 0);
  CHECK(strcmp(get_localized_text("foo"), "val_a") == 0);

  REQUIRE(load_csv_text(bufB, "foo;val_b"));
  CHECK(g_dup_count == 1);
  CHECK(strcmp(g_last_dup_key.c_str(), "foo") == 0);
  CHECK(strcmp(get_localized_text("foo"), "val_b") == 0);
}

TEST_CASE("Cross-file duplicate with skip preserves original value", "[localization][duplicates]")
{
  DuplicateCounterScope scope(/*overwrite=*/false);

  Tab<char> bufA, bufB;
  REQUIRE(load_csv_text(bufA, "foo;val_a"));
  REQUIRE(load_csv_text(bufB, "foo;val_b"));

  CHECK(g_dup_count == 1);
  CHECK(strcmp(g_last_dup_key.c_str(), "foo") == 0);
  CHECK(strcmp(get_localized_text("foo"), "val_a") == 0);
}

TEST_CASE("In-file duplicate key fires callback", "[localization][duplicates]")
{
  DuplicateCounterScope scope(/*overwrite=*/true);

  Tab<char> buf;
  REQUIRE(load_csv_text(buf, "foo;val_a\nfoo;val_b"));
  CHECK(g_dup_count == 1);
  CHECK(strcmp(g_last_dup_key.c_str(), "foo") == 0);
  CHECK(strcmp(get_localized_text("foo"), "val_b") == 0);
}

TEST_CASE("Distinct keys across files do not fire duplicate callback", "[localization][duplicates]")
{
  DuplicateCounterScope scope(/*overwrite=*/true);

  Tab<char> bufA, bufB;
  REQUIRE(load_csv_text(bufA, "foo;val_a"));
  REQUIRE(load_csv_text(bufB, "bar;val_b"));

  CHECK(g_dup_count == 0);
  CHECK(strcmp(get_localized_text("foo"), "val_a") == 0);
  CHECK(strcmp(get_localized_text("bar"), "val_b") == 0);
}

TEST_CASE("Cross-file duplicate detected across more than two files", "[localization][duplicates]")
{
  DuplicateCounterScope scope(/*overwrite=*/true);

  Tab<char> bufA, bufB, bufC;
  REQUIRE(load_csv_text(bufA, "foo;a"));
  REQUIRE(load_csv_text(bufB, "foo;b"));
  REQUIRE(load_csv_text(bufC, "foo;c"));

  CHECK(g_dup_count == 2);
  CHECK(strcmp(g_last_dup_key.c_str(), "foo") == 0);
  CHECK(strcmp(get_localized_text("foo"), "c") == 0);
}

// Cross-file wyhash collision detection (LocKeyDump::tryAddString collided
// flag) depends on the dump accumulating keys from every file the table
// loads -- otherwise a fresh dump in file B has no record of file A's keys
// and would silently store a colliding-hash entry under its new bytes. This
// test verifies a caller-supplied dump persists across loadCsv calls: every
// loaded key is visible via get_all_localization regardless of which file
// it came from.
TEST_CASE("Caller-supplied LocKeyDump accumulates keys across files", "[localization][keydump]")
{
  DuplicateCounterScope scope;

  LocKeyDump *dump = make_loc_key_dump();
  REQUIRE(dump != nullptr);

  Tab<char> bufA, bufB;
  const int nA = (int)strlen("foo;val_a") + 1;
  bufA.resize(nA);
  memcpy(bufA.data(), "foo;val_a", nA - 1);
  bufA[nA - 1] = '\n';
  {
    InPlaceMemLoadCB cb(bufA.data(), bufA.size());
    REQUIRE(load_localization_table_from_csv(&cb, 0, dump));
  }
  const int nB = (int)strlen("bar;val_b") + 1;
  bufB.resize(nB);
  memcpy(bufB.data(), "bar;val_b", nB - 1);
  bufB[nB - 1] = '\n';
  {
    InPlaceMemLoadCB cb(bufB.data(), bufB.size());
    REQUIRE(load_localization_table_from_csv(&cb, 0, dump));
  }

  Tab<const char *> keys, vals;
  get_all_localization(dump, keys, vals);

  bool sawFoo = false, sawBar = false;
  for (int i = 0; i < (int)keys.size(); i++)
  {
    if (strcmp(keys[i], "foo") == 0)
    {
      sawFoo = true;
      CHECK(strcmp(vals[i], "val_a") == 0);
    }
    else if (strcmp(keys[i], "bar") == 0)
    {
      sawBar = true;
      CHECK(strcmp(vals[i], "val_b") == 0);
    }
  }
  CHECK(sawFoo);
  CHECK(sawBar);

  destroy_loc_key_dump(dump);
}
