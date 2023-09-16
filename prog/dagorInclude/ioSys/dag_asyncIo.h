//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_baseIo.h>
#include <util/dag_simpleString.h>

#include <supp/dag_define_COREIMP.h>


// generic load interface implemented as async reader
class AsyncLoadCB : public IBaseLoad
{
  struct
  {
    int size;
    int pos;
    void *handle;
  } file;

  struct
  {
    int minimumChunk; // if 1 - buffering should be on
    int size;         /// size= (pow of 2) * minimumChunk;
    int used;
    int pos;
    char *data;
  } buf;
  SimpleString targetFilename;

  void readBuffered(void *ptr, int size);

public:
  KRNLIMP AsyncLoadCB(const char *fpath);
  KRNLIMP ~AsyncLoadCB();

  inline bool isOpen() { return file.handle != NULL; }
  inline int fileSize() { return file.size; }

  KRNLIMP virtual void read(void *ptr, int size);
  KRNLIMP virtual int tryRead(void *ptr, int size);
  KRNLIMP virtual int tell();
  KRNLIMP virtual void seekto(int);
  KRNLIMP virtual void seekrel(int ofs);
  KRNLIMP virtual const char *getTargetName() { return targetFilename.str(); }
  KRNLIMP int64_t getTargetDataSize() override { return file.size; }
};

#include <supp/dag_undef_COREIMP.h>
