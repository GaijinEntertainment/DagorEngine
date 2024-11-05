// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_computeShaders.h>
#include <render/computeShaderFallback/voltexRenderer.h>

#include <3d/dag_resPtr.h>

class GenNoise
{
public:
  void close();
  void init();

  void reset();

  // Returns true, if noise textures have just been loaded
  bool isPrepareRequired() const;

  // Returns true where noise loaded
  bool isReady() const { return (state != EMPTY && state != LOADING); }

  /* Returns true if at least one of noise textures was updated */
  bool render();
  void setVars();

  SharedTexHolder resCloud1, resCloud2;

private:
  /* Return true if curl was rendered */
  bool renderCurl();
  void generateMips3d(const ManagedTex &tex);

  bool loadVolmap(const char *resname, int resolution, SharedTexHolder &tex);
  static int get_blocked_mips(const ManagedTex &tex);

  void closeCompressor();
  void initCompressor();
  void initCompressorBuffer();

  void compressBC4(const ManagedTex &tex, const ManagedTex &dest);

  int shapeRes = 128, detailRes = 64;
  int shapeResShift = 1, detailResShift = 2;
  static constexpr int MAX_CLOUD_DETAIL_MIPS = 4;
  enum GenNoiseState
  {
    EMPTY,
    LOADING,
    LOADED,
    RENDERED
  };
  GenNoiseState state = EMPTY;
  bool curlRendered = false;
  bool compressSupported = false;

  eastl::unique_ptr<ComputeShaderElement> genCurl2d, genCurl3d;
  PostFxRenderer genCurl2dPs;
  VoltexRenderer genCloudShape, genCloudDetail, genMips3d;

  UniqueTexHolder cloud1, cloud2;
  UniqueTexHolder cloudsCurl2d;
  VoltexRenderer compress3D;

  UniqueTex cloud1Compressed, cloud2Compressed;
  UniqueTex buffer;
};