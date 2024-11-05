// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/blockFileUtil.h>
#include <debug/dag_except.h>


BlkFile::BlkFile(int id, char ce) : data(tmpmem), blk(tmpmem), cpstk(tmpmem)
{
  fileid = id;
  chkerr = ce;
  curpos = blko = blkl = 0;
}

#define rd(p, l)                                                         \
  {                                                                      \
    if (df_read(h, p, l) != l)                                           \
    {                                                                    \
      if (chkerr)                                                        \
        DAGOR_THROW(BlkFileException("error reading file", df_tell(h))); \
      return 0;                                                          \
    }                                                                    \
  }
int BlkFile::loadfile(file_ptr_t h, int loadid)
{
  clearfile();
  if (loadid)
  {
    int id;
    rd(&id, 4);
    if (id != fileid)
    {
      if (chkerr)
        DAGOR_THROW(BlkFileException("invalid file id", df_tell(h) - 4));
      return 0;
    }
  }
  int l;
  rd(&l, 4);
  data.resize(l);
  blkl = l;
  rd(&data[0], l);
  return 1;
}
#undef rd

#define wr(p, l)                                                         \
  {                                                                      \
    if (df_write(h, p, l) != l)                                          \
    {                                                                    \
      if (chkerr)                                                        \
        DAGOR_THROW(BlkFileException("error writing file", df_tell(h))); \
      return 0;                                                          \
    }                                                                    \
  }
int BlkFile::savefile(file_ptr_t h, int saveid)
{
  if (saveid)
    wr(&fileid, 4);
  int l = data_size(data);
  wr(&l, 4);
  wr(&data[0], l);
  return 1;
}
#undef wr

int BlkFile::loadfile(const char *fn, int flg)
{
  file_ptr_t h = df_open(fn, flg | DF_READ);
  if (!h)
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("can't open file"));
    return 0;
  }
  int r;
  DAGOR_TRY { r = loadfile(h); }
  DAGOR_CATCH(BlkFileException e)
  {
    df_close(h);
    DAGOR_THROW(e);
  }
  df_close(h);
  return r;
}

int BlkFile::savefile(const char *fn, int flg)
{
  file_ptr_t h = df_open(fn, flg | DF_WRITE);
  if (!h)
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("can't create/open file"));
    return 0;
  }
  int r;
  DAGOR_TRY { r = savefile(h); }
  DAGOR_CATCH(BlkFileException e)
  {
    df_close(h);
    DAGOR_THROW(e);
  }
  df_close(h);
  return r;
}

void BlkFile::clearfile()
{
  clear_and_shrink(data);
  clear_and_shrink(blk);
  clear_and_shrink(cpstk);
  curpos = blko = blkl = 0;
}

void BlkFile::resetfile()
{
  clear_and_shrink(blk);
  clear_and_shrink(cpstk);
  curpos = 0;
  blko = 0;
  blkl = data_size(data);
}

int BlkFile::getblk()
{
  if (rest() < 4)
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("invalid block", curpos));
    return 0;
  }
  BlkFileBlk b;
  b.l = *(int *)ptr();
  if (b.l + 4 > rest())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("invalid block", curpos));
    return 0;
  }
  curpos += 4;
  b.o = curpos;
  blk.push_back(b);
  blko = b.o;
  blkl = b.l;
  return 1;
}

int BlkFile::insertblk()
{
  BlkFileBlk b;
  b.l = 0;
  if (!insert(4, &b.l))
    return 0;
  b.o = curpos;
  blk.push_back(b);
  blko = b.o;
  blkl = b.l;
  return 1;
}

int BlkFile::eraseblk()
{
  if (rest() < 4)
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("invalid block", curpos));
    return 0;
  }
  return erase(*(int *)ptr() + 4);
}

int BlkFile::goup()
{
  if (blk.empty())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("can't go up", curpos));
    return 0;
  }
  if (!seekend())
    return 0;
  blk.resize(blk.size() - 1);
  if (!blk.size())
  {
    blko = 0;
    blkl = data_size(data);
  }
  else
  {
    blko = blk.back().o;
    blkl = blk.back().l;
  }
  return 1;
}

int BlkFile::goupstart()
{
  if (blk.empty())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("can't go up", curpos));
    return 0;
  }
  if (!seekset(0))
    return 0;
  blk.resize(blk.size() - 1);
  if (blk.empty())
  {
    blko = 0;
    blkl = data_size(data);
  }
  else
  {
    blko = blk.back().o;
    blkl = blk.back().l;
  }
  return seekcur(-4);
}

int BlkFile::seekset(int o)
{
  if (o < 0 || o > blkl)
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("seek out of block", curpos));
    return 0;
  }
  curpos = blko + o;
  return 1;
}

int BlkFile::insert(int n, void *p)
{
  insert_items(data, curpos, n, (char *)p);
  curpos += n;
  blkl += n;
  for (int i = 0; i < blk.size(); ++i)
  {
    blk[i].l += n;
    *(int *)&data[blk[i].o - 4] += n;
  }
  return 1;
}

int BlkFile::erase(int n)
{
  if (!n)
    return 1;
  if (n > rest())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("erase out of block", curpos));
    return 0;
  }
  erase_items(data, curpos, n);
  blkl -= n;
  for (int i = 0; i < blk.size(); ++i)
  {
    blk[i].l -= n;
    *(int *)&data[blk[i].o - 4] -= n;
  }
  return 1;
}

int BlkFile::pushcurpos()
{
  BlkFileCP cp;
  cp.lev = blk.size();
  cp.o = blko;
  cp.cp = curpos;
  cpstk.push_back(cp);
  return 1;
}

int BlkFile::popcurpos()
{
  if (cpstk.empty())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("CP stack underflow", curpos));
    return 0;
  }
  BlkFileCP &cp = cpstk.back();
  if (cp.lev > blk.size())
  {
    if (chkerr)
      DAGOR_THROW(BlkFileException("can't pop curpos", curpos));
    return 0;
  }
  blk.resize(cp.lev);
  if (cp.lev)
  {
    blko = blk[cp.lev - 1].o;
    blkl = blk[cp.lev - 1].l;
  }
  else
  {
    blko = 0;
    blkl = data_size(data);
  }
  curpos = cp.cp;
  cpstk.resize(cpstk.size() - 1);
  return 1;
}
