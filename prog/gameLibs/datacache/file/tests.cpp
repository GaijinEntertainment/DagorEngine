#include <UnitTest++/UnitTestPP.h>
#include "filebackend.h"
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
#include <time.h>
#include <sys/stat.h>
#include <osApiWrappers/dag_direct.h>
#include "fileutil.h"

struct CacheObj
{
  datacache::Backend *cache;
  CacheObj(const char *path = ".", int max_size = 0, const char *ro_mnt_path = NULL)
  {
    DataBlock params;
    params.setStr("mountPath", path);
    if (ro_mnt_path)
      params.setStr("roMountPath", ro_mnt_path);
    params.setInt64("maxSize", max_size);
    params.setInt("traceLevel", 0);
    cache = datacache::FileBackend::create(params);
  }
  ~CacheObj() { delete cache; }
  datacache::Backend *operator->() { return cache; }
};

static void gen_file(const char *path, int i, int size)
{
  char buf[DAGOR_MAX_PATH];
  SNPRINTF(buf, sizeof(buf), "%s/%d.bin", path, i);
  FILE *f = fopen(buf, "wb");
  G_ASSERT(f);
  char zero = 0;
  for (int i = 0; i < size; ++i)
    fwrite(&zero, 1, 1, f);
  fclose(f);
}

TEST(CacheEnumerate)
{
  CacheObj cache;
  Tab<alefind_t> files;
  char tmpBuf[DAGOR_MAX_PATH];
  find_files_recursive(".", files, tmpBuf);
  void *iter = NULL;
  int cnt = 0;
  for (datacache::EntryHolder e = cache->nextEntry(&iter); e; e = cache->nextEntry(&iter))
    ++cnt;
  cache->endEnumeration(&iter);
  CHECK_EQUAL(files.size(), cnt);
}

TEST(CacheHit)
{
  CacheObj cache;
  datacache::EntryHolder entry = cache->get("tests.cpp");
  CHECK_EQUAL(0, strcmp(entry->getKey(), "tests.cpp"));
}

TEST(CacheMiss)
{
  CacheObj cache;
  datacache::EntryHolder entry = cache->get("faewfawe");
  CHECK_EQUAL((datacache::Entry *)NULL, entry.get());
}

static const char dataRef[] = "#include <UnitTest++/UnitTestPP.h>";
TEST(CacheMMAPData)
{
  CacheObj cache;
  datacache::EntryHolder entry = cache->get("tests.cpp");
  CHECK_EQUAL(0, memcmp(&entry->getData()[0], dataRef, sizeof(dataRef) - 1));
}

TEST(CacheReadData)
{
  CacheObj cache;
  datacache::EntryHolder entry = cache->get("tests.cpp");
  char buf[sizeof(dataRef) - 1];
  CHECK_EQUAL(sizeof(buf), entry->getReadStream()->tryRead(buf, sizeof(buf)));
  CHECK_EQUAL(0, memcmp(buf, dataRef, sizeof(buf)));
}

TEST(SetModTime)
{
  CacheObj cache;
  {
    datacache::EntryHolder(cache->set("test.bin", 100500))->getWriteStream();
  }
  struct stat st;
  G_VERIFY(stat("test.bin", &st) == 0);
  CHECK_EQUAL(100500, st.st_mtime);
  cache->del("test.bin");
}

TEST(DirPath)
{
  CacheObj cache;
  const char *longPath = "subdir/test.bin";
  {
    datacache::EntryHolder(cache->set(longPath))->getWriteStream();
  }
  {
    datacache::Entry *entry = cache->get(longPath);
    CHECK(entry);
    entry->del();
    entry->free();
  }
  G_ASSERT(rmdir("subdir") == 0);
}

static const char garbagef[] = ".#test.bin";
TEST(GarbageFileWithOpenedFiles)
{
  fclose(fopen(garbagef, "wb"));
  CacheObj cache;
  datacache::Entry *e = cache->get("tests.cpp");
  cache->getEntriesCount(); // populate
  e->free();
  struct stat st;
  CHECK_EQUAL(0, stat(garbagef, &st));
  dd_erase(garbagef);
}

TEST(GarbageFileWithoutOpenedFiles)
{
  fclose(fopen(garbagef, "wb"));
  CacheObj cache;
  cache->getEntriesCount(); // populate
  struct stat st;
  CHECK_EQUAL(-1, stat(garbagef, &st));
  dd_erase(garbagef);
}

TEST(EnumerateDirPath)
{
  const char *name = "subdir/test.bin";
  dd_mkdir("subdir");
  fclose(fopen(name, "wb")); // touch
  {
    CacheObj cache;
    void *iter = NULL;
    bool found = false;
    for (datacache::Entry *entry = cache->nextEntry(&iter); entry; entry = cache->nextEntry(&iter))
    {
      found = strcmp(entry->getKey(), name) == 0;
      entry->free();
      if (found)
        break;
    }
    cache->endEnumeration(&iter);
    G_ASSERT(found);
  }
  remove(name);
  rmdir("subdir");
}

TEST(RecursiveEntry)
{
  CacheObj cache;
  datacache::EntryHolder ent = cache->get("tests.cpp");
  ent = cache->get("tests.cpp");
}

TEST(EntryHolderAssignment)
{
  CacheObj cache;
  datacache::EntryHolder ent1 = cache->get("tests.cpp");
  datacache::EntryHolder ent2 = cache->get("jamfile");
  ent1 = ent2;
}

