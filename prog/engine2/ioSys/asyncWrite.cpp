#include <ioSys/dag_asyncWrite.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_unicode.h>
#include <memory/dag_framemem.h>

#if _TARGET_APPLE | _TARGET_PC_LINUX
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <io.h>
#include <stddef.h>
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#define THREAD_YIELD ((void)0) // Win sleeps within poll_aio_result
#elif _TARGET_APPLE | _TARGET_PC_LINUX
#include <aio.h>
#define THREAD_YIELD sleep_msec(0)
#else
#define THREAD_YIELD sleep_msec(0)
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX
typedef HANDLE file_handle_t;
#define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
#else
typedef int file_handle_t;
#define INVALID_FILE_HANDLE -1
#endif

#define WRITE_DONE        (-1)
#define WRITE_IN_PROGRESS (0)
#define WRITE_FAILED      (1)

class AsyncWriterCB final : public IGenSave
{
  int done;
  int errorsLeft;
  file_handle_t fileHandle;
  int offs;
  const int bufMaxSize;
  Tab<char> buf, bufNext;

#if _TARGET_APPLE | _TARGET_PC_LINUX
  aiocb aio;
#elif _TARGET_PC_WIN | _TARGET_XBOX
  OVERLAPPED overlapped;
#endif

public:
  AsyncWriterCB(int buf_size);
  bool open(const char *fname, AsyncWriterMode mode);
  bool openTemp(char *fname);
  ~AsyncWriterCB();
  void close();

#if _TARGET_PC_WIN | _TARGET_XBOX
  static void __stdcall on_write_done_cb(DWORD dwErr, DWORD cbWritten, OVERLAPPED *lpOverLap);
  void poll_aio_result()
  {
    (done == WRITE_IN_PROGRESS) ? (void)SleepEx(0, TRUE) : ((void)0);
  } // aleartable sleep to be able get AIO callbacks
#elif _TARGET_APPLE | _TARGET_PC_LINUX
  void poll_aio_result();
#else
  inline void poll_aio_result() {}
#endif

  void waitPendingIO()
  {
    while (done == WRITE_IN_PROGRESS)
    {
      THREAD_YIELD;
      poll_aio_result();
    }
  }

  void write(const void *ptr, int size) override;

  int tell() override { return offs + buf.size() + bufNext.size(); }
  void seekto(int ofsabs) override
  {
    G_ASSERTF(buf.empty() && bufNext.empty(),
      "seekto() can't be called while there are exist some data in flight. call flush() first");
    offs = ofsabs;
  }
  void seektoend(int) override { issueFatal(); }
  const char *getTargetName() override { return ""; }
  void flush() override;
  void beginBlock() override { issueFatal(); }
  void endBlock(unsigned /*block_flags*/ = 0) { issueFatal(); }
  int getBlockLevel() override
  {
    issueFatal();
    return 0;
  }

  void issueFatal() { G_ASSERTF(0, "restricted by design"); }
};

#define ERRORS_BEFORE_FAILING 16

AsyncWriterCB::AsyncWriterCB(int buf_size) :
  done(WRITE_DONE),
  errorsLeft(ERRORS_BEFORE_FAILING),
  offs(0),
  buf(midmem),
  bufNext(midmem),
  bufMaxSize(buf_size),
  fileHandle(INVALID_FILE_HANDLE)
{
#if _TARGET_APPLE | _TARGET_PC_LINUX
  memset(&aio, 0, sizeof(aio));
#elif _TARGET_PC_WIN | _TARGET_XBOX
  memset(&overlapped, 0, sizeof(overlapped));
#endif
}

bool AsyncWriterCB::open(const char *fname, AsyncWriterMode mode)
{
#if _TARGET_APPLE | _TARGET_PC_LINUX
  fileHandle = ::open(fname, O_WRONLY | O_CREAT | (mode == AsyncWriterMode::TRUNC ? O_TRUNC : O_APPEND), 0666);
#elif _TARGET_PC_WIN | _TARGET_XBOX
  Tab<wchar_t> fname_u16(framemem_ptr());
  fileHandle = CreateFileW(convert_path_to_u16(fname_u16, fname), (mode == AsyncWriterMode::TRUNC ? GENERIC_WRITE : FILE_APPEND_DATA),
    FILE_SHARE_READ, NULL, (mode == AsyncWriterMode::TRUNC ? CREATE_ALWAYS : OPEN_EXISTING), FILE_FLAG_OVERLAPPED, NULL);
#else
  G_UNUSED(fname);
  G_UNUSED(mode);
#endif
  return fileHandle != INVALID_FILE_HANDLE;
}

bool AsyncWriterCB::openTemp(char *fname)
{
#if _TARGET_APPLE | _TARGET_PC_LINUX
  fileHandle = mkstemp(fname);
#elif _TARGET_PC_WIN | _TARGET_XBOX
  Tab<wchar_t> fname_u16(tmpmem);
  if (_mktemp_s(fname, DAGOR_MAX_PATH) == 0)
    fileHandle = CreateFileW(convert_path_to_u16(fname_u16, fname), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
      FILE_FLAG_OVERLAPPED, NULL);
#else
  G_UNUSED(fname);
#endif
  return fileHandle != INVALID_FILE_HANDLE;
}

AsyncWriterCB::~AsyncWriterCB() { close(); }

void AsyncWriterCB::close()
{
  if (fileHandle == INVALID_FILE_HANDLE)
    return;
  flush();
#if _TARGET_APPLE | _TARGET_PC_LINUX
  G_VERIFY(::close(fileHandle) == 0);
#elif _TARGET_PC_WIN | _TARGET_XBOX
  G_VERIFY(CloseHandle(fileHandle));
#endif
  fileHandle = INVALID_FILE_HANDLE;

  buf.clear();
  bufNext.clear();
}

#if _TARGET_PC_WIN | _TARGET_XBOX
void __stdcall AsyncWriterCB::on_write_done_cb(DWORD dwErr, DWORD cbWritten, OVERLAPPED *lpOverLap)
{
  AsyncWriterCB *wcb = (AsyncWriterCB *)((char *)lpOverLap - offsetof(AsyncWriterCB, overlapped));
  if (dwErr == ERROR_SUCCESS)
  {
    G_ASSERT(wcb->buf.size() == cbWritten);
    wcb->offs += cbWritten;
    wcb->buf.clear();
    wcb->done = WRITE_DONE;
  }
  else
  {
    wcb->done = WRITE_FAILED;
    clear_and_shrink(wcb->buf);
    clear_and_shrink(wcb->bufNext);
  }
}
#elif _TARGET_APPLE | _TARGET_PC_LINUX
void AsyncWriterCB::poll_aio_result()
{
  int bwr = 0;
  if (done == WRITE_IN_PROGRESS)
    switch (aio_error(&aio))
    {
      case EINPROGRESS: return;
      case 0:
        bwr = aio_return(&aio);
        G_ASSERT(buf.size() == bwr);
        offs += bwr;
        G_FAST_ASSERT(offs >= 0); // Attempt to write > 2G of data?
        buf.clear();
        done = WRITE_DONE;
        break;
      default: done = WRITE_FAILED; break;
    }
}
#endif

void AsyncWriterCB::write(const void *ptr, int size)
{
  G_ASSERT(fileHandle != INVALID_FILE_HANDLE);

  poll_aio_result();

  if (done == WRITE_IN_PROGRESS)
  {
    if (bufNext.size() + size <= bufMaxSize)
    {
      append_items(bufNext, size, (char *)ptr);
      return;
    }
    else // too big to grow
      waitPendingIO();
  }
  else if (done == WRITE_FAILED)
    return;

  G_ASSERT(done == WRITE_DONE);
  G_ASSERT(buf.empty());
#if (_TARGET_PC_WIN | _TARGET_XBOX) && DAGOR_DBGLEVEL > 0
  DWORD lpNumberOfBytesTransferred = 0;
  G_ASSERTF(GetOverlappedResult(fileHandle, &overlapped, &lpNumberOfBytesTransferred, /*wait*/ FALSE), "%d", GetLastError());
#endif

  buf.swap(bufNext);
  append_items(buf, size, (char *)ptr);
  if (buf.empty())
    return;

#if _TARGET_APPLE | _TARGET_PC_LINUX
  G_ASSERT(offs >= 0);
  memset(&aio, 0, sizeof(aio));
  aio.aio_fildes = fileHandle;
  aio.aio_offset = offs;
  aio.aio_buf = buf.data();
  aio.aio_nbytes = buf.size();
  aio.aio_sigevent.sigev_notify = SIGEV_NONE;
  aio.aio_sigevent.sigev_notify_function = NULL;
#elif _TARGET_PC_WIN | _TARGET_XBOX
  overlapped.Offset = offs;
#endif

  done = WRITE_IN_PROGRESS;

#if _TARGET_APPLE | _TARGET_PC_LINUX
  int failcnt = 0;
  while (aio_write(&aio) != 0)
    if (errno == EAGAIN)
      continue;
    else if (failcnt < ERRORS_BEFORE_FAILING)
      failcnt++;
    else
    {
      done = WRITE_FAILED;
      break;
    }
#elif _TARGET_PC_WIN | _TARGET_XBOX
  BOOL result = WriteFileEx(fileHandle, buf.data(), buf.size(), &overlapped, on_write_done_cb);
  // APC can't be delivered to different thread then the one that done prior request,
  // so we have no choice but to do synchronous write (wait) when this function is called from non main thread
  if (!is_main_thread() && result)
  {
    // Note: GetOverlappedResultEx is more effective then manual loop but it's not supported on win7
    DWORD _1;
    G_VERIFY(GetOverlappedResult(fileHandle, &overlapped, &_1, /*wait*/ TRUE));
    while (done == WRITE_IN_PROGRESS)
      SleepEx(0, TRUE);
  }

  if (result) // Note: this is probably not needed since drop support of Xbox360
  {
    errorsLeft = ERRORS_BEFORE_FAILING;
  }
  else
  {
    errorsLeft--;
    if (errorsLeft < 0)
    {
      done = WRITE_FAILED;
      clear_and_shrink(buf);
      clear_and_shrink(bufNext);
    }
    else
    {
      done = WRITE_DONE;
      bufNext.resize(buf.size());
      memcpy(bufNext.data(), buf.data(), buf.size());
      buf.clear();
    }
  }
#endif
}

void AsyncWriterCB::flush()
{
  waitPendingIO();
  if (bufNext.size() > 0)
  {
    write(NULL, 0);
    waitPendingIO();
  }
  G_ASSERT(buf.empty() && bufNext.empty());
}

IGenSave *create_async_writer(const char *fname, int buf_size, AsyncWriterMode mode)
{
  AsyncWriterCB *ret = new AsyncWriterCB(buf_size);
  if (!ret->open(fname, mode))
    del_it(ret);
  return ret;
}

IGenSave *create_async_writer_temp(char *in_out_fname, int buf_size)
{
  AsyncWriterCB *ret = new AsyncWriterCB(buf_size);
  if (!ret->openTemp(in_out_fname))
    del_it(ret);
  return ret;
}

#define EXPORT_PULL dll_pull_iosys_asyncWrite
#include <supp/exportPull.h>
