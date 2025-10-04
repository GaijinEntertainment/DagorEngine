// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <ioSys/dag_genIo.h>
#include <util/le2be.h>
#include <EASTL/vector.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include "obsolete_hashNameMap.h"
#include <generic/dag_tabSort.h>
#include <supp/dag_define_KRNLIMP.h>

using obsolete::HashNameMap;
namespace dblk
{
struct ExposedDataBlock;
struct StringValMap;
KRNLIMP void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);

static void save(const ExposedDataBlock &blk, IGenSave &cb, dblk::StringValMap &stringMap, HashNameMap &nameMap);

struct ExposedDataBlock : public DataBlock
{
  friend void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);
  friend void save(const ExposedDataBlock &blk, IGenSave &cb, dblk::StringValMap &stringMap, HashNameMap &nameMap);
};
} // namespace dblk

struct dblk::StringValMap
{
  struct LambdaStrcmp
  {
    static int order(const char *const &a, const char *const &b) { return strcmp(a, b); }
  };

public:
  StringValMap() : fastMap(tmpmem), totalSz(0) {}

  void addName(const char *name)
  {
    int idx = getNameIndex(name);
    if (idx == -1)
    {
      fastMap.insert(name, LambdaStrcmp(), 32);
      int len = (int)strlen(name);
      totalSz += len + encode_len_sz(len);
    }
  }

  int getNameIndex(const char *name) const { return fastMap.find(name, LambdaStrcmp()); }

  bool saveCompact(IGenSave &cwr)
  {
    const bool save_le = true;
    if (!fastMap.size())
    {
      writeInt32(cwr, 0, save_le);
      return true;
    }
    unsigned pk = get_count_storage_type(fastMap.size());

    writeInt32(cwr, totalSz | (pk << 30), save_le);
    if ((totalSz | pk) == 0)
      return true;
    write_count_compact(cwr, save_le, pk, fastMap.size());

    for (int i = 0; i < fastMap.size(); i++)
    {
      char buf[4];
      int len = (int)strlen(fastMap[i]);
      int sz_len = encode_len(len, buf);
      cwr.write(buf, sz_len);
      cwr.write(fastMap[i], len);
    }
    cwr.alignOnDword(totalSz + get_storage_type_sz(pk));
    return true;
  }
  dag::Span<char *> getRawMap() const { return dag::Span<char *>(const_cast<char **>(fastMap.data()), fastMap.size()); }

protected:
  TabSortedFast<const char *> fastMap;
  Tab<char> stor;
  unsigned totalSz;
};

static void fillNameMap(const DataBlock &blk, dblk::StringValMap &stringMap)
{
  for (int i = 0; i < blk.paramCount(); ++i)
    if (blk.getParamType(i) == blk.TYPE_STRING)
      stringMap.addName(blk.getStr(i));

  for (int i = 0; i < blk.blockCount(); ++i)
    fillNameMap(*blk.getBlock(i), stringMap);
}

static void dblk::save(const ExposedDataBlock &blk, IGenSave &cb, dblk::StringValMap &stringMap, HashNameMap &nameMap)
{
  G_ASSERT(blk.paramCount() <= 0xffff);
  G_ASSERT(blk.blockCount() <= 0xffff);

  int i;
  cb.writeIntP<2>(blk.paramCount());
  cb.writeIntP<2>(blk.blockCount());
  for (i = 0; i < blk.paramCount(); ++i)
  {
    int p_nameId = nameMap.getNameId(blk.getParamName(i));
    int p_type = blk.getParamType(i);
    G_ASSERT(p_nameId <= 0xFFFFFF);
    unsigned hdr = (p_nameId & 0xFFFFFF) | (unsigned(p_type) << 24);

    if (p_type == blk.TYPE_BOOL && blk.getBool(i))
      hdr |= 0x80000000;
    cb.writeInt(hdr);
  }
  for (i = 0; i < blk.paramCount(); ++i)
  {
    const ExposedDataBlock::Param &p = blk.isOwned() ? blk.getParam<true>(i) : blk.getParam<false>(i);
    switch (p.type)
    {
      case DataBlock::TYPE_STRING: cb.writeInt(stringMap.getNameIndex(blk.getStr(i))); break;
      case DataBlock::TYPE_BOOL: break;
      case DataBlock::TYPE_INT:
      case DataBlock::TYPE_REAL:
      case DataBlock::TYPE_POINT3:
      case DataBlock::TYPE_IPOINT3:
      case DataBlock::TYPE_IPOINT4:
      case DataBlock::TYPE_POINT2:
      case DataBlock::TYPE_IPOINT2:
      case DataBlock::TYPE_POINT4:
      case DataBlock::TYPE_MATRIX:
      case DataBlock::TYPE_E3DCOLOR:
      case DataBlock::TYPE_INT64: cb.write(blk.getParamData(p), get_type_size(p.type)); break;

      default: G_ASSERT(0);
    }
  }
  for (i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &b = *blk.getBlock(i);
    cb.writeInt(nameMap.getNameId(b.getBlockName()));
    save(static_cast<const ExposedDataBlock &>(b), cb, stringMap, nameMap);
  }
}

void dblk::save_to_bbf3(const DataBlock &_blk, IGenSave &cwr)
{
  auto &blk = static_cast<const ExposedDataBlock &>(_blk);
  StringValMap stringMap;
  fillNameMap(blk, stringMap);

  unsigned ver = 3;
  int nmsz = 16;
  G_ASSERT(nmsz < 0x7FFF);
  ver |= nmsz << 16;

  cwr.writeInt(_MAKE4C('BBF'));
  cwr.writeInt(ver);
  int start = cwr.tell();
  cwr.writeInt(0);

  HashNameMap nameMap(64 * (1 << 2), tmpmem);
  for (int i = 0, e = blk.shared->nameCount(); i < e; ++i)
    nameMap.addNameId(blk.shared->getName(i));
  nameMap.saveCompact(cwr, true);

  stringMap.saveCompact(cwr);
  save(blk, cwr, stringMap, nameMap);

  int end = cwr.tell();
  cwr.seekto(start);
  cwr.writeInt(end - start - 4);
  cwr.seekto(end);
}
