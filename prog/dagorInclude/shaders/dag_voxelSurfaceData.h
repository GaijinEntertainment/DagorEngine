//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_DObject.h>
#include <shaders/dag_atlasBlockManager.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <util/dag_stdint.h>
#include <EASTL/array.h>


class VoxelSurfaceData : public DObject
{
public:
  decl_class_name(VoxelSurfaceData);
  DAG_DECLARE_NEW(midmem)

  static VoxelSurfaceData *load(IGenLoad &crd, int size);
  static VoxelSurfaceData *getById(uint32_t id);

  static void onAfterD3dReset();

  ~VoxelSurfaceData();

  static constexpr uint32_t INVALID_ID = ~0u;
  uint32_t getId() const { return id; }

  // On failure, returns number of rgba and norm blocks needed.
  eastl::array<uint32_t, 2> allocateAtlas();

  // Returns number of rgba and norm blocks released.
  eastl::array<uint32_t, 2> releaseAtlas();

  bool isLoadedToAtlas() const { return loadedToTex; }
  bool loadToAtlas();

  void setToShader() const;
  static void setToShader(uint32_t id);

protected:
  dag::Span<char> compressed;
  uint32_t id = INVALID_ID;
  AtlasBlockManager::ChunkLocation rgbaLoc, normLoc;
  bool loadedToTex = false;

  uint32_t numRgbaBlocks = 0, numNormBlocks = 0;
  uint32_t normCompressedOffset = 0;

  VoxelSurfaceData();
  static constexpr int dumpStartOfs() { return offsetof(VoxelSurfaceData, numRgbaBlocks); }
  void *dumpStartPtr() { return &numRgbaBlocks; }
  void *dataStartPtr() { return &normCompressedOffset + 1; }

  friend class ShaderResUnitedVdata<RenderableInstanceLodsResource>;
};
