//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_baseIo.h>
#include <util/dag_simpleString.h>

// forward declarations for external classes
typedef void *file_ptr_t;

#include <supp/dag_define_KRNLIMP.h>

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// Serialization callbacks.


/// File save callback.
class LFileGeneralSaveCB : public IBaseSave
{
public:
  file_ptr_t fileHandle;
  SimpleString targetFilename;

  KRNLIMP LFileGeneralSaveCB(file_ptr_t handle = NULL);

  KRNLIMP void write(const void *ptr, int size);
  KRNLIMP int tryWrite(const void *ptr, int size);
  KRNLIMP int tell();
  KRNLIMP void seekto(int);
  KRNLIMP void seektoend(int ofs = 0);
  KRNLIMP virtual const char *getTargetName() { return targetFilename.str(); }
  KRNLIMP virtual void flush();
};


/// File load callback.
class LFileGeneralLoadCB : public IBaseLoad
{
public:
  file_ptr_t fileHandle;
  SimpleString targetFilename;

  KRNLIMP LFileGeneralLoadCB(file_ptr_t handle = NULL);

  KRNLIMP void read(void *ptr, int size);
  KRNLIMP int tryRead(void *ptr, int size);
  KRNLIMP int tell();
  KRNLIMP void seekto(int);
  KRNLIMP void seekrel(int);
  KRNLIMP virtual const char *getTargetName() { return targetFilename.str(); }
  KRNLIMP virtual const VirtualRomFsData *getTargetVromFs() const;
};


/// Callback for reading whole file. Closes file in destructor.
class FullFileLoadCB : public LFileGeneralLoadCB
{
  int64_t targetDataSz = -1;

public:
  inline FullFileLoadCB(const char *fname, int mode)
  {
    fileHandle = NULL;
    targetFilename = fname;
    open(fname, mode);
  }
  KRNLIMP FullFileLoadCB(const char *fname);
  inline ~FullFileLoadCB() { close(); }

  KRNLIMP bool open(const char *fname, int mode);
  KRNLIMP void close();
  KRNLIMP void beginFullFileBlock();
  KRNLIMP const char *getTargetName() override { return targetFilename.str(); }
  KRNLIMP int64_t getTargetDataSize() override { return targetDataSz; }
  KRNLIMP dag::ConstSpan<char> getTargetRomData() const override;
};


/// Callback for writing whole file. Closes file in destructor.
class FullFileSaveCB : public LFileGeneralSaveCB
{
public:
  inline FullFileSaveCB() {}

  inline FullFileSaveCB(const char *fname, int mode)
  {
    fileHandle = NULL;
    targetFilename = fname;
    open(fname, mode);
  }

  KRNLIMP FullFileSaveCB(const char *fname);

  inline ~FullFileSaveCB() { close(); }

  KRNLIMP bool open(const char *fname, int mode);
  KRNLIMP void close();
  KRNLIMP virtual const char *getTargetName() { return targetFilename.str(); }
};


/// @}

/// @}

#include <supp/dag_undef_KRNLIMP.h>
