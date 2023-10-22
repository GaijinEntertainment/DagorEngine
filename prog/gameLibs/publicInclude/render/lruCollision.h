//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_baseDef.h>
#include <util/dag_stdint.h>
#include <generic/dag_span.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMathDecl.h>
#include <rendInst/riexHandle.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_serialIntBuffer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <memory/dag_linearHeapAllocator.h>

class Sbuffer;

class LRURendinstCollision
{
public:
  LRURendinstCollision();
  LRURendinstCollision(const LRURendinstCollision &) = delete;
  ~LRURendinstCollision();

  bool updateLRU(dag::ConstSpan<rendinst::riex_handle_t> ri);
  void reset();
  void clearRiInfo();
  bool canVoxelize() const { return (voxelizeCollisionElem || voxelize_collision_cs); }
  void voxelize(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha);
  void draw(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha, uint32_t inst_mul,
    ShaderElement &elem);
  void dispatchInstances(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha,
    ComputeShaderElement &cs);
  ShaderElement *getVoxelizeCollisionElem() { return voxelizeCollisionElem; }

protected:
  void drawInstances(dag::ConstSpan<rendinst::riex_handle_t> handles, VolTexture *color, VolTexture *alpha, uint32_t inst_mul,
    ShaderElement &elem);
  uint32_t getMaxBatchSize() const;
  uint32_t getVbCapacity() const;
  uint32_t getIbCapacity() const;
  bool allocateEntries(const uint32_t *begin, const uint32_t *end, uint32_t &vMaxSz, uint32_t &iMaxSz, uint32_t cframe);

  struct RiDataInfo
  {
    uint32_t ibSize = 0, vbSize = 0; // in bytes
    bool isEmpty() const { return ibSize == 0; }
  };
  eastl::vector<RiDataInfo> riInfo; // direct mapping.
  RiDataInfo getRiData(uint32_t type);
  struct LRUEntry
  {
    LinearHeapAllocatorSbuffer::RegionId vb, ib;
  };
  eastl::vector<LRUEntry> riToLRUmap;
  eastl::vector<uint32_t> entryLastFrameUsed;

  // uint32_t currentTick = 0;
  struct CollisionVertex
  {
    uint16_t x, y, z, w;
  };

  static const int COLLISION_BOX_INDICES_NUM = 36;
  static const int COLLISION_BOX_VERTICES_NUM = 8;

  carray<uint16_t, COLLISION_BOX_INDICES_NUM> boxIndices;

  void drawInstances(uint32_t st_inst, const uint32_t *types_counts, uint32_t batches, VolTexture *color, VolTexture *alpha,
    uint32_t inst_mul);
  void dispatchInstances(uint32_t st_inst, const uint32_t *types_counts, uint32_t batches, VolTexture *color, VolTexture *alpha,
    uint32_t tsz);
  void batchInstances(const rendinst::riex_handle_t *begin, const rendinst::riex_handle_t *end,
    eastl::function<void(uint32_t start_instance, const uint32_t *types_counts, uint32_t count)> cb);
  UniqueBufHolder instanceTms;
  LinearHeapAllocatorSbuffer vbAllocator, ibAllocator;
  UniqueBuf multiDrawBuf;
  bool supportNoOverwrite = false;
  uint32_t lastUpdatedInstance = 0;
  uint32_t lastFrameTmsUsed = 0;
  eastl::unique_ptr<ComputeShaderElement> voxelize_collision_cs;
  eastl::unique_ptr<ShaderMaterial> voxelizeCollisionMat;
  ShaderElement *voxelizeCollisionElem = 0;
  serial_buffer::SerialBufferCounter serialBuf;
};
