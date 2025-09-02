//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <EASTL/array.h>
#include <EASTL/bitset.h>
#include <EASTL/unique_ptr.h>
#include <render/lightCube.h>
#include <shaders/dag_postFxRenderer.h>

class BcCompressor;
class ComputeShaderElement;
class LightProbe;

class LightProbeSpecularCubesContainer
{
public:
  static constexpr int INDOOR_PROBES = 32;

private:
  static constexpr int CAPACITY = INDOOR_PROBES;
  eastl::bitset<CAPACITY> usedIndices;
  UniqueTexHolder cubesArray;
  UniqueTex lastMipsBc6HTarget;

  struct LightProbeDestructor
  {
    void operator()(light_probe::Cube *ptr) { ptr ? light_probe::destroy(ptr) : (void)0; }
  };
  eastl::unique_ptr<light_probe::Cube, LightProbeDestructor> rtCube;
  eastl::unique_ptr<BcCompressor> compressor;
  d3d::SamplerHandle sampler;
  eastl::unique_ptr<ComputeShaderElement> bc6hHighQualityCompressor;

  int cubeSize = 0;
  int sizeInCompressedBlocks = 0;
  int specularMips = 0;
  int compressedMips = 0;

  eastl::array<LightProbe *, CAPACITY + 1> updateQueue = {0};
  uint32_t updateQueueBegin = 0, updateQueueEnd = 0;
  uint32_t renderedCubeFaces = 0;
  uint32_t processedSpecularFaces = 0;
  uint32_t compressedFaces = 0;

  bool compressionAvailable = false;

  void updateMips(int face_start, int face_count, int from_mip, int mips_count)
  {
    light_probe::update(rtCube.get(), nullptr, face_start, face_count, from_mip, mips_count);
  }

  void compressMips(int cube_index, int face_start, int face_count, int from_mip, int mips_count);

  bool calcDiffuse(float gamma, float brightness, bool force_recalc)
  {
    return light_probe::calcDiffuse(rtCube.get(), nullptr, gamma, brightness, force_recalc);
  }

  void onProbeDone(LightProbe *probe);
  static constexpr int FACES_COUNT = 6;

public:
  LightProbeSpecularCubesContainer();
  ~LightProbeSpecularCubesContainer();

  void init(int cube_size, uint32_t texture_format);
  int allocate();
  void deallocate(int index);

  const ManagedTex *getManagedTex() { return light_probe::getManagedTex(rtCube.get()); }
  using CubeUpdater = eastl::function<void(const ManagedTex *, const Point3 &position, int face_number)>;
  void update(const CubeUpdater &cube_updater, const Point3 &origin);
  void addProbeToUpdate(LightProbe *probe);
  void invalidateProbe(LightProbe *probe);
  bool isCubeUpdateRequired() const { return updateQueueBegin != updateQueueEnd && renderedCubeFaces < FACES_COUNT; }
};
