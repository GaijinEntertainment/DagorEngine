#pragma once


#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <gameRes/grpData.h>
#include <3d/ddsxTex.h>
#include <util/dag_roNameMap.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>

struct GrpBinData
{
  gamerespackbin::GrpData *data;
  SmallTab<char, TmpmemAlloc> hdrData;
  int fullSz;
  String fn;
  SimpleString hashMD5;
  FastStrMapT<int> resMap;
  Tab<int> resSz;
  Tab<int> changedIdx;
  int verLabel = _MAKE4C('GRP2');

public:
  GrpBinData() : data(NULL), fullSz(0), resSz(tmpmem) {}
  GrpBinData(const GrpBinData &) = delete;
  ~GrpBinData()
  {
    memfree(data, tmpmem);
    data = NULL;
    fullSz = 0;
  }

  bool buildPatch(mkbindump::BinDumpSaveCB &cwr);

  static bool findRes(dag::Span<GrpBinData> old_grp, int pair_idx, const char *res_name, int &old_idx, int &old_rec_idx);
  static bool writeGrpHdr(mkbindump::BinDumpSaveCB &cwr, const char *grp_fname);

  void buildResMap()
  {
    if (resMap.strCount())
      return;
    for (int i = 0; i < data->resTable.size(); i++)
      resMap.addStrId(data->getName(data->resTable[i].resId), i);
  }
  void getResRefs(Tab<SimpleString> &out_refs, int resId)
  {
    out_refs.clear();
    for (int i = 0; i < data->resData.size(); ++i)
      if (resId == data->resData[i].resId)
      {
        dag::ConstSpan<uint16_t> refs(data->resData[i].refResIdPtr, data->resData[i].refResIdCnt);
        out_refs.resize(refs.size());
        for (int j = 0; j < refs.size(); j++)
          if (refs[j] != 0xFFFF)
            out_refs[j] = data->getName(refs[j]);
        return;
      }
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
  SimpleString hashMD5;

  void *oldDump32;
  Tab<PatchablePtr<const char>> oldMap;
  Tab<Rec> oldRec;
  Tab<int> changedIdx;
  int ver = 2;

public:
  DxpBinData() : data(NULL), fullSz(0), oldDump32(NULL) {}
  DxpBinData(const DxpBinData &) = delete;
  ~DxpBinData()
  {
    memfree(data, tmpmem);
    data = NULL;
    fullSz = 0;
    memfree(oldDump32, tmpmem);
  }

  bool buildPatch(mkbindump::BinDumpSaveCB &cwr);

  static bool findTex(dag::ConstSpan<DxpBinData> old_dxp, int pair_idx, const char *tex_name, const ddsx::Header &hdr, int &old_idx,
    int &old_rec_idx);

  static bool writeDxpHdr(mkbindump::BinDumpSaveCB &cwr, const char *dxp_fname);

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
