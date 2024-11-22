// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_linearHeapAllocator.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>


struct FrameBoundaryElem
{
  using RegionId = LinearHeapAllocatorNull::RegionId;

  FrameBoundaryElem(RegionId region_id, TEXTUREID texture_id, IPoint2 frame_dim) :
    regionId(region_id), textureId(texture_id), frameDim(frame_dim)
  {}

  TEXTUREID textureId;
  IPoint2 frameDim;
  RegionId regionId;
};


class FrameBoundaryBufferManager
{
  friend FrameBoundaryElem;

public:
  using Region = LinearHeapAllocatorNull::Region;
  using RegionId = LinearHeapAllocatorNull::RegionId;

  FrameBoundaryBufferManager();
  ~FrameBoundaryBufferManager();

  FrameBoundaryBufferManager(FrameBoundaryBufferManager &&);

  FrameBoundaryBufferManager(const FrameBoundaryBufferManager &) = delete;
  FrameBoundaryBufferManager &operator=(const FrameBoundaryBufferManager &) = delete;
  FrameBoundaryBufferManager &operator=(FrameBoundaryBufferManager &&) = delete;


  int acquireFrameBoundary(TEXTUREID texture_id, IPoint2 frame_dim);

  void afterDeviceReset();

  void init(bool use_sbuffer, bool approximate_fill);
  void update(unsigned int current_frame);
  void prepareRender();

  void drawDebugWindow();

protected:
  static constexpr int MIN_BUFFER_SIZE = 64;
  static constexpr int DEBUG_TEX_SIZE = 1024;


  FrameBoundaryElem createElem(TEXTUREID texture_id, IPoint2 frame_dim);
  void resizeIncrement(size_t min_size_increment);
  void updateFrameElems(unsigned int currect_frame);
  void updateDebugTexture();
  void resetFrameBoundaryResult();

  bool isSupported = false;
  bool approximateFill = false;
  unsigned int lastUpdatedFrame = -1;

  LinearHeapAllocatorNull frameBoundaryAllocator;
  dag::Vector<FrameBoundaryElem> frameBoundaryElemArr;
  dag::Vector<FrameBoundaryElem> dirtyElemArr;

  int frameBoundaryBufferInitialSize = 0;
  int bufferElemCnt = 0;

  UniqueBuf frameBoundaryBuffer;
  UniqueBuf frameBoundaryBufferTmp;
  UniqueTexHolder debugTexture;

  Ptr<ComputeShaderElement> fillBoundaryLegacyCs;
  Ptr<ComputeShaderElement> fillBoundaryApproxCs;
  Ptr<ComputeShaderElement> fillBoundaryOptCs;
  Ptr<ComputeShaderElement> fillBoundaryOptStartCs;
  Ptr<ComputeShaderElement> fillBoundaryOptEndCs;
  Ptr<ComputeShaderElement> clearBoundaryCs;
  Ptr<ComputeShaderElement> updateDebugTexCs;
};
