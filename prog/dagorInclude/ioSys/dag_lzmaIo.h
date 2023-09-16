//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_baseIo.h>

#include <supp/dag_define_COREIMP.h>


enum
{
  SIZE_OF_LZMA_DEC = 136,
  LZMA_LOAD_BUFFER_SIZE = (16 << 10),
};


class LzmaLoadCB : public IGenLoad
{
public:
  LzmaLoadCB(IGenLoad &in_crd, int in_size) : loadCb(NULL), isStarted(false), isFinished(false) { open(in_crd, in_size); }
  ~LzmaLoadCB() { close(); }

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

  KRNLIMP void open(IGenLoad &in_crd, int in_size);
  KRNLIMP void close();

  //! stop reading compressed data (can be issued before end of compressed data)
  //! doesn't move stream pointer to end (this can be done with wrapping block), but
  //! prevents fatal on close
  KRNLIMP bool ceaseReading();

protected:
  bool isFinished, isStarted;
  IGenLoad *loadCb;
  size_t inBufLeft;
  unsigned char strm[SIZE_OF_LZMA_DEC]; // CLzmaDec strm;
  unsigned char rdBuf[LZMA_LOAD_BUFFER_SIZE];
  size_t rdBufAvail, rdBufPos;

  KRNLIMP void issueFatal();

  inline int tryReadImpl(void *ptr, int size);
};


class BufferedLzmaLoadCB : public LzmaLoadCB
{
public:
  BufferedLzmaLoadCB(IGenLoad &in_crd, int in_size) : LzmaLoadCB(in_crd, in_size), curPos(0), totalOut(0) {}

  KRNLIMP virtual int tryRead(void *ptr, int size);
  KRNLIMP virtual void read(void *ptr, int size);

  inline void open(IGenLoad &in_crd, int in_size)
  {
    LzmaLoadCB::open(in_crd, in_size);
    curPos = 0;
    totalOut = 0;
  }
  inline void close()
  {
    LzmaLoadCB::close();
    curPos = 0;
    totalOut = 0;
  };

protected:
  static constexpr int OUT_BUF_SZ = (4 << 10);
  unsigned char outBuf[OUT_BUF_SZ];
  unsigned curPos, totalOut;
};


KRNLIMP int lzma_compress_data(IGenSave &dest, int compression_level, IGenLoad &src, int sz, int dict_sz = 1 << 20);

#include <supp/dag_undef_COREIMP.h>
