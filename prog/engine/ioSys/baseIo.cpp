// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_baseIo.h>

IBaseSave::IBaseSave() : blocks(tmpmem_ptr()) {}

IBaseSave::~IBaseSave() {}

void IBaseSave::beginBlock()
{
  int ofs = 0;
  write(&ofs, sizeof(int));

  int o = tell();

  int i = append_items(blocks, 1);
  blocks[i].ofs = o;
}

void IBaseSave::endBlock(unsigned block_flags)
{
  G_ASSERTF(block_flags <= 0x3, "block_flags=%08X", block_flags); // 2 bits max
  if (blocks.size() <= 0)
    DAGOR_THROW(SaveException("block not begun", tell()));

  Block &b = blocks.back();
  int o = tell();
  seekto(b.ofs - sizeof(int));
  int l = o - b.ofs;
  G_ASSERTF(l >= 0 && !(l & 0xC0000000), "o=%08X b.ofs=%08X l=%08X", o, b.ofs, l);
  l |= (block_flags << 30);
  write(&l, sizeof(int));
  seekto(o);

  blocks.pop_back();
}

int IBaseSave::getBlockLevel() { return blocks.size(); }

IBaseLoad::IBaseLoad() : blocks(tmpmem_ptr()) {}

IBaseLoad::~IBaseLoad() {}

int IBaseLoad::beginBlock(unsigned *out_block_flags)
{
  int l = 0;
  read(&l, sizeof(int));
  if (out_block_flags)
    *out_block_flags = (l >> 30) & 0x3u;
  l &= 0x3FFFFFFF;

  int o = tell();

  int i = append_items(blocks, 1);

  blocks[i].ofs = o;
  blocks[i].len = l;

  return l;
}

void IBaseLoad::endBlock()
{
  if (blocks.size() <= 0)
    DAGOR_THROW(LoadException("endBlock without beginBlock", tell()));

  Block &b = blocks.back();
  seekto(b.ofs + b.len);

  blocks.pop_back();
}

int IBaseLoad::getBlockLength()
{
  if (blocks.size() <= 0)
    DAGOR_THROW(LoadException("block not begun", tell()));

  Block &b = blocks.back();
  return b.len;
}

int IBaseLoad::getBlockRest()
{
  if (blocks.size() <= 0)
    DAGOR_THROW(LoadException("block not begun", tell()));

  Block &b = blocks.back();
  return b.ofs + b.len - tell();
}

int IBaseLoad::getBlockLevel() { return blocks.size(); }

#define EXPORT_PULL dll_pull_iosys_baseIo
#include <supp/exportPull.h>
