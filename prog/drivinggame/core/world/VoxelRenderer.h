#pragma once

#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include "VoxelWorld.h"
#include <workCycle/dag_gameScene.h>
#include <3d/dag_resMgr.h>
#include <drv/hid/dag_hiDecl.h>
#include <drv/hid/dag_hiKeybData.h>
#include <drv/hid/dag_hiPointingData.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>

namespace drivinggame
{

struct VoxelVertex
{
  Point3 position;
  uint32_t normal;      // packed normal (octahedral encoding)
  uint32_t texCoord;    // packed tex coords
  uint8_t  voxelType;   // voxel type for shader
  uint8_t  ao;          // ambient occlusion
  uint16_t padding;
};

struct ChunkSectionMesh
{
  eastl::vector<VoxelVertex> vertices;
  eastl::vector<uint32_t> indices;
  eastl::vector<VoxelVertex> transparentVertices;
  eastl::vector<uint32_t> transparentIndices;
  bool hasOpaque = false;
  bool hasTransparent = false;
  
  void clear()
  {
    vertices.clear();
    indices.clear();
    transparentVertices.clear();
    transparentIndices.clear();
    hasOpaque = false;
    hasTransparent = false;
  }
};

struct VoxelBlockType
{
  VoxelType type;
  uint16_t textureTop;
  uint16_t textureBottom;
  uint16_t textureSide;
  bool isOpaque;
  bool isSolid;
  float opacity;
};

class VoxelRenderer : public ::DagorGameScene
{
public:
  VoxelRenderer(VoxelWorld* world);
  ~VoxelRenderer();
  
  bool init();
  void shutdown();
  
  void tick(float dt);
  void updateVisibleChunks(const Point3& cameraPos, float viewDist);
  
  // DagorGameScene interface
  void actScene() override;
  void drawScene() override;
  void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec) override;
  void sceneSelected(::DagorGameScene* prev_scene) override;
  void sceneDeselected(::DagorGameScene* new_scene) override;
  bool canPresentAndReset() override { return true; }
  
  // Mesh management
  void rebuildChunkSection(const ChunkPos& chunkPos, int sectionIdx);
  void markChunkModified(const ChunkPos& pos, int sectionIdx = -1);
  
  // Voxel type lookup
  void initBlockTypes();
  const VoxelBlockType& getBlockType(VoxelType type) const;
  
  // Public interface for external systems
  void onChunkGenerated(const ChunkPos& pos);
  void onVoxelChanged(const ChunkPos& chunkPos, int sectionIdx, int x, int y, int z, VoxelType newType);

private:
  VoxelWorld* world_ = nullptr;
  
  struct ChunkRenderData
  {
    ChunkPos pos;
    eastl::array<ChunkSectionMesh, 32> sections; // CHUNK_SECTION_COUNT
    bool needsRebuild = false;
    eastl::array<bool, 32> sectionDirty;
    uint64_t lastRebuildTick = 0;
  };
  
  eastl::unordered_map<ChunkPos, ChunkRenderData, ChunkHash> chunkRenderData_;
  eastl::vector<ChunkPos> dirtyChunks_;
  eastl::array<VoxelBlockType, 256> blockTypes_;
  bool blockTypesInitialized_ = false;
  
  // GPU resources
  D3DRESID voxelAtlas_ = BAD_D3DRESID;
  int voxelAtlasVarId = -1;
  int voxelGlobalsVarId = -1;
  int chunkTransformVarId = -1;
  
  // GPU buffers for voxel rendering
  D3DRESID voxelVertexBuffer_ = BAD_D3DRESID;
  D3DRESID voxelIndexBuffer_ = BAD_D3DRESID;
  D3DRESID voxelConstantsBuffer_ = BAD_D3DRESID;
  
  // Mesh rebuilding
  void rebuildSectionMesh(const ChunkPos& chunkPos, int sectionIdx);
  bool isFaceVisible(const Chunk& chunk, int x, int y, int z, int faceDir) const;
  void addFace(ChunkSectionMesh& mesh, const Point3& localPos, int faceDir, VoxelType voxelType, int lx, int ly, int lz);
  uint32_t packNormal(int nx, int ny, int nz) const;
  uint32_t packTexCoord(uint16_t u, uint16_t v) const;
  
  // GPU resource management
  void createVoxelAtlas();
  void uploadSectionMesh(const ChunkPos& chunkPos, int sectionIdx);
  void uploadMeshes();
  void unloadChunkBuffers(const ChunkPos& pos);
  void cleanupBuffers();
  
  // Shader setup
  void setupShaders();
  void cleanupShaders();
  
  // GPU buffer management
  void createGPUBuffers();
  void cleanupGPUBuffers();
  void uploadMeshData();
  
  // daFrameGraph integration
  void setupFrameGraphNodes();
  
  // Visibility
  bool isChunkVisible(const ChunkPos& chunkPos) const;
};

} // namespace drivinggame