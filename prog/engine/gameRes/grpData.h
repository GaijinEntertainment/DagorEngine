#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_patchTab.h>

namespace gamerespackbin
{
struct ResEntry
{
  uint32_t classId;
  uint32_t offset;
  uint16_t resId, _resv;
};

struct ResDataV2
{
  uint32_t classId;
  uint16_t resId, _resv;
  uint32_t refResIdOfs, refResIdCnt, _resv1, _resv2;
};
struct ResData
{
  uint32_t classId;
  uint16_t resId, refResIdCnt;
  PatchablePtr<uint16_t> refResIdPtr;

  static void cvtRecInplaceVer2to3(ResData *data, unsigned size)
  {
    for (ResDataV2 *old = (ResDataV2 *)data; size > 0; old++, data++, size--)
    {
      data->classId = old->classId;
      data->resId = old->resId;
      data->refResIdCnt = old->refResIdCnt;
      data->refResIdPtr.setPtr((void *)(uintptr_t)old->refResIdOfs);
    }
  }
};

struct GrpData
{
  PatchableTab<int> nameMap;
  PatchableTab<ResEntry> resTable;
  PatchableTab<ResData> resData;

  inline char *dumpBase() const;
  void patchDescOnly(uint32_t hdr_label);
  void patchData();
  const char *getName(int idx) const { return nameMap[idx] != -1 ? dumpBase() + nameMap[idx] : NULL; }
};

struct GrpHeader
{
  uint32_t label;
  uint32_t descOnlySize;
  uint32_t fullDataSize;
  uint32_t restFileSize;
};

inline char *GrpData::dumpBase() const { return ((char *)this) - sizeof(GrpHeader); }
} // namespace gamerespackbin

void repack_real_game_res_id_table();
