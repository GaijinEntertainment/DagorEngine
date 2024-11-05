// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_smallTab.h>
#include <gameRes/grpData.h>
#include <3d/ddsxTex.h>
#include <util/dag_roNameMap.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_string.h>

struct GrpBinData
{
  gamerespackbin::GrpData *data;
  SmallTab<char, TmpmemAlloc> hdrData;
  int fullSz;
  String fn;
  FastStrMapT<int> resMap;
  Tab<int> resSz;

public:
  GrpBinData() : data(NULL), fullSz(0), resSz(tmpmem) {}
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
  GrpBinData(const GrpBinData &) = delete;
#endif
  ~GrpBinData()
  {
    memfree(data, tmpmem);
    data = NULL;
    fullSz = 0;
  }

  static bool findRes(dag::Span<GrpBinData> old_grp, int pair_idx, const char *res_name, int psz, int &old_idx, int &old_rec_idx);
  void buildResMap()
  {
    if (resMap.strCount())
      return;
    for (int i = 0; i < data->resTable.size(); i++)
      resMap.addStrId(data->getName(data->resTable[i].resId), i);
  }
};
DAG_DECLARE_RELOCATABLE(GrpBinData);

struct DxpBinData
{
  struct Rec
  {
    int texId;
    int ofs;
    int packedDataSize;
  };
  struct Dump
  {
    RoNameMap texNames;
    PatchableTab<ddsx::Header> texHdr;
    PatchableTab<Rec> texRec;
  };

  Dump *data;
  SmallTab<char, TmpmemAlloc> hdrData;
  int fullSz;
  String fn;

  void *oldDump32;
  Tab<PatchablePtr<const char>> oldMap;
  Tab<Rec> oldRec;

public:
  DxpBinData() : data(NULL), fullSz(0), oldDump32(NULL) {}
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
  DxpBinData(const DxpBinData &) = delete;
#endif
  ~DxpBinData()
  {
    memfree(data, tmpmem);
    data = NULL;
    fullSz = 0;
    memfree(oldDump32, tmpmem);
  }

  static bool findTex(dag::ConstSpan<DxpBinData> old_dxp, int pair_idx, const char *tex_name, const ddsx::Header &hdr, int psz,
    int &old_idx, int &old_rec_idx);

  static int resolveNameId(dag::ConstSpan<PatchablePtr<const char>> map, const char *name, int name_len)
  {
    int lo = 0, hi = map.size() - 1, m;
    int cmp;

    // check bounds first
    if (hi < 0)
      return -1;

    cmp = strncmp(name, map[0], name_len);
    if (cmp < 0)
      return -1;
    if (cmp == 0)
      return 0;

    if (hi != 0)
      cmp = strncmp(name, map[hi], name_len);
    if (cmp > 0)
      return -1;
    if (cmp == 0)
      return hi;

    // binary search
    while (lo < hi)
    {
      m = (lo + hi) >> 1;
      cmp = strncmp(name, map[m], name_len);

      if (cmp == 0)
        return m;
      if (m == lo)
        return -1;

      if (cmp < 0)
        hi = m;
      else
        lo = m;
    }
    return -1;
  }

  static bool cvtRecInplaceVer2to3(dag::Span<Rec> texRec, int rest_dump_sz)
  {
    if (rest_dump_sz < texRec.size() * 24)
      return false;
    int *dst = (int *)texRec.data();
    int *src = dst + 2;
    for (auto dst_e = (int *)texRec.cend(); dst < dst_e; dst += 3, src += 6)
      dst[0] = /*BAD_TEXTUREID*/ 0, dst[1] = src[1], dst[2] = src[2];
    return true;
  }
};
DAG_DECLARE_RELOCATABLE(DxpBinData);
