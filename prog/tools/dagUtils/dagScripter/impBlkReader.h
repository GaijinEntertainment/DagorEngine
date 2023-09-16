#pragma once

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/dagFileRW/dagFileExport.h>

#include <stdio.h>

class ImpBlkReader : public FullFileLoadCB
{
  SimpleString buf;
  Tab<int> blkTag;

public:
  SimpleString fname;
  char *str;
  char *str1;

public:
  ImpBlkReader(const char *fn) : FullFileLoadCB(fn), str(NULL), str1(NULL), blkTag(tmpmem)
  {
    if (fileHandle)
    {
      fname = fn;
      buf.allocBuffer(256 * 3);
      str = buf;
      str1 = str + 256;
    }
  }

  inline bool readSafe(void *buf, int len) { return tryRead(buf, len) == len; }
  inline bool seekrelSafe(int ofs)
  {
    DAGOR_TRY { seekrel(ofs); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }

    return true;
  }
  inline bool seektoSafe(int ofs)
  {
    DAGOR_TRY { seekto(ofs); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }

    return true;
  }

  int beginBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { tag = beginTaggedBlock(); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return -1; }
    blkTag.push_back(tag);
    return getBlockLength() - 4;
  }
  bool endBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { endBlock(); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }
    blkTag.pop_back();
    return true;
  }
  int getBlockTag()
  {
    int i = blkTag.size() - 1;
    if (i < 0)
    {
      level_err();
      return -1;
    }
    return blkTag[i];
  }
  int getBlockLen()
  {
    int i = blkTag.size() - 1;
    if (i < 0)
    {
      level_err();
      return -1;
    }
    return getBlockLength() - 4;
  }

  void read_err()
  {
    // cb.read_error(fname);
  }
  void level_err()
  {
    // cb.format_error(fname);
  }
};

#define READ_ERR    \
  do                \
  {                 \
    rdr.read_err(); \
    goto err;       \
  } while (0)
#define READ(p, l)           \
  do                         \
  {                          \
    if (!rdr.readSafe(p, l)) \
      READ_ERR;              \
  } while (0)
#define BEGIN_BLOCK               \
  do                              \
  {                               \
    if (rdr.beginBlockSafe() < 0) \
      goto err;                   \
  } while (0)
#define END_BLOCK            \
  do                         \
  {                          \
    if (!rdr.endBlockSafe()) \
      goto err;              \
  } while (0)
