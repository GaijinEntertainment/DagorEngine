// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/bvh_process_tree_vertices.hlsli"
#include "../shaders/bvh_skinned_instance_data_batched.hlsli"

namespace bvh
{

void IndexProcessor::process(Sbuffer *, BVHGeometryBufferWithOffset &, int, int, int, int, ContextId) {}
bool SkinnedVertexProcessor::process(ContextId, Sbuffer *, int, uint32_t, UniqueOrReferencedBVHBuffer &, uint32_t &, ProcessArgs &,
  bool) const
{
  return false;
}
bool SkinnedVertexProcessorBatched::process(ContextId, Sbuffer *, int, uint32_t, UniqueOrReferencedBVHBuffer &, uint32_t &,
  ProcessArgs &, bool) const
{
  return false;
}
bool TreeVertexProcessor::process(ContextId, Sbuffer *, int, uint32_t, UniqueOrReferencedBVHBuffer &, uint32_t &, ProcessArgs &,
  bool) const
{
  return false;
}
bool ImpostorVertexProcessor::process(ContextId, Sbuffer *, int, uint32_t, UniqueOrReferencedBVHBuffer &, uint32_t &, ProcessArgs &,
  bool) const
{
  return false;
}
bool BakeTextureToVerticesProcessor::process(ContextId, Sbuffer *, int, uint32_t, UniqueOrReferencedBVHBuffer &, uint32_t &,
  ProcessArgs &, bool) const
{
  return false;
}
void TreeVertexProcessor::begin() const {}
void TreeVertexProcessor::end(bool is_prototype_buidling) const { G_UNUSED(is_prototype_buidling); }
TreeVertexProcessor::DispatchData::~DispatchData() = default;
void SkinnedVertexProcessorBatched::begin() const {}
void SkinnedVertexProcessorBatched::end(bool is_prototype_buidling) const { G_UNUSED(is_prototype_buidling); }
SkinnedVertexProcessorBatched::DispatchData::~DispatchData() = default;

} // namespace bvh