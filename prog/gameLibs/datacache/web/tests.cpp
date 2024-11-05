// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <UnitTest++/UnitTestPP.h>
#include "webbackend.h"
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_miscApi.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
#include <ioSys/dag_datablock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_findFiles.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include "webutil.h"
#include <zlib.h>

struct WebCacheFixture // python -m SimpleHTTPServer
{
#define WCPATH "testcache"
#define INAME  "00index.gz"
  datacache::Backend *cache;
  bool done;
  static void genindex()
  {
    gzFile findex = gzopen(INAME, "wb");
    uint8_t hash[SHA_DIGEST_LENGTH] = {0}, strHash[SHA_DIGEST_LENGTH * 2 + 1];
    alefind_t fnd = {0};
    for (int found = dd_find_first("*", DA_FILE, &fnd); found; found = dd_find_next(&fnd))
    {
      if (strcmp(fnd.name, INAME) == 0)
        continue;
      struct stat st = {0};
      stat(fnd.name, &st);
      uint8_t *mem = (uint8_t *)malloc(st.st_size);
      {
        FILE *ff = fopen(fnd.name, "rb");
        G_VERIFY(fread(mem, 1, st.st_size, ff) == st.st_size);
        fclose(ff);
      }
      SHA1(mem, st.st_size, hash);
      free(mem);
      gzprintf(findex, "%s %d %d %s\n", fnd.name, (int)st.st_size, (int)st.st_mtime, datacache::hashstr(hash, (char *)strHash));
    }
    dd_find_close(&fnd);
    gzclose(findex);
  }
  WebCacheFixture(bool stale = true, bool index = true) : done(false)
  {
    DataBlock params;
    params.setStr("mountPath", WCPATH);
    params.addBlock("baseUrls")->setStr("url", "http://localhost:8000");
    params.setBool("allowReturnStaleData", stale);
    params.setInt("traceLevel", 0);
    if (!index)
      params.setBool("noIndex", true);
    cache = datacache::WebBackend::create(params);
    G_ASSERT(cache);
    if (index)
      genindex();
  }
  ~WebCacheFixture()
  {
    delete cache;
    Tab<SimpleString> files;
    system("rm -rf " WCPATH);
    dd_erase(INAME);
  }
  static void onFileLoaded(const char *, datacache::ErrorCode, datacache::Entry *entry, void *arg)
  {
    if (entry)
      entry->free();
    *(bool *)arg = true;
  }
};

TEST_FIXTURE(WebCacheFixture, WebCacheBasic)
{
  const char *toCheck[] = {"jamfile", "tests.cpp"};
  for (int i = 0; i < countof(toCheck); ++i)
  {
    done = false;
    datacache::ErrorCode err = datacache::ERR_UNKNOWN;
    {
      datacache::EntryHolder(cache->get(toCheck[i], &err, WebCacheFixture::onFileLoaded, &done));
    }
    CHECK_EQUAL(datacache::ERR_PENDING, err); // web server isn't running?
    while (err == datacache::ERR_PENDING && !done)
    {
      cache->poll();
      sleep_msec(0);
      cpujobs::release_done_jobs();
    }
  }
}

TEST_FIXTURE(WebCacheFixture, WebCacheDownloadIndexPlusGet)
{
  cache->control(_MAKE4C('DLIX'));
  sleep_msec(30);
  cache->poll();
  datacache::ErrorCode err = datacache::ERR_UNKNOWN;
  {
    datacache::EntryHolder(cache->get("tests.cpp", &err, WebCacheFixture::onFileLoaded, &done));
  }
}

struct WebCacheFixtureStrict : public WebCacheFixture
{
public:
  WebCacheFixtureStrict() : WebCacheFixture(false) {}
};
TEST_FIXTURE(WebCacheFixtureStrict, WebCacheDoubleGet)
{
  datacache::ErrorCode err0 = datacache::ERR_UNKNOWN, err1 = datacache::ERR_UNKNOWN;
  bool done0 = false, done1 = false;
  datacache::EntryHolder(cache->get("tests.cpp", &err0, WebCacheFixture::onFileLoaded, &done0));
  datacache::EntryHolder(cache->get("tests.cpp", &err1, WebCacheFixture::onFileLoaded, &done1));
  for (int i = 0; i < 2; ++i)
  {
    CHECK_EQUAL(datacache::ERR_PENDING, i ? err1 : err0); // web server isn't running?
    while ((i ? err1 : err0) == datacache::ERR_PENDING && !(i ? done1 : done0))
    {
      cache->poll();
      sleep_msec(0);
      cpujobs::release_done_jobs();
    }
  }
}

TEST_FIXTURE(WebCacheFixture, WebCacheWriteRead)
{
  // download index
  cache->control(_MAKE4C('DLIX'));
  sleep_msec(30);
  cpujobs::release_done_jobs(); // onFileDownload
  cache->poll();
  sleep_msec(30);
  cpujobs::release_done_jobs(); // index load
  cache->poll();

  datacache::ErrorCode err0 = datacache::ERR_UNKNOWN, err1 = datacache::ERR_UNKNOWN;
  bool done0 = false, done1 = false;
  datacache::Entry *e0 = cache->get("tests.cpp", &err0, WebCacheFixture::onFileLoaded, &done0);
  sleep_msec(30);
  cache->poll();
  cpujobs::release_done_jobs(); // onFileDownload
  sleep_msec(30);               // hashstream

  datacache::Entry *e1 = cache->get("tests.cpp", &err1, WebCacheFixture::onFileLoaded, &done1);

  if (e0)
    e0->free();
  if (e1)
    e1->free();
}

struct WebCacheFixtureNoIndex : public WebCacheFixture
{
public:
  WebCacheFixtureNoIndex() : WebCacheFixture(true, false) {}
};
TEST_FIXTURE(WebCacheFixtureNoIndex, WebCacheNoIndexDirectUrl)
{
  datacache::ErrorCode err0 = datacache::ERR_UNKNOWN;
  bool done0 = false;
  const char *url = "http://localhost:8000/tests.cpp";
  G_VERIFY(cache->get(url, &err0, WebCacheFixture::onFileLoaded, &done0) == NULL);
  G_ASSERT(err0 == datacache::ERR_PENDING);
  while (!done0)
  {
    sleep_msec(0);
    cache->poll();
    cpujobs::release_done_jobs();
  }
  datacache::EntryHolder e0 = cache->get(url);
  G_ASSERT(e0.get());
  DagorStat st;
  G_VERIFY(df_stat("tests.cpp", &st) == 0);
  CHECK_EQUAL(st.size, e0->getDataSize());
}

#define LNAME "tests.cpp"
static void no_index_file_loaded(const char *name, datacache::ErrorCode, datacache::Entry *entry, void *arg)
{
  if (entry)
    entry->free();
  CHECK_EQUAL(LNAME, name);
  *(bool *)arg = true;
}
TEST_FIXTURE(WebCacheFixtureNoIndex, WebCacheNoIndexRelativeUrl)
{
  datacache::ErrorCode err0 = datacache::ERR_UNKNOWN;
  bool done0 = false;
  cache->get(LNAME, &err0, no_index_file_loaded, &done0);
  G_ASSERT(err0 == datacache::ERR_PENDING);
  while (!done0)
  {
    sleep_msec(0);
    cache->poll();
    cpujobs::release_done_jobs();
  }
  datacache::EntryHolder e0 = cache->get(LNAME);
  G_ASSERT(e0.get());
  DagorStat st;
  G_VERIFY(df_stat(LNAME, &st) == 0);
  CHECK_EQUAL(st.size, e0->getDataSize());
}

extern void init_main_thread_id();
struct GlobalInit
{
  GlobalInit()
  {
    dd_add_base_path(""); /* otherwise dd_mkdir() doesn't work */
    init_main_thread_id();
    cpujobs::init();
  }
  ~GlobalInit() { cpujobs::term(false); }
};

#define USE_EASTL
#define CUSTOM_UNITTEST_CODE GlobalInit g_init;
#include <unittest/main.inc.cpp>
