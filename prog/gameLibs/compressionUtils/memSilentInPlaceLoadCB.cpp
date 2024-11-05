// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_memBase.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_globDef.h>
#include <compressionUtils/memSilentInPlaceLoadCB.h>


MemSilentInPlaceLoadCB::MemSilentInPlaceLoadCB(const void *ptr, int sz, const VirtualRomFsData *_vfs)
{
  error = false;

  curptr = 0;
  dataptr = (const unsigned char *)ptr;
  datasize = sz;
  vfs = _vfs;
}


MemSilentInPlaceLoadCB::~MemSilentInPlaceLoadCB() { dataptr = NULL; }


void MemSilentInPlaceLoadCB::read(void *ptr, int size)
{
  if (size < 0)
  {
    error = true;
    return;
  }

  if (!size || !ptr)
    return;

  if (curptr + size > datasize)
  {
    memset(ptr, 0, size);
    error = true;
    return;
  }

  memcpy(ptr, dataptr + curptr, size);
  curptr += size;
}


int MemSilentInPlaceLoadCB::tryRead(void *ptr, int size)
{
  if (size <= 0 || !ptr)
    return 0;
  if (curptr + size > datasize)
    size = datasize - curptr;
  memcpy(ptr, dataptr + curptr, size);
  curptr += size;
  return size;
}


int MemSilentInPlaceLoadCB::tell(void)
{
  if (curptr < 0 || curptr > datasize)
  {
    error = true;
    return 0;
  }

  return curptr;
}


void MemSilentInPlaceLoadCB::seekto(int ofs)
{
  if (ofs < 0 || ofs > datasize)
  {
    error = true;
    return;
  }
  curptr = ofs;
}


void MemSilentInPlaceLoadCB::seekrel(int ofs)
{
  if (curptr + ofs < 0 || curptr + ofs > datasize)
  {
    error = true;
    return;
  }
  curptr += ofs;
}


void MemSilentInPlaceLoadCB::close(void) { G_ASSERT(0); }


void MemSilentInPlaceLoadCB::clear(void)
{
  G_ASSERT((dataptr && datasize) || (!dataptr && !datasize));
  dataptr = NULL;
  datasize = 0;
  curptr = 0;
}


void MemSilentInPlaceLoadCB::resize(int sz)
{
  G_UNREFERENCED(sz);
  G_ASSERT(0);
}


int MemSilentInPlaceLoadCB::size(void) { return datasize; }


const unsigned char *MemSilentInPlaceLoadCB::data(void) { return dataptr; }


unsigned char *MemSilentInPlaceLoadCB::copy(void)
{
  if (!size())
    return NULL;
  unsigned char *data = (unsigned char *)memalloc(datasize, globmem);
  memcpy(data, dataptr, datasize);
  return data;
}


int MemSilentInPlaceLoadCB::beginBlock(unsigned *)
{
  G_ASSERT(0);
  return -1;
}


void MemSilentInPlaceLoadCB::endBlock() { G_ASSERT(0); }


int MemSilentInPlaceLoadCB::getBlockLength()
{
  G_ASSERT(0);
  return -1;
}


int MemSilentInPlaceLoadCB::getBlockRest()
{
  G_ASSERT(0);
  return -1;
}


int MemSilentInPlaceLoadCB::getBlockLevel()
{
  G_ASSERT(0);
  return -1;
}
