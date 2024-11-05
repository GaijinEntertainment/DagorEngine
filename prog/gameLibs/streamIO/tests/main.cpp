// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <UnitTest++/UnitTestPP.h>
#include <streamIO/streamIO.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <debug/dag_logSys.h>
#include <sys/stat.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#if _TARGET_PC_WIN
#include <io.h>
#endif
#define CURL_STATICLIB
#include <curl/curl.h>

static streamio::Context *ctx = NULL;
struct SyncCtx
{
  int err;
  IGenLoad **ptr;
  int64_t lastModified;
  volatile bool done;
};

static void on_file_read_cb(const char *, int err, IGenLoad *ptr, void *arg, int64_t last_modified, intptr_t /*req_id*/)
{
  SyncCtx *sctx = (SyncCtx *)arg;
  sctx->err = err;
  *sctx->ptr = ptr;
  sctx->lastModified = last_modified;
  sctx->done = true;
}

static int open_stream(const char *url, IGenLoad **load, int64_t modified_since = -1, int64_t *last_modified = NULL)
{
  SyncCtx sctx = {-1, load, -1, false};
  ctx->createStream(url, on_file_read_cb, nullptr, nullptr, nullptr, &sctx, modified_since);
  while (!sctx.done)
  {
    sleep_msec(1);
    cpujobs::release_done_jobs();
    ctx->poll();
  }
  if (last_modified)
    *last_modified = sctx.lastModified;
  return sctx.err;
}

static void test_read(IGenLoad *load)
{
  char buf[40] = {0};
  CHECK(load);
  CHECK(load && load->tryRead(buf, sizeof(buf) - 1) == sizeof(buf) - 1);
  CHECK(strstr(buf, "UnitTestPP.h"));
}

struct FileTestFixture
{
  IGenLoad *load;
  FileTestFixture() : load(NULL) { open_stream("main.cpp", &load); }
  ~FileTestFixture() { delete load; }
};
TEST_FIXTURE(FileTestFixture, FileRead) { test_read(load); }

struct HttpTestFixture // python -m SimpleHTTPServer
{
  IGenLoad *load;
  int64_t lastModified;
  HttpTestFixture() : load(NULL), lastModified(-1) { open_stream("http://localhost:8000/main.cpp", &load, -1, &lastModified); }
  ~HttpTestFixture() { delete load; }
};
TEST_FIXTURE(HttpTestFixture, HttpRead) { test_read(load); }

TEST(BadUrl)
{
  IGenLoad *load = NULL;
  int err = open_stream("http://localhost:8000/bad_url", &load);
  CHECK(err != 0 && !load);
}

TEST(BadFile)
{
  IGenLoad *load = NULL;
  int err = open_stream("bad_file", &load);
  CHECK(err != 0 && !load);
}

TEST_FIXTURE(HttpTestFixture, LastModified)
{
  struct stat st = {0};
  G_VERIFY(stat("main.cpp", &st) == 0);
  CHECK_EQUAL(st.st_mtime, (time_t)lastModified);
}

// python's SimpleHTTPServer doen't support If-Modified-Since, so we have to rely to external content here
/*TEST(ModifiedSince)
{
  const char *lastModified = "Wed, 15 Apr 2015 22:16:42 GMT";
  IGenLoad *load = NULL;
  int ret = open_stream("http://93610467.r.cdn77.net/warthunder_1.47.11.62.yup", &load, curl_getdate(lastModified, NULL));
  CHECK_EQUAL(streamio::ERR_NOT_MODIFIED, ret);
  CHECK_EQUAL((IGenLoad*)NULL, load);
  delete load;
}*/

struct GlobalInit
{
  GlobalInit()
  {
#if 0 // uncomment this if you want to see debugs in stdout
    dup(_fileno(stdout));
    out_debug_console_handle = _get_osfhandle(_fileno(stdout));
    start_classic_debug_system("NUL");
#endif
#if _TARGET_PC_LINUX
    set_debug_console_handle((intptr_t)stdout);
#endif

    dd_add_base_path("");
    dd_init_local_conv();
    cpujobs::init();
    ctx = streamio::create();
  }
  ~GlobalInit()
  {
    delete ctx;
    cpujobs::term(false);
    close_debug_files();
  }
};

#define CUSTOM_UNITTEST_CODE GlobalInit g_init;
#include <unittest/main.inc.cpp>
