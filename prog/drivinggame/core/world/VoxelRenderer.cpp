#include "core/world/VoxelRenderer.h"
#include "core/world/VoxelWorld.h"
#include "core/world/Biome.h"
#include <EASTL/algorithm.h>
#include <EASTL/numeric_limits.h>
#include <stdio.h>
#include <math.h>
#include <3d/dag_resMgr.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVar.h>

namespace drivinggame
{

static constexpr int FACE_DIRS[6][3] = {
  {1, 0, 0},  // +X right
  {-1, 0, 0}, // -X left
  {0, 1, 0},  // +Y top
  {0, -1, 0}, // -Y bottom
  {0, 0, 1},  // +Z front
  {0, 0, -1}  // -Z back
};

static constexpr float FACE_VERTS[6][4][3] = {
  {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}},
  {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
  {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
  {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
  {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
  {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
};

static constexpr uint16_t FACE_INDICES[6] = {0, 1, 2, 2, 3, 0};

VoxelRenderer::VoxelRenderer(VoxelWorld* world) : world_(world)
{
  initBlockTypes();
}

VoxelRenderer::~VoxelRenderer()
{
  shutdown();
}

bool VoxelRenderer::init()
{
  initBlockTypes();
  return true;
}

void VoxelRenderer::shutdown()
{
  cleanupBuffers();
  chunkRenderData_.clear();
  dirtyChunks_.clear();
}

void VoxelRenderer::initBlockTypes()
{
  if (blockTypesInitialized_)
    return;
  
  for (int i = 0; i < 256; ++i)
  {
    VoxelBlockType& bt = blockTypes_[i];
    bt.type = static_cast<VoxelType>(i);
    bt.textureTop = i;
    bt.textureBottom = i;
    bt.textureSide = i;
    bt.isOpaque = (i != 0);
    bt.isSolid = (i != 0);
    bt.opacity = bt.isOpaque ? 1.0f : 0.0f;
  }
  
  blockTypes_[static_cast<int>(VoxelType::AIR)] = {VoxelType::AIR, 0, 0, 0, false, false, 0.0f};
  blockTypes_[static_cast<int>(VoxelType::GRASS_BLOCK)] = {VoxelType::GRASS_BLOCK, 1, 2, 3, true, true, 1.0f};
  blockTypes_[static_cast<int>(VoxelType::DIRT)] = {VoxelType::DIRT, 2, 2, 2, true, true, 1.0f};
  blockTypes_[static_cast<int>(VoxelType::STONE)] = {VoxelType::STONE, 4, 4, 4, true, true, 1.0f};
  blockTypes_[static_cast<int>(VoxelType::SAND)] = {VoxelType::SAND, 5, 5, 5, true, true, 1.0f};
  blockTypes_[static_cast<int>(VoxelType::OAK_LOG)] = {VoxelType::OAK_LOG, 6, 6, 7, true, true, 1.0f};
  blockTypes_[static_cast<int>(VoxelType::OAK_LEAVES)] = {VoxelType::OAK_LEAVES, 8, 8, 8, false, true, 0.5f};
  blockTypes_[static_cast<int>(VoxelType::WATER)] = {VoxelType::WATER, 9, 9, 9, false, false, 0.8f};
  blockTypes_[static_cast<int>(VoxelType::LAVA)] = {VoxelType::LAVA, 10, 10, 10, false, false, 0.9f};
  
  blockTypesInitialized_ = true;
}

const VoxelBlockType& VoxelRenderer::getBlockType(VoxelType type) const
{
  int idx = static_cast<int>(type);
  if (idx >= 0 && idx < 256)
    return blockTypes_[idx];
  return blockTypes_[0];
}

void VoxelRenderer::tick(float dt)
{
  const int MAX_REBUILDS_PER_TICK = 4;
  int processed = 0;
  
  while (!dirtyChunks_.empty() && processed < MAX_REBUILDS_PER_TICK)
  {
    ChunkPos pos = dirtyChunks_.back();
    dirtyChunks_.pop_back();
    
    auto it = chunkRenderData_.find(pos);
    if (it != chunkRenderData_.end())
    {
      ChunkRenderData& data = it->second;
      for (int s = 0; s < 32; ++s)
      {
        if (data.sectionDirty[s])
        {
          rebuildSectionMesh(pos, s);
          data.sectionDirty[s] = false;
          processed++;
          if (processed >= MAX_REBUILDS_PER_TICK)
            break;
        }
      }
      data.needsRebuild = false;
    }
  }
}

void VoxelRenderer::updateVisibleChunks(const Point3& cameraPos, float viewDist)
{
  int viewDistChunks = static_cast<int>(viewDist / 32.0f) + 1;
  int camChunkX = static_cast<int>(floor(cameraPos.x / 32.0f));
  int camChunkZ = static_cast<int>(floor(cameraPos.z / 32.0f));
  
  for (auto it = chunkRenderData_.begin(); it != chunkRenderData_.end();)
  {
    const ChunkPos& pos = it->first;
    int dx = abs(pos.x - camChunkX);
    int dz = abs(pos.z - camChunkZ);
    if (dx > viewDistChunks + 2 || dz > viewDistChunks + 2)
    {
      it = chunkRenderData_.erase(it);
    }
    else
    {
      ++it;
    }
  }
  
  for (int dx = -viewDistChunks; dx <= viewDistChunks; ++dx)
  {
    for (int dz = -viewDistChunks; dz <= viewDistChunks; ++dz)
    {
      ChunkPos pos(camChunkX + dx, camChunkZ + dz);
      if (chunkRenderData_.find(pos) == chunkRenderData_.end())
      {
        ChunkRenderData data;
        data.pos = pos;
        data.sectionDirty.fill(true);
        chunkRenderData_.emplace(pos, data);
        dirtyChunks_.push_back(pos);
      }
    }
  }
}

void VoxelRenderer::markChunkModified(const ChunkPos& pos, int sectionIdx)
{
  auto it = chunkRenderData_.find(pos);
  if (it != chunkRenderData_.end())
  {
    ChunkRenderData& data = it->second;
    data.needsRebuild = true;
    if (sectionIdx >= 0)
      data.sectionDirty[sectionIdx] = true;
    else
      data.sectionDirty.fill(true);
    
    if (data.needsRebuild)
      dirtyChunks_.push_back(pos);
  }
}

void VoxelRenderer::rebuildChunkSection(const ChunkPos& chunkPos, int sectionIdx)
{
  auto it = chunkRenderData_.find(chunkPos);
  if (it != chunkRenderData_.end())
  {
    it->second.sectionDirty[sectionIdx] = true;
    it->second.needsRebuild = true;
    dirtyChunks_.push_back(chunkPos);
  }
}

void VoxelRenderer::rebuildSectionMesh(const ChunkPos& chunkPos, int sectionIdx)
{
  Chunk* chunk = world_->getChunk(chunkPos);
  if (!chunk || !chunk->isGenerated)
    return;
  
  ChunkRenderData& renderData = chunkRenderData_[chunkPos];
  ChunkSectionMesh& mesh = renderData.sections[sectionIdx];
  mesh.clear();
  
  const ChunkSection& section = chunk->sections[sectionIdx];
  if (section.isSectionEmpty())
    return;
  
  int baseY = sectionIdx * 16;
  
  for (int y = 0; y < 16; ++y)
  {
    for (int z = 0; z < 16; ++z)
    {
      for (int x = 0; x < 16; ++x)
      {
        VoxelType voxelType = section.get(x, y, z);
        if (voxelType == VoxelType::AIR)
          continue;
        
        int worldX = chunkPos.x * 32 + x;
        int worldY = baseY + y;
        int worldZ = chunkPos.z * 32 + z;
        
        for (int face = 0; face < 6; ++face)
        {
          if (isFaceVisible(*chunk, worldX + FACE_DIRS[face][0], 
                           worldY + FACE_DIRS[face][1], 
                           worldZ + FACE_DIRS[face][2], face))
          {
            addFace(mesh, Point3(x, y, z), face, voxelType, x, y, z);
          }
        }
      }
    }
  }
  
  mesh.hasOpaque = !mesh.vertices.empty();
  mesh.hasTransparent = !mesh.transparentVertices.empty();
}

bool VoxelRenderer::isFaceVisible(const Chunk& chunk, int x, int y, int z, int faceDir) const
{
  VoxelType neighbor = chunk.getVoxel(x, y, z);
  if (neighbor == VoxelType::AIR)
    return true;
  
  const VoxelBlockType& bt = getBlockType(neighbor);
  return !bt.isOpaque;
}

void VoxelRenderer::addFace(ChunkSectionMesh& mesh, const Point3& localPos, int faceDir, VoxelType voxelType, int lx, int ly, int lz)
{
  const VoxelBlockType& bt = getBlockType(voxelType);
  if (!bt.isSolid)
    return;
  
  int vertOffset = mesh.vertices.size();
  
  for (int i = 0; i < 4; ++i)
  {
    VoxelVertex v;
    v.position = Point3(
      localPos.x + FACE_VERTS[faceDir][i][0],
      localPos.y + FACE_VERTS[faceDir][i][1],
      localPos.z + FACE_VERTS[faceDir][i][2]
    );
    
    int nx = FACE_DIRS[faceDir][0];
    int ny = FACE_DIRS[faceDir][1];
    int nz = FACE_DIRS[faceDir][2];
    v.normal = packNormal(nx, ny, nz);
    
    uint16_t texIdx = static_cast<uint16_t>(voxelType);
    v.texCoord = packTexCoord(texIdx, faceDir);
    
    v.voxelType = static_cast<uint8_t>(voxelType);
    v.ao = 255;
    v.padding = 0;
    
    mesh.vertices.push_back(v);
  }
  
  for (int i = 0; i < 6; ++i)
  {
    mesh.indices.push_back(mesh.vertices.size() - 4 + FACE_INDICES[i]);
  }
}

uint32_t VoxelRenderer::packNormal(int nx, int ny, int nz) const
{
  float fx = nx / (abs(nx) + abs(ny) + abs(nz) + 0.001f);
  float fy = ny / (abs(nx) + abs(ny) + abs(nz) + 0.001f);
  uint32_t u = static_cast<uint32_t>((fx + 1.0f) * 127.5f) & 0xFF;
  uint32_t v = static_cast<uint32_t>((fy + 1.0f) * 127.5f) & 0xFF;
  return (u << 8) | v;
}

uint32_t VoxelRenderer::packTexCoord(uint16_t u, uint16_t v) const
{
  return (static_cast<uint32_t>(u) << 16) | v;
}

void VoxelRenderer::actScene()
{
  Point3 camPos(0, 0, 0);
  updateVisibleChunks(camPos, 256.0f);
  
  tick(1.0f / 60.0f);
}

void VoxelRenderer::drawScene()
{
}

void VoxelRenderer::beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
{
}

void VoxelRenderer::sceneSelected(::DagorGameScene* prev_scene)
{
  debug("VoxelRenderer selected");
}

void VoxelRenderer::sceneDeselected(::DagorGameScene* new_scene)
{
  debug("VoxelRenderer deselected");
}

bool VoxelRenderer::isChunkVisible(const ChunkPos& chunkPos) const
{
  Point3 camPos(0, 0, 0);
  float dx = (chunkPos.x * 32.0f + 16.0f) - camPos.x;
  float dz = (chunkPos.z * 32.0f + 16.0f) - camPos.z;
  float distSq = dx * dx + dz * dz;
  return distSq < (256.0f * 256.0f);
}

void VoxelRenderer::onChunkGenerated(const ChunkPos& pos)
{
  markChunkModified(pos, -1);
}

void VoxelRenderer::onVoxelChanged(const ChunkPos& chunkPos, int sectionIdx, int x, int y, int z, VoxelType newType)
{
  markChunkModified(chunkPos, sectionIdx);
}

void VoxelRenderer::cleanupBuffers()
{
  chunkRenderData_.clear();
  dirtyChunks_.clear();
}

} // namespace drivinggame