// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <ioSys/dag_genIo.h>
#include <util/le2be.h>
#include <EASTL/vector.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

static inline unsigned string_hash(const char *name)
{
  const unsigned char *str = (const unsigned char *)name;
  unsigned hash = 5381;

  while (*str)
    hash = hash * 33 + *str++;

  return hash;
}

static bool namemap_loadCompact(DataBlockShared &nameMap, eastl::vector<int> &conv, class IGenLoad &crd)
{
  const bool load_le = true;

  int buckets_cnt = readInt16(crd, load_le);
  int pk = buckets_cnt >> 14;
  if (!pk)
  {
    crd.seekrel(2);
    return true;
  }

  unsigned total_sz = 2 + get_storage_type_sz(pk);
  int num = read_count_compact(crd, load_le, pk);
  if (num < 0 || num > (2 << 20) || (buckets_cnt & 0x3FFF) < 1 || !is_pow_of2(buckets_cnt & 0x3FFF))
    return false;

  uint32_t bucketsCnt = buckets_cnt & 0x3FFF;
  eastl::vector<int> bucketsNames(bucketsCnt);
  eastl::vector<char> tempString;
  int snum = num;
  uint32_t allStringsLen = 0;
  for (int i = 0; i < snum; i++)
  {
    int len = decode_len(crd, total_sz);
    tempString.resize(len + 1);
    char *str = (char *)tempString.data();
    if (len)
    {
      if (crd.tryRead(str, len) != len)
        return false;
    }
    total_sz += len;
    str[len] = 0;
    allStringsLen += len;
    unsigned hash = string_hash(str) % bucketsCnt;
    const int oldNameId = bucketsNames[hash] * bucketsCnt + hash;
    if (conv.size() <= oldNameId)
      conv.resize(oldNameId + 1, -1);
    conv[oldNameId] = len ? nameMap.addNameId(str, len) : -1;
    bucketsNames[hash]++;
  }

  crd.alignOnDword(total_sz);
  return true;
}

static bool string_loadCompact(IGenLoad &crd, DBNameMap &stringMap)
{
  stringMap.reset();
  const bool load_le = true;

  uint32_t totalSz = readInt32(crd, load_le);
  if (!totalSz)
    return true;

  unsigned pk = totalSz >> 30, cnt = 0;
  totalSz &= 0x3FFFFFFF;
  if (totalSz > (256 << 20))
    return false;
  cnt = read_count_compact(crd, load_le, pk);
  if (int(cnt) < 0 || cnt > (64 << 20))
    return false;

  auto page = stringMap.allocator.addPageUnsafe(totalSz + 1);
  if (crd.tryRead(page->data.get(), totalSz) != totalSz)
    return false;
  page->data[totalSz] = 0;
  eastl::swap(page->used, page->left);
  stringMap.allocator.page_shift = get_bigger_log2(page->used);
  stringMap.allocator.start_page_shift = stringMap.allocator.page_shift;
  stringMap.allocator.max_page_shift = max(stringMap.allocator.page_shift, stringMap.allocator.max_page_shift);
  crd.alignOnDword(totalSz + get_storage_type_sz(pk));
  stringMap.strings.resize(cnt);

  char *p = page->data.get();
  char *top = page->data.get() + page->used;
  if (!p)
    return false;
  for (int i = 0; i < stringMap.strings.size(); i++)
  {
    int inc, len;
    if (!decode_len(p, inc, len, top - p + 1))
      return false;
    stringMap.strings[i] = p + inc - page->data.get();
    *p = 0;
    p += inc + len;
    if (p > top)
      return false;
  }
  return true;
}

bool DataBlock::doLoadFromStreamBBF3(IGenLoad &crd)
{
  eastl::vector<int> conv;
  if (!namemap_loadCompact(*shared, conv, crd))
  {
    issue_error_load_failed(crd.getTargetName(), "incompatible namemap params in BBF\\3 format");
    return false;
  }
  DBNameMap stringMap;
  if (!string_loadCompact(crd, stringMap))
  {
    issue_error_load_failed(crd.getTargetName(), "bad strings table in BBF\\3 format");
    return false;
  }
  return loadFromStreamBBF3(crd, conv.begin(), conv.end(), stringMap);
}

