// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "debug_impl.h"

namespace omm
{

ommResult SaveAsImagesImpl(StdAllocator<uint8_t> &, const ommCpuBakeInputDesc &, const ommCpuBakeResultDesc *,
  const ommDebugSaveImagesDesc &)
{
  return ommResult_NOT_IMPLEMENTED;
}

ommResult GetStatsImpl(StdAllocator<uint8_t> &, const ommCpuBakeResultDesc *, const float *, ommDebugStats *)
{
  return ommResult_NOT_IMPLEMENTED;
}

ommResult SaveBinaryToDiskImpl(const Logger &, const ommCpuBlobDesc &, const char *) { return ommResult_NOT_IMPLEMENTED; }

} // namespace omm