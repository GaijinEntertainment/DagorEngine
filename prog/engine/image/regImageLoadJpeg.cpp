// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_jpeg.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_files.h>
#include <math/dag_mathBase.h>

// could be easily changed to accept this as parameter in runtime
#if _TARGET_PC
#define CACHE_THRESHOLD 0 // PC already have embedded cache
#else
#define CACHE_THRESHOLD (128 << 10)
#endif

class CachedFullFileLoadCB : public FullFileLoadCB
{
private:
  char *cacheBuf;
  uint32_t curOffs, fileLen;
  using FullFileLoadCB::fileHandle; // May not be used to check status if CACHE_THRESHOLD is enabled.

public:
  CachedFullFileLoadCB(const char *fname) : FullFileLoadCB(fname), cacheBuf(NULL), curOffs(0), fileLen(0)
  {
    if (fileHandle == NULL)
      return;
    fileLen = (uint32_t)df_length(fileHandle);
    if (fileLen && fileLen <= CACHE_THRESHOLD)
    {
      cacheBuf = (char *)memalloc(fileLen, tmpmem);
      int r = df_read(fileHandle, cacheBuf, fileLen);
      G_ASSERT(r == fileLen);
      df_close(fileHandle);
      fileHandle = NULL;
    }
  }

  ~CachedFullFileLoadCB()
  {
    if (cacheBuf)
      memfree((char *)cacheBuf, tmpmem);
  }

  bool isOK() { return cacheBuf || fileHandle; }

  void read(void *ptr, int size)
  {
    if (!cacheBuf)
      return FullFileLoadCB::read(ptr, size);
    int r = tryRead(ptr, size);
    G_ASSERT(r == size);
  }

  int tryRead(void *ptr, int size)
  {
    if (!cacheBuf)
      return FullFileLoadCB::tryRead(ptr, size);
    if (curOffs < fileLen)
    {
      uint32_t ret = min(fileLen - curOffs, (uint32_t)size);
      memcpy(ptr, cacheBuf + curOffs, ret);
      curOffs += ret;
      return ret;
    }
    return 0;
  }

  int tell() { return cacheBuf ? (int)curOffs : FullFileLoadCB::tell(); }
  void seekto(int w) // absolute position
  {
    if (!cacheBuf)
      return FullFileLoadCB::seekto(w);
    G_ASSERT(w >= 0);
    if (w >= 0)
      curOffs = min<uint32_t>(w, fileLen);
  }

  void seekrel(int w) // relative to current
  {
    if (!cacheBuf)
      return FullFileLoadCB::seekrel(w);
    uint32_t a = min<uint32_t>(curOffs + (uint32_t)w, fileLen);
    uint32_t b = (uint32_t)max<int>(0, (int)curOffs - w);
    curOffs = w >= 0 ? a : b;
  }
};

class JpegLoadImageFactory : public ILoadImageFactory
{
public:
  static inline bool checkFnExt(const char *ext) { return ext && (dd_stricmp(ext, ".jpg") == 0 || dd_stricmp(ext, ".jpeg") == 0); }
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    out_may_have_alpha = false;
    return checkFnExt(fn_ext) ? read_jpeg32_dimensions(fn, out_w, out_h) : false;
  }
  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    TexImage32 *im = checkFnExt(fn_ext) ? load_jpeg32(fn, mem) : nullptr;
    if (im && out_used_alpha)
      *out_used_alpha = false;
    return im;
  }
  TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    TexImage32 *im = checkFnExt(fn_ext) ? load_jpeg32(crd, mem) : nullptr;
    if (im && out_used_alpha)
      *out_used_alpha = false;
    return im;
  }
  bool supportLoadImage2() override { return true; }
  void *loadImage2(const char *fn, IAllocImg &a, const char *fn_ext) override
  {
    if (!checkFnExt(fn_ext))
      return nullptr;
    CachedFullFileLoadCB crd(fn);
    return crd.isOK() ? load_jpeg32(crd, a) : nullptr;
  }
  void *loadImage2(IGenLoad &crd, IAllocImg &a, const char *fn_ext) override
  {
    return checkFnExt(fn_ext) ? load_jpeg32(crd, a) : nullptr;
  }
};

static JpegLoadImageFactory jpeg_load_image_factory;

void register_jpeg_tex_load_factory() { add_load_image_factory(&jpeg_load_image_factory); }
