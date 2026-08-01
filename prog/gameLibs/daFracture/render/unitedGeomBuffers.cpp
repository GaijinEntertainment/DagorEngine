// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daFracture/render/unitedGeomBuffers.h>

#include <EASTL/numeric.h>
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hash.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>


namespace frx
{

void UnifiedGeomBuffers::allocate(dag::Span<AllocRequest> requests)
{
  FRAMEMEM_REGION;
  TIME_PROFILE(UnifiedGeomBuffers__upload);

  dag::Vector<int, framemem_allocator> reqSortedOrder;
  reqSortedOrder.resize_noinit(requests.size());
  eastl::iota(reqSortedOrder.begin(), reqSortedOrder.end(), 0);
  stlsort::sort_branchless(reqSortedOrder.begin(), reqSortedOrder.end(),
    [&](int a, int b) { return requests[a].elemCnt > requests[b].elemCnt; });

  // 2 passes - allocate in existing free regions, add missing pages, allocate again (must succeed)
  bufLock.lockWrite();
  int newBuffersCnt = 0, uploadSizeBytes = 0;
  for (int passIdx = 0; passIdx < 2; passIdx++)
  {
    eastl::vector_map<BufferProps, int, eastl::less<BufferProps>, framemem_allocator> allocFailedSize;
    for (int reqId : reqSortedOrder)
    {
      AllocRequest &__restrict req = requests[reqId];
      G_ASSERT(req.bufProp.elemSize);
      if (req.allocId != -1)
        continue;
      // find best region
      uint32_t foundSize = ~0u;
      int foundPage = -1, foundReg = -1;
      for (Page &pg : pages)
      {
        if (pg.prop != req.bufProp)
          continue;
        for (Region &reg : pg.freeRegions)
        {
          if (reg.cnt >= req.elemCnt && reg.cnt < foundSize)
          {
            foundSize = reg.cnt;
            foundPage = &pg - pages.data();
            foundReg = &reg - pg.freeRegions.data();
          }
        }
      }
      // not found
      if (foundPage == -1)
      {
        allocFailedSize.emplace(req.bufProp, 0).first->second += req.elemCnt;
        continue;
      }
      // find allocation slot
      if (!freeAllocSlots.empty())
      {
        req.allocId = freeAllocSlots.back();
        freeAllocSlots.pop_back();
      }
      else
      {
        req.allocId = allocations.size();
        allocations.push_back();
      }
      // allocate
      Page &pg = pages[foundPage];
      Region &reg = pg.freeRegions[foundReg];
      Allocation &alloc = allocations[req.allocId];
      alloc.page = foundPage;
      alloc.reg.ofs = reg.ofs;
      alloc.reg.cnt = req.elemCnt;
      reg.cnt -= req.elemCnt;
      reg.ofs += req.elemCnt;
      uploadSizeBytes += req.elemCnt * req.bufProp.elemSize;
      if (reg.cnt == 0)
        pg.freeRegions.erase(&reg);
    }

    if (allocFailedSize.empty())
      break;
    G_ASSERTF(passIdx == 0, "some allocations failed on second pass");

    // add more pages
    for (const auto [bufProp, allocElemCnt] : allocFailedSize)
    {
      Page &pg = pages.push_back();
      pg.prop = bufProp;
      uint32_t pageElemCnt = eastl::max<uint32_t>(allocElemCnt, (minPageSize + pg.prop.elemSize - 1) / pg.prop.elemSize);
      pg.freeRegions.push_back(Region{0, pageElemCnt});

      eastl::string name;
      name.sprintf("%s_%d", bufferBaseName.c_str(), nextPageId++);
      pg.buf = dag::create_sbuffer(pg.prop.elemSize, pageElemCnt, pg.prop.type == BufferType::VB ? SBCF_BIND_VERTEX : SBCF_BIND_INDEX,
        0, name.c_str(), nullptr);
      newBuffersCnt++;
    }
  }

  struct UploadBatchItem
  {
    Sbuffer *sb;
    const uint8_t *data;
    int ofs, size;
  };
  dag::Vector<UploadBatchItem, framemem_allocator> uploadBatches;
  uploadBatches.reserve(requests.size());
  for (const AllocRequest &req : requests)
  {
    if (!req.data)
      continue;
    AllocInfo a = get(req.allocId);
    UploadBatchItem &upload = uploadBatches.push_back();
    upload.sb = a.sb;
    upload.ofs = a.ofs * a.elemSz;
    upload.size = a.cnt * a.elemSz;
    upload.data = req.data;
  }
  bufLock.unlockWrite();

  stlsort::sort(uploadBatches.begin(), uploadBatches.end(), [&](UploadBatchItem &a, UploadBatchItem &b) {
    if (a.sb != b.sb)
      return uintptr_t(a.sb) < uintptr_t(b.sb);
    return a.ofs < b.ofs;
  });

  // upload
  defragLock.lockRead();
  dag::Vector<uint8_t, framemem_allocator> tmpBuf;
  int regionsCnt = 0;
  for (int i = 0; i < uploadBatches.size();)
  {
    // find continious memory span
    const int start = i++;
    while (i < uploadBatches.size() && uploadBatches[i].sb == uploadBatches[i - 1].sb &&
           uploadBatches[i].ofs == uploadBatches[i - 1].ofs + uploadBatches[i - 1].size)
      i++;
    const int end = i;
    regionsCnt++;

    if (end - start == 1)
    {
      uploadBatches[start].sb->updateData(uploadBatches[start].ofs, uploadBatches[start].size, uploadBatches[start].data, 0);
    }
    else
    {
      const int bufferSz = uploadBatches[end - 1].ofs + uploadBatches[end - 1].size - uploadBatches[start].ofs;
      tmpBuf.clear();
      tmpBuf.resize_noinit(bufferSz);
      for (int j = start, ofs = 0; j < end; j++)
      {
        memcpy(tmpBuf.data() + ofs, uploadBatches[j].data, uploadBatches[j].size);
        ofs += uploadBatches[j].size;
      }
      uploadBatches[start].sb->updateData(uploadBatches[start].ofs, tmpBuf.size(), tmpBuf.data(), 0);
    }
  }
  defragLock.unlockRead();

  DA_PROFILE_TAG(UnifiedGeomBuffers__upload, "allocs:%d/writes:%d/new_buf:%d/kb:%d", int(requests.size()), regionsCnt, newBuffersCnt,
    uploadSizeBytes >> 10);
}

void UnifiedGeomBuffers::free(int alloc_id)
{
  ScopedLockWrite lock(bufLock);
  Allocation &alloc = allocations[alloc_id];
  G_ASSERT_RETURN(alloc.page != uint16_t(~0), );
  Page &pg = pages[alloc.page];
  auto it = eastl::lower_bound(pg.freeRegions.begin(), pg.freeRegions.end(), alloc.reg, [&](auto a, auto b) { return a.ofs < b.ofs; });
  Region *next = it != pg.freeRegions.end() ? it : nullptr;
  Region *prev = it != pg.freeRegions.begin() ? it - 1 : nullptr;
  G_ASSERT(!next || alloc.reg.ofs + alloc.reg.cnt <= next->ofs);
  G_ASSERT(!prev || prev->ofs + prev->cnt <= alloc.reg.ofs);
  if (next && alloc.reg.ofs + alloc.reg.cnt < next->ofs)
    next = nullptr;
  if (prev && prev->ofs + prev->cnt < alloc.reg.ofs)
    prev = nullptr;
  if (next && prev)
  {
    G_ASSERT(next->ofs - prev->ofs == prev->cnt + alloc.reg.cnt);
    prev->cnt = (next->ofs + next->cnt) - prev->ofs;
    pg.freeRegions.erase(next);
  }
  else if (next)
  {
    next->cnt += alloc.reg.cnt;
    next->ofs -= alloc.reg.cnt;
  }
  else if (prev)
  {
    prev->cnt += alloc.reg.cnt;
  }
  else
  {
    pg.freeRegions.insert(it, alloc.reg);
  }
  freeAllocSlots.push_back(&alloc - allocations.data());
  alloc = Allocation();
}


} // namespace frx