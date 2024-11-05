// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <unistd.h>
#include <aio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <debug/dag_debug.h>

#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

struct AsyncReadContext : public aiocb
{
  int code;
  int bytesRead;
} __attribute__((aligned(16)));

static AsyncReadContext ovPool[64] __attribute__((aligned(16)));
static uint64_t ovFreeBitmask = (uint64_t)-1;
static CritSecStorage critSec;
static bool critSecInited = false;

static int use_bit()
{
  if (!ovFreeBitmask)
    return -1;

  uint64_t test_bit = 1, i = 0;
  for (; i < 64; i++, test_bit <<= 1)
    if (ovFreeBitmask & test_bit)
      break;

  ovFreeBitmask &= ~test_bit;
  return i;
}

static bool unuse_bit(int idx)
{
  uint64_t test_bit = uint64_t(1) << idx;

  if (ovFreeBitmask & test_bit)
    return false;
  ovFreeBitmask |= test_bit;
  return true;
}


void *dfa_open_for_read(const char *fpath, bool non_cached)
{
  int fd = open(fpath, O_RDONLY);
  if (fd < 0)
  {
    if (dag_on_file_not_found)
      dag_on_file_not_found(fpath);
    return NULL;
  }
  if (dag_on_file_open)
    dag_on_file_open(fpath, (void *)(intptr_t)fd, DF_READ);
  return (void *)(intptr_t)fd;
}

void dfa_close(void *handle)
{
  if (handle)
  {
    close((int)(intptr_t)handle);
    if (dag_on_file_close)
      dag_on_file_close(handle);
  }
}

unsigned dfa_chunk_size(const char *fname)
{
  return 2048; //==
}

int dfa_file_length(void *handle)
{
  struct stat s;
  if (fstat((int)(intptr_t)handle, &s) != 0)
    return 0;
  return s.st_size;
}

static int check_unused_bit(uint64_t bits, int idx) { return (bits >> idx) & 1; }

int dfa_alloc_asyncdata()
{
  if (!critSecInited)
  {
    ::create_critical_section(critSec);
    critSecInited = true;
  }
  ::enter_critical_section(critSec);

  int idx = use_bit();
  ::leave_critical_section(critSec);
  if (idx >= 0)
    return idx;

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
  if (!unuse_bit(data_handle))
    DEBUG_CTX("already freed handle: %d", data_handle);
  else
  {
    G_ASSERT(ovPool[data_handle].code >= 0);
    ovPool[data_handle].code = 0;
  }
  ::leave_critical_section(critSec);
}

bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len)
{
  G_ASSERT(ovPool[asyncdata_handle].code >= 0);
  AsyncReadContext &p = ovPool[asyncdata_handle];

  memset(&p, 0, sizeof(p));
  p.aio_fildes = (int)(intptr_t)handle;
  p.aio_offset = offset;
  p.aio_buf = buf;
  p.aio_nbytes = len;
  p.aio_sigevent.sigev_notify = SIGEV_NONE;
  p.aio_sigevent.sigev_notify_function = NULL;
  p.code = -1;
  p.bytesRead = 0;

  int id;
  int retry = 0;
  while (aio_read(&p) != 0)
  {
    if (errno == EAGAIN)
    {
      if (retry > 20)
      {
        lseek(p.aio_fildes, offset, SEEK_SET);
        p.bytesRead = read(p.aio_fildes, buf, len);
        p.code = 0;
        logwarn("AIO failed, using sync IO: read(%d,%d)->%d", offset, len, p.bytesRead);
        return true;
      }
      sleep_msec(1);
      retry++;
      continue;
    }
    if (dag_on_read_error_cb && dag_on_read_error_cb(handle, offset, len))
      continue;
    return false;
  }

  return true;
}

bool dfa_check_complete(int asyncdata_handle, int *read_len)
{
  G_ASSERT(asyncdata_handle >= 0 && asyncdata_handle < 64);
  AsyncReadContext &p = ovPool[asyncdata_handle];
  if (p.code == 0)
  {
    *read_len = p.bytesRead;
    return true;
  }

  if (aio_error(&p) == EINPROGRESS)
  {
    *read_len = 0;
    return false;
  }

  p.code = aio_error(&p);
#if SIMULATE_READ_ERRORS
  if (p.bytesRead > 0 && grnd() < 1024)
    p.code = EIO;
#endif

  if (p.code != 0)
  {
    if (p.code == ECANCELED)
    {
      // The I/O operation has been aborted because of either a thread exit or an application request.
      *read_len = 0;
      return true;
    }

    errno = p.code;
    if (dag_on_read_error_cb && dag_on_read_error_cb((void *)(intptr_t)p.aio_fildes, (int)p.aio_offset, (int)p.aio_nbytes))
    {
      *read_len = 0;
      p.code = -1;
      if (aio_read(&p) == 0)
        return false;
    }

    // report failure
    *read_len = -1;
    return true;
  }

  *read_len = p.code == 0 ? aio_return(&p) : -1;
  return true;
}

#define EXPORT_PULL dll_pull_osapiwrappers_asyncRead
#include <supp/exportPull.h>
