// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_asyncRead.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <atomic>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_atomic.h>

#define SIMULATE_READ_ERRORS 0
#if SIMULATE_READ_ERRORS
#include <math/random/dag_random.h>
#endif

struct AsyncReadData
{
  std::atomic<int> code;
  int bytesRead;

  void *handle;
  int offset;
  void *buf;
  int len;
} __attribute__((aligned(16)));

static AsyncReadData ovPool[64] __attribute__((aligned(16)));
static uint64_t ovFreeBitmask = (uint64_t)-1;
static pthread_mutex_t ov_mutex = PTHREAD_MUTEX_INITIALIZER;

#define FD2HANDLE(fd) ((void *)(intptr_t)((fd) | 0x40000000))
#define HANDLE2FD(h)  (((int)(intptr_t)(h)) & ~0x40000000)

class AsyncReadThread : public DaThread
{
public:
  AsyncReadThread() : DaThread("posix thread async reader", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), activeOvMask(0)
  {
    os_event_create(&wakeEvent, "posix_thread_read_wake_event");
  }

  ~AsyncReadThread()
  {
    terminate(false, -1, &wakeEvent);
    os_event_destroy(&wakeEvent);
  }

  void execute() override
  {
    while (!isThreadTerminating())
    {
      uint64_t toProcess = activeOvMask.exchange(0);
      if (!toProcess)
      {
        os_event_wait(&wakeEvent, OS_WAIT_INFINITE);
        toProcess = activeOvMask.exchange(0);
      }

      int idx = 0;
      while (toProcess)
      {
        if (toProcess & 1)
          process(ovPool[idx]);

        toProcess >>= 1;
        ++idx;
      }
    }
  }

  void process(AsyncReadData &ov)
  {
    int fd = HANDLE2FD(ov.handle);
    lseek(fd, ov.offset, SEEK_SET);
    ov.bytesRead = read(fd, ov.buf, ov.len);
    if (ov.bytesRead < 0)
    {
      logerr("[posix_thread_read] dfa: error %i processing (bytesRead: %i, fd: %i, offset: %i, length: %i)", errno, ov.bytesRead, fd,
        ov.offset, ov.len);
      ov.bytesRead = -1; // clamp to -1, for consistency with other AIO impls
    }

    ov.code.store(0, std::memory_order_release);
  }

  void wake(int ovIdx)
  {
    if (!activeOvMask.fetch_or(uint64_t(1) << ovIdx))
      os_event_set(&wakeEvent);
  }

  void stop()
  {
    terminate(true, -1, &wakeEvent);
    DEBUG_CTX("[posix_thread_read] dfa: read thread exited");
  }

private:
  std::atomic<uint64_t> activeOvMask;
  os_event_t wakeEvent;
};

AsyncReadThread aioReadThread;

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

  if (!aioReadThread.isThreadStarted())
  {
    if (!aioReadThread.start())
    {
      LOGERR_CTX("[posix_thread_read] dfa: failed to start read thread");
      pthread_mutex_unlock(&ov_mutex);
      return -1;
    }
  }

  int idx = use_bit();
  pthread_mutex_unlock(&ov_mutex);

  if (idx >= 0)
  {
    return idx;
  }

  DEBUG_CTX("[posix_thread_read] dfa: no more free handles");
  return -1;
}

void dfa_free_asyncdata(int data_handle)
{
  G_ASSERTF(data_handle >= 0 && data_handle < 64, "[posix_thread_read] dfa: incorrect handle: %d", data_handle);

  pthread_mutex_lock(&ov_mutex);
  bool unused = unuse_bit(data_handle);
  if (unused)
  {
    G_ASSERT(ovPool[data_handle].code >= 0);
    ovPool[data_handle].code = 0;

    if (ovFreeBitmask == (uint64_t)-1)
      aioReadThread.stop();
  }
  pthread_mutex_unlock(&ov_mutex);
  if (!unused)
    DEBUG_CTX("[posix_thread_read] dfa: already freed handle: %d", data_handle);
}

bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len)
{
  G_ASSERT(ovPool[asyncdata_handle].code >= 0);
  AsyncReadData &p = ovPool[asyncdata_handle];

  // sync with release fence in thread wake
  p.code.store(-1, std::memory_order_relaxed);
  p.bytesRead = 0;
  p.handle = handle;
  p.offset = offset;
  p.buf = buf;
  p.len = len;

  aioReadThread.wake(asyncdata_handle);

  return true;
}

bool dfa_check_complete(int asyncdata_handle, int *read_len)
{
  G_ASSERT(asyncdata_handle >= 0 && asyncdata_handle < 64);
  AsyncReadData &p = ovPool[asyncdata_handle];
  if (p.code.load(std::memory_order_acquire) == 0)
  {
#if SIMULATE_READ_ERRORS
    if (p.bytesRead > 0 && grnd() < 1024)
    {
      *read_len = -1;
      return true;
    }
#endif
    *read_len = p.bytesRead;
    return true;
  }

  *read_len = 0;
  return false;
}

#define EXPORT_PULL dll_pull_osapiwrappers_asyncRead
#include <supp/exportPull.h>
