#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <unistd.h>
#include <aio.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <debug/dag_debug.h>

#define DAGOR_ASYNC_IO 1

#if DAGOR_ASYNC_IO
#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

struct AsyncReadContext : public aiocb
{
  int code;
  int bytesRead;
} __attribute__((aligned(16)));

#else
struct AsyncReadContext
{
  int code;
  int bytesRead;
};
#endif

static AsyncReadContext ovPool[64] __attribute__((aligned(16)));
static uint64_t ovFreeBitmask = (uint64_t)-1;
static pthread_mutex_t ov_mutex = PTHREAD_MUTEX_INITIALIZER;

#define FD2HANDLE(fd) ((void *)(intptr_t)((fd) | 0x40000000))
#define HANDLE2FD(h)  (((int)(intptr_t)(h)) & ~0x40000000)

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
  G_UNREFERENCED(non_cached);
  int fd = open(fpath, O_RDONLY);
  if (fd < 0)
  {
    if (dag_on_file_not_found)
      dag_on_file_not_found(fpath);
    return NULL;
  }
  void *h = FD2HANDLE(fd);
  if (dag_on_file_open)
    dag_on_file_open(fpath, h, DF_READ);
  return h;
}

void dfa_close(void *handle)
{
  if (!handle)
    return;
  close(HANDLE2FD(handle));
  if (dag_on_file_close)
    dag_on_file_close(handle);
}

unsigned dfa_chunk_size(const char *fname)
{
  G_UNREFERENCED(fname);
  return 2048; //==
}

int dfa_file_length(void *handle)
{
  struct stat st;
  return (fstat(HANDLE2FD(handle), &st) == 0) ? st.st_size : 0;
}

static int check_unused_bit(uint64_t bits, int idx) { return (bits >> idx) & 1; }

int dfa_alloc_asyncdata()
{
  pthread_mutex_lock(&ov_mutex);
  int idx = use_bit();
  pthread_mutex_unlock(&ov_mutex);
  if (idx >= 0)
    return idx;

  debug_ctx("no more free handles");
  return -1;
}

void dfa_free_asyncdata(int data_handle)
{
  if (data_handle < 0 || data_handle >= 64)
  {
    debug_ctx("incorrect handle: %d", data_handle);
    return;
  }

  pthread_mutex_lock(&ov_mutex);
  bool unused = unuse_bit(data_handle);
  if (unused)
  {
    G_ASSERT(ovPool[data_handle].code >= 0);
    ovPool[data_handle].code = 0;
  }
  pthread_mutex_unlock(&ov_mutex);
  if (!unused)
    debug_ctx("already freed handle: %d", data_handle);
}

bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len)
{
  G_ASSERT(ovPool[asyncdata_handle].code >= 0);
  AsyncReadContext &p = ovPool[asyncdata_handle];

#if DAGOR_ASYNC_IO
  memset(&p, 0, sizeof(p));
  p.aio_fildes = HANDLE2FD(handle);
  p.aio_offset = offset;
  p.aio_buf = buf;
  p.aio_nbytes = len;
  p.aio_sigevent.sigev_notify = SIGEV_NONE;
  p.aio_sigevent.sigev_notify_function = NULL;
#endif
  p.code = -1;
  p.bytesRead = 0;

#if DAGOR_ASYNC_IO
  while (aio_read(&p) != 0)
  {
    if (errno == EAGAIN)
      continue;
    // debug("errno=%d, p[%d]=(%d,%d,%p,%d,%d)",
    //   errno, asyncdata_handle, p.aio_fildes, p.aio_offset, p.aio_buf, p.aio_nbytes, p.aio_reqprio);
    if (dag_on_read_error_cb && dag_on_read_error_cb(handle, offset, len))
      continue;
    return false;
  }
#else
  lseek(HANDLE2FD(handle), offset, SEEK_SET);
  p.bytesRead = read(HANDLE2FD(handle), buf, len);
  p.code = 0;
#endif

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
#if DAGOR_ASYNC_IO
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
    if (dag_on_read_error_cb && dag_on_read_error_cb(FD2HANDLE(p.aio_fildes), (int)p.aio_offset, (int)p.aio_nbytes))
    {
      *read_len = 0;
      if (aio_read(&p) == 0)
      {
        p.code = -1;
        return false;
      }
    }

    // report failure
    *read_len = -p.code;
    return true;
  }

  *read_len = p.code == 0 ? aio_return(&p) : -p.code;
  return true;
#else
  *read_len = 0;
  return false;
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_asyncRead
#include <supp/exportPull.h>
