//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>

struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;
struct ZSTD_CDict_s;
struct ZSTD_DDict_s;

#include <supp/dag_define_KRNLIMP.h>

class ZstdLoadFromMemCB : public IGenLoad
{
public:
  ZstdLoadFromMemCB() = default;
  ZstdLoadFromMemCB(dag::ConstSpan<char> enc_data, const ZSTD_DDict_s *dict = nullptr, bool tmp = false)
  {
    initDecoder(enc_data, dict, tmp);
  }
  ~ZstdLoadFromMemCB() { termDecoder(); }

  KRNLIMP bool initDecoder(dag::ConstSpan<char> enc_data, const ZSTD_DDict_s *dict, bool tmp = false);
  KRNLIMP void termDecoder();

  KRNLIMP void read(void *ptr, int size) override;
  KRNLIMP int tryRead(void *ptr, int size) override;
  int tell() override
  {
    issueFatal();
    return 0;
  }
  void seekto(int) override { issueFatal(); }
  KRNLIMP void seekrel(int) override;
  int beginBlock(unsigned * = nullptr) override
  {
    issueFatal();
    return 0;
  }
  void endBlock() override { issueFatal(); }
  int getBlockLength() override
  {
    issueFatal();
    return 0;
  }
  int getBlockRest() override
  {
    issueFatal();
    return 0;
  }
  int getBlockLevel() override
  {
    issueFatal();
    return 0;
  }
  const char *getTargetName() override { return nullptr; }
  bool ceaseReading() override { return true; }

protected:
  ZSTD_DCtx_s *dstrm = nullptr;
  dag::ConstSpan<unsigned char> encDataBuf;
  unsigned encDataPos = 0;

  virtual bool supplyMoreData() { return false; }
  KRNLIMP void issueFatal();

  inline int tryReadImpl(void *ptr, int size);
};

class ZstdLoadCB : public ZstdLoadFromMemCB
{
public:
  ZstdLoadCB() = default; //-V730   /* since rdBuf shall not be filled in ctor for performance reasons */
  ZstdLoadCB(IGenLoad &in_crd, int in_size, const ZSTD_DDict_s *dict = nullptr, bool tmp = false) { open(in_crd, in_size, dict, tmp); }
  ~ZstdLoadCB() { close(); }

  const char *getTargetName() override { return loadCb ? loadCb->getTargetName() : NULL; }

  KRNLIMP void open(IGenLoad &in_crd, int in_size, const ZSTD_DDict_s *dict = nullptr, bool tmp = false);
  KRNLIMP void close();
  KRNLIMP bool ceaseReading() override;

protected:
  static constexpr int RD_BUFFER_SIZE = (32 << 10);
  unsigned inBufLeft = 0;
  IGenLoad *loadCb = nullptr;
  alignas(16) char rdBuf[RD_BUFFER_SIZE];

  KRNLIMP bool supplyMoreData() override;
};

class ZstdSaveCB : public IGenSave
{
public:
  KRNLIMP ZstdSaveCB(IGenSave &dest_cwr, int compression_level);
  KRNLIMP ~ZstdSaveCB();

  KRNLIMP void write(const void *ptr, int size) override;
  KRNLIMP void finish();

  int tell() override
  {
    issueFatal();
    return 0;
  }
  void seekto(int) override { issueFatal(); }
  void seektoend(int) override { issueFatal(); }
  void beginBlock() override { issueFatal(); }
  void endBlock(unsigned) override { issueFatal(); }
  int getBlockLevel() override
  {
    issueFatal();
    return 0;
  }
  const char *getTargetName() override { return cwrDest ? cwrDest->getTargetName() : NULL; }
  void flush() override;

protected:
  static constexpr int BUFFER_SIZE = (32 << 10);

  IGenSave *cwrDest = nullptr;
  ZSTD_CCtx_s *zstdStream = nullptr;
  uint32_t wrBufUsed = 0, zstdBufUsed = 0, zstdBufSize = 0;
  uint8_t *wrBuf = nullptr;

  KRNLIMP void issueFatal();
  void compress(const void *ptr, int sz);
  void compressBuffer()
  {
    compress(wrBuf, wrBufUsed);
    wrBufUsed = 0;
  }
};


class ZstdDecompressSaveCB : public IGenSave
{
public:
  explicit KRNLIMP ZstdDecompressSaveCB(IGenSave &dest_cwr, const ZSTD_DDict_s *dict = nullptr, bool tmp = false,
    bool multiframe = true);
  ~ZstdDecompressSaveCB();

  KRNLIMP void write(const void *ptr, int size) override;
  KRNLIMP void finish();

  int tell() override
  {
    issueFatal();
    return 0;
  }
  void seekto(int) override { issueFatal(); }
  void seektoend(int) override { issueFatal(); }
  void beginBlock() override { issueFatal(); }
  void endBlock(unsigned) override { issueFatal(); }
  int getBlockLevel() override
  {
    issueFatal();
    return 0;
  }
  const char *getTargetName() override { return cwrDest.getTargetName(); }
  void flush() override;

private:
  bool doProcessStep(void *opaqueInBuf);

protected:
  static constexpr int RD_BUFFER_SIZE = (16 << 10);
  IGenSave &cwrDest;
  ZSTD_DCtx_s *zstdStream = nullptr;
  int processedIn = 0;
  int processedOut = 0;
  bool isFrameFinished = true;
  bool isFinished = false;
  bool isBroken = false;
  bool allowMultiFrame = false;
  alignas(16) char rdBuf[RD_BUFFER_SIZE]; //-V730_NOINIT

