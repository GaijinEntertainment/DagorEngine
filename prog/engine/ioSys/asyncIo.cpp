// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_asyncIo.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <stdio.h>

extern void sleep_msec_ex(int ms);

// generic load interface implemented as async reader
AsyncLoadCB::AsyncLoadCB(const char *realname)
{
  memset(&file, 0, sizeof(file));
  memset(&buf, 0, sizeof(buf));
  file.pos = -1;
  file.size = -1;

  buf.size = 64 << 10;

  file.pos = 0;
  // debug("load: %s", fpath);
  buf.minimumChunk = dfa_chunk_size(realname);
  if (buf.minimumChunk)
  {
    buf.size = buf.minimumChunk * 128;
    if (buf.size > (128 << 10))
    {
      buf.size = 128 << 10;
      if (buf.size < buf.minimumChunk)
        buf.size = buf.minimumChunk;
    }
    // debug("sectorSize = %d", bytesPerSector);
    file.handle = dfa_open_for_read(realname, true);
  }
  else
  {
    DEBUG_CTX("!getFreeSize");
    buf.minimumChunk = 1;
    buf.size = 64 << 10;
    file.handle = dfa_open_for_read(realname, false);
  }

  targetFilename = realname;
  if (!file.handle)
    return;

  file.size = dfa_file_length(file.handle);
  if (file.size <= 0)
  {
    dfa_close(file.handle);
    file.handle = NULL;
    return;
  }

  // if ( buf.size > file.size ) buf.size = file.size;
  buf.data = (char *)tmpmem->allocAligned(buf.size, buf.minimumChunk);
}

AsyncLoadCB::~AsyncLoadCB()
{
  if (buf.data)
    tmpmem->freeAligned(buf.data);
  if (file.handle)
    dfa_close(file.handle);

  buf.data = NULL;
  file.handle = NULL;
}

void AsyncLoadCB::readBuffered(void *ptr, int size)
{
  if (!size)
    return;
  if (file.pos + size > file.size)
  {
    DEBUG_CTX("read(%p,%d), file.size=%d, file.pos=%d", ptr, size, file.size, file.pos);
    DAGOR_THROW(LoadException("eof", file.pos));
  }
  if (file.pos >= buf.pos && file.pos < buf.pos + buf.used)
  {
    int sz;

    if (file.pos + size < buf.pos + buf.used)
      sz = size;
    else
      sz = buf.pos + buf.used - file.pos;
    // debug ( "read to %p  sz=%d (from %p)", ptr, sz, file.pos );
    memcpy(ptr, buf.data + file.pos - buf.pos, sz);

    file.pos += sz;
    ptr = ((char *)ptr) + sz;
    size -= sz;
    // measure.stop();
    // debug("memcpy %d", (int)measure.getTotal());
  }

  if (!size)
    return;

  if (size > buf.size)
  {
    // read to data ptr immediately
    int asyncdata_handle = dfa_alloc_asyncdata();
    while (asyncdata_handle < 0)
    {
      sleep_msec_ex(1);
      asyncdata_handle = dfa_alloc_asyncdata();
    }

    if (!dfa_read_async(file.handle, asyncdata_handle, file.pos, ptr, size))
    {
      dfa_free_asyncdata(asyncdata_handle);
      DAGOR_THROW(LoadException("can't place read request", file.pos));
      return;
    }
    sleep_msec_ex(0);

    int lres;
    while (!dfa_check_complete(asyncdata_handle, &lres))
    {
      sleep_msec_ex(1);
    }
    dfa_free_asyncdata(asyncdata_handle);

    if (lres != size)
    {
      DEBUG_CTX("read(%p,%d), file.size=%d, file.pos=%d", ptr, size, file.size, file.pos);
      DAGOR_THROW(LoadException("incomplete read", file.pos));
    }

    file.pos += size;
    return;
  }
  // read to buffer and copy to data ptr
  // measure.go();
  int asyncdata_handle = dfa_alloc_asyncdata();
  while (asyncdata_handle < 0)
  {
    sleep_msec_ex(1);
    asyncdata_handle = dfa_alloc_asyncdata();
  }
  // measure.stop();
  // debug("alloc %d", (int)measure.getTotal());

  if (file.pos + buf.size > file.size)
    buf.used = file.size - file.pos;
  else
    buf.used = buf.size;
  buf.pos = file.pos;

  if (!dfa_read_async(file.handle, asyncdata_handle, buf.pos, buf.data, buf.used))
  {
    dfa_free_asyncdata(asyncdata_handle);
    DAGOR_THROW(LoadException("can't place read request", file.pos));
    return;
  }
  sleep_msec_ex(0);

  int lres;
  while (!dfa_check_complete(asyncdata_handle, &lres))
  {
    sleep_msec_ex(1);
  }

  dfa_free_asyncdata(asyncdata_handle);
  if (lres != buf.used)
  {
    DEBUG_CTX("read(%p,%d), file.size=%d, file.pos=%d", ptr, size, file.size, file.pos);
    DAGOR_THROW(LoadException("incomplete read", file.pos));
  }

  // debug ( "read to %p  sz=%d (from %p)", ptr, size, file.pos );
  memcpy(ptr, buf.data, size);
  file.pos += size;
}


