// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <EASTL/unordered_map.h>

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
  struct MeshStats
  {
    int meshCount = 0;
    int64_t meshBlasSize = 0;
    int64_t meshVBSize = 0;
  };
  eastl::unordered_map<const char *, MeshStats> meshStats;
  int skinCount;
  int64_t skinBLASSize;
  int64_t skinVBSize;
  int skinCacheCount;
  int64_t skinCacheBLASSize;
  int treeCount;
  int64_t treeBLASSize;
  int64_t treeVBSize;
  int stationaryTreeCount;
  int64_t stationaryTreeBLASSize;
  int64_t stationaryTreeVBSize;
  int treeCacheCount;
  int64_t treeCacheBLASSize;
  int treeRiExCount;
  int64_t treeRiExBLASSize;
  int64_t treeRiExVBSize;
  int treeRiExCacheCount;
  int64_t treeRiExCacheBLASSize;
  int flagCount;
  int64_t flagBLASSize;
  int64_t flagVBSize;
  int64_t dynamicVBAllocatorSize;
  int64_t dynamicVBAllocatorFreeSize;
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
  int binSceneCount;
  int waterCount;
  int64_t waterBLASSize;
  int64_t waterVBSize;
  int64_t waterIBSize;
  int gpuGrassCount;
  int64_t gpuGrassMemory;
  int64_t gpuGrassTexturesMemory;
  int64_t gobjMetaSize;
  int64_t gobjQuerySize;
  int splineGenCount;
  int64_t splineGenBLASSize;
  int64_t splineGenVBSize;
  int blasCount;
  int64_t perInstanceDataSize;
  int64_t compactionSize;
  int64_t peakCompactionSize;
  int compactionCount;
  int compactionSizeBufferSize;
  int64_t atmosphereTextureSize;
  int64_t totalMemory;
  int deathRowBufferCount;
  int64_t deathRowBufferSize;
  int indexProcessorBufferCount;
  int64_t indexProcessorBufferSize;
};

MemoryStatistics get_memory_statistics(ContextId context_id);

} // namespace bvh