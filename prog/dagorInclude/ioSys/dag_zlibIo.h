//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_baseIo.h>

#include <supp/dag_define_KRNLIMP.h>


class ZLibGeneralWriter;

enum
{
  SIZE_OF_Z_STREAM = 128,
  ZLIB_LOAD_BUFFER_SIZE = (16 << 10),
};


class ZlibLoadCB : public IGenLoad
{
public:
  ZlibLoadCB(IGenLoad &in_crd, int in_size, bool raw_inflate = false, bool fatal_errors = true) :
    loadCb(NULL), isStarted(false), isFinished(false), fatalErrors(fatal_errors)
  {
    open(in_crd, in_size, raw_inflate);
  }
  ~ZlibLoadCB() { close(); }

  KRNLIMP virtual void read(void *ptr, int size);

  KRNLIMP virtual int tryRead(void *ptr, int size);
  virtual int tell()
  {
    issueFatal();
    return 0;
  }
  virtual void seekto(int) { issueFatal(); }
  KRNLIMP virtual void seekrel(int);
  virtual int beginBlock(unsigned * /*out_blk_flg*/ = nullptr)
  {
    issueFatal();
    return 0;
  }
  virtual void endBlock() { issueFatal(); }
  virtual int getBlockLength()
  {
    issueFatal();
    return 0;
  }
  virtual int getBlockRest()
  {
    issueFatal();
    return 0;
  }
  virtual int getBlockLevel()
  {
    issueFatal();
    return 0;
  }
  virtual const char *getTargetName() { return loadCb ? loadCb->getTargetName() : NULL; }

  KRNLIMP void open(IGenLoad &in_crd, int in_size, bool raw_inflate = false);
  KRNLIMP void close();

  //! stop reading compressed data (can be issued before end of compressed data)
  //! doesn't move stream pointer to end (this can be done with wrapping block), but
  //! prevents fatal on close
  KRNLIMP bool ceaseReading();

protected:
  bool isFinished, isStarted;
  bool rawInflate;
  bool fatalErrors;
  IGenLoad *loadCb;
  int inBufLeft;
  unsigned char strm[SIZE_OF_Z_STREAM]; // z_stream strm;
  unsigned char buffer[ZLIB_LOAD_BUFFER_SIZE];

  KRNLIMP void issueFatal();

  inline int tryReadImpl(void *ptr, int size);
  static unsigned fetchInput(void *handle, void *strm);
};


class BufferedZlibLoadCB : public ZlibLoadCB
{
public:
  BufferedZlibLoadCB(IGenLoad &in_crd, int in_size) : ZlibLoadCB(in_crd, in_size), curPos(0), totalOut(0) {}

  KRNLIMP virtual int tryRead(void *ptr, int size);
  KRNLIMP virtual void read(void *ptr, int size);

  inline void open(IGenLoad &in_crd, int in_size)
  {
    ZlibLoadCB::open(in_crd, in_size);
    curPos = 0;
    totalOut = 0;
  }
  inline void close()
  {
    ZlibLoadCB::close();
    curPos = 0;
    totalOut = 0;
  };

protected:
  static constexpr int OUT_BUF_SZ = (4 << 10);
  unsigned char outBuf[OUT_BUF_SZ];
  unsigned curPos, totalOut;
};


class ZlibSaveCB : public IGenSave
{
public:
  KRNLIMP ZlibSaveCB(IGenSave &in_save_cb, int compression_level, bool raw_inflate = false);
  KRNLIMP ~ZlibSaveCB();

  KRNLIMP virtual void write(const void *ptr, int size);
  KRNLIMP void finish();

  virtual int tell()
  {
    issueFatal();
    return 0;
  }
  virtual void seekto(int) { issueFatal(); }
  virtual void seektoend(int /*ofs*/ = 0) { issueFatal(); }
  virtual void beginBlock() { issueFatal(); }
  virtual void endBlock(unsigned /*block_flags*/) { issueFatal(); }
  virtual int getBlockLevel()
  {
    issueFatal();
    return 0;
  }
  virtual const char *getTargetName() { return saveCb ? saveCb->getTargetName() : NULL; }
  KRNLIMP void syncFlush();
  virtual void flush()
  {
    syncFlush();
    if (saveCb)
      saveCb->flush();
  }

