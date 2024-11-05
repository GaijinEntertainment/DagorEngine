//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>

#include <supp/dag_define_KRNLIMP.h>

class OodleLoadCB : public IGenLoad
{
public:
  OodleLoadCB(IGenLoad &in_crd, int enc_size, int dec_size) :
    loadCb(NULL),
    strm(NULL),
    isFinished(false),
    encDataLeft(enc_size),
    decDataSize(dec_size),
    rdPos(0),
    decDataOffset(0),
    encBuf(NULL),
    encBufSize(0),
    encBufRdPos(0),
    encBufWrPos(0),
    decBuf(NULL),
    decBufSize(0),
    decBufPos(0)
  {
    open(in_crd, enc_size, dec_size);
  }
  ~OodleLoadCB() { close(); }

  KRNLIMP virtual void read(void *ptr, int size);

  KRNLIMP virtual int tryRead(void *ptr, int size);
  virtual int tell()
  {
    issueFatal();
    return 0;
  }
  virtual void seekto(int) { issueFatal(); }
  KRNLIMP virtual void seekrel(int);
  virtual int beginBlock(unsigned * = nullptr)
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

  KRNLIMP void open(IGenLoad &in_crd, int enc_size, int dec_size);
  KRNLIMP void close();

  KRNLIMP virtual bool ceaseReading();

protected:
  void *strm;
  IGenLoad *loadCb;
  int encDataLeft, decDataSize, rdPos, decDataOffset;
  int encBufSize, encBufRdPos, encBufWrPos;
  int decBufSize, decBufPos;
  char *encBuf, *decBuf;
  bool isFinished;

  KRNLIMP void issueFatal();

  inline bool partialDecBuf() const;
  inline void preloadEncData(int size);
  inline int decodeData(int size);
};


KRNLIMP int oodle_compress_data(IGenSave &dest, IGenLoad &src, int sz, int compressionLevel = 19);
KRNLIMP int oodle_decompress_data(IGenSave &dest, IGenLoad &src, int compr_sz, int orig_sz);

KRNLIMP size_t oodle_compress(void *dst, size_t maxDstSize, const void *src, size_t srcSize, int compressionLevel = 19);
KRNLIMP size_t oodle_decompress(void *dst, size_t maxOriginalSize, const void *src, size_t compressedSize);

#include <supp/dag_undef_KRNLIMP.h>