  KRNLIMP void issueFatal();
};


// will compress only sz data passed
// compressed size will be 2-3 bytes bigger, both compression and then decompression speed will be 5-10% faster (zstd 1.4.5)
// if max_window_log set > 0 then it is used as max window size (1<<max_window_log); 0 means using default for compression_level
KRNLIMP int64_t zstd_stream_compress_data(IGenSave &dest, IGenLoad &src, const size_t sz, int compression_level,
  unsigned max_window_log = 0);

// if compression will read from src using tryRead until compression stream ends.compr_sz CAN NOT be zero, it is error
KRNLIMP int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src, const size_t compr_sz);

// if compression will read from src using tryRead untill compression stream ends
KRNLIMP int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src);

// will compress in one call, not using streaming. Difference in result is neglible (usually within 1-4 bytes, if any)
// minimize calls to tryRead/write, but allocates ~2*sz memory
// if max_window_log set > 0 then it is used as max window size (1<<max_window_log); 0 means using default for compression_level
KRNLIMP int64_t zstd_compress_data_solid(IGenSave &dest, IGenLoad &src, const size_t sz, int compressionLevel = 18,
  unsigned max_window_log = 0);

// legacy API, but that's how it used to be, so otherwise we will get huge patch
inline int64_t zstd_compress_data(IGenSave &dest, IGenLoad &src, size_t sz, size_t solid_threshold = 1 << 20,
  int compressionLevel = 18, unsigned max_window_log = 0)
{
  return (sz < solid_threshold) ? zstd_compress_data_solid(dest, src, sz, compressionLevel, max_window_log)
                                : zstd_stream_compress_data(dest, src, sz, compressionLevel, max_window_log);
}

// old names
inline int64_t zstd_decompress_data(IGenSave &dest, IGenLoad &src, size_t compr_sz)
{
  return zstd_stream_decompress_data(dest, src, compr_sz);
}
inline int64_t zstd_decompress_data(IGenSave &dest, IGenLoad &src) { return zstd_stream_decompress_data(dest, src); }

// Maximum compressed size in worst case single-pass scenario
KRNLIMP size_t zstd_compress_bound(size_t srcSize);
KRNLIMP size_t zstd_compress(void *dst, size_t maxDstSize, const void *src, size_t srcSize, int compressionLevel = 18,
  unsigned max_window_log = 0);
KRNLIMP size_t zstd_decompress(void *dst, size_t maxOriginalSize, const void *src, size_t compressedSize);

KRNLIMP size_t zstd_compress_with_dict(ZSTD_CCtx_s *ctx, void *dst, size_t dstSize, const void *src, size_t srcSize,
  const ZSTD_CDict_s *dict);
KRNLIMP size_t zstd_decompress_with_dict(ZSTD_DCtx_s *ctx, void *dst, size_t dstSize, const void *src, size_t srcSize,
  const ZSTD_DDict_s *dict);

// trains dictionary buffer with sample_buf and sample_sizes and return size of used dictionary
KRNLIMP size_t zstd_train_dict_buffer(dag::Span<char> dict_buf, int compressionLevel, dag::ConstSpan<char> sample_buf,
  dag::ConstSpan<size_t> sample_sizes);

// creates dictionary from trained buffer (for compression), optionally reference dict_buf without copying
KRNLIMP ZSTD_CDict_s *zstd_create_cdict(dag::ConstSpan<char> dict_buf, int compressionLevel, bool use_buf_ref = false);
// destroys compression dictionary
KRNLIMP void zstd_destroy_cdict(ZSTD_CDict_s *dict);

// creates dictionary from trained buffer (for decompression), optionally reference dict_buf without copying
KRNLIMP ZSTD_DDict_s *zstd_create_ddict(dag::ConstSpan<char> dict_buf, bool use_buf_ref = false);
// destroys decompression dictionary
KRNLIMP void zstd_destroy_ddict(ZSTD_DDict_s *dict);

KRNLIMP ZSTD_CCtx_s *zstd_create_cctx();
KRNLIMP void zstd_destroy_cctx(ZSTD_CCtx_s *ctx);

KRNLIMP ZSTD_DCtx_s *zstd_create_dctx(bool tmp = false);
KRNLIMP void zstd_destroy_dctx(ZSTD_DCtx_s *ctx);

// compresses stream using dictionary (created with zstd_create_dict)
// if max_window_log set > 0 then it is used as max window size (1<<max_window_log); 0 means using default for compression_level
KRNLIMP int64_t zstd_stream_compress_data_with_dict(IGenSave &dest, IGenLoad &src, const size_t sz, int cLev, const ZSTD_CDict_s *dict,
  unsigned max_window_log = 0);

// decompresses stream using dictionary (created with zstd_create_dict)
KRNLIMP int64_t zstd_stream_decompress_data(IGenSave &dest, IGenLoad &src, const size_t compr_sz, const ZSTD_DDict_s *dict);

#include <supp/dag_undef_KRNLIMP.h>
