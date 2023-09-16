// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>

LFileGeneralSaveCB::LFileGeneralSaveCB(file_ptr_t handle) : fileHandle(handle) {}

void LFileGeneralSaveCB::write(const void *ptr, int size)
{
  if (!fileHandle)
    DAGOR_THROW(SaveException("file not open", 0));
  if (df_write(fileHandle, (void *)ptr, size) != size)
    DAGOR_THROW(SaveException("write error", tell()));
}

int LFileGeneralSaveCB::tryWrite(const void *ptr, int size) { return fileHandle ? df_write(fileHandle, ptr, size) : -1; }

int LFileGeneralSaveCB::tell()
{
  if (!fileHandle)
    DAGOR_THROW(SaveException("file not open", 0));
  int o = df_tell(fileHandle);
  if (o == -1)
    DAGOR_THROW(SaveException("tell returns error", 0));
  return o;
}

void LFileGeneralSaveCB::seekto(int o)
{
  if (!fileHandle)
    DAGOR_THROW(SaveException("file not open", 0));
  if (df_seek_to(fileHandle, o) == -1)
    DAGOR_THROW(SaveException("seek error", tell()));
}

void LFileGeneralSaveCB::seektoend(int o)
{
  if (!fileHandle)
    DAGOR_THROW(SaveException("file not open", 0));
  if (df_seek_end(fileHandle, o) == -1)
    DAGOR_THROW(SaveException("seek error", tell()));
}

void LFileGeneralSaveCB::flush()
{
  if (fileHandle)
    df_flush(fileHandle);
}

LFileGeneralLoadCB::LFileGeneralLoadCB(file_ptr_t handle) : fileHandle(handle) {}

const VirtualRomFsData *LFileGeneralLoadCB::getTargetVromFs() const { return df_get_vromfs_for_file_ptr(fileHandle); }
void LFileGeneralLoadCB::read(void *ptr, int size)
{
  if (!fileHandle)
    DAGOR_THROW(LoadException("file not open", 0));
  if (df_read(fileHandle, ptr, size) != size)
    DAGOR_THROW(LoadException("read error", tell()));
}
int LFileGeneralLoadCB::tryRead(void *ptr, int size)
{
  if (!fileHandle)
    return 0;
  return df_read(fileHandle, ptr, size);
}

int LFileGeneralLoadCB::tell()
{
  if (!fileHandle)
    DAGOR_THROW(LoadException("file not open", 0));
  int o = df_tell(fileHandle);
  if (o == -1)
    DAGOR_THROW(LoadException("tell returns error", 0));
  return o;
}

void LFileGeneralLoadCB::seekto(int o)
{
  if (!fileHandle)
    DAGOR_THROW(LoadException("file not open", 0));
  if (df_seek_to(fileHandle, o) == -1)
    DAGOR_THROW(LoadException("seek error", tell()));
}

void LFileGeneralLoadCB::seekrel(int o)
{
  if (!fileHandle)
    DAGOR_THROW(LoadException("file not open", 0));
  if (df_seek_rel(fileHandle, o) == -1)
    DAGOR_THROW(LoadException("seek error", tell()));
}


FullFileLoadCB::FullFileLoadCB(const char *fname)
{
  fileHandle = NULL;
  open(fname, DF_READ);
}

bool FullFileLoadCB::open(const char *fname, int mode)
{
  close();
  targetFilename = fname ? fname : "";
  targetDataSz = -1;
  if (!fname)
    return false;
  fileHandle = df_open(fname, mode);
  targetDataSz = df_length(fileHandle);
  return fileHandle != NULL;
}

void FullFileLoadCB::close()
{
  if (fileHandle)
  {
    df_close(fileHandle);
    fileHandle = NULL;
  }
}

void FullFileLoadCB::beginFullFileBlock()
{
  G_VERIFY(blocks.size() == 0);

  int i = append_items(blocks, 1);
  blocks[i].ofs = 0;
  blocks[i].len = df_length(fileHandle);
}

dag::ConstSpan<char> FullFileLoadCB::getTargetRomData() const
{
  int data_sz = 0;
  if (const char *data = df_get_vromfs_file_data_for_file_ptr(fileHandle, data_sz))
    return make_span_const(data, data_sz);
  return {};
}

FullFileSaveCB::FullFileSaveCB(const char *fname)
{
  fileHandle = NULL;
  open(fname, DF_WRITE | DF_CREATE);
}

bool FullFileSaveCB::open(const char *fname, int mode)
{
  close();
  fileHandle = df_open(fname, mode);
  targetFilename = fname;
  return fileHandle != NULL;
}

void FullFileSaveCB::close()
{
  if (fileHandle)
  {
    df_close(fileHandle);
    fileHandle = NULL;
  }
}

#define EXPORT_PULL dll_pull_iosys_fileIo
#include <supp/exportPull.h>
