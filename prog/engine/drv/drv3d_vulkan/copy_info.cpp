// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include "copy_info.h"

using namespace drv3d_vulkan;

namespace
{
bool isSameImageSection(const VkBufferImageCopy &l, const VkBufferImageCopy &r)
{
  return 0 == memcmp(&l.imageSubresource, &r.imageSubresource, sizeof(l.imageSubresource)) &&
         0 == memcmp(&l.imageOffset, &r.imageOffset, sizeof(l.imageOffset)) &&
         0 == memcmp(&l.imageExtent, &r.imageExtent, sizeof(l.imageExtent));
}
} // namespace

void BufferCopyInfo::optimizeBufferCopies(dag::Vector<BufferCopyInfo> &info, dag::Vector<VkBufferCopy> &copies)
{
  // group copy ops by src/dst match, allows easy merge of multiple copies with same src/dst
  eastl::sort(begin(info), end(info),
    [](const BufferCopyInfo &l, const BufferCopyInfo &r) //
    {
      if (l.src < r.src)
        return true;
      if (l.src > r.src)
        return false;
      if (l.dst < r.dst)
        return true;
      if (l.dst > r.dst)
        return false;
      return l.copyIndex < r.copyIndex;
    });

  // reorganized the buffer copies to match the ordering of the infos
  // this approach is simple but requires additional memory and copies
  // every entry once
  // don't reallocate while we copy stuff around inside this vector
  copies.reserve(copies.size() * 2);
  for (auto &&upload : info)
  {
    auto start = begin(copies) + upload.copyIndex;
    auto stop = start + upload.copyCount;
    upload.copyIndex = copies.size();
    copies.insert(end(copies), start, stop);
  }

  // now merge with same src/dst pair
  for (uint32_t i = info.size() - 1; i > 0; --i)
  {
    auto &l = info[i - 1];
    auto &r = info[i];

    if (l.src != r.src)
      continue;
    if (l.dst != r.dst)
      continue;
    // those are guaranteed to be back to back
    l.copyCount += r.copyCount;
    // remove right copy
    info.erase(begin(info) + i);
  }
}

void ImageCopyInfo::deduplicate(dag::Vector<ImageCopyInfo> &info, dag::Vector<VkBufferImageCopy> &copies)
{
  for (uint32_t i = 0; i < info.size(); ++i)
  {
    auto &base = info[info.size() - 1 - i];
    for (uint32_t j = i + 1; j < info.size(); ++j)
    {
      auto &compare = info[info.size() - 1 - j];
      if (compare.image != base.image)
        continue;

      for (uint32_t k = 0; k < base.copyCount; ++k)
      {
        auto &copyBase = copies[base.copyIndex + k];

        for (uint32_t l = 0; l < compare.copyCount; ++l)
        {
          auto &copyCompare = copies[compare.copyIndex + l];
          if (isSameImageSection(copyBase, copyCompare))
          {
            copyCompare = copies[compare.copyIndex + compare.copyCount - 1];
            --compare.copyCount;
            // we assume per copy set there is no duplicated copy op, so we can stop
            // duplicated copy sections in the same block are not possible
            break;
          }
        }
      }
    }
  }

  // tidy up empty copies in a second step, makes the checking loop much simpler
  info.erase(eastl::remove_if(begin(info), end(info), [](auto &info) { return info.copyCount == 0; }), end(info));
}
