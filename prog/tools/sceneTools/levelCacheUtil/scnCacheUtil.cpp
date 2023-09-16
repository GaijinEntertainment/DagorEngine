#include "scnCacheUtil.h"
#include <libTools/util/binDumpReader.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <scene/dag_loadLevelVer.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <hash/md5.h>
#include <debug/dag_debug.h>
#include <stdio.h>

class QuietGenProgressInd : public IGenProgressInd
{
public:
  virtual void setActionDescFmt(const char *, const DagorSafeArg *arg, int anum) {}
  virtual void setTotal(int total_cnt) {}
  virtual void setDone(int done_cnt) {}
  virtual void incDone(int inc = 1) {}
  virtual void destroy() {}
};
static QuietGenProgressInd quiet_pbar;


static bool check_ribb_cache(IGenLoad &crd, const char *ribb_cache_fn, bool &out_err, IGenProgressInd *pbar)
{
  int tag;

  out_err = true;
  if (crd.readInt() != _MAKE4C('DBLD'))
    return false;
  if (crd.readInt() != DBLD_Version)
    return false;
  crd.readInt(); // tex count

  for (;;)
  {
    tag = crd.beginTaggedBlock();

    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      crd.endBlock();
      break;
    }

    if (tag == _MAKE4C('RIBz'))
    {
      struct Hdr
      {
        int sz;
        char md5[16];
      };
      FullFileLoadCB fcrd(ribb_cache_fn);
      Hdr hdr, footer;

      crd.read(&hdr, sizeof(Hdr));

      if (fcrd.fileHandle && df_length(fcrd.fileHandle) == hdr.sz + 4)
      {
        fcrd.seekto(df_length(fcrd.fileHandle) - sizeof(Hdr));
        fcrd.read(&footer, sizeof(Hdr));
        if (memcmp(&hdr, &footer, sizeof(Hdr)) == 0)
        {
          out_err = false;
          crd.endBlock();
          return true;
        }
      }

      fcrd.close();
      dd_mkpath(ribb_cache_fn);
      dd_erase(ribb_cache_fn);

      crd.beginBlock();
      {
        LzmaLoadCB lzma_crd(crd, crd.getBlockRest());
        FullFileSaveCB fcwr(ribb_cache_fn);

        if (!fcwr.fileHandle)
        {
          crd.endBlock();
          return true;
        }

        pbar->setActionDesc("updating cache: %s", ribb_cache_fn);
        copy_stream_to_stream(lzma_crd, fcwr, hdr.sz - sizeof(hdr.md5));
        lzma_crd.ceaseReading();
        df_flush(fcwr.fileHandle);

        fcwr.write(&hdr, sizeof(Hdr));
        df_flush(fcwr.fileHandle);
      }
      crd.endBlock();

      out_err = false;
      crd.endBlock();
      return true;
    }

    crd.endBlock();
  }
  out_err = false;
  return false;
}

static void gather_level_files(FastNameMapEx &files, const char *folder_path)
{
  alefind_t ff;

  if (::dd_find_first(String(260, "%s/*.bin", folder_path), 0, &ff))
  {
    do
    {
      int len = strlen(ff.name);
      if (len > 8 && stricmp(".dxp.bin", &ff.name[len - 8]) == 0)
        continue;
      if (len > 11 && stricmp(".vromfs.bin", &ff.name[len - 11]) == 0)
        continue;

      files.addNameId(String(260, "%s/%s", folder_path, ff.name));
    } while (dd_find_next(&ff));
    dd_find_close(&ff);
  }

  if (::dd_find_first(String(260, "%s/*", folder_path), DA_SUBDIR, &ff))
  {
    do
      if (ff.attr & DA_SUBDIR)
      {
        if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0 || dd_stricmp(ff.name, ".git") == 0)
          continue;

        gather_level_files(files, String(260, "%s/%s", folder_path, ff.name));
      }
    while (dd_find_next(&ff));
    dd_find_close(&ff);
  }
}


bool update_level_caches(const char *cache_dir, const FastNameMapEx &level_dirs, bool clean, IGenProgressInd *pbar, bool quiet)
{
  alefind_t ff;
  FastNameMapEx cache_files;
  FastNameMapEx level_files;

  if (!pbar)
    pbar = &quiet_pbar;

  pbar->setDone(0);
  if (!quiet)
    pbar->setActionDesc("scanning cache files");
  if (::dd_find_first(String(260, "%s/*.*", cache_dir), 0, &ff))
  {
    do
      cache_files.addNameId(String(260, "%s/%s", cache_dir, ff.name));
    while (dd_find_next(&ff));
    dd_find_close(&ff);
  }

  if (!quiet)
    pbar->setActionDesc("scanning level files");
  for (int i = 0; i < level_dirs.nameCount(); i++)
    gather_level_files(level_files, level_dirs.getName(i));

  // for (int i = 0; i < cache_files.nameCount(); i ++)
  //   printf("cache[%2d] %s\n", i, cache_files.getName(i));
  // for (int i = 0; i < level_files.nameCount(); i ++)
  //   printf("level[%2d] %s\n", i, level_files.getName(i));

  FastNameMapEx used_cache_files;
  pbar->setTotal(level_files.nameCount() + 1);
  pbar->setActionDesc("checking/updating level cache");
  int fail_cnt = 0;
  for (int i = 0; i < level_files.nameCount(); i++)
  {
    FullFileLoadCB fcrd(level_files.getName(i));
    if (!quiet)
      pbar->setActionDesc("checking cache for %s", level_files.getName(i));
    pbar->incDone();
    if (!fcrd.fileHandle)
    {
      pbar->setActionDesc("failed to open %s", cache_files.getName(i));
      continue;
    }

    String ribb_cache_fn(260, "%s/%s.ribb", cache_dir, dd_get_fname(fcrd.getTargetName()));
    bool err;

    if (check_ribb_cache(fcrd, ribb_cache_fn, err, pbar))
      used_cache_files.addNameId(ribb_cache_fn);
    else if (err)
    {
      fail_cnt++;
      pbar->setActionDesc("failed to open %s", cache_files.getName(i));
    }
  }

  pbar->incDone();
  if (clean)
    for (int i = 0; i < cache_files.nameCount(); i++)
      if (used_cache_files.getNameId(cache_files.getName(i)) < 0)
      {
        pbar->setActionDesc("removing unused cache %s", cache_files.getName(i));
        dd_erase(cache_files.getName(i));
      }
  pbar->setActionDesc("");
  return !fail_cnt;
}

bool verify_level_caches_files(const char *cache_dir, IGenProgressInd *pbar, bool quiet)
{
  alefind_t ff;
  FastNameMapEx cache_files;

  if (!pbar)
    pbar = &quiet_pbar;

  pbar->setDone(0);
  if (!quiet)
    pbar->setActionDesc("scanning cache files");
  if (::dd_find_first(String(260, "%s/*.*", cache_dir), 0, &ff))
  {
    do
      cache_files.addNameId(String(260, "%s/%s", cache_dir, ff.name));
    while (dd_find_next(&ff));
    dd_find_close(&ff);
  }

  pbar->setTotal(cache_files.nameCount());
  pbar->setActionDesc("checking cache files");
  int fail_cnt = 0;
  for (int i = 0; i < cache_files.nameCount(); i++)
  {
    FullFileLoadCB crd(cache_files.getName(i));
    if (!quiet)
      pbar->setActionDesc("checking %s", cache_files.getName(i));
    pbar->incDone();
    if (!crd.fileHandle)
    {
      pbar->setActionDesc("failed to open %s", cache_files.getName(i));
      continue;
    }

    struct Hdr
    {
      int sz;
      md5_byte_t md5[16];
    };
    Hdr hdr, footer;
    bool cache_valid = false;

    hdr.sz = df_length(crd.fileHandle);
    if (hdr.sz > sizeof(Hdr))
    {
      crd.seekto(hdr.sz - sizeof(Hdr));
      crd.read(&footer, sizeof(Hdr));
      if (hdr.sz == footer.sz + sizeof(footer.sz))
      {
        static const int BUFFER_SIZE = 32 << 10;
        unsigned char buf[BUFFER_SIZE];

        hdr.sz -= sizeof(hdr.sz);
        crd.seekto(0);

        md5_state_t hashState;
        md5_init(&hashState);

        for (int i = hdr.sz - sizeof(hdr.md5); i > 0; i -= BUFFER_SIZE)
          if (int read = crd.tryRead(buf, i > BUFFER_SIZE ? BUFFER_SIZE : i))
            md5_append(&hashState, buf, read);
        md5_finish(&hashState, hdr.md5);

        if (memcmp(&hdr, &footer, sizeof(Hdr)) == 0)
          cache_valid = true;
      }
    }
    crd.close();

    if (!cache_valid)
    {
      fail_cnt++;
      dd_erase(cache_files.getName(i));
      pbar->setActionDesc("removed inconsistent %s", cache_files.getName(i));
    }
  }
  pbar->setActionDesc("");
  return fail_cnt == 0;
}