bool DataBlock::loadFromStreamBBF3(IGenLoad &crd, const int *cnv, const int *cnvEnd, const DBNameMap &strings)
{
  const bool load_le = true;
  uint16_t paramsNum = readInt16(crd, load_le);
  uint16_t blocksNum = readInt16(crd, load_le);
  if (!blocksNum && !paramsNum)
    return true;
  if (paramsNum)
  {
    toOwned();
    data->data.resize(paramsNum * sizeof(Param));
    Param *params = (Param *)data->data.data();

    int i;
    int cp_cnt = 0;
    uint32_t s_idx;

    for (int i = 0; i < paramsNum; ++i, ++params)
    {
      Param &p = *params;
      unsigned hdr = readInt32(crd, load_le);
      const uint32_t oldNameId = hdr & 0xFFFFFF;
      if (oldNameId >= cnvEnd - cnv)
        return false;
      const int newNameId = cnv[oldNameId];
      if (newNameId < 0)
      {
        issue_error_load_failed(crd.getTargetName(), "empty param name in BBF\\3 format");
        return false;
      }
      p.nameId = newNameId;
      p.type = (hdr >> 24) & 0x7F;
      p.v = 0;

      uint32_t ct = dblk::get_type_size(p.type);
      if (ct > 4 && p.type != TYPE_STRING)
      {
        cp_cnt += ct;
      }

      if (p.type == TYPE_BOOL)
      {
        p.v = (hdr & 0x80000000) ? 1 : 0;
        continue;
      }
    }

    paramsCount = paramsNum;
    data->data.reserve(paramsCount * sizeof(Param) + cp_cnt);
    uint32_t complexParamsData = 0;
    for (i = 0; i < paramsCount; ++i)
    {
      Param &p = getParams<true>()[i];
      if (p.type == TYPE_BOOL)
        continue;

      int ct = dblk::get_type_size(p.type);
      int buf[12];
      int64_t i64;
      switch (p.type)
      {
        case TYPE_STRING:
          s_idx = readInt32(crd, load_le);
          if (s_idx < strings.strings.size())
          {
            const char *s = strings.getStringDataUnsafe(s_idx);
            p.v = insertNewString(s, strlen(s));
          }
          else
            return false;
          break;
        case TYPE_INT: p.v = readInt32(crd, load_le); break;
        case TYPE_REAL: p.v = memcpy_cast<uint32_t, float>(readReal32(crd, load_le)); break;
        case TYPE_POINT3:
        case TYPE_IPOINT3:
        case TYPE_POINT2:
        case TYPE_IPOINT2:
        case TYPE_POINT4:
        case TYPE_MATRIX:
          for (int i = 0; i < ct; i += 4)
            buf[i / 4] = readInt32(crd, load_le);
          p.v = complexParamsData;
          insertAt(data->data.size(), ct, (const char *)buf);
          complexParamsData += ct;
          break;
        case TYPE_E3DCOLOR:
        {
          int c = readInt32(crd, load_le);
          p.v = memcpy_cast<uint32_t, E3DCOLOR>(E3DCOLOR((c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
        }
        break;
        case TYPE_INT64:
          p.v = complexParamsData;
          i64 = readInt64(crd, load_le);
          insertAt(data->data.size(), ct, (const char *)&i64);
          complexParamsData += ct;
          break;
        default: return false;
      }
    }
    G_ASSERT(complexParamsData == complexParamsSize());
  }
  for (int i = 0; i < blocksNum; ++i)
  {
    int oldNameId = readInt32(crd, load_le);
    if (oldNameId >= cnvEnd - cnv)
      return false;
    int newNameId = cnv[oldNameId];
    if (!addNewBlock(newNameId < 0 ? "" : shared->getName(newNameId))->loadFromStreamBBF3(crd, cnv, cnvEnd, strings))
      return false;
  }

  return true;
}

#define EXPORT_PULL dll_pull_iosys_datablock_readbbf3
#include <supp/exportPull.h>
