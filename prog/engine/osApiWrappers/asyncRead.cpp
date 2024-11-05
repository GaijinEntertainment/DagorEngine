// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <io.h>
#include <malloc.h>

#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

static CritSecStorage critSec;
static bool critSecInited = false;

struct AsyncReadContext : public OVERLAPPED
{
  int bytesRead;
  bool complete;
  void *bufPtr;
};

static AsyncReadContext ovPool[64];
static unsigned ovFreeBitmask1 = 0xFFFFFFFF;
static unsigned ovFreeBitmask2 = 0xFFFFFFFF;

void *dfa_open_for_read(const char *fpath, bool non_cached)
{
  if (dag_on_file_pre_open)
    if (!dag_on_file_pre_open(fpath))
    {
      DEBUG_CTX("error opening <%s> for read; err=0x%p", fpath, GetLastError());
      if (dag_on_file_not_found)
        dag_on_file_not_found(fpath);
      return NULL;
    }

  int fpath_slen = i_strlen(fpath);
  wchar_t *fpath_u16 = (wchar_t *)alloca((fpath_slen + 1) * sizeof(wchar_t));
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fpath, fpath_slen + 1, fpath_u16, fpath_slen + 1) == 0)
    MultiByteToWideChar(CP_ACP, 0, fpath, fpath_slen + 1, fpath_u16, fpath_slen + 1);

  HANDLE h = CreateFileW(fpath_u16, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN | (non_cached ? FILE_FLAG_NO_BUFFERING : 0), NULL);
  if (h == INVALID_HANDLE_VALUE)
  {
    DEBUG_CTX("error opening <%s> for read; err=0x%p", fpath, GetLastError());
    if (dag_on_file_not_found)
      dag_on_file_not_found(fpath);
    return NULL;
  }
  // DEBUG_CTX("fpath=<%s>, handle=%p", fpath, h);
  if (dag_on_file_open)
    dag_on_file_open(fpath, h, DF_READ);
  return h;
}

void dfa_close(void *handle)
{
  if (handle == INVALID_HANDLE_VALUE)
  {
    DEBUG_CTX("invalid handle=%p", handle);
    return;
  }
  CloseHandle(handle);
  if (dag_on_file_close)
    dag_on_file_close(handle);
}

unsigned dfa_chunk_size(const char *fname)
{
#if _TARGET_PC
  char pathname[DAGOR_MAX_PATH];
  dd_get_fname_location(pathname, fname);

  DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
  if (GetDiskFreeSpace(pathname[0] ? pathname : NULL, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters))
    return bytesPerSector;
  else
    return 2048;
#elif _TARGET_XBOX
  (void)fname;
  return 4096;
#else
  (void)fname;
  return 2048; // sector size for DVD-ROM (used on XBOX/XBOX360)
#endif
}

int dfa_file_length(void *handle)
{
  LARGE_INTEGER size;
  BOOL res = GetFileSizeEx(handle, &size);
  if (!res)
    return INVALID_FILE_SIZE;

  G_ASSERT(size.HighPart == 0 && size.LowPart < 0x7FFFFFFF);
  return size.LowPart;
}

static int use_bit(unsigned *bits)
{
  unsigned mask = *bits;
  if (!mask)
    return -1;

  unsigned test_bit = 1, i = 0;
  for (; i < 32; i++, test_bit <<= 1)
    if (mask & test_bit)
      break;

  *bits = mask & ~test_bit;
  return i;
}

static bool unuse_bit(unsigned *bits, int idx)
{
  unsigned test_bit = 1U << idx, mask = *bits;

  if (mask & test_bit)
    return false;
  *bits = mask | test_bit;
  return true;
}

static int check_unused_bit(unsigned bits, int idx) { return bits & (1U << idx); }

