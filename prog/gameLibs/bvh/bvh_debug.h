// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>

namespace bvh
{

enum class DebugMode
{
  Unknown,
  None,
  Lit,
  DiffuseColor,
  Normal,
  Texcoord,
  SecTexcoord,
  CamoTexcoord,
  VertexColor,
  GI,
  TwoSided,
  Paint,
  IntersectionCount,
  Instances,
};

struct MemoryStatistics
{
  int64_t tlasSize;
  int64_t tlasUploadSize;
  int64_t scratchBuffersSize;
  int64_t transformBuffersSize;
  int64_t meshMetaSize;
  int meshCount;
  int64_t meshBlasSize;
  int64_t meshVBSize;
  int64_t meshVBCopySize;
  int64_t meshIBSize;
  int skinCount;
  int64_t skinBLASSize;
  int64_t skinVBSize;
  int skinCacheCount;
  int64_t skinCacheBLASSize;
  int treeCount;
  int64_t treeBLASSize;
  int64_t treeVBSize;
  int treeCacheCount;
  int64_t treeCacheBLASSize;
  int64_t dynamicVBAllocatorSize;
  int64_t terrainBlasSize;
  int64_t terrainVBSize;
  int64_t grassBlasSize;
  int64_t grassVBSize;
  int64_t grassIBSize;
  int64_t grassMetaSize;
  int64_t grassQuerySize;
  int64_t cableBLASSize;
  int64_t cableVBSize;
  int64_t cableIBSize;
  int64_t gobjMetaSize;
  int64_t gobjQuerySize;
  int blasCount;
  int64_t perInstanceDataSize;
  int64_t compactionSize;
  int64_t peakCompactionSize;
  int64_t atmosphereTextureSize;
  int64_t totalMemory;
  int deathRowBufferCount;
  int64_t deathRowBufferSize;
  int indexProcessorBufferCount;
  int64_t indexProcessorBufferSize;
};

MemoryStatistics get_memory_statistics(ContextId context_id);

} // namespace bvh