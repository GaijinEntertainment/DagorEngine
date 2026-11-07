// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <util/dag_localization.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <string.h>
#include <atomic>
#include <thread>

namespace
{
const char *const LANGS[] = {"English", "Russian", "German", "Japanese"};
constexpr int LANG_COUNT = 4;
constexpr int FILE_A_ROWS = 1200, FILE_B_FIRST_ROW = 1000, TOTAL_ROWS = 1700;
const char *const FILE_A = "_loc_full_test_a.csv";
const char *const FILE_B = "_loc_full_test_b.csv";

eastl::string key_name(int row)
{
  eastl::string key;
  key.sprintf("key_%d", row);
  return key;
}

eastl::string text_for(int row, int lang, char file_tag)
{
  if (row % 11 == 5 && lang == 3)
    return {};
  eastl::string text;
  text.sprintf("%c:%s:%d", file_tag, LANGS[lang], row);
  if (row % 7 == 0)
    text += " \"quoted\"; semicolon";
  if (row % 13 == 0)
    text += "\xe6\x97\xa5\xd0\x96";
  if (row % 17 == 0)
    text.append(300, 'x');
  return text;
}

eastl::string expected_text(int row, int lang, char file_b_tag = 'b')
{
  return text_for(row, lang, row >= FILE_B_FIRST_ROW ? file_b_tag : 'a');
}

void append_cell(eastl::string &csv, const eastl::string &cell)
{
  csv += '"';
  for (char c : cell)
    csv.append(c == '"' ? 2 : 1, c);
  csv += '"';
}

void write_csv(const char *name, int first_row, int end_row, char file_tag)
{
  eastl::string csv = "\"<ID|readonly|noverify>\"";
  for (const char *lang : LANGS)
    csv.append_sprintf(";\"<%s>\"", lang);
  csv += "\r\n";
  for (int row = first_row; row < end_row; row++)
  {
    append_cell(csv, key_name(row));
    for (int lang = 0; lang < LANG_COUNT; lang++)
    {
      csv += ';';
      append_cell(csv, text_for(row, lang, file_tag));
    }
    csv += "\r\n";
  }
  file_ptr_t fp = df_open(name, DF_WRITE | DF_CREATE);
  REQUIRE(fp);
  const int written = df_write(fp, csv.data(), (int)csv.size());
  df_close(fp);
  REQUIRE(written == (int)csv.size());
}

struct FullTableFiles
{
  DataBlock blk;

  FullTableFiles()
  {
    write_csv(FILE_A, 0, FILE_A_ROWS, 'a');
    write_csv(FILE_B, FILE_B_FIRST_ROW, TOTAL_ROWS, 'b');
    DataBlock *files = blk.addBlock("locTableFull");
    files->addStr("file", FILE_A);
    files->addStr("file", FILE_B);
    DataBlock *langs = blk.addBlock("text_translation");
    for (const char *lang : LANGS)
      langs->addStr("lang", lang);
  }
  ~FullTableFiles()
  {
    shutdown_localization();
    dd_erase(FILE_A);
    dd_erase(FILE_B);
  }
};

void check_all_texts(char file_b_tag = 'b')
{
  for (int row = 0; row < TOTAL_ROWS; row++)
    for (int lang = 0; lang < LANG_COUNT; lang++)
    {
      const eastl::string key = key_name(row);
      const char *text = get_localized_text_for_lang_id(key.c_str(), lang);
      REQUIRE(text);
      REQUIRE(expected_text(row, lang, file_b_tag) == text);
      REQUIRE(get_localized_text_for_lang(key.c_str(), LANGS[lang]) == text);
    }
}

int g_dup_count = 0;
bool g_dup_overwrite = true;
bool count_dup(const char *)
{
  g_dup_count++;
  return g_dup_overwrite;
}

struct DupCounterScope
{
  bool (*prev)(const char *) = dag_on_duplicate_loc_key;
  DupCounterScope(bool overwrite)
  {
    dag_on_duplicate_loc_key = &count_dup;
    g_dup_count = 0;
    g_dup_overwrite = overwrite;
  }
  ~DupCounterScope() { dag_on_duplicate_loc_key = prev; }
};
} // namespace

TEST_CASE("Full table gives every language of every row", "[localization][full]")
{
  FullTableFiles files;
  REQUIRE(startup_localization_V2(files.blk, "English"));
  for (int lang = 0; lang < LANG_COUNT; lang++)
    CHECK(getLangId(LANGS[lang]) == lang);
  check_all_texts();
}

TEST_CASE("Full table gives null for an unknown language, key or id", "[localization][full]")
{
  FullTableFiles files;
  REQUIRE(startup_localization_V2(files.blk, "English"));
  CHECK(get_localized_text_for_lang("key_1", "Klingon") == nullptr);
  CHECK(get_localized_text_for_lang("no_such_key", "German") == nullptr);
  CHECK(get_localized_text_for_lang_id("key_1", -1) == nullptr);
  CHECK(get_localized_text_for_lang_id("key_1", LANG_COUNT) == nullptr);
  CHECK(getLangId("Klingon") == -1);
  CHECK(strcmp(getLangById(2), "German") == 0);
}

TEST_CASE("Full table texts keep their address", "[localization][full]")
{
  FullTableFiles files;
  REQUIRE(startup_localization_V2(files.blk, "Russian"));
  eastl::vector<const char *> first;
  for (int lang = LANG_COUNT - 1; lang >= 0; lang--)
    for (int row = TOTAL_ROWS - 1; row >= 0; row -= 3)
      first.push_back(get_localized_text_for_lang(key_name(row).c_str(), LANGS[lang]));
  check_all_texts();
  int i = 0;
  for (int lang = LANG_COUNT - 1; lang >= 0; lang--)
    for (int row = TOTAL_ROWS - 1; row >= 0; row -= 3, i++)
    {
      REQUIRE(get_localized_text_for_lang(key_name(row).c_str(), LANGS[lang]) == first[i]);
      REQUIRE(expected_text(row, lang) == first[i]);
    }
}

TEST_CASE("Full table gives the same texts after a language switch", "[localization][full]")
{
  FullTableFiles files;
  for (const char *current : {"English", "Japanese", "German"})
  {
    shutdown_localization();
    REQUIRE(startup_localization_V2(files.blk, current));
    for (int lang = 0; lang < LANG_COUNT; lang++)
      CHECK(getLangId(LANGS[lang]) == lang);
    check_all_texts();
  }
}

TEST_CASE("Full table unpacks blocks on demand from several threads", "[localization][full]")
{
  FullTableFiles files;
  REQUIRE(startup_localization_V2(files.blk, "English"));
  constexpr int THREADS = 4, TEXTS = TOTAL_ROWS * LANG_COUNT;
  eastl::vector<const char *> texts[THREADS];
  std::atomic<int> ready{0};
  eastl::vector<std::thread> threads;
  for (int t = 0; t < THREADS; t++)
    threads.emplace_back([&, t] {
      texts[t].resize(TEXTS);
      ready++;
      while (ready < THREADS) {}
      get_localized_text_for_lang("key_0", "Russian");
      for (int i = 0; i < TEXTS; i++)
      {
        const int idx = (i + t * 397) % TEXTS;
        texts[t][idx] = get_localized_text_for_lang(key_name(idx / LANG_COUNT).c_str(), LANGS[idx % LANG_COUNT]);
      }
    });
  for (std::thread &thread : threads)
    thread.join();
  for (int t = 0; t < THREADS; t++)
    for (int idx = 0; idx < TEXTS; idx++)
    {
      REQUIRE(texts[t][idx]);
      REQUIRE(expected_text(idx / LANG_COUNT, idx % LANG_COUNT) == texts[t][idx]);
      REQUIRE(texts[t][idx] == texts[0][idx]);
    }
}

TEST_CASE("Full table reload without shutdown keeps the old texts", "[localization][full]")
{
  FullTableFiles files;
  DupCounterScope dups(/*overwrite=*/true);
  REQUIRE(startup_localization_V2(files.blk, "English"));
  CHECK(g_dup_count == FILE_A_ROWS - FILE_B_FIRST_ROW);
  const char *oldText = get_localized_text_for_lang("key_1300", "German");
  REQUIRE(oldText);
  const eastl::string oldCopy = oldText;
  write_csv(FILE_B, FILE_B_FIRST_ROW, TOTAL_ROWS, 'c');
  g_dup_count = 0;
  REQUIRE(startup_localization_V2(files.blk, "English"));
  CHECK(g_dup_count == FILE_A_ROWS - FILE_B_FIRST_ROW);
  CHECK(oldCopy == oldText);
  check_all_texts('c');
}

TEST_CASE("Full table keeps the first text of a rejected duplicate", "[localization][full]")
{
  FullTableFiles files;
  DupCounterScope dups(/*overwrite=*/false);
  REQUIRE(startup_localization_V2(files.blk, "English"));
  CHECK(g_dup_count == FILE_A_ROWS - FILE_B_FIRST_ROW);
  for (int row = FILE_B_FIRST_ROW; row < TOTAL_ROWS; row++)
    for (int lang = 0; lang < LANG_COUNT; lang++)
    {
      const char *text = get_localized_text_for_lang(key_name(row).c_str(), LANGS[lang]);
      REQUIRE(text);
      REQUIRE(text_for(row, lang, row < FILE_A_ROWS ? 'a' : 'b') == text);
    }
}