void AsyncLoadCB::read(void *ptr, int size)
{
  if (buf.minimumChunk == 1)
  {
    readBuffered(ptr, size);
    return;
  }
  if (file.pos + size > file.size)
  {
    DEBUG_CTX("read(%p,%d), file.size=%d, file.pos=%d", ptr, size, file.size, file.pos);
    DAGOR_THROW(LoadException("eof", file.pos));
  }

  if (file.pos >= buf.pos && file.pos < buf.pos + buf.used)
  {
    int sz;

    if (file.pos + size < buf.pos + buf.used)
      sz = size;
    else
      sz = buf.pos + buf.used - file.pos;
    // memcpy ( ptr, buf.data+file.pos-buf.pos, sz );
    memcpy(ptr, buf.data + file.pos - buf.pos, sz);

    file.pos += sz;
    ptr = ((char *)ptr) + sz;
    size -= sz;
  }
  if (!size)
    return;


  int posStart = file.pos;
  int readSize = size;
  int chunkBits = (buf.minimumChunk - 1);
  if (file.pos & chunkBits)
  {
    posStart = file.pos & (~chunkBits);
    readSize += file.pos - posStart;
  }

  while (readSize > buf.size)
  {
    int asyncdata_handle = dfa_alloc_asyncdata();
    while (asyncdata_handle < 0)
    {
      sleep_msec_ex(1);
      asyncdata_handle = dfa_alloc_asyncdata();
    }

    if (!dfa_read_async(file.handle, asyncdata_handle, posStart, buf.data, buf.size))
    {
      dfa_free_asyncdata(asyncdata_handle);
      DAGOR_THROW(LoadException("can't place read request", file.pos));
      return;
    }

    int lres;
    while (!dfa_check_complete(asyncdata_handle, &lres))
    {
      sleep_msec_ex(0);
    }
    dfa_free_asyncdata(asyncdata_handle);

    if (lres != buf.size)
    {
      DEBUG_CTX("read(%p,%d), file.size=%d, file.pos=%d", ptr, buf.size, file.size, file.pos);
      DAGOR_THROW(LoadException("incomplete read", file.pos));
    }

    int offset = file.pos - posStart;
    posStart += buf.size;
    file.pos = posStart;
    memcpy(ptr, buf.data + offset, buf.size - offset);
    ptr = (char *)ptr + buf.size - offset;
    readSize -= buf.size;
  }

  if (!readSize)
    return;


  int sizeLeft = file.size - posStart;
  if (sizeLeft > buf.size)
    sizeLeft = buf.size;
  else if (sizeLeft & chunkBits)
  {
    sizeLeft = buf.minimumChunk + (sizeLeft & (~chunkBits));
  }

  int asyncdata_handle = dfa_alloc_asyncdata();
  while (asyncdata_handle < 0)
  {
    sleep_msec_ex(1);
    asyncdata_handle = dfa_alloc_asyncdata();
  }

  if (!dfa_read_async(file.handle, asyncdata_handle, posStart, buf.data, sizeLeft))
  {
    dfa_free_asyncdata(asyncdata_handle);
    DAGOR_THROW(LoadException("can't place read request", file.pos));
    return;
  }

  int lres;
  while (!dfa_check_complete(asyncdata_handle, &lres))
  {
    sleep_msec_ex(0);
  }
  dfa_free_asyncdata(asyncdata_handle);

  if (lres != sizeLeft && lres != file.size - posStart)
  { // readSize - useLessData
    DEBUG_CTX("%d read(%p, size=%d readSize=%d), file.size=%d, file.pos=%d posStart=%d", lres, ptr, size, sizeLeft, file.size,
      file.pos, posStart);
    DAGOR_THROW(LoadException("incomplete read", file.pos));
  }
  buf.pos = posStart;
  buf.used = lres;

  int offset = file.pos - posStart;
  file.pos = posStart + readSize;
  memcpy(ptr, buf.data + offset, readSize - offset);
}


int AsyncLoadCB::tryRead(void *ptr, int size)
{
  if (file.pos + size > file.size)
    size = file.size - file.pos;
  if (size <= 0)
    return 0;
  DAGOR_TRY { AsyncLoadCB::read(ptr, size); }
  DAGOR_CATCH(LoadException) { return 0; }

  return size;
}

int AsyncLoadCB::tell() { return file.pos; }

void AsyncLoadCB::seekto(int pos)
{
  if (pos < 0 || pos > file.size)
  {
    DEBUG_CTX("seekto(%d), file.size=%d, file.pos=%d", pos, file.size, file.pos);
    DAGOR_THROW(LoadException("seek out of range", file.pos));
  }
  // DEBUG_CTX("seekto(%d), file.size=%d, file.pos=%d", pos, file.size, file.pos);
  file.pos = pos;
}

void AsyncLoadCB::seekrel(int ofs) { seekto(file.pos + ofs); }

#define EXPORT_PULL dll_pull_iosys_asyncIo
#include <supp/exportPull.h>
