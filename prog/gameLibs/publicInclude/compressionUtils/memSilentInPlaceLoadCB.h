//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>

class MemSilentInPlaceLoadCB : public IGenLoad
{
public:
  MemSilentInPlaceLoadCB(const void *ptr, int sz, const VirtualRomFsData *_vfs = nullptr);
  virtual ~MemSilentInPlaceLoadCB(void);

  virtual int beginBlock(unsigned *out_block_flg = nullptr);
  virtual void endBlock();
  virtual int getBlockLength();
  virtual int getBlockRest();
  virtual int getBlockLevel();

  void read(void *ptr, int size);
  int tryRead(void *ptr, int size);
  int tell(void);
  void seekto(int ofs);
  void seekrel(int ofs);
  virtual const char *getTargetName() { return "(mem)"; }

  const VirtualRomFsData *getTargetVromFs() const override { return vfs; }
  dag::ConstSpan<char> getTargetRomData() const override { return make_span_const((const char *)dataptr, datasize); }

  void close();


  void clear();

  void resize(int sz);

  int size(void);

  /// Returns pointer to buffer data.
  const unsigned char *data(void);

  /// Returns pointer to copy of buffer data.
  unsigned char *copy(void);

  bool wasError() const { return error; }

protected:
  const unsigned char *dataptr;
  int datasize;
  int curptr;

  bool error;
  const VirtualRomFsData *vfs = nullptr;
};
