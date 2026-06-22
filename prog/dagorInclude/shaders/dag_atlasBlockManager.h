//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <util/dag_simpleString.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_relocatableFixedVector.h>
#include <mutex>

class IGenLoad;

class AtlasBlockManager
{
public:
  static constexpr uint32_t POOL_BITS = 8;
  static constexpr uint32_t MAX_POOLS = (1u << POOL_BITS) - 1;
  static constexpr uint32_t MAX_POOL_SIZE = 1u << (32 - POOL_BITS);

  struct ChunkLocation
  {
    uint32_t pool : POOL_BITS;
    uint32_t firstBlock : 32 - POOL_BITS;

    ChunkLocation() : pool(MAX_POOLS), firstBlock(0) {}
    ChunkLocation(uint32_t p, uint32_t f) : pool(p), firstBlock(f) {}
    bool valid() const { return pool < MAX_POOLS; }
  };
  static_assert(sizeof(ChunkLocation) == 4);

  struct Chunk : ChunkLocation
  {
    uint32_t blockCount;

    Chunk() : blockCount(0) {}
    Chunk(ChunkLocation loc, uint32_t num_blocks) : ChunkLocation(loc), blockCount(num_blocks) {}
    Chunk(uint32_t pool, uint32_t first, uint32_t num_blocks) : ChunkLocation(pool, first), blockCount(num_blocks) {}
    bool empty() const { return blockCount == 0; }
  };
  static_assert(sizeof(Chunk) == 8);

  AtlasBlockManager(const char *name, int texture_flags);

  void setHints(const DataBlock &hints_blk);
  void clear();

  static bool isStreamingEnabled() { return true; }

  Chunk allocateChunk(uint32_t block_count);
  void releaseChunk(Chunk chunk);

  bool loadChunkData(Chunk chunk, IGenLoad &source);

  eastl::pair<BaseTexture *, uint32_t> getChunkPosition(ChunkLocation chunk) const;

  void onAfterD3dReset();

protected:
  struct Config
  {
    dag::RelocatableFixedVector<IPoint2, 4> texSizes;
    bool exactSize = false;

    Config();
    Config(const DataBlock &hints_blk);
  };
  Config getConfig() const;

  struct Pool
  {
    UniqueTex texture;
    Tab<Chunk> freeChunks;
    IPoint2 texSize;
    uint32_t numBlocks;
    bool needResize = false;

    Pool();
    ~Pool();
    Pool(Pool &&from) = default;
    Pool &operator=(Pool &&from) = default;
    int findTopFreeChunk() const;
  };

  SimpleString texName;
  Config config;
  mutable std::mutex configMutex;
  Tab<Pool> pools;
  mutable std::mutex poolMutex;
  int texFlags;
  bool needRebuild = false;
};


namespace voxeldata
{
extern AtlasBlockManager rgbaAtlasManager, normAtlasManager;
} // namespace voxeldata
