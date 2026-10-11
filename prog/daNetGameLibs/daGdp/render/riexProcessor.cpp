// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riexProcessor.h"

namespace dagdp
{

const RiexProcessor::ResourceState *RiexProcessor::find(RenderableInstanceResource *res) const
{
  const auto result = resources.find(res);
  return result == resources.end() ? nullptr : &result->second;
}

const RiexProcessor::ResourceState *RiexProcessor::ask(RenderableInstanceResource *res)
{
  if (const auto *state = find(res))
    return state;

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
