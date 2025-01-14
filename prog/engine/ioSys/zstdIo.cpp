// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_zstdIo.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_physMem.h>
#define ZSTD_STATIC_LINKING_ONLY  1
#define ZDICT_STATIC_LINKING_ONLY 1
#include <zstd.h>
#include <dictBuilder/zdict.h>
#include <memory/dag_framemem.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_span.h>

#define CHECK_ERROR(enc_size) \
  if (ZSTD_isError(enc_sz))   \
    logerr("%s err=%08X %s srcSz=%d", __FUNCTION__, enc_sz, ZSTD_getErrorName(enc_sz), srcSize);

enum ZstdAllocMethod
{
  TryAlloc = -1,
  PhysMem,
  Alloc
};

template <IMemAlloc *(mem_ptr)()>
static void *zstd_alloc(void *, size_t size)
{
#if _TARGET_STATIC_LIB
  using namespace dagor_phys_memory;
  size += sizeof(size_t) * 2; // To consider: may be alignment 8 is enough?
  // There is no much point to fallback to phys mem for small allocations - if we can't alloc at least
  // this much memory we will likely fail later anyway
  int i = size < (512 << 10) ? Alloc : TryAlloc;
  void *ptr = nullptr;
  for (; !ptr; ++i)
    switch (i)
    {
      case TryAlloc: ptr = mem_ptr()->tryAlloc(size); break;
      case PhysMem: ptr = alloc_phys_mem(size, PM_ALIGN_PAGE, PM_PROT_CPU_ALL, /*cpu*/ true, /*logfail*/ false); break;
      default: ptr = mem_ptr()->alloc(size);
    }
  *(size_t *)ptr = --i;
  return (size_t *)ptr + 2;
#else
  return mem_ptr()->alloc(size);
#endif
}
template <IMemAlloc *(mem_ptr)()>
static void zstd_free(void *, void *ptr)
{
#if _TARGET_STATIC_LIB
  size_t *pPtr = (size_t *)ptr - 2;
  G_FAST_ASSERT(*pPtr == TryAlloc || *pPtr == PhysMem || *pPtr == Alloc);
  if (*pPtr != PhysMem)
    mem_ptr()->free(pPtr);
  else
    dagor_phys_memory::free_phys_mem(pPtr);
#else
  mem_ptr()->free(ptr);
#endif
}
static ZSTD_customMem const ZSTD_dagorCMem = {&zstd_alloc<tmpmem_ptr>, &zstd_free<tmpmem_ptr>, NULL};
static ZSTD_customMem const ZSTD_framememCMem = {&zstd_alloc<framemem_ptr>, &zstd_free<framemem_ptr>, NULL};

size_t zstd_compress_bound(size_t srcSize) { return ZSTD_compressBound(srcSize); }
size_t zstd_compress(void *dst, size_t maxDstSize, const void *src, size_t srcSize, int compressionLevel, unsigned max_window_log)
{
  ZSTD_CCtx *ctx = zstd_create_cctx();
  if (max_window_log)
    ZSTD_CCtx_setParameter(ctx, ZSTD_c_windowLog, max_window_log);
  size_t enc_sz = ZSTD_compressCCtx(ctx, dst, maxDstSize, src, srcSize, compressionLevel);
  zstd_destroy_cctx(ctx);
  CHECK_ERROR(enc_sz);
  return enc_sz;
}
size_t zstd_decompress(void *dst, size_t maxOriginalSize, const void *src, size_t srcSize)
{
  size_t enc_sz = ZSTD_decompress(dst, maxOriginalSize, src, srcSize);
  CHECK_ERROR(enc_sz);
  return enc_sz;
}
size_t zstd_compress_with_dict(ZSTD_CCtx *ctx, void *dst, size_t dstSize, const void *src, size_t srcSize, const ZSTD_CDict *dict)
{
  size_t enc_sz = ZSTD_compress_usingCDict(ctx, dst, dstSize, src, srcSize, dict);
  CHECK_ERROR(enc_sz);
  return enc_sz;
}
size_t zstd_decompress_with_dict(ZSTD_DCtx *ctx, void *dst, size_t dstSize, const void *src, size_t srcSize, const ZSTD_DDict *dict)
{
  size_t enc_sz = ZSTD_decompress_usingDDict(ctx, dst, dstSize, src, srcSize, dict);
  CHECK_ERROR(enc_sz);
  return enc_sz;
}

int64_t zstd_compress_data_solid(IGenSave &dest, IGenLoad &src, const size_t sz, int compressionLevel, unsigned max_window_log)
{
  SmallTab<uint8_t, TmpmemAlloc> srcBuf, dstBuf;
  clear_and_resize(srcBuf, sz);
  clear_and_resize(dstBuf, ZSTD_compressBound(sz));
  src.readTabData(srcBuf);
  size_t enc_sz = zstd_compress(dstBuf.data(), data_size(dstBuf), srcBuf.data(), data_size(srcBuf), compressionLevel, max_window_log);
  if (ZSTD_isError(enc_sz))
    return (int)enc_sz;
  dest.write(dstBuf.data(), enc_sz);
  return enc_sz;
}

static int64_t zstd_stream_compress_data_base(IGenSave &dest, IGenLoad &src, const int64_t sz, int compressionLevel,
  unsigned max_window_log, const ZSTD_CDict_s *dict = nullptr)
{
  ZSTD_CStream *strm = ZSTD_createCStream_advanced(ZSTD_dagorCMem);
  ZSTD_inBuffer inBuf;
  ZSTD_outBuffer outBuf;

  ZSTD_initCStream_srcSize(strm, compressionLevel, sz >= 0 ? sz : ZSTD_CONTENTSIZE_UNKNOWN);
  if (max_window_log)
    ZSTD_CCtx_setParameter(strm, ZSTD_c_windowLog, max_window_log);
  const size_t outBufStoreSz = ZSTD_CStreamOutSize(), inBufStoreSz = min(sz >= 0 ? (size_t)sz : outBufStoreSz, outBufStoreSz);
  SmallTab<uint8_t, TmpmemAlloc> tempBuf;
  clear_and_resize(tempBuf, inBufStoreSz + outBufStoreSz);
  uint8_t *inBufStore = tempBuf.data(), *outBufStore = tempBuf.data() + inBufStoreSz;
  G_VERIFY(ZSTD_CCtx_refCDict(strm, dict) == 0);

  inBuf.src = inBufStore;
  inBuf.size = inBufStoreSz;
  inBuf.pos = 0;
  outBuf.dst = outBufStore;
  outBuf.size = outBufStoreSz;
  outBuf.pos = 0;

  const size_t tryReadSz = (sz >= 0 && sz < inBufStoreSz) ? sz : inBufStoreSz;
  inBuf.size = src.tryRead(inBufStore, tryReadSz);
  size_t sizeLeft = sz >= 0 ? sz : 1;
  if (sz >= 0)
    sizeLeft -= inBuf.size;
  if (inBuf.size < tryReadSz)
    sizeLeft = 0;
  size_t enc_sz = 0;
  while (sizeLeft + inBuf.size > inBuf.pos)
  {
    size_t ret = ZSTD_compressStream(strm, &outBuf, &inBuf);
    if (ZSTD_isError(ret))
    {
      ZSTD_freeCStream(strm);
      return (int)ret;
    }
    if (inBuf.pos == inBuf.size && sizeLeft > 0)
    {
      const size_t tryReadSz = (sz >= 0 && sizeLeft < inBufStoreSz) ? sizeLeft : inBufStoreSz;
      inBuf.size = src.tryRead(inBufStore, tryReadSz);
      if (sz >= 0)
        sizeLeft -= inBuf.size;
      if (inBuf.size < tryReadSz)
        sizeLeft = 0;
      inBuf.pos = 0;
    }
    if (outBuf.pos == outBuf.size || (!sizeLeft && outBuf.pos))
    {
      enc_sz += outBuf.pos;
      dest.write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
  }
  for (;;)
  {
    const size_t ret = ZSTD_endStream(strm, &outBuf);
    if (outBuf.pos)
    {
      enc_sz += outBuf.pos;
      dest.write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
    if (ZSTD_isError(ret))
    {
      logerr("err=%08X %s", ret, ZSTD_getErrorName(ret));
      ZSTD_freeCStream(strm);
      return -1;
    }
    if (ret == 0)
      break;
  }
  ZSTD_freeCStream(strm);
  return enc_sz;
}

int64_t zstd_stream_compress_data(IGenSave &dest, IGenLoad &src, const size_t sz, int compression_level, unsigned max_window_log)
{
  return zstd_stream_compress_data_base(dest, src, sz, compression_level, max_window_log);
}

static int64_t zstd_stream_decompress_data_base(IGenSave &dest, IGenLoad &src, const size_t compr_sz,
  const ZSTD_DDict_s *dict = nullptr)
{
  ZSTD_DStream *dstrm = ZSTD_createDStream_advanced(ZSTD_framememCMem);
  ZSTD_inBuffer inBuf;
  ZSTD_outBuffer outBuf;
  const size_t outBufStoreSz = ZSTD_DStreamOutSize(), inBufStoreSz = min(compr_sz != 0 ? compr_sz : 65536, (size_t)65536);
  SmallTab<uint8_t, TmpmemAlloc> tempBuf;
  clear_and_resize(tempBuf, inBufStoreSz + outBufStoreSz);
  uint8_t *inBufStore = tempBuf.data(), *outBufStore = tempBuf.data() + inBufStoreSz;
  ZSTD_initDStream(dstrm);
  ZSTD_DCtx_refDDict(dstrm, dict);

  inBuf.src = inBufStore;
  inBuf.size = inBufStoreSz;
  inBuf.pos = 0;
  outBuf.dst = outBufStore;
  outBuf.size = outBufStoreSz;
  outBuf.pos = 0;

  const size_t tryReadSz = (compr_sz > 0 && compr_sz < inBufStoreSz) ? compr_sz : inBufStoreSz;
  inBuf.size = src.tryRead(inBufStore, tryReadSz);
  size_t sizeLeft = compr_sz != 0 ? compr_sz : 1;
  if (compr_sz != 0)
    sizeLeft -= inBuf.size;
  if (inBuf.size < tryReadSz)
    sizeLeft = 0;
  size_t dec_sz = 0;
  while (size_t ret = ZSTD_decompressStream(dstrm, &outBuf, &inBuf))
  {
    if (ZSTD_isError(ret))
    {
      logerr("err=%08X %s", ret, ZSTD_getErrorName(ret));
      ZSTD_freeDStream(dstrm);
      return -1;
    }
    if (outBuf.pos == outBuf.size)
    {
      dec_sz += outBuf.pos;
      dest.write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
    if (inBuf.pos == inBuf.size)
    {
      const size_t tryReadSz = (compr_sz != 0 && sizeLeft < inBufStoreSz) ? sizeLeft : inBufStoreSz;
      inBuf.size = tryReadSz > 0 ? src.tryRead(inBufStore, tryReadSz) : 0;
      if (compr_sz != 0)
        sizeLeft -= inBuf.size;
      if (inBuf.size < tryReadSz)
        sizeLeft = 0;
      inBuf.pos = 0;
    }
  }
  if (inBuf.pos < inBuf.size)
    src.seekrel(-int(inBuf.size - inBuf.pos));
  if (outBuf.pos)
  {
    dec_sz += outBuf.pos;
    dest.write(outBuf.dst, outBuf.pos);
  }
  ZSTD_freeDStream(dstrm);
  return dec_sz;
}

int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src, const size_t compr_sz)
{
  if (compr_sz == 0)
  {
    logerr("compressed size can not be zero, that's an error");
    return -1;
  }
  return zstd_stream_decompress_data_base(dest, src, compr_sz);
}

int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src) { return zstd_stream_decompress_data_base(dest, src, 0); }

size_t zstd_train_dict_buffer(dag::Span<char> dict_buf, int compressionLevel, dag::ConstSpan<char> sample_buf,
  dag::ConstSpan<size_t> sample_sizes)
{
  ZDICT_fastCover_params_t params;
  memset(&params, 0, sizeof(params));
  params.k = 1058;
  params.d = 8;
  params.steps = 40;
  params.zParams.compressionLevel = compressionLevel;
  size_t sz = ZDICT_optimizeTrainFromBuffer_fastCover(dict_buf.data(), dict_buf.size(), sample_buf.data(), sample_sizes.data(),
    sample_sizes.size(), &params);
  if (ZDICT_isError(sz))
    return 0;
  return sz;
}
ZSTD_CDict_s *zstd_create_cdict(dag::ConstSpan<char> dict_buf, int compressionLevel, bool use_buf_ref)
{
  if (!dict_buf.size())
    return nullptr;
  return use_buf_ref ? ZSTD_createCDict_byReference(dict_buf.data(), dict_buf.size(), compressionLevel)
                     : ZSTD_createCDict(dict_buf.data(), dict_buf.size(), compressionLevel);
}
void zstd_destroy_cdict(ZSTD_CDict_s *cdict) { ZSTD_freeCDict(cdict); }
ZSTD_DDict_s *zstd_create_ddict(dag::ConstSpan<char> dict_buf, bool use_buf_ref)
{
  if (!dict_buf.size())
    return nullptr;
  return use_buf_ref ? ZSTD_createDDict_byReference(dict_buf.data(), dict_buf.size())
                     : ZSTD_createDDict(dict_buf.data(), dict_buf.size());
}
void zstd_destroy_ddict(ZSTD_DDict_s *ddict) { ZSTD_freeDDict(ddict); }

ZSTD_CCtx *zstd_create_cctx() { return ZSTD_createCCtx_advanced(ZSTD_dagorCMem); }
void zstd_destroy_cctx(ZSTD_CCtx *ctx) { ZSTD_freeCCtx(ctx); }

ZSTD_DCtx *zstd_create_dctx(bool tmp) { return ZSTD_createDCtx_advanced(tmp ? ZSTD_framememCMem : ZSTD_dagorCMem); }
void zstd_destroy_dctx(ZSTD_DCtx *ctx) { ZSTD_freeDCtx(ctx); }

int64_t zstd_stream_compress_data_with_dict(IGenSave &dest, IGenLoad &src, const size_t sz, int cLev, const ZSTD_CDict_s *dict,
  unsigned max_window_log)
{
  return zstd_stream_compress_data_base(dest, src, sz, cLev, max_window_log, dict);
}
int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src, const size_t compr_sz, const ZSTD_DDict_s *dict)
{
  return zstd_stream_decompress_data_base(dest, src, compr_sz, dict);
}

bool ZstdLoadFromMemCB::initDecoder(dag::ConstSpan<char> enc_data, const ZSTD_DDict_s *dict, bool tmp)
{
  encDataBuf.set((const unsigned char *)enc_data.data(), enc_data.size());
  encDataPos = 0;

  ZSTD_DStream *strm = ZSTD_createDStream_advanced(tmp ? ZSTD_framememCMem : ZSTD_dagorCMem);
  G_VERIFY(ZSTD_initDStream(strm) != 0);
  if (dict)
    ZSTD_DCtx_refDDict(strm, dict);
  dstrm = strm;
  return true;
}
void ZstdLoadFromMemCB::termDecoder()
{
  if (dstrm)
    ZSTD_freeDStream(dstrm);
  dstrm = nullptr;
}

inline int ZstdLoadFromMemCB::tryReadImpl(void *ptr, int size)
{
  if (!size)
    return 0;

  G_ASSERT(dstrm);
  if (encDataPos >= encDataBuf.size())
    if (!supplyMoreData())
      return 0;

  ZSTD_inBuffer inBuf;
  inBuf.src = encDataBuf.data();
  inBuf.size = encDataBuf.size();
  inBuf.pos = encDataPos;

  ZSTD_outBuffer outBuf;
  outBuf.dst = ptr;
  outBuf.size = size;
  outBuf.pos = 0;

  while (size_t ret = ZSTD_decompressStream(dstrm, &outBuf, &inBuf))
  {
    if (ZSTD_isError(ret))
    {
      DAG_FATAL("zstd error %d (%s) in %s\nsource: '%s'\n", ret, ZSTD_getErrorName(ret), "ZSTD_decompressStream", getTargetName());
      RETURN_X_AFTER_FATAL(0);
    }
    if (outBuf.pos == outBuf.size)
      break;
    if (inBuf.pos == inBuf.size)
    {
      if (supplyMoreData())
      {
        inBuf.src = encDataBuf.data();
        inBuf.size = encDataBuf.size();
        inBuf.pos = encDataPos;
      }
      else
        break;
    }
  }

  encDataPos = inBuf.pos;
  return outBuf.pos;
}

int ZstdLoadFromMemCB::tryRead(void *ptr, int size) { return ZstdLoadFromMemCB::tryReadImpl(ptr, size); }
void ZstdLoadFromMemCB::read(void *ptr, int size)
{
  int rd_sz = ZstdLoadFromMemCB::tryReadImpl(ptr, size);
  while (rd_sz && rd_sz < size)
  {
    ptr = (char *)ptr + rd_sz;
    size -= rd_sz;
    rd_sz = ZstdLoadFromMemCB::tryReadImpl(ptr, size);
  }

  if (rd_sz != size)
  {
    logerr("Zstd read error: rd_sz=%d != size=%d, encDataBuf=%p,%d encDataPos=%d", rd_sz, size, encDataBuf.data(), encDataBuf.size(),
      encDataPos);
    termDecoder();
    DAGOR_THROW(LoadException("Zstd read error", -1));
  }
}
void ZstdLoadFromMemCB::seekrel(int ofs)
{
  if (ofs < 0)
    issueFatal();
  else
    while (ofs > 0)
    {
      char buf[4096];
      int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
      read(buf, sz);
      ofs -= sz;
    }
}

void ZstdLoadCB::open(IGenLoad &in_crd, int in_size, const ZSTD_DDict_s *dict, bool tmp)
{
  G_ASSERT(!loadCb && "already opened?");
  G_ASSERT(in_size > 0);
  loadCb = &in_crd;
  inBufLeft = in_size;
  initDecoder(make_span_const(rdBuf, 0), dict, tmp);
}
void ZstdLoadCB::close()
{
  if (dstrm && !inBufLeft && encDataPos >= encDataBuf.size())
    ceaseReading();

  G_ASSERT(!dstrm);
  loadCb = NULL;
  inBufLeft = 0;
}
bool ZstdLoadCB::supplyMoreData()
{
  if (loadCb && inBufLeft > 0)
  {
    encDataPos = 0;
    encDataBuf.set(encDataBuf.data(), loadCb->tryRead(rdBuf, int(inBufLeft > RD_BUFFER_SIZE ? RD_BUFFER_SIZE : inBufLeft)));
    inBufLeft -= encDataBuf.size();
  }
  return encDataPos < encDataBuf.size();
}
bool ZstdLoadCB::ceaseReading()
{
  if (!dstrm)
    return true;

  loadCb->seekrel(int(inBufLeft > 0x70000000 ? encDataPos - encDataBuf.size() : inBufLeft));
  termDecoder();
  return true;
}

ZstdSaveCB::ZstdSaveCB(IGenSave &dest_cwr, int compression_level) : cwrDest(&dest_cwr)
{
  ZSTD_CStream *strm = ZSTD_createCStream_advanced(ZSTD_dagorCMem);
  ZSTD_initCStream_srcSize(strm, compression_level, ZSTD_CONTENTSIZE_UNKNOWN);
  zstdBufSize = ZSTD_CStreamOutSize();
  wrBuf = (uint8_t *)memalloc(BUFFER_SIZE + zstdBufSize, tmpmem);
  zstdStream = strm;
}
ZstdSaveCB::~ZstdSaveCB()
{
  ZSTD_freeCStream(zstdStream);
  zstdStream = nullptr;
  cwrDest = nullptr;
  memfree(wrBuf, tmpmem);
  wrBuf = 0;
}

void ZstdSaveCB::write(const void *ptr, int size)
{
  if (wrBufUsed + size <= BUFFER_SIZE)
  {
    memcpy(wrBuf + wrBufUsed, ptr, size);
    wrBufUsed += size;
    if (wrBufUsed == BUFFER_SIZE)
      compressBuffer();
  }
  else if (size <= BUFFER_SIZE)
  {
    uint32_t rest = BUFFER_SIZE - wrBufUsed;
    memcpy(wrBuf + wrBufUsed, ptr, rest);
    wrBufUsed += rest;
    ptr = ((const char *)ptr) + rest;
    size -= rest;
    compressBuffer();

    memcpy(wrBuf + wrBufUsed, ptr, size);
    wrBufUsed += size;
    if (wrBufUsed == BUFFER_SIZE)
      compressBuffer();
  }
  else
  {
    if (wrBufUsed)
      compressBuffer();
    compress(ptr, size);
  }
}
void ZstdSaveCB::compress(const void *ptr, int sz)
{
  ZSTD_inBuffer inBuf;
  ZSTD_outBuffer outBuf;

  inBuf.src = ptr;
  inBuf.size = sz;
  inBuf.pos = 0;
  outBuf.dst = wrBuf + BUFFER_SIZE;
  outBuf.size = zstdBufSize;
  outBuf.pos = zstdBufUsed;

  while (inBuf.pos < inBuf.size)
  {
    size_t ret = ZSTD_compressStream(zstdStream, &outBuf, &inBuf);
    if (ZSTD_isError(ret))
    {
      LOGERR_CTX("err=%08X %s", ret, ZSTD_getErrorName(ret));
      DAGOR_THROW(SaveException("ZSTD_compressStream error", (int)ret));
      return;
    }
    if (inBuf.pos < inBuf.size)
    {
      cwrDest->write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
  }
  zstdBufUsed = outBuf.pos;
}
void ZstdSaveCB::finish()
{
  if (wrBufUsed)
    compressBuffer();

  ZSTD_outBuffer outBuf;
  outBuf.dst = wrBuf + BUFFER_SIZE;
  outBuf.size = zstdBufSize;
  outBuf.pos = zstdBufUsed;

  for (;;)
  {
    const size_t ret = ZSTD_endStream(zstdStream, &outBuf);
    if (outBuf.pos)
    {
      cwrDest->write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
    if (ZSTD_isError(ret))
    {
      LOGERR_CTX("err=%08X %s", ret, ZSTD_getErrorName(ret));
      DAGOR_THROW(SaveException("ZSTD_endStream error", (int)ret));
      return;
    }
    if (ret == 0)
      break;
  }
  zstdBufUsed = 0;
}

void ZstdSaveCB::flush()
{
  ZSTD_outBuffer outBuf;

  outBuf.dst = wrBuf + BUFFER_SIZE;
  outBuf.size = zstdBufSize;
  outBuf.pos = zstdBufUsed;

  for (;;)
  {
    const size_t ret = ZSTD_flushStream(zstdStream, &outBuf);
    if (outBuf.pos)
    {
      cwrDest->write(outBuf.dst, outBuf.pos);
      outBuf.pos = 0;
    }
    if (!ret)
      break;
    if (ZSTD_isError(ret))
    {
      LOGERR_CTX("err=%08X %s", ret, ZSTD_getErrorName(ret));
      DAGOR_THROW(SaveException("flushStream error", (int)ret));
      return;
    }
  }
  zstdBufUsed = 0;
  cwrDest->flush();
}


ZstdDecompressSaveCB::ZstdDecompressSaveCB(IGenSave &dest_cwr, const ZSTD_DDict_s *dict, bool tmp, bool multiframe) :
  cwrDest(dest_cwr), allowMultiFrame(multiframe)
{
  zstdStream = ZSTD_createDStream_advanced(tmp ? ZSTD_framememCMem : ZSTD_dagorCMem);
  G_VERIFY(zstdStream);
  G_VERIFY(ZSTD_initDStream(zstdStream) != 0);
  if (dict)
    ZSTD_DCtx_refDDict(zstdStream, dict);
}

ZstdDecompressSaveCB::~ZstdDecompressSaveCB()
{
  if (zstdStream)
    ZSTD_freeDStream(zstdStream);
}

void ZstdDecompressSaveCB::flush()
{
  ZSTD_inBuffer inBuf;
  inBuf.src = nullptr;
  inBuf.size = 0;
  inBuf.pos = 0;

  bool haveMore = true;
  while (haveMore)
    haveMore = doProcessStep(&inBuf);
  cwrDest.flush();
}

void ZstdDecompressSaveCB::write(const void *ptr, int size)
{
  G_ASSERT_RETURN(size >= 0, );
  ZSTD_inBuffer inBuf;
  inBuf.src = ptr;
  inBuf.size = (size_t)size;
  inBuf.pos = 0;

  while (inBuf.pos < inBuf.size)
  {
    if (!doProcessStep(&inBuf))
      break;
  }
}

void ZstdDecompressSaveCB::finish()
{
  flush();

  isFinished = isFinished || (allowMultiFrame && isFrameFinished);

  if (!isFinished && !isBroken)
    logerr("Zstd stream wasn't finished, processedId=%d, processedOut=%d", processedIn, processedOut);
}

bool ZstdDecompressSaveCB::doProcessStep(void *opaqueInBuf)
{
  if (isBroken || isFinished)
    return false;

  ZSTD_inBuffer &inBuf = *reinterpret_cast<ZSTD_inBuffer *>(opaqueInBuf);
  ZSTD_outBuffer outBuf;
  outBuf.dst = &rdBuf[0];
  outBuf.size = RD_BUFFER_SIZE;
  outBuf.pos = 0;

  if (isFrameFinished && inBuf.size > inBuf.pos)
    isFrameFinished = false;
  int oldPos = inBuf.pos;
  size_t ret = ZSTD_decompressStream(zstdStream, &outBuf, &inBuf);
  int stepProcessedIn = inBuf.pos - oldPos;
  processedIn += stepProcessedIn;
  if (ZSTD_isError(ret))
  {
    logerr("Zstd decode error: %08X %s, processedIn: %d, processedOut: %d", ret, ZSTD_getErrorName(ret), processedIn, processedOut);
    isBroken = true;
    return false;
  }
  if (outBuf.pos > 0)
  {
    cwrDest.write(outBuf.dst, outBuf.pos);
    processedOut += outBuf.pos;
  }

  if (ret == 0)
  {
    isFrameFinished = true;
    if (!allowMultiFrame)
      isFinished = true;
    return false;
  }

  return stepProcessedIn > 0 || outBuf.pos > 0;
}

#define EXPORT_PULL dll_pull_iosys_zstdIo
#include <supp/exportPull.h>