  KRNLIMP float getCompressionRatio();

  enum
  {
    CL_NoCompression = 0,
    CL_BestSpeed = 1,
    //... valuse between 1 and 9 are intermediate compression/speed ratios
    // -9 provide almost the same compression ratio (2.84 vs 2.75) on small amount of data (>1MB) but 4 times slower
    // -1 is about 4 times slower than -0 on small amount of data and provides ~2 times compression
    // consider use of
    // * LZ4 for extreme fast realtime compression, Lz4hc for extreme fast decompression (and still faster compression than zlib, but a
    // bit worse ratio)
    // * LZMA for extreme compression ratios
    // * LZHAM for faster decompression
    // * zstd 0.1 to be 20 times faster on decompression but 10% less ratio.
    // * zstd 1.2 is supposed to compress better than zlib and still faster than any zlib compression ratio (probably even -0)
    // * they said that zstd with prebaked dictionay is faster and better in compression than Oodle
    // (http://www.radgametools.com/oodle.htm)
    // * Oodle proved to be good way to compress traffic in different shipped games with 'plug-and-play' scenario
    /*
             Name            Ratio  C.speed D.speed   (MB/s)
              zlib 1.2.8 -6   3.099     18     275
              zstd 0.1        2.872    201     498  //Old, zstd 1.2 differs a lot
              zlib 1.2.8 -1   2.730     58     250
              LZ4 HC r127     2.720     26    1720
              QuickLZ 1.5.1b6 2.237    323     373
              LZO 2.06        2.106    351     510
              Snappy 1.1.0    2.091    238     964
              LZ4 r127        2.084    370    1590
              LZF 3.6         2.077    220     502
    */
    // Zlib is good but old - usaully not perfect in any extreme usage (realtime, extreme compression ratio, extreme decompression
    // speed)
    /*
      text file   zlib compress                     zlib decompress
      0   6.98ms 640599 640700 1.000           0   1.54ms
      1  21.22ms 640599 274195 2.336           1   6.39ms
      2  25.08ms 640599 261638 2.448           2   6.13ms
      3  34.24ms 640599 249649 2.566           3   6.02ms
      4  36.41ms 640599 241500 2.653           4   6.22ms
      5  54.24ms 640599 232545 2.755           5   5.96ms
      6  77.22ms 640599 228621 2.802           6   5.94ms
      7  87.94ms 640599 228032 2.809           7   5.90ms
      8 112.49ms 640599 227622 2.814           8   5.89ms
      9 113.03ms 640599 227622 2.814           9   5.94ms
    */
    CL_BestCompression = 9,

    CL_DefaultCompressin = -1
  };

protected:
  IGenSave *saveCb;
  ZLibGeneralWriter *zlibWriter;
  bool isFinished;

  KRNLIMP void issueFatal();
};


class ZlibDecompressSaveCB : public IGenSave
{
public:
  explicit KRNLIMP ZlibDecompressSaveCB(IGenSave &in_save_cb, bool raw_inflate = false, bool fatal_errors = true);
  KRNLIMP ~ZlibDecompressSaveCB();

  KRNLIMP void write(const void *ptr, int size) override;
  KRNLIMP void finish();

  int tell() override
  {
    issueFatal();
    return 0;
  }
  void seekto(int) override { issueFatal(); }
  void seektoend(int /*ofs*/ = 0) override { issueFatal(); }
  void beginBlock() override { issueFatal(); }
  void endBlock(unsigned /*block_flags*/) override { issueFatal(); }
  int getBlockLevel() override
  {
    issueFatal();
    return 0;
  }
  const char *getTargetName() override { return saveCb.getTargetName(); }
  KRNLIMP void syncFlush();
  void flush() override
  {
    syncFlush();
    saveCb.flush();
  }

private:
  bool doProcessStep();

protected:
  IGenSave &saveCb;
  bool isFinished = false;
  bool isBroken = false;
  bool fatalErrors;
  alignas(16) unsigned char strm[SIZE_OF_Z_STREAM]; // z_stream strm;
  unsigned char buffer[ZLIB_LOAD_BUFFER_SIZE];

  KRNLIMP void issueFatal();
};


KRNLIMP int zlib_compress_data(IGenSave &dest, int compression_level, IGenLoad &src, int sz);

#include <supp/dag_undef_KRNLIMP.h>
