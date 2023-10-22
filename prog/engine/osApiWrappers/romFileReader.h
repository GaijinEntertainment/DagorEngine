#pragma once

#include <generic/dag_span.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_stdint.h>
#include <stdio.h>
#include <osApiWrappers/dag_fileIoErr.h>
#if _TARGET_ANDROID
#include <osApiWrappers/dag_progGlobals.h>
#include <supp/dag_android_native_app_glue.h>
#include <android/asset_manager.h>
#endif

class RomFileReader
{
public:
  RomFileReader() : data(NULL), size(-1), pos(0) {}

  int read(void *buf, int sz)
  {
    if (sz <= 0 || pos >= size)
      return 0;
    if (pos + sz > size)
      sz = size - pos;

    if (fp)
    {
    retry:
      int ret = (int)fread(buf, 1, sz, fp);
      if (ret != sz)
      {
        int ofs = ftell(fp) - (ret > 0 ? ret : 0);
        bool doRetry = false;
        if (feof(fp))
        {
          if (dag_on_read_beyond_eof_cb)
            doRetry = dag_on_read_beyond_eof_cb(fp, ofs, sz, ret);
        }
        else
        {
          if (dag_on_read_error_cb)
            doRetry = dag_on_read_error_cb(fp, ofs, sz);
        }

        if (doRetry)
        {
          fseek(fp, ofs, SEEK_SET);
          goto retry;
        }
      }
      pos = ftell(fp) - fpOfs;
      return ret;
    }

    memcpy(buf, data + pos, sz);
    pos += sz;
    return sz;
  }
  char *gets(char *buf, int max_sz)
  {
    if (fp)
    {
      if (pos >= size)
        return NULL;
      buf = fgets(buf, max_sz, fp);
      int rd = ftell(fp) - fpOfs - pos;
      if (pos + rd > size)
      {
        if (buf)
          buf[size - pos] = 0;
        fseek(fp, (pos = size) + fpOfs, SEEK_SET);
      }
      else
        pos += rd;
      return buf;
    }

    if (pos + max_sz > size)
      max_sz = size - pos;
    if (max_sz < 1)
      return NULL;

    char *p = (char *)memchr(data + pos, '\n', max_sz - 1);
    if (p)
      max_sz = p - (data + pos);

    memcpy(buf, data + pos, max_sz);
    buf[max_sz] = 0;
    pos += max_sz + 1;
    return buf;
  }

  int seekTo(int ofs_abs)
  {
    if (ofs_abs < 0 || ofs_abs > size)
      return -1;
    pos = ofs_abs;
    if (fp)
      return fseek(fp, pos + fpOfs, SEEK_SET);
    return 0;
  }
  int seekRel(int ofs_rel) { return seekTo(pos + ofs_rel); }
  int seekEnd(int ofs_end) { return seekTo(size + ofs_end); }

  int tell() const { return pos; }
  int getLength() const { return size; }

  void *mmap(int *base, int offset, int *out_file_len, FILE **out_fp)
  {
    *out_file_len = size;
    if (fp)
    {
      *out_fp = fp;
      *base = fpOfs;
      return NULL;
    }
    else
      return (offset >= size) ? NULL : (void *)((const char *)data + offset);
  }

public:
  static constexpr int MAX_FILE_RD = 128;
  static RomFileReader rdPool[MAX_FILE_RD];
  static WinCritSec critSec;

  static int getBaseOfs(dag::ConstSpan<char> p)
  {
    int ofs = ptrdiff_t(p.data());
    return ofs > 0 ? ofs : 0;
  }
  static RomFileReader *open(dag::ConstSpan<char> p, VirtualRomFsPack *pack, VirtualRomFsData *vrom_src)
  {
    int i;
    {
      WinAutoLock lock(critSec);
      for (i = 0; i < MAX_FILE_RD; i++)
        if (rdPool[i].size < 0)
        {
          rdPool[i].data = pack ? NULL : p.data();
          rdPool[i].size = p.size();
          break;
        }
    }

    if (i < MAX_FILE_RD)
    {
      if (pack)
      {
        rdPool[i].fp = NULL;
        const char *fn = pack->getFilePath();
        if (fn)
          rdPool[i].fp = fopen(fn, "rb");
        if (rdPool[i].fp)
        {
          fseek(rdPool[i].fp, rdPool[i].fpOfs = getBaseOfs(p), SEEK_SET);
          if (dag_on_file_open)
            dag_on_file_open(fn, rdPool[i].fp, DF_READ); // df_close will call dag_on_file_close
        }
        else
        {
          rdPool[i].size = -1;
          return NULL;
        }
      }
      else
      {
        rdPool[i].fp = NULL;
        rdPool[i].fpOfs = 0;
      }
      rdPool[i].pos = 0;
      rdPool[i].srcVrom = vrom_src;
      return &rdPool[i];
    }
    return NULL;
  }
  static RomFileReader *openB64(const char *data_b64)
  {
    RomFileReader *h = nullptr;
    {
      WinAutoLock lock(critSec);
      for (RomFileReader &rfr : rdPool)
        if (rfr.size < 0)
        {
          h = &rfr;
          rfr.size = 0;
          break;
        }
      if (!h)
        return h;
    }

    const char *data_b64_end = strchr(data_b64, '.');
    if (!data_b64_end)
      data_b64_end = data_b64 + strlen(data_b64);

    h->data = decodeB64(data_b64, data_b64_end, &h->size);
    h->fp = NULL;
    h->fpOfs = 0xAAAA5555;
    h->pos = 0;
    h->srcVrom = nullptr;
    return h;
  }
#if _TARGET_ANDROID
  static RomFileReader *openAsset(const char *fpath)
  {
    android_app *state = (android_app *)win32_get_instance();
    if (!state)
      return nullptr;
    AAsset *a = AAssetManager_open(state->activity->assetManager, fpath, AASSET_MODE_BUFFER);
    if (!a)
      return nullptr;

    RomFileReader *h = nullptr;
    {
      WinAutoLock lock(critSec);
      for (RomFileReader &rfr : rdPool)
        if (rfr.size < 0)
        {
          h = &rfr;
          rfr.size = 0;
          break;
        }
      if (!h)
        return h;
    }

    h->size = AAsset_getLength(a);
    h->data = (const char *)AAsset_getBuffer(a);
    h->fp = NULL;
    h->fpOfs = 0;
    h->pos = 0;
    h->srcVrom = nullptr;
    h->asset = a;
    return h;
  }
  static int getAssetSize(const char *fpath)
  {
    if (android_app *state = (android_app *)win32_get_instance())
      if (AAsset *a = AAssetManager_open(state->activity->assetManager, fpath, AASSET_MODE_UNKNOWN))
      {
        int sz = AAsset_getLength(a);
        AAsset_close(a);
        return sz;
      }
    return -1;
  }
#else
  static RomFileReader *openAsset(const char *) { return nullptr; }
  static int getAssetSize(const char *) { return -1; }
#endif
  static bool close(RomFileReader *rd)
  {
    WinAutoLock lock(critSec);
    if (rd < &rdPool[0] || rd >= &rdPool[MAX_FILE_RD])
      return false;

    int i = rd - &rdPool[0];
    if (rdPool[i].size < 0)
      return false;

    if (!rdPool[i].fp && rdPool[i].fpOfs == 0xAAAA5555)
      free((void *)rdPool[i].data);
    rdPool[i].data = NULL;
    rdPool[i].size = -1;
    rdPool[i].pos = 0;
    rdPool[i].srcVrom = nullptr;
    if (rdPool[i].fp)
    {
      df_close(rdPool[i].fp);
      rdPool[i].fp = NULL;
    }
#if _TARGET_ANDROID
    if (rdPool[i].asset)
    {
      AAsset_close(rdPool[i].asset);
      rdPool[i].asset = nullptr;
    }
#endif
    return true;
  }

  static bool munmap(const void *start)
  {
    WinAutoLock lock(critSec);
    for (int i = 0; i < MAX_FILE_RD; ++i)
      if (rdPool[i].data && (const char *)start >= rdPool[i].data && start < (rdPool[i].data + rdPool[i].size))
        return true;
    return false;
  }

  static char *decodeB64(const char *data, const char *data_end, int *out_sz)
  {
    int sz = (data_end - data) / 4 * 3;
    char *buf = (char *)malloc(sz);
    if (!buf)
    {
      *out_sz = 0;
      return nullptr;
    }

    if (data_end - 1 > data && *(data_end - 1) == '=')
    {
      sz--;
      if (data_end - 2 > data && *(data_end - 2) == '=')
        sz--;
    }
    *out_sz = sz;

    for (uint8_t *p = (uint8_t *)buf; data < data_end; p += 3, data += 4)
    {
      uint8_t b6[4];
      for (int i = 0; i < 4; i++)
      {
        uint8_t C = data[i];
        if (C >= 'A' && C < 'A' + 26)
          b6[i] = C - 'A';
        else if (C >= 'a' && C < 'a' + 26)
          b6[i] = C - 'a' + 26;
        else if (C >= '0' && C < '0' + 10)
          b6[i] = C - '0' + 52;
        else
          switch (C)
          {
            case '+': b6[i] = 62; break;
            case '/': b6[i] = 63; break;
            default: b6[i] = 0;
          }
      }

      p[0] = (b6[0] << 2) | (b6[1] >> 4);
      p[1] = (b6[1] << 4) | (b6[2] >> 2);
      p[2] = (b6[2] << 6) | b6[3];
    }
    return buf;
  }

  const VirtualRomFsData *getSrcVrom() const { return srcVrom; }
  const char *getFileData(int &out_data_sz) const
  {
    out_data_sz = size;
    return !fp ? data : nullptr;
  }

protected:
  const char *data;
  int size, pos;
  FILE *fp;
  int fpOfs;
  const VirtualRomFsData *srcVrom;
#if _TARGET_ANDROID
  AAsset *asset = nullptr;
#endif
};


inline file_ptr_t to_file_ptr_t(RomFileReader *r) { return (file_ptr_t)(r ? uintptr_t(r) | 0x1 : 0); }

inline RomFileReader *to_rom_file_reader(file_ptr_t fp)
{
  uintptr_t ptr = uintptr_t(fp);
  return (ptr & 0x1) ? (RomFileReader *)(ptr & ~0x1) : NULL;
}
