//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>


#include <supp/dag_define_KRNLIMP.h>

// simple circular buffer writer with block support (no mem allocations)
class SimpleBlockSave : public IGenSave
{
public:
  SimpleBlockSave();
  virtual ~SimpleBlockSave() {}

  void setCircularBuffer(void *mem, int len);
  void setLimits(int start_pos, int end_pos);

  int getEndPos() const { return limEnd; }
  int getLimSize() const { return limSize; }
  int getRootBlockCount() const { return rootBlkNum; }

  virtual void beginBlock();
  virtual void endBlock(unsigned block_flags_2bits = 0);
  virtual int getBlockLevel();

  virtual void write(const void *ptr, int size);
  KRNLIMP virtual int tell();
  virtual void seekto(int abs_ofs);
  virtual void seektoend(int rel_ofs = 0);
  virtual const char *getTargetName() { return "(msg)"; }
  virtual void flush()
  { /*noop*/
  }

protected:
  static constexpr int BLOCK_MAX = 32;
  int blkOfs[BLOCK_MAX];
  int blkUsed, rootBlkNum;

  char *buffer;
  int bufferSize;
  int limStart, limEnd, limSize, curPos;
};


// simple circular buffer reader with block support (no mem allocations)
class SimpleBlockLoad : public IGenLoad
{
public:
  SimpleBlockLoad();
  virtual ~SimpleBlockLoad() {}

  void setCircularBuffer(const void *mem, int len);
  void setLimits(int start_pos, int end_pos);

  int getEndPos() const { return limEnd; }
  int getLimSize() const { return limSize; }

  virtual int beginBlock(unsigned *out_blk_flg = NULL);
  virtual void endBlock();
  virtual int getBlockLength();
  virtual int getBlockRest();
  virtual int getBlockLevel();

  virtual void read(void *ptr, int size);
  virtual int tryRead(void *ptr, int size);
  virtual int tell();
  virtual void seekto(int abs_ofs);
  virtual void seekrel(int rel_ofs);
  KRNLIMP virtual const char *getTargetName() { return "(msg)"; }

protected:
  static constexpr int BLOCK_MAX = 32;
  int blkOfs[BLOCK_MAX];
  int blkLen[BLOCK_MAX];
  int blkUsed;

  const char *buffer;
  int bufferSize;
  int limStart, limEnd, limSize, curPos;
};


//
// Thread-safe message IO (supports write/read form different threads with fast locks and without memory allocation)
//
class ThreadSafeMsgIo
{
public:
  KRNLIMP ThreadSafeMsgIo(int buf_sz = (16 << 10));
  KRNLIMP virtual ~ThreadSafeMsgIo();

  //! returns pointer to reader interface (or NULL, if msg_count==0)
  KRNLIMP IGenLoad *startRead(int &msg_count);
  //! finished reading and removes read content from buffer
  KRNLIMP void endRead();

  //! returns pointer to writer interface
  KRNLIMP IGenSave *startWrite();
  //! finished writing and updates buffer pointers
  KRNLIMP void endWrite();

  inline int getWriteAvailableSize() const { return interlocked_acquire_load(availWrSize); }

protected:
  WinCritSec cc;
  void *buffer;
  int bufferSize;
  int wrPos, rdPos;
  int msgCount, rdMsgCount;
  volatile int availWrSize;

  SimpleBlockSave cwr;
  SimpleBlockLoad crd;
};


//
// Thread-safe message IO with multiple-write-single-read ability
//
class ThreadSafeMsgIoEx : public ThreadSafeMsgIo
{
public:
  ThreadSafeMsgIoEx(int buf_sz = (16 << 10)) : ThreadSafeMsgIo(buf_sz) {}

  inline IGenSave *startWrite()
  {
    ccWrite.lock();
    return ThreadSafeMsgIo::startWrite();
  }
  inline void endWrite()
  {
    ThreadSafeMsgIo::endWrite();
    ccWrite.unlock();
  }

protected:
  WinCritSec ccWrite;
};

#include <supp/dag_undef_KRNLIMP.h>
