//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdarg.h>

#include <util/dag_baseDef.h>
#include <supp/dag_define_KRNLIMP.h>
#include <util/dag_stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Compare filenames taking into consideration '*' and '?' symbols
  // Returns 0, if filenames are equal, 1 otherwise
  KRNLIMP bool dd_fname_equal(const char *f1, const char *fn2);

  // remove leading and trailing spaces, remove quotes and extra slashes,
  // remove extra up-dirs (e.g. some\dir\..\tex => some\tex)
  KRNLIMP void dd_simplify_fname_c(char *fn);

  // returns pointer to extension inside filename or NULL if no extension
  // first character in returned string will be dot (.)
  KRNLIMP const char *dd_get_fname_ext(const char *pathname);

  // returns pointer to filena with extension inside pathname
  // or NULL for empty pathname
  KRNLIMP const char *dd_get_fname(const char *pathname);

  // returns pointer to buf containing location of filename
  // size of buffer must be sufficiently large (260 at max for Win32)
  KRNLIMP char *dd_get_fname_location(char *buf, const char *pathname);

  // returns pointer to inside buf containing location of filename, without path and extenstion
  // size of buffer must be sufficiently large (260 at max for Win32)
  KRNLIMP const char *dd_get_fname_without_path_and_ext(char *buf, int buf_size, const char *path);

  // append slash, if there is no slash
  // NOTE: target buffer must have enough space for this slash
  KRNLIMP void dd_append_slash_c(char *pathname);

  enum
  {
    DA_FILE = 0x0,
    DA_SUBDIR = 0x10,

    DA_READONLY = 0x01,
    DA_HIDDEN = 0x02,
    DA_SYSTEM = 0x04,
  };

  // structure to be used for dd_find_first/dd_find_next
  struct alefind_t
  {
    int grp, fattr;
    void *data;
    char fmask[DAGOR_MAX_PATH - sizeof(int) * 4 - sizeof(void *) - sizeof(int64_t) * 3];
    char name[DAGOR_MAX_PATH]; //< filename
    // times are set to -1 if underlying implementation or fs is not supports them
    int64_t atime; //< time of most recent access (disabled by default on Windows since Vista)
    int64_t mtime; //< time of most recent content modification
    int64_t ctime; //< platform dependent: time of most recent metadata change on Unix, time of creation on Windows
    int size;      //< file size
    int attr;      //< file atributes
  };

  // rename file
  // return 0 on error
  // silenly rewrites destination if it exists (POSIX behavior)
  KRNLIMP bool dd_rename(const char *oldname, const char *newname);

  // Erase file
  // Returns 0 if error occured
  KRNLIMP bool dd_erase(const char *filename);

  // make dir, returns 0 on error
  KRNLIMP bool dd_mkdir(const char *path);

  // make all dirs of the path, returns 0 on error
  KRNLIMP bool dd_mkpath(const char *path);

  // remove dir, returns 0 on error
  KRNLIMP bool dd_rmdir(const char *path);

  // Find first file
  // Returns 0 if error occured
  KRNLIMP int dd_find_first(const char *mask, char attr, alefind_t *);

  // Find next file
  // Returns 0 if error occured
  KRNLIMP int dd_find_next(alefind_t *);

  // Finalize file search
  // Returns 0 if error occured
  KRNLIMP int dd_find_close(alefind_t *);

  KRNLIMP bool dd_file_exist(const char *fn);

  KRNLIMP bool dd_dir_exist(const char *fn);

  inline bool dd_file_exists(const char *fn) { return dd_file_exist(fn); }
  inline bool dd_dir_exists(const char *fn) { return dd_dir_exist(fn); }

#ifdef __cplusplus
}

// Wrapper for dd_find_first/dd_find_next/dd_find_close
// Usage:
//   for (const alefind_t &ff : dd_find_iterator(".txt", DA_FILE))
//       debug("%s %d", ff.name, ff.size);

struct dd_find_iterator
{
private:
  alefind_t result;
  bool firstSearchIsSuccessful;

public:
  struct dd_iterator_iml
  {
  private:
    friend dd_find_iterator;
    alefind_t &result_ref;
    bool canContinue;
    dd_iterator_iml(alefind_t &result, bool can_continue) : result_ref(result), canContinue(can_continue) {}

  public:
    const alefind_t &operator*() { return result_ref; }
    void operator++() { canContinue = ::dd_find_next(&result_ref); }
    bool operator!=(const dd_iterator_iml &other) { return this->canContinue != other.canContinue; }
  };

  // mask and attributes the same as in 'dd_find_first'
  dd_find_iterator(const char *mask, char attributes) { firstSearchIsSuccessful = ::dd_find_first(mask, attributes, &result); }
  ~dd_find_iterator() { ::dd_find_close(&result); }

  dd_find_iterator(const dd_find_iterator &) = delete;
  dd_find_iterator(dd_find_iterator &&) = delete;
  dd_find_iterator &operator=(const dd_find_iterator &) = delete;
  dd_find_iterator &operator=(dd_find_iterator &&) = delete;

  dd_iterator_iml begin() { return dd_iterator_iml(result, firstSearchIsSuccessful); }
  dd_iterator_iml end() { return dd_iterator_iml(result, false); }
};

#endif

#include <supp/dag_undef_KRNLIMP.h>
