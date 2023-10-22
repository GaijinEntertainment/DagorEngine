#include <ioSys/dag_chainedMemIo.h>
#include <stdio.h>

// Memory save callback interface
MemorySaveCB::MemorySaveCB()
{
  mcd = NULL;
  autoDelete = false;
  initDefs();
}
MemorySaveCB::MemorySaveCB(MemoryChainedData *_ch, bool auto_delete)
{
  mcd = _ch;
  autoDelete = auto_delete;
  initDefs();
}
MemorySaveCB::MemorySaveCB(int init_sz)
{
  mcd = MemoryChainedData::create(init_sz);
  autoDelete = true;
  initDefs();
}
MemorySaveCB::~MemorySaveCB()
{
  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(mcd);
  mcd = NULL;
  autoDelete = false;
}
MemoryChainedData *MemorySaveCB::takeMem()
{
  while (blocks.size() > 0)
    endBlock();

  MemoryChainedData *p = mcd;
  mcd = NULL;
  autoDelete = false;
  return p;
}
void MemorySaveCB::deleteMem()
{
  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(mcd);
  clear_and_shrink(blocks);
  mcd = NULL;
  autoDelete = false;
}
void MemorySaveCB::setMem(MemoryChainedData *_ch, bool auto_delete)
{
  while (blocks.size() > 0)
    endBlock();

  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(mcd);
  mcd = _ch;
  autoDelete = auto_delete;
  initDefs();
}
// Currently maxPos member is 64bit value. We can support 64bit value for argument here aswell, but it requires to change seekto method
// and base interface.
void MemorySaveCB::setRange(int min_pos, int max_pos)
{
  minPos = min_pos;
  maxPos = max_pos < 0x7FFFFFFF ? max_pos : 0x7FFFFFFF;
  if (!mcd)
    return;

  int curPos = MemorySaveCB::tell();
  if (curPos < minPos)
    MemorySaveCB::seekto(minPos);
  else if (curPos > maxPos)
    MemorySaveCB::seekto(maxPos);
}
void MemorySaveCB::setMcdMinMax(int min_size, int max_size)
{
  minMcdSz = min_size;
  maxMcdSz = max_size;
}

void MemorySaveCB::write(const void *ptr, int size)
{
  ptrdiff_t sz = (pos + size > maxPos) ? maxPos - pos : size;

  if (pos + sz > totalSize)
    extendMemory(pos + sz);

  while (sz > 0)
  {
    ptrdiff_t ofs = (pos - cMcd->offset);
    ptrdiff_t msz = cMcd->size - ofs;
    if (msz > sz)
      msz = sz;
    memcpy(cMcd->data + ofs, ptr, msz);
    sz -= msz;
    ptr = (char *)ptr + msz;
    pos += msz;

    ofs += msz;
    if (ofs > cMcd->used)
      cMcd->used = ofs;

    if (ofs >= cMcd->size)
      cMcd = cMcd->next;
  }
}
void MemorySaveCB::seekto(int _pos)
{
  if (_pos < minPos)
    _pos = minPos;
  else if (pos > maxPos)
    _pos = maxPos;

  if (_pos > totalSize)
    extendMemory(_pos);

  pos = _pos;
  if (cMcd && pos >= cMcd->offset)
  {
    while (pos >= cMcd->offset + cMcd->size && cMcd->next)
      cMcd = cMcd->next;
  }
  else
  {
    cMcd = mcd;
    while (pos >= cMcd->offset + cMcd->size && cMcd->next)
      cMcd = cMcd->next;
  }
}

void MemorySaveCB::initDefs()
{
  minPos = 0;
  maxPos = size_t(ptrdiff_t(-1)) >> 1;
  pos = 0;
  cMcd = mcd;
  totalSize = MemoryChainedData::calcTotalSize(mcd);
  minMcdSz = 16 << 10;
  maxMcdSz = 8 << 20;
}
void MemorySaveCB::extendMemory(ptrdiff_t sz)
{
  MemoryChainedData *m = cMcd ? cMcd : mcd;
  while (m && m->next)
    m = m->next;

  sz -= totalSize;
  while (sz > 0)
  {
    if (m)
      m->used = m->size;

    ptrdiff_t msz = sz;
    if (msz > maxMcdSz)
      msz = maxMcdSz;
    else if (msz < minMcdSz)
      msz = minMcdSz;
    m = MemoryChainedData::create(msz, m);
    if (!mcd)
    {
      mcd = m;
      cMcd = m;
    }

    sz -= m->size;
    totalSize += m->size;
  }

  if (!cMcd)
    MemorySaveCB::seekto((int)pos);
}


// Memory load callback interface
MemoryLoadCB::MemoryLoadCB()
{
  mcd = NULL;
  autoDelete = false;
  targetDataSz = -1;
  initDefs();
}
MemoryLoadCB::MemoryLoadCB(const MemoryChainedData *_ch, bool auto_delete)
{
  mcd = _ch;
  autoDelete = auto_delete;
  targetDataSz = MemoryChainedData::calcTotalUsedSize(mcd);
  initDefs();
}
MemoryLoadCB::~MemoryLoadCB()
{
  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(const_cast<MemoryChainedData *>(mcd));
  mcd = NULL;
  autoDelete = false;
  initDefs();
}

MemoryChainedData *MemoryLoadCB::takeMem()
{
  MemoryChainedData *p = const_cast<MemoryChainedData *>(mcd);
  mcd = NULL;
  autoDelete = false;
  targetDataSz = -1;
  return p;
}

void MemoryLoadCB::deleteMem()
{
  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(const_cast<MemoryChainedData *>(mcd));
  mcd = NULL;
  autoDelete = false;
  targetDataSz = -1;
}
void MemoryLoadCB::setMem(const MemoryChainedData *_ch, bool auto_delete)
{
  if (autoDelete && mcd)
    MemoryChainedData::deleteChain(const_cast<MemoryChainedData *>(mcd));
  mcd = _ch;
  autoDelete = auto_delete;
  targetDataSz = MemoryChainedData::calcTotalUsedSize(mcd);
  initDefs();
}

// rest of IGenLoad interface to implement
int MemoryLoadCB::tryRead(void *ptr, int _size)
{
  ptrdiff_t size = _size;
  ptrdiff_t read_size = 0, fsize = size;

  if (blocks.size() > 0)
  {
    // when open block present, limit size by blockRest
    if (MemoryLoadCB::getBlockRest() < size)
      size = MemoryLoadCB::getBlockRest();
  }

  while (cMcd && size > 0)
  {
    ptrdiff_t ofs = (pos - cMcd->offset);
    ptrdiff_t msz = cMcd->used - ofs;

    if (msz > size)
      msz = size;

    if (msz > 0)
    {
      memcpy(ptr, cMcd->data + ofs, msz);
      size -= msz;
      ptr = (char *)ptr + msz;
      pos += msz;
      read_size += msz;

      ofs += msz;
    }
    if (ofs >= cMcd->used)
    {
      if (cMcd->next)
        cMcd = cMcd->next;
      else
        break;
    }
  }

  if (read_size < fsize)
    memset(ptr, 0, fsize - read_size);
  return (int)read_size;
}
void MemoryLoadCB::seekrel(int rel_ofs)
{
  int curPos = MemoryLoadCB::tell();
  MemoryLoadCB::seekto(curPos + rel_ofs);
  if (MemoryLoadCB::tell() - curPos != rel_ofs)
    DAGOR_THROW(LoadException("seek error", curPos));
}
void MemoryLoadCB::seekto(int _pos)
{
  pos = (_pos < 0) ? 0 : _pos;

  if (!cMcd || pos < cMcd->offset)
    cMcd = mcd;

  while (pos >= cMcd->offset + cMcd->used)
    if (cMcd->next)
      cMcd = cMcd->next;
    else
    {
      pos = cMcd->offset + cMcd->used;
      break;
    }
}

void MemoryLoadCB::initDefs()
{
  pos = 0;
  cMcd = mcd;
}

#define EXPORT_PULL dll_pull_iosys_chainedMemIo
#include <supp/exportPull.h>