TEST(EntryHolderSelfAssignment)
{
  CacheObj cache;
  datacache::EntryHolder ent = cache->get("tests.cpp");
  ent = ent;
}

TEST(EntryHolderGetAssignment)
{
  CacheObj cache;
  datacache::EntryHolder ent = cache->get("tests.cpp");
  ent = cache->get("jamfile");
}

TEST(UnlinkedEntry)
{
  CacheObj cache;
  datacache::EntryHolder entry = cache->get("tests.cpp");
  cache->getEntriesCount(); // implicit populate
}

TEST(RecursiveDel)
{
  const char *n = "test.bin";
  struct stat st;
  fclose(fopen(n, "wb")); // touch
  CacheObj cache;
  datacache::Entry *e1 = cache->get(n);
  e1->del();
  datacache::Entry *e2 = cache->get(n);
  e1->free();
  CHECK_EQUAL(0, stat(n, &st)); // should been removed on last free
  e2->free();
  G_ASSERT(remove(n) != 0);
}

TEST(RecursiveDelUnlinkedEntry)
{
  const char *n = "test.bin";
  fclose(fopen(n, "wb")); // touch
  CacheObj cache;
  datacache::Entry *e1 = cache->get(n);
  e1->del();
  datacache::Entry *e2 = cache->get(n);
  cache->getEntriesCount(); // populate
  e1->free();
  e2->free();
  G_ASSERT(remove(n) != 0);
}

struct EvictionFixture
{
#define EVPATH "eviction"
  CacheObj cache;
  EvictionFixture(int sz = 5 << 10, int cnt = 4, bool wait = true) : cache(EVPATH, sz)
  {
    time_t curTime = time(NULL);
    for (int i = 0; i < cnt; ++i)
    {
      gen_file(EVPATH, i + 1, 1024);
      while (wait && i == 0 && time(NULL) == curTime) // wait until next sec
        sleep_msec(0);
    }
  }
  ~EvictionFixture()
  {
    cache->delAll();
    G_ASSERT(rmdir(EVPATH "/subdir") == 0);
    G_ASSERT(rmdir(EVPATH) == 0);
  }
};
TEST_FIXTURE(EvictionFixture, EvictionOnWrite)
{
  char dummy[1536];
  CHECK_EQUAL(4, cache->getEntriesCount());
  {
    datacache::EntryHolder(cache->set("subdir/5.bin"))->getWriteStream()->write(dummy, sizeof(dummy));
  }
  CHECK_EQUAL((datacache::Entry *)NULL, cache->get("1.bin")); // should been evicted
  CHECK_EQUAL(4, cache->getEntriesCount());
  {
    datacache::EntryHolder(cache->set("subdir/6.bin"))->getWriteStream()->write(dummy, 512); // should not be evicted
  }
  CHECK_EQUAL(5, cache->getEntriesCount());
}

struct EvictionFixture2 : public EvictionFixture
{
  EvictionFixture2() : EvictionFixture(4 << 10, 4, false) {}
};
TEST_FIXTURE(EvictionFixture2, EvictionOnDel)
{
  char dummy[1024];
  CHECK_EQUAL(4, cache->getEntriesCount());
  {
    datacache::EntryHolder(cache->get("1.bin"))->del();
  }
  CHECK_EQUAL(3, cache->getEntriesCount());
  {
    datacache::EntryHolder(cache->set("subdir/5.bin"))->getWriteStream()->write(dummy, sizeof(dummy));
  } // should not been evicted
  CHECK_EQUAL(4, cache->getEntriesCount());
}

struct RoMountFixture
{
  CacheObj cache;
  time_t curTime;
  RoMountFixture() : cache("whatever", 0, "romount")
  {
    dd_mkdir("romount");
    curTime = time(NULL);
    gen_file("romount", 1, 1024);
  }
  ~RoMountFixture()
  {
    dd_erase("romount/1.bin");
    dd_rmdir("romount");
    dd_rmdir("whatever");
  }
};

TEST_FIXTURE(RoMountFixture, RoMountBasic)
{
  datacache::EntryHolder ent = cache->get("1.bin");
  CHECK(ent.get());
}

TEST_FIXTURE(RoMountFixture, RoMountDifferentFiles)
{
  datacache::EntryHolder ent1 = cache->get("1.bin");
  datacache::EntryHolder ent2 = cache->set("1.bin");
  CHECK(ent1.get() != ent2.get());
}

TEST_FIXTURE(RoMountFixture, RoMountCheckWriteOnGet)
{
  datacache::EntryHolder ent = cache->get("1.bin");
  CHECK_ASSERT(ent->getWriteStream());
}

struct RoMountFixtureEx : public RoMountFixture
{
  RoMountFixtureEx()
  {
    while (time(NULL) == curTime) // wait until next sec
      sleep_msec(0);
    gen_file("whatever", 1, 512);
  }
  ~RoMountFixtureEx() { dd_erase("whatever/1.bin"); }
};

TEST_FIXTURE(RoMountFixtureEx, RoMountCheckGetPrioSingle)
{
  datacache::EntryHolder ent = cache->get("1.bin");
  CHECK_EQUAL(512, ent->getDataSize());
}

TEST_FIXTURE(RoMountFixtureEx, RoMountCheckGetPrioPopulate)
{
  cache->getEntriesCount(); // populate
  datacache::EntryHolder ent = cache->get("1.bin");
  CHECK_EQUAL(512, ent->getDataSize());
}

#define USE_EASTL
#define CUSTOM_UNITTEST_CODE dd_add_base_path("");
#include <unittest/main.inc.cpp>
