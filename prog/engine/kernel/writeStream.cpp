// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "writeStream.h"
#include <ioSys/dag_asyncWrite.h>

namespace debug_internal
{
#if _TARGET_PC_WIN // need additional investigation about alertable states and WriteFileEx from different threads
#define USE_ASYNC_WRITER 0
#else
#define USE_ASYNC_WRITER 1
#endif

#if USE_ASYNC_WRITER
#define IS_FILE_STREAM(s) ((uintptr_t)s & 1)
#else
#define IS_FILE_STREAM(s) (1)
#endif

static const char *WRITE_MODE = "wb";
void write_stream_set_fmode(const char *fmode) { WRITE_MODE = fmode; }

static inline FILE *stream2file(write_stream_t stream)
{
  G_ASSERT(IS_FILE_STREAM(stream));
  return (FILE *)((uintptr_t)stream ^ 1);
}

write_stream_t write_stream_open(const char *path, int buf_size, FILE *fp, bool reopen)
{
  if (fp)
    return reopen ? file2stream(freopen(path, "wt", fp)) : file2stream(fp);
  else
  {
    (void)buf_size;
#if USE_ASYNC_WRITER
    const AsyncWriterMode mode = WRITE_MODE[0] == 'a' ? AsyncWriterMode::APPEND : AsyncWriterMode::TRUNC;
    IGenSave *ret = create_async_writer(path, buf_size, mode);
    if (ret)
      return ret;
    else // fallback to synchronous API in case if async writer is not supported on this platform
#endif
      return file2stream(fopen(path, WRITE_MODE));
  }
}

void write_stream_close(write_stream_t stream)
{
  if (IS_FILE_STREAM(stream))
    fclose(stream2file(stream));
  else
    delete (IGenSave *)stream;
}

void write_stream_flush(write_stream_t stream)
{
  if (IS_FILE_STREAM(stream))
    fflush(stream2file(stream));
  else
    ((IGenSave *)stream)->flush();
}

void write_stream_write(const void *ptr, size_t len, write_stream_t stream)
{
  if (IS_FILE_STREAM(stream))
  {
    if (fwrite(ptr, 1, len, stream2file(stream)))
    { // workaround against gcc's warn_unused_result warning
    }
  }
  else
    ((IGenSave *)stream)->write(ptr, (int)len);
}

} // namespace debug_internal
