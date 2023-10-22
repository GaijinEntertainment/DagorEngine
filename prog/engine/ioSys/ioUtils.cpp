// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>

#if _TARGET_PC
static constexpr int BUF_SZ = 32 << 10;
#else
static constexpr int BUF_SZ = 16 << 10;
#endif

void write_zeros(IGenSave &cwr, int size)
{

  char zero[BUF_SZ];
  memset(zero, 0, BUF_SZ);

  while (size > BUF_SZ)
  {
    cwr.write(zero, BUF_SZ);
    size -= BUF_SZ;
  }

  if (size)
    cwr.write(zero, size);
}

void copy_stream_to_stream(IGenLoad &crd, IGenSave &cwr, int size)
{
  int len;
  char buf[BUF_SZ];
  while (size > 0)
  {
    len = size > BUF_SZ ? BUF_SZ : size;
    size -= len;
    crd.read(buf, len);
    cwr.write(buf, len);
  }
}
void copy_file_to_stream(file_ptr_t fp, IGenSave &cwr, int size)
{
  int len;
  char buf[BUF_SZ];
  while (size > 0)
  {
    len = size > BUF_SZ ? BUF_SZ : size;
    size -= len;
    df_read(fp, buf, len);
    cwr.write(buf, len);
  }
}
void copy_file_to_stream(file_ptr_t fp, IGenSave &cwr) { copy_file_to_stream(fp, cwr, df_length(fp)); }


void copy_file_to_stream(const char *fname, IGenSave &cwr)
{
  file_ptr_t fp = df_open(fname, DF_READ);
  if (!fp)
    DAGOR_THROW(IGenSave::SaveException("file not found", 0));
  copy_file_to_stream(fp, cwr, df_length(fp));
  df_close(fp);
}

#define EXPORT_PULL dll_pull_iosys_ioUtils
#include <supp/exportPull.h>
