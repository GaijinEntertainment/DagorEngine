// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <backend/resourceScheduling/packer.h>
#include <dag/dag_vector.h>

namespace dabfg
{

class BaselinePacker
{
public:
  PackerOutput operator()(PackerInput input)
  {
    uint64_t offset = 0;
    offsets.resize(input.resources.size(), PackerOutput::NOT_ALLOCATED);
    for (uint32_t i = 0, ie = input.resources.size(); i < ie; ++i)
    {
      const auto resSize = input.resources[i].sizeWithPadding(offset);
      if (DAGOR_LIKELY(resSize <= input.maxHeapSize && offset <= input.maxHeapSize - resSize))
      {
        offsets[i] = input.resources[i].doAlign(offset);
        offset += resSize;
      }
      else
      {
        offsets[i] = PackerOutput::NOT_SCHEDULED;
      }
    }
    PackerOutput output;
    output.offsets = offsets;
    output.heapSize = offset;
    return output;
  }

private:
  dag::Vector<uint64_t> offsets;
};

Packer make_baseline_packer() { return BaselinePacker(); }

} // namespace dabfg
