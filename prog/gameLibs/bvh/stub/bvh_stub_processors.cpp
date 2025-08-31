// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>

namespace bvh
{

void IndexProcessor::process(Sbuffer *, UniqueBVHBufferWithOffset &, int, int, int, int) {}
bool SkinnedVertexProcessor::process(ContextId, Sbuffer *, UniqueOrReferencedBVHBuffer &, ProcessArgs &, bool) const { return false; }
bool TreeVertexProcessor::process(ContextId, Sbuffer *, UniqueOrReferencedBVHBuffer &, ProcessArgs &, bool) const { return false; }
bool ImpostorVertexProcessor::process(ContextId, Sbuffer *, UniqueOrReferencedBVHBuffer &, ProcessArgs &, bool) const { return false; }
bool BakeTextureToVerticesProcessor::process(ContextId, Sbuffer *, UniqueOrReferencedBVHBuffer &, ProcessArgs &, bool) const
{
  return false;
}
void TreeVertexProcessor::begin() const {}
void TreeVertexProcessor::end() const {}

} // namespace bvh