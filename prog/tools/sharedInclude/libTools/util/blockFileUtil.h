//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/*
  Class BlkFile is designed to load some almost arbitrary formatted file,
  analize and modify its contents by adding/modifying/erasing data and blocks,
  and then save it back. It can also be used to create such file from scratch.
  Can be slow for large files.
  File format:
    4-byte file id: should be used to identify file type, not version;
    4-byte main block data length;
    main block data.
  Data inside main block may contain blocks at any position, interleaved with
  some data or not, and those block may also contain blocks within them.
  Block format is same as for main block: 4-byte block size followed by block
  contents. You can put any header at the beginning of the block data to help
  identify its contents.
  Macro MAKE4C could be used to make int ids from 4 chars.
  BlkFile can throw exception on error, or just return error code from most
  methods. get() methods, however, don't return error codes - use exceptions.
*/


#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>


class BlkFileException
{
public:
  const char *msg;
  int pos;
  BlkFileException(const char *m, int p = 0)
  {
    msg = m;
    pos = p;
  }
};

struct BlkFileBlk
{
  int o, l;
};

struct BlkFileCP
{
  int lev, o, cp;
};

class BlkFile
{
protected:
  Tab<char> data;
  Tab<BlkFileBlk> blk;
  Tab<BlkFileCP> cpstk;
  int curpos, blko, blkl;

public:
  DAG_DECLARE_NEW(tmpmem)

  int fileid;  // used to identify file type
  char chkerr; // if non-0, throw BlkFileException on error

  BlkFile(int id, char chkerr);
  int getabscurpos() { return curpos; } // for debugging
  // load file
  // returns 0 on error
  int loadfile(file_ptr_t, int loadid = 1);
  // typical flags: 0
  int loadfile(const char *fn, int flg);
  // save file
  // returns 0 on error
  int savefile(file_ptr_t, int saveid = 1);
  // typical flags: DF_CREATE
  int savefile(const char *fn, int flg);
  // empty file
  void clearfile();
  // reset to the start of file, clear curpos stack
  void resetfile();
  // get block at current position
  // returns 0 on error
  int getblk();
  // insert block at current position
  // returns 0 on error
  int insertblk();
  // seek to end and insert block
  int appendblk()
  {
    if (!seekend())
      return 0;
    return insertblk();
  }
  // erase block at current position
  // returns 0 on error
  int eraseblk();
  // goes out of current block - current position is set after this block
  // returns 0 on error (when in base block)
  int goup();
  // goes out of current block - current position is set to the start of it
  // you can use getblk() to get into it again
  // returns 0 on error (when in base block)
  int goupstart();
  // size of current block
  int size() { return blkl; }
  // size of remaining data in current block
  int rest() { return blko + blkl - curpos; }
  // get current position in current block
  int tell() { return curpos - blko; }
  // set current position in current block
  // returns 0 on error
  int seekset(int ofs);
  int seekcur(int ofs) { return seekset(curpos - blko + ofs); }
  int seekend(int ofs = 0) { return seekset(blkl + ofs); }
  int skip(int n) { return seekcur(n); }
  // get pointer to data at current position
  // NOTE: this pointer becomes invalid when something is inserted/erased
  // NOTE: no bound checking performed
  char *ptr(int o = 0) { return &data[curpos + o]; }
  operator char *() { return ptr(); }
  char &operator[](int i) { return data[curpos + i]; }
  // insert data at current position
  // returns 0 on error
  int insert(int n, void *p = NULL);
  // seek to end and insert data
  int append(int n, void *p = NULL)
  {
    if (!seekend())
      return 0;
    return insert(n, p);
  }
  // returns 0 on error
  int get(int n, void *d)
  {
    char *p = ptr();
    if (!skip(n))
      return 0;
    if (d)
      memcpy(d, p, n);
    return 1;
  }
  // insert various data
  int putint(int a) { return insert(sizeof(a), &a); }
  int putshort(short a) { return insert(sizeof(a), &a); }
  int putchar(char a) { return insert(sizeof(a), &a); }
  int putreal(real a) { return insert(sizeof(a), &a); }
  int putpoint2(Point2 a) { return insert(sizeof(a), &a); }
  int putpoint3(Point3 a) { return insert(sizeof(a), &a); }
  int puttm(TMatrix a) { return insert(sizeof(a), &a); }
  int putstrz(char *s)
  {
    if (!s)
    {
      char c = '\0';
      return insert(1, &c);
    }

    return insert((int)strlen(s) + 1, s);
  }

  // get various data
  int getint()
  {
    char *p = ptr();
    skip(sizeof(int));
    return *(int *)p;
  }
  int getshort()
  {
    char *p = ptr();
    skip(sizeof(short));
    return *(short *)p;
  }
  int getchar()
  {
    char *p = ptr();
    skip(sizeof(char));
    return *(char *)p;
  }
  real getreal()
  {
    char *p = ptr();
    skip(sizeof(real));
    return *(real *)p;
  }
  Point2 getpoint2()
  {
    char *p = ptr();
    skip(sizeof(Point2));
    return *(Point2 *)p;
  }
  Point3 getpoint3()
  {
    char *p = ptr();
    skip(sizeof(Point3));
    return *(Point3 *)p;
  }
  TMatrix gettm()
  {
    char *p = ptr();
    skip(sizeof(TMatrix));
    return *(TMatrix *)p;
  }
  char *getstrz()
  {
    char *p = ptr();
    skip((int)strlen(p) + 1);
    return p;
  }
  // erase data at current position
  // returns 0 on error
  int erase(int n);
  // returns 0 on error
  int pushcurpos();
  // this may be only used to go up and back
  // returns 0 on error
  int popcurpos();
};
