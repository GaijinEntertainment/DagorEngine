//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdarg.h>

struct VirtualRomFsData;

#include <supp/dag_define_KRNLIMP.h>
#include <util/dag_stdint.h>

typedef void *file_ptr_t;

#ifdef __cplusplus
#include <util/dag_safeArg.h>
#define DSA_OVERLOADS_PARAM_DECL file_ptr_t fp,
#define DSA_OVERLOADS_PARAM_PASS fp,
DECLARE_DSA_OVERLOADS_FAMILY(static inline int df_printf, extern "C" KRNLIMP int df_printf, return df_printf);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

extern "C"
{
#endif

  enum
  {
    DF_READ = 0x1,
    DF_WRITE = 0x2,
    DF_CREATE = 0x4,
    DF_RDWR = DF_READ | DF_WRITE,
    DF_APPEND = 0x8,
    DF_IGNORE_MISSING = 0x10,
    DF_VROM_ONLY = 0x20,
    DF_REALFILE_ONLY = 0x40,
  };


  // open file with name fname
  // searches for a file in groups in the order in which the groups were created (see above)
  // if the DF_CREATE flag is set, the file will be created.
  // returns NULL on error
  KRNLIMP file_ptr_t df_open(const char *fname, int flags);

  // close file that opened with df_open
  // returns -1 on error
  KRNLIMP int df_close(file_ptr_t fp);

  KRNLIMP void df_flush(file_ptr_t fp);

  // read from file
  // returns the number of bytes read or -1 on error
  KRNLIMP int df_read(file_ptr_t fp, void *buf, int len);

  // write to file
  // returns the number of bytes written or -1 on error
  KRNLIMP int df_write(file_ptr_t fp, const void *buf, int len);


  // returns 0 on error
  // 2K characters max
  KRNLIMP int df_vprintf(file_ptr_t fp, const char *fmt, va_list lst);
  KRNLIMP int df_cprintf(file_ptr_t fp, const char *fmt, ...);

  // analog of fgets
  KRNLIMP char *df_gets(char *buf, int n, file_ptr_t fp);


  // move the read/write pointer
  // returns the current pointer or -1 on error
  KRNLIMP int df_seek_to(file_ptr_t fp, int offset_abs);
  KRNLIMP int df_seek_rel(file_ptr_t fp, int offset_rel);
  KRNLIMP int df_seek_end(file_ptr_t fp, int offset_from_end);

  // returns the current pointer or -1 on error
  KRNLIMP int df_tell(file_ptr_t fp);

  // returns file length or -1 on error
  KRNLIMP int df_length(file_ptr_t fp);

  // returns pointer to vromfs if 'fp' was opened as ROM-file from vromfs; otherwise returns nullptr
  KRNLIMP const VirtualRomFsData *df_get_vromfs_for_file_ptr(file_ptr_t fp);
  // returns pointert to ROM data and size of data in vromfs if 'fp' was opened as ROM-file from vromfs; otherwise returns (nullptr,0)
  KRNLIMP const char *df_get_vromfs_file_data_for_file_ptr(file_ptr_t fp, int &out_data_sz);

  struct DagorStat
  {
    int64_t size; // size of file, in bytes
    // times are set to -1 if underlying implementation or fs is not supports them
    int64_t atime; // time of most recent access (disabled by default on Windows since Vista)
    int64_t mtime; // time of most recent content modification
    int64_t ctime; // platform dependent: time of most recent metadata change on Unix, time of creation on Windows
  };

  // fills buf with file statistics or returns -1 on error
  KRNLIMP int df_stat(const char *path, DagorStat *buf);
  KRNLIMP int df_fstat(file_ptr_t fp, DagorStat *buf);

  // creates read-only file mapping
  KRNLIMP const void *df_mmap(file_ptr_t fp, int *out_file_len = NULL, int length = 0, int offset = 0);
  // deletes file mapping
  KRNLIMP void df_unmap(const void *start, int length);

  // returns pointer to string containing full path to specified file_name; returns NULL if file not exists
  // NOTE: uses static buffer, don't store this pointer for long
  KRNLIMP const char *df_get_real_name(const char *file_name);

  // returns pointer to string containing full path to specified folder_name; returns NULL if folder not exists
  // NOTE: uses static buffer, don't store this pointer for long; folder_name may be '/' terminated or not
  KRNLIMP const char *df_get_real_folder_name(const char *folder_name);

  // returns pointer to string containing full path to specified file_name; returns NULL if file not exists
  // finds and adds base-path where file is located; works for both real and vromfs files
  // NOTE: uses static buffer, don't store this pointer for long
  KRNLIMP const char *df_get_abs_fname(const char *file_name);

  // creates & opens file with unique temporary name
  // last 6 charachers of 'templ' must be "XXXXXX" and these are replaced with string that makes the filename unique
  // file is avaible both for read & write access
  KRNLIMP file_ptr_t df_mkstemp(char *templ);

#ifdef __cplusplus

  class DagorFileCloser
  {
  public:
    void operator()(file_ptr_t fp) { df_close(fp); }
  };

} // extern "C"
#endif

#include <supp/dag_undef_KRNLIMP.h>
