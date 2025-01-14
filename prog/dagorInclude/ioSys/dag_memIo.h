//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_baseIo.h>
#include <dag/dag_vector.h>

#include <supp/dag_define_KRNLIMP.h>

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// Serialization callbacks.


/// Callback to write into dynamically allocated memory buffer.
class DynamicMemGeneralSaveCB : public IBaseSave
{
public:
  KRNLIMP DynamicMemGeneralSaveCB(IMemAlloc *_allocator, size_t sz = 0, size_t quant = 64 << 10);
  DynamicMemGeneralSaveCB(DynamicMemGeneralSaveCB &&) = default;
  DynamicMemGeneralSaveCB &operator=(DynamicMemGeneralSaveCB &&) = default;
  KRNLIMP virtual ~DynamicMemGeneralSaveCB(void);

  KRNLIMP void write(const void *ptr, int size);
  KRNLIMP int tell(void);
  KRNLIMP void seekto(int ofs);
  KRNLIMP void seektoend(int ofs = 0);
  KRNLIMP virtual const char *getTargetName() { return "(mem)"; }
  KRNLIMP virtual void flush()
  { /*noop*/
  }

  KRNLIMP void resize(intptr_t sz);
  KRNLIMP void setsize(intptr_t sz);

  /// Returns size of written buffer.
  inline intptr_t size(void) const { return datasize; }

  /// Returns pointer to buffer data.
  inline unsigned char *data(void) { return dataptr; }
  inline const unsigned char *data(void) const { return dataptr; }

  /// Returns pointer to copy of buffer data.
  KRNLIMP unsigned char *copy(void);

protected:
  unsigned char *dataptr;
  intptr_t datasize, data_avail, data_quant;
  intptr_t curptr;
  IMemAlloc *allocator;
};


/// Constrained save to memory region (without any allocations and reallocations)
/// if memory region size is exceeded during write operations exception is thrown
class ConstrainedMemSaveCB : public DynamicMemGeneralSaveCB
{
public:
  ConstrainedMemSaveCB(void *data, int sz) : DynamicMemGeneralSaveCB(nullptr, 0, 0) { setDestMem(data, sz); }
  ~ConstrainedMemSaveCB() { dataptr = nullptr; }
  void setDestMem(void *data, int sz)
  {
    dataptr = (unsigned char *)data;
    data_avail = sz;
    datasize = curptr = 0;
  }
};


/// Callback for reading from memory buffer. Allocates memory from #globmem allocator.
class MemGeneralLoadCB : public IBaseLoad
{
public:
  /// Allocates buffer and copies data to it.
  KRNLIMP MemGeneralLoadCB(const void *ptr, int sz);
  MemGeneralLoadCB(const MemGeneralLoadCB &) = default;
  MemGeneralLoadCB(MemGeneralLoadCB &&) = default;
  MemGeneralLoadCB &operator=(const MemGeneralLoadCB &) = default;
  MemGeneralLoadCB &operator=(MemGeneralLoadCB &&) = default;
  KRNLIMP virtual ~MemGeneralLoadCB(void);

  KRNLIMP void read(void *ptr, int size) override;
  KRNLIMP int tryRead(void *ptr, int size) override;
  KRNLIMP int tell(void) override;
  KRNLIMP void seekto(int ofs) override;
  KRNLIMP void seekrel(int ofs) override;
  KRNLIMP const char *getTargetName() override { return "(mem)"; }
  KRNLIMP int64_t getTargetDataSize() override { return datasize; }
  dag::ConstSpan<char> getTargetRomData() const override { return make_span_const((const char *)dataptr, datasize); }

  KRNLIMP void close();

  /// Free buffer.
  KRNLIMP void clear();

  /// Resize (reallocate) buffer.
  KRNLIMP void resize(int sz);

  /// Returns buffer size.
  KRNLIMP int size(void);

  /// Returns pointer to buffer data.
  KRNLIMP const unsigned char *data(void);

  /// Returns pointer to copy of buffer data.
  KRNLIMP unsigned char *copy(void);

protected:
  const unsigned char *dataptr;
  int datasize;
  int curptr;
};


/// In-place (no copy) load from memory interface (fully inline).
class InPlaceMemLoadCB : public MemGeneralLoadCB
{
public:
  InPlaceMemLoadCB(const void *ptr, int sz) : MemGeneralLoadCB(NULL, 0)
  {
    dataptr = (const unsigned char *)ptr;
    datasize = sz;
  }
  InPlaceMemLoadCB(const InPlaceMemLoadCB &) = default;
  InPlaceMemLoadCB(InPlaceMemLoadCB &&) = default;
  InPlaceMemLoadCB &operator=(const InPlaceMemLoadCB &) = default;
  InPlaceMemLoadCB &operator=(InPlaceMemLoadCB &&) = default;
  ~InPlaceMemLoadCB() { dataptr = NULL; }

  const void *readAny(int sz)
  {
    const void *p = dataptr + curptr;
    seekrel(sz);
    return p;
  }

  void setSrcMem(const void *data, int sz)
  {
    dataptr = (const unsigned char *)data;
    datasize = sz;
    curptr = 0;
  }
};

enum class StreamDecompressResult
{
  FAILED,
  FINISH,
  NEED_MORE_INPUT,
};

struct IStreamDecompress
{
  virtual ~IStreamDecompress() {}
  virtual StreamDecompressResult decompress(dag::ConstSpan<uint8_t> in, Tab<char> &out, size_t *nbytes_read = nullptr) = 0;
};


/// @}

/// @}

#include <supp/dag_undef_KRNLIMP.h>
