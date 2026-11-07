// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <math/dag_intrin.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <EASTL/algorithm.h>
#include <stdio.h>
#include <limits.h>
#include <io.h>
#include <malloc.h>

#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

#define MAX_RETRIES_ON_READ_ERROR 3
#define SLEEP_MSEC_ON_READ_ERROR  50

struct AsyncReadContext : public OVERLAPPED
{
  void *bufPtr;
  int bytesRead;
  uint8_t retryCount;
  bool complete;
  bool errReported;
};

static volatile uint64_t ovFreeBitmask = ~uint64_t(0);
static constexpr int ASYNCDATA_COUNT = sizeof(ovFreeBitmask) * CHAR_BIT;
static AsyncReadContext ovPool[ASYNCDATA_COUNT];

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

  int fpath_slen = (int)strlen(fpath);
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

static int use_bit()
{
  for (uint64_t cur = interlocked_relaxed_load(ovFreeBitmask); cur;)
  {
    const uint64_t prev = interlocked_compare_exchange(ovFreeBitmask, __blsr(cur), cur);
    if (prev == cur)
      return __ctz_unsafe(cur);
    cur = prev; // lost the race, retry against what the winner left
  }
  return -1;
}

static bool unuse_bit(int idx) // false when the bit was set already, the handle is freed twice
{
  const uint64_t test_bit = uint64_t(1) << idx;
  return (interlocked_or(ovFreeBitmask, test_bit) & test_bit) == 0;
}

static bool check_unused_bit(int idx) { return (interlocked_relaxed_load(ovFreeBitmask) >> idx) & 1; }

int dfa_alloc_asyncdata()
{
  int idx = use_bit();
  if (idx >= 0)
    return idx;

  DEBUG_CTX("no more free handles");
  return -1;
}

void dfa_free_asyncdata(int data_handle)
{
  if ((unsigned)data_handle >= ASYNCDATA_COUNT)
  {
    DEBUG_CTX("incorrect handle: %d", data_handle);
    return;
  }

  if (!unuse_bit(data_handle))
    DEBUG_CTX("already freed handle: %d", data_handle);
}

static void __stdcall file_io_cr(unsigned long dwErrorCode, unsigned long dwNumberOfBytesTransfered, OVERLAPPED *lpOverlapped)
{
  AsyncReadContext *ctx = (AsyncReadContext *)lpOverlapped;
  if (dwErrorCode == ERROR_SUCCESS)
    ctx->bytesRead = dwNumberOfBytesTransfered;
  else
  {
    ctx->InternalHigh = ctx->bytesRead;
    ctx->bytesRead = -(int)dwErrorCode; // Store negative error code for future retrieval by `dfa_check_complete`
  }
  ctx->complete = true;
}

static inline bool place_read_request(AsyncReadContext &ctx, decltype(dag_on_read_error_cb) errcb)
{
  while (1)
  {
    BOOL ret = ReadFileEx(ctx.hEvent, ctx.bufPtr, ctx.bytesRead, &ctx, file_io_cr);
    if (int err = ret ? ERROR_SUCCESS : GetLastError(); err != ERROR_SUCCESS)
    {
      logwarn("error placing async ReadFileEx(h=%p, ofs=%d, len=%d, buf=%p)=%d", ctx.hEvent, ctx.Offset, ctx.bytesRead, ctx.bufPtr,
        err);
      if (errcb && errcb(ctx.hEvent, ctx.Offset, ctx.bytesRead))
        continue;
      return false;
    }
    return true;
  }
}

bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len)
{
  if ((unsigned)asyncdata_handle >= ASYNCDATA_COUNT)
  {
    DEBUG_CTX("incorrect handle: %d", asyncdata_handle);
    return false;
  }
  if (check_unused_bit(asyncdata_handle))
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

  return place_read_request(p, dag_on_read_error_cb);
}

bool dfa_check_complete(int asyncdata_handle, int *read_len)
{
  G_ASSERT((unsigned)asyncdata_handle < ASYNCDATA_COUNT);

  // DEBUG_CTX("asyncdata_handle=%d complete=%d", asyncdata_handle, ovPool[asyncdata_handle].complete);
  if (!ovPool[asyncdata_handle].complete)
  {
    SleepEx(0, TRUE);
    if (!ovPool[asyncdata_handle].complete)
      return false;
  }

  if (auto ctx = &ovPool[asyncdata_handle]; ctx->bytesRead < 0) [[unlikely]] // Error condition?
  {
    // Limited retry count with small chance of error being transient
    static constexpr int transientErrCodes[] = {ERROR_IO_DEVICE, ERROR_CRC, ERROR_SEM_TIMEOUT, ERROR_DEVICE_NOT_CONNECTED,
      ERROR_DEVICE_HARDWARE_ERROR, ERROR_NOT_READY, ERROR_BUSY, ERROR_RETRY, ERROR_DEVICE_UNREACHABLE, ERROR_NO_SYSTEM_RESOURCES,
      ERROR_WORKING_SET_QUOTA, ERROR_NOT_ENOUGH_MEMORY};
    using namespace eastl;
    int maxRetries = 0;
    if (find(begin(transientErrCodes), end(transientErrCodes), -ctx->bytesRead) != end(transientErrCodes))
      maxRetries = MAX_RETRIES_ON_READ_ERROR;
    if (ctx->retryCount < maxRetries)
    {
      const int err = ctx->bytesRead, len = (int)ctx->InternalHigh;
      logwarn("ReadFileEx(h=%p, ofs=%d, len=%d, buf=%p) failed with %d, sleep %d ms and do %d-th retry", ctx->hEvent, ctx->Offset, len,
        ctx->bufPtr, -err, SLEEP_MSEC_ON_READ_ERROR << ctx->retryCount, ctx->retryCount);
      Sleep(SLEEP_MSEC_ON_READ_ERROR << ctx->retryCount++);
      ctx->bytesRead = len;
      ctx->Internal = ctx->InternalHigh = 0;
      ctx->complete = false;
      if (place_read_request(*ctx, nullptr))
        return false;
      // refused before dispatch: a bad request, not a device error, so there is nothing to retry
      ctx->bytesRead = err;
      ctx->InternalHigh = len;
      ctx->complete = true;
    }
    if (dag_on_read_error_cb && !eastl::exchange(ctx->errReported, true))
      dag_on_read_error_cb(ctx->hEvent, (int)ctx->Offset, (int)ctx->InternalHigh);
  }

  // DEBUG_CTX("asyncdata_handle=%d complete=%d errCode=%p bytesRed=%d",
  //   asyncdata_handle, ovPool[asyncdata_handle].complete, ovPool[asyncdata_handle].errCode, ovPool[asyncdata_handle].bytesRead);

  if (read_len)
    *read_len = ovPool[asyncdata_handle].bytesRead;
  return true;
}

void dfa_wait_until_complete(int adh, int *readed_len)
{
  while (!dfa_check_complete(adh, readed_len))
    SleepEx(INFINITE, TRUE); // file_io_cr is an APC to the thread that placed the request, it ends the sleep
}

#define EXPORT_PULL dll_pull_osapiwrappers_asyncRead
#include <supp/exportPull.h>