int dfa_alloc_asyncdata()
{
  if (!critSecInited)
  {
    ::create_critical_section(critSec);
    critSecInited = true;
  }
  ::enter_critical_section(critSec);

  int idx = use_bit(&ovFreeBitmask1);
  if (idx >= 0)
  {
    ::leave_critical_section(critSec);
    return idx;
  }

  idx = use_bit(&ovFreeBitmask2);
  ::leave_critical_section(critSec);
  if (idx >= 0)
    return idx + 32;

  DEBUG_CTX("no more free handles");
  return -1;
}

void dfa_free_asyncdata(int data_handle)
{
  if (data_handle < 0 || data_handle >= 64)
  {
    DEBUG_CTX("incorrect handle: %d", data_handle);
    return;
  }
  if (!critSecInited)
    return;

  ::enter_critical_section(critSec);
  if (!unuse_bit(data_handle < 32 ? &ovFreeBitmask1 : &ovFreeBitmask2, data_handle % 32))
    DEBUG_CTX("already freed handle: %d", data_handle);
  ::leave_critical_section(critSec);
}

static void __stdcall file_io_cr(unsigned long dwErrorCode, unsigned long dwNumberOfBytesTransfered, OVERLAPPED *lpOverlapped)
{
  AsyncReadContext *ctx = (AsyncReadContext *)lpOverlapped;
  if (dwErrorCode == ERROR_SUCCESS)
    ctx->bytesRead = dwNumberOfBytesTransfered;
  else
  {
    int len = ctx->bytesRead;
    ctx->bytesRead = -(int)dwErrorCode; // store negative error code for future retrieval by dfa_check_complete
    if (dag_on_read_error_cb)
      dag_on_read_error_cb(ctx->hEvent, (int)ctx->Offset, len); // TODO: pass errcode to callback
  }
  ctx->complete = true;
}

bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len)
{
  if (asyncdata_handle < 0 || asyncdata_handle >= 64)
  {
    DEBUG_CTX("incorrect handle: %d", asyncdata_handle);
    return false;
  }
  if (check_unused_bit(asyncdata_handle < 32 ? ovFreeBitmask1 : ovFreeBitmask2, asyncdata_handle % 32))
  {
    DEBUG_CTX("not-opened handle: %d", asyncdata_handle);
    return false;
  }

  AsyncReadContext &p = ovPool[asyncdata_handle];
  memset(&p, 0, sizeof(p));

  p.Offset = offset;
  p.hEvent = handle; // "The ReadFileEx function ignores the OVERLAPPED structure's hEvent member."
  p.bufPtr = buf;
  p.bytesRead = len;

place_req:
  int ret = ReadFileEx(handle, buf, len, &p, file_io_cr);
  if (!ret && GetLastError() != ERROR_SUCCESS)
  {
    DEBUG_CTX("error starting async read ReadFileEx(h=%p, ofs=%d, len=%d, buf=%p); ret=%d err=0x%p", handle, offset, len, buf, ret,
      GetLastError());
    if (dag_on_read_error_cb && dag_on_read_error_cb(handle, offset, len))
      goto place_req;
    return false;
  }

  return true;
}

bool dfa_check_complete(int asyncdata_handle, int *read_len)
{
  G_ASSERT(asyncdata_handle >= 0 && asyncdata_handle < 64);

  // DEBUG_CTX("asyncdata_handle=%d complete=%d", asyncdata_handle, ovPool[asyncdata_handle].complete);
  if (!ovPool[asyncdata_handle].complete)
  {
    SleepEx(0, TRUE);
    if (!ovPool[asyncdata_handle].complete)
      return false;
  }
  // DEBUG_CTX("asyncdata_handle=%d complete=%d errCode=%p bytesRed=%d",
  //   asyncdata_handle, ovPool[asyncdata_handle].complete, ovPool[asyncdata_handle].errCode, ovPool[asyncdata_handle].bytesRead);

  if (read_len)
    *read_len = ovPool[asyncdata_handle].bytesRead;
  return true;
}

#define EXPORT_PULL dll_pull_osapiwrappers_asyncRead
#include <supp/exportPull.h>
