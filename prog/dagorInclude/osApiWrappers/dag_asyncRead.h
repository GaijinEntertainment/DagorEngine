//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdarg.h>

#include <osApiWrappers/dag_miscApi.h>
#include <supp/dag_define_KRNLIMP.h>

//
// additional routines for async reading of files
// all routines call Win32 directly, so there is no prebuffering or caching
//

#ifdef __cplusplus
extern "C"
{
#endif

  // opens real file for reading
  KRNLIMP void *dfa_open_for_read(const char *fpath, bool non_cached);
  // closes real file handle
  KRNLIMP void dfa_close(void *handle);

  // returns associated file sector size (uses path of file)
  KRNLIMP unsigned dfa_chunk_size(const char *fname);

  // returns associated file size
  KRNLIMP int dfa_file_length(void *handle);

  // allocates handle for async read operation
  KRNLIMP int dfa_alloc_asyncdata();
  // deallocates handle, allocated with dfa_alloc_asyncdata;
  // must not be called before async read completion
  KRNLIMP void dfa_free_asyncdata(int data_handle);

  // places request to read asynchronously data from real file; returns false on failure
  KRNLIMP bool dfa_read_async(void *handle, int asyncdata_handle, int offset, void *buf, int len);
  // checks for async read completion
  KRNLIMP bool dfa_check_complete(int asyncdata_handle, int *read_len);
  // wait for request's completion
#if _TARGET_C1 | _TARGET_C2

#else
inline void dfa_wait_until_complete(int adh, int *readed_len)
{
  spin_wait_no_profile([=] { return !dfa_check_complete(adh, readed_len); });
}
#endif

#ifdef __cplusplus
}
#endif

#include <supp/dag_undef_KRNLIMP.h>
