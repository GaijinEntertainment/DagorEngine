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
};

struct MemoryStatistics
{
  int tlasSize;
  int tlasUploadSize;
  int scratchBuffersSize;
  int transformBuffersSize;
  int meshMetaSize;
  int meshCount;
  int meshBlasSize;
  int meshVBSize;
  int meshVBCopySize;
  int meshIBSize;
  int skinCount;
  int skinBLASSize;
  int skinVBSize;
  int treeCount;
  int treeBLASSize;
  int treeVBSize;
  int treeCacheCount;
  int treeCacheBLASSize;
  int dynamicVBAllocatorSize;
  int terrainBlasSize;
  int terrainVBSize;
  int grassBlasSize;
  int grassVBSize;
  int grassIBSize;
  int grassMetaSize;
  int grassQuerySize;
  int cableBLASSize;
  int cableVBSize;
  int cableIBSize;
  int gobjMetaSize;
  int gobjQuerySize;
  int blasCount;
  int perInstanceDataSize;
  int compactionSize;
  int peakCompactionSize;
  int atmosphereTextureSize;
  int totalMemory;
  int demoiserCommonSizeMB;
  int demoiserAoSizeMB;
  int demoiserReflectionSizeMB;
  int demoiserTransientSizeMB;
  int deathRowBufferCount;
  int deathRowBufferSize;
  int indexProcessorBufferCount;
  int indexProcessorBufferSize;
};

MemoryStatistics get_memory_statistics(ContextId context_id);

} // namespace bvh