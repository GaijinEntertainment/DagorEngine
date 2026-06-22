// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_atlasBlockManager.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>


AtlasBlockManager voxeldata::rgbaAtlasManager("voxelSurfRgba", TEXFMT_BC7);
AtlasBlockManager voxeldata::normAtlasManager("voxelSurfNorm", TEXFMT_ATI2N);


AtlasBlockManager::AtlasBlockManager(const char *name, int tf) : texName(name), texFlags(tf | TEXCF_WRITEONLY), pools(midmem_ptr()) {}

AtlasBlockManager::Config::Config() {}

AtlasBlockManager::Config::Config(const DataBlock &hints_blk)
{
  char name[16];
  for (int i = 1;; i++)
  {
    snprintf(name, sizeof(name), "tex%d", i);
    int pi = hints_blk.findParam(name);
    if (pi == -1)
      break;
    if (hints_blk.getParamType(pi) != DataBlock::TYPE_IPOINT2)
      break;
    IPoint2 s = hints_blk.getIPoint2(pi);
    texSizes.push_back(min(s, IPoint2(16, 16)));
  }

  exactSize = hints_blk.getBool("exactSize", exactSize);
}

void AtlasBlockManager::setHints(const DataBlock &hints_blk)
{
  Config cfg{hints_blk};
  std::lock_guard guard{configMutex};
  config = cfg;
}

AtlasBlockManager::Config AtlasBlockManager::getConfig() const
{
  std::lock_guard guard{configMutex};
  return config;
}


AtlasBlockManager::Pool::Pool() : freeChunks(midmem), texSize(0, 0), numBlocks(0) {}

AtlasBlockManager::Pool::~Pool() {}

int AtlasBlockManager::Pool::findTopFreeChunk() const
{
  for (int i = freeChunks.size() - 1; i >= 0; i--)
    if (freeChunks[i].firstBlock + freeChunks[i].blockCount == numBlocks)
      return i;
  return -1;
}


static uint32_t enlarge_size(IPoint2 &tex_size, uint32_t block_count, int maxw, int maxh)
{
  uint32_t numBlocks = (tex_size.x >> 2) * (tex_size.y >> 2);
  IPoint2 newSize = tex_size;
  if (int h = min(newSize.x, min<int>(maxh, AtlasBlockManager::MAX_POOL_SIZE / (newSize.x >> 4))); newSize.y < h)
    newSize.y = h;
  uint32_t freeBlocks = (newSize.x >> 2) * (newSize.y >> 2) - numBlocks;
  if (freeBlocks < block_count)
  {
    // still not enough, enlarge to max size
    newSize.y = maxh;
    freeBlocks = (newSize.x >> 2) * (newSize.y >> 2) - numBlocks;
  }
  if (freeBlocks < block_count)
    return 0;
  tex_size = newSize;
  return freeBlocks;
}

AtlasBlockManager::Chunk AtlasBlockManager::allocateChunk(uint32_t block_count)
{
  if (block_count == 0)
    return {};
  if (block_count > MAX_POOL_SIZE)
  {
    logerr("block chunk too big for internal limit (%u>%u), atlas '%s'", block_count, MAX_POOL_SIZE, texName.c_str());
    return {};
  }

  std::lock_guard guard(poolMutex);
  for (auto &pool : pools)
  {
    for (int ci = 0, cn = pool.freeChunks.size(); ci < cn; ci++)
    {
      auto &c = pool.freeChunks[ci];
      if (c.blockCount < block_count)
        continue;
      Chunk res = c;
      res.blockCount = block_count;
      c.firstBlock += block_count;
      c.blockCount -= block_count;
      if (c.blockCount <= 0)
        pool.freeChunks.erase(pool.freeChunks.begin() + ci);
      return res;
    }
  }

  auto cfg = getConfig();
  int maxw = d3d::get_driver_desc().maxtexw;
  int maxh = d3d::get_driver_desc().maxtexh;
  if (cfg.texSizes.empty())
  {
    if (cfg.exactSize and pools.empty())
      logerr("exactSize=true for atlas '%s', but no texture sizes specified in blk", texName.c_str());
    cfg.texSizes.push_back(IPoint2(maxw, cfg.exactSize ? maxh : 64));
    // cfg.texSizes.push_back(IPoint2(1024, 1024)); cfg.exactSize = true; //== to test with limited atlas space
  }

  if (cfg.exactSize and pools.size() >= cfg.texSizes.size())
    return {};

  if (!cfg.exactSize and !pools.empty())
  {
    // try enlarging last texture
    auto &pool = pools.back();
    int top = pool.findTopFreeChunk();
    uint32_t topFree = (top >= 0 ? pool.freeChunks[top].blockCount : 0);
    G_ASSERT(topFree < block_count);

    IPoint2 newSize = pool.texSize;
    uint32_t freeBlocks = enlarge_size(newSize, block_count - topFree, maxw, maxh) + topFree;
    if (freeBlocks >= block_count)
    {
      if (top < 0)
      {
        top = pool.freeChunks.size();
        pool.freeChunks.emplace_back(pools.size() - 1, pool.numBlocks, freeBlocks);
      }
      else
        pool.freeChunks[top].blockCount = freeBlocks;
      pool.texSize = newSize;
      pool.numBlocks = (newSize.x >> 2) * (newSize.y >> 2);
      pool.needResize = true;
      Chunk &c = pool.freeChunks[top];
      Chunk res = c;
      res.blockCount = block_count;
      c.firstBlock += block_count;
      c.blockCount -= block_count;
      G_ASSERT(c.firstBlock + c.blockCount == pool.numBlocks);
      if (c.blockCount <= 0)
        pool.freeChunks.erase(pool.freeChunks.begin() + top);
      return res;
    }
  }

  // add new pool
  if (pools.size() >= MAX_POOLS or (cfg.exactSize and pools.size() >= cfg.texSizes.size()))
    return {};

  IPoint2 texSize = cfg.texSizes[pools.size() < cfg.texSizes.size() ? pools.size() : cfg.texSizes.size() - 1];
  if (!cfg.exactSize)
  {
    texSize = min(texSize, IPoint2(maxw, maxh));
    texSize = max(texSize, IPoint2(d3d::get_driver_desc().mintexw, d3d::get_driver_desc().mintexh));
    texSize.x &= ~3;
    texSize.y &= ~3;
    enlarge_size(texSize, block_count, maxw, maxh);
  }

  uint32_t freeBlocks = (texSize.x >> 2) * (texSize.y >> 2);
  if (freeBlocks < block_count)
  {
    logerr("can't fit %d blocks in new %dx%d atlas texture '%s%d'", block_count, P2D(texSize), texName.c_str(), pools.size());
    return {};
  }

  auto &pool = pools.emplace_back();
  pool.texSize = texSize;
  pool.numBlocks = (texSize.x >> 2) * (texSize.y >> 2);
  if (block_count < pool.numBlocks)
    pool.freeChunks.emplace_back(pools.size() - 1, block_count, pool.numBlocks - block_count);
  return {pools.size() - 1, 0, block_count};
}


void AtlasBlockManager::releaseChunk(Chunk chunk)
{
  if (chunk.empty() or !chunk.valid())
    return;

  std::lock_guard guard(poolMutex);
  G_ASSERT_RETURN(chunk.pool < pools.size(), );
  auto &pool = pools[chunk.pool];
  G_ASSERT_RETURN(chunk.firstBlock < pool.numBlocks and chunk.firstBlock + chunk.blockCount <= pool.numBlocks, );
  for (bool done = false; !done;)
  {
    done = true;
    for (int ci = 0, cn = pool.freeChunks.size(); ci < cn; ci++)
    {
      auto &c = pool.freeChunks[ci];
      if (c.firstBlock + c.blockCount == chunk.firstBlock)
      {
        chunk.firstBlock = c.firstBlock;
        chunk.blockCount += c.blockCount;
        pool.freeChunks.erase(pool.freeChunks.begin() + ci);
        done = false;
        break;
      }
      if (chunk.firstBlock + chunk.blockCount == c.firstBlock)
      {
        chunk.blockCount += c.blockCount;
        pool.freeChunks.erase(pool.freeChunks.begin() + ci);
        done = false;
        break;
      }

      if (ci + 1 == cn and c.firstBlock + c.blockCount == pool.numBlocks)
      {
        // insert before top block
        pool.freeChunks.insert(pool.freeChunks.begin() + ci, chunk);
        return;
      }
    }
  }

  pool.freeChunks.push_back(chunk);
}


void AtlasBlockManager::clear()
{
  std::lock_guard guard(poolMutex);
  clear_and_shrink(pools);
}


void AtlasBlockManager::onAfterD3dReset() {}


bool AtlasBlockManager::loadChunkData(Chunk chunk, IGenLoad &source)
{
  if (!chunk.valid())
    return false;
  if (chunk.empty())
    return true;

  std::lock_guard guard(poolMutex);
  G_ASSERT_RETURN(chunk.pool < pools.size(), false);
  auto &pool = pools[chunk.pool];
  G_ASSERT_RETURN(chunk.firstBlock < pool.numBlocks and chunk.firstBlock + chunk.blockCount <= pool.numBlocks, false);

  bool ok = true;
  if (!pool.texture or pool.needResize)
  {
    String name(0, "%s%d", texName.c_str(), chunk.pool);
    UniqueTex tmpTex = dag::create_tex(nullptr, P2D(pool.texSize), texFlags, 1, name.c_str());
    if (!tmpTex)
    {
      logerr("failed to create %dx%d atlas texture '%s'", P2D(pool.texSize), name.c_str());
      return false;
    }
    debug("created %dx%d atlas texture '%s'", P2D(pool.texSize), name.c_str());

    pool.needResize = false;
    eastl::swap(pool.texture, tmpTex);

    if (tmpTex)
    {
      // resize
      TextureInfo info;
      tmpTex.getTex2D()->getinfo(info);
      debug("resizing atlas '%s' from %dx%d to %dx%d", name.c_str(), info.w, info.h, P2D(pool.texSize));

      int srcBlockWidth = info.w >> 2;
      int dstBlockWidth = pool.texSize.x >> 2;
      for (int y = 0, srcRowStart = 0; y < info.h; y += 4, srcRowStart += srcBlockWidth)
      {
        for (int bs = srcRowStart; bs < srcRowStart + srcBlockWidth;)
        {
          int be = srcRowStart + srcBlockWidth;
          for (const auto &c : pool.freeChunks)
            if (bs < c.firstBlock + c.blockCount and be > c.firstBlock)
            {
              if (bs < c.firstBlock)
                be = c.firstBlock;
              else
                bs = c.firstBlock + c.blockCount;

              if (bs >= be)
                break;
            }

          if (bs < be)
          {
            int x = (bs - srcRowStart) << 2;
            int dy0 = (bs / dstBlockWidth) << 2;
            int dx0 = (bs % dstBlockWidth) << 2;
            int dy1 = ((be - 1) / dstBlockWidth) << 2;
            int dx1 = ((be - 1) % dstBlockWidth) << 2;
            for (int dy = dy0; dy <= dy1; dy += 4)
            {
              int dx = dy == dy0 ? dx0 : 0;
              int w = (dy == dy1 ? dx1 + 4 : dstBlockWidth << 2) - dx;
              if (!pool.texture.getTex2D()->updateSubRegionNoOrder(tmpTex.getTex2D(), 0, x, y, 0, w, 4, 1, 0, dx, dy, 0))
                ok = false;
            }
          }

          bs = be;
        }
      }
    }
  }

  int bw = pool.texSize.x >> 2;
  int y0 = (chunk.firstBlock / bw) << 2;
  int x0 = (chunk.firstBlock % bw) << 2;
  int y1 = ((chunk.firstBlock + chunk.blockCount - 1) / bw) << 2;
  int x1 = ((chunk.firstBlock + chunk.blockCount - 1) % bw) << 2;

  for (int y = y0; y <= y1; y += 4)
  {
    int x = y == y0 ? x0 : 0;
    int w = (y == y1 ? x1 + 4 : bw << 2) - x;
    debug("loading %d,%d..%d to %s%d", x, y, x + w, texName.c_str(), chunk.pool);
    if (auto rub = d3d::allocate_update_buffer_for_tex_region(pool.texture.getTex2D(), 0, 0, x, y, 0, w, 4, 1))
    {
      void *dest = d3d::get_update_buffer_addr_for_write(rub);
      size_t dest_stride = d3d::get_update_buffer_pitch(rub);
      source.read(dest, w << 2); //== TODO: support formats other than 8 bit per pixel?
      d3d::update_texture_and_release_update_buffer(rub);
    }
    else
      ok = false;
  }
  return ok;
}


eastl::pair<BaseTexture *, uint32_t> AtlasBlockManager::getChunkPosition(ChunkLocation chunk) const
{
  static constexpr auto INVALID = eastl::pair<BaseTexture *, uint32_t>(nullptr, 0);
  if (!chunk.valid())
    return INVALID;

  std::lock_guard guard(poolMutex);
  G_ASSERT_RETURN(chunk.pool < pools.size(), INVALID);
  auto &pool = pools[chunk.pool];
  G_ASSERT_RETURN(chunk.firstBlock < pool.numBlocks, INVALID);
  return {pool.texture.getTex2D(), uint32_t(chunk.firstBlock)};
}
