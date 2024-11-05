// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_asyncIoCached.h>
#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <stdio.h>

extern void sleep_msec_ex(int ms);

// generic load interface implemented as async reader
AsyncLoadCachedCB::AsyncLoadCachedCB(const char *realname)
{
  memset(&file, 0, sizeof(file));
  memset(&buf, 0, sizeof(buf));
  file.pos = -1;
  file.size = -1;

  file.handle = dfa_open_for_read(realname, false);
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

  file.pos = 0;
  buf.size = 128 << 10;
  if (buf.size > file.size)
    buf.size = file.size;
  buf.data = (char *)memalloc(buf.size, tmpmem);
}
AsyncLoadCachedCB::~AsyncLoadCachedCB()
{
  if (buf.data)
    memfree(buf.data, tmpmem);
  if (file.handle)
    dfa_close(file.handle);

  buf.data = NULL;
  file.handle = NULL;
}

void AsyncLoadCachedCB::read(void *ptr, int size)
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
      sleep_msec_ex(1);
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
  int asyncdata_handle = dfa_alloc_asyncdata();
  while (asyncdata_handle < 0)
  {
    sleep_msec_ex(1);
    asyncdata_handle = dfa_alloc_asyncdata();
  }

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
    sleep_msec_ex(1);

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
int AsyncLoadCachedCB::tryRead(void *ptr, int size)
{
  if (file.pos + size > file.size)
    size = file.size - file.pos;
  if (size <= 0)
    return 0;
  DAGOR_TRY { AsyncLoadCachedCB::read(ptr, size); }
  DAGOR_CATCH(LoadException) { return 0; }

  return size;
}
int AsyncLoadCachedCB::tell() { return file.pos; }
void AsyncLoadCachedCB::seekto(int pos)
{
  if (pos < 0 || pos > file.size)
  {
    DEBUG_CTX("seekto(%d), file.size=%d, file.pos=%d", pos, file.size, file.pos);
    DAGOR_THROW(LoadException("seek out of range", file.pos));
  }
  file.pos = pos;
}
void AsyncLoadCachedCB::seekrel(int ofs) { seekto(file.pos + ofs); }

#define EXPORT_PULL dll_pull_iosys_asyncIoCached
#include <supp/exportPull.h>
