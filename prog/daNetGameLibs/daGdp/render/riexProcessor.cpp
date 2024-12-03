// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riexProcessor.h"

namespace dagdp
{

const RiexProcessor::ResourceState *RiexProcessor::ask(RenderableInstanceResource *res)
{
  const auto result = resources.find(res);

  if (result != resources.end())
    return &result->second;

  if (!nextToProcess)
  {
    nextToProcess = res;
    nextToProcess->addRef();
  }

  return nullptr;
}

void RiexProcessor::markCurrentAsProcessed(AreasIndices &&areasIndices)
{
  if (nextToProcess)
  {
    auto &item = resources[nextToProcess];
    item.areasIndices = eastl::move(areasIndices);
    nextToProcess = nullptr;
  }
}

void RiexProcessor::resetCurrent()
{
  if (nextToProcess)
  {
    nextToProcess->delRef();
    nextToProcess = nullptr;
  }
}

} // namespace dagdp
