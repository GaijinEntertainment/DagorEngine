//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/makeBindump.h>

class DataBlock;


namespace mkbindump
{
class StrCollector
{
public:
  StrCollector() {}

  void reset()
  {
    str.reset();
    clear_and_shrink(ofs);
  }

  const char *addString(const char *s)
  {
    if (s && ofs.size() == 0)
      return str.getName(str.addNameId(s));
    return NULL;
  }

  int strCount() const { return str.nameCount(); }

  void writeStrings(BinDumpSaveCB &cwr);
  void writeStringsEx(BinDumpSaveCB &cwr, dag::Span<StrCollector> written_str);

  int getOffset(const char *s)
  {
    if (!s || ofs.size() == 0)
      return -1;
    int id = str.getNameId(s);
    return (id >= 0) ? ofs[id] : -1;
  }

  const FastNameMapEx &getMap() const { return str; }

protected:
  FastNameMapEx str;
  SmallTab<int, TmpmemAlloc> ofs;
};


class RoNameMapBuilder
{
public:
  RoNameMapBuilder()
  {
    ptStr.reset();
    ptIdx.reset();
  }

  template <typename NM>
  void prepare(const NM &src, StrCollector &common_names, bool no_idx_remap = true)
  {
    strNames = &common_names;
    noIdxRemap = no_idx_remap;
    map.reset();

    iterate_names_in_id_order(src, [&](int, const char *name) {
      map.addNameId(name);
      common_names.addString(name);
    });

    map.prepareXmap();
  }
  void prepare(StrCollector &direct_namemap);

  void writeHdr(BinDumpSaveCB &cwr);
  void writeMap(BinDumpSaveCB &cwr);

  int getFinalNameId(const char *name) const
  {
    if (!name)
      return -1;
    int id = map.getNameId(name);
    return noIdxRemap ? id : map.xmap[id];
  }

protected:
  StrCollector *strNames = nullptr;
  GatherNameMap map;
  PatchTabRef ptStr, ptIdx;
  bool noIdxRemap = true;
};


class RoDataBlockBuilder
{
public:
  RoDataBlockBuilder() : params(tmpmem), blocks(tmpmem) {}

  void prepare(const DataBlock &blk, bool write_be);

  void writeHdr(BinDumpSaveCB &cwr);
  void writeData(BinDumpSaveCB &cwr);

protected:
  struct ParamRec
  {
    const char *name;
    union
    {
      int val;
      float rval;
      const char *sval;
    };
    int type;
  };
  struct BlockRec
  {
    PatchTabRef ptParam;
    PatchTabRef ptBlock;

    const char *name;
    uint16_t stBlock, blockNum;
    int stParam, paramNum;
  };

  StrCollector varNames;
  StrCollector valStr;
  RoNameMapBuilder nameMap;
  SharedStorage<real> realStor;
  SharedStorage<int> intStor;

  Tab<ParamRec> params;
  Tab<BlockRec> blocks;
  int nameMapOfs = -1, nameMapRefPos = -1;

  void enumDataBlock(const DataBlock &blk, int bi, bool write_be);
  void enumWriteBlocks(BinDumpSaveCB &cwr, int bi);
};


template <typename NM>
inline void write_ro_namemap(BinDumpSaveCB &cwr, const NM &nm)
{
  StrCollector all_names;
  RoNameMapBuilder b;

  b.prepare(nm, all_names);

  cwr.beginBlock();
  cwr.setOrigin();

  b.writeHdr(cwr);
  all_names.writeStrings(cwr);
  b.writeMap(cwr);

  cwr.popOrigin();
  cwr.endBlock();
}

inline void write_ro_datablock(BinDumpSaveCB &cwr, const DataBlock &blk)
{
  RoDataBlockBuilder b;

  b.prepare(blk, cwr.WRITE_BE);

  cwr.beginBlock();
  cwr.setOrigin();

  b.writeHdr(cwr);
  b.writeData(cwr);

  cwr.popOrigin();
  cwr.endBlock();
}
} // namespace mkbindump
