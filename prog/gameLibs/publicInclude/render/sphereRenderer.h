//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

class TMatrix;
class DynamicShaderHelper;

class SphereRenderer
{
public:
  SphereRenderer() = default;
  explicit SphereRenderer(int slices, int stacks);

  ~SphereRenderer() { close(); }

  void init(int slices, int stacks);
  void close();
  bool isInited() const;
  void createAndFillBuffers();

  float getMaxRadiusError(float radius) const;

  void render(DynamicShaderHelper &shader, TMatrix view_tm) const;

protected:
  UniqueBuf sphereVb;
  UniqueBuf sphereIb;

  uint32_t slices = 0, stacks = 0;
  uint32_t vertexCount = 0, faceCount = 0;
  float maxRelativeRadiusError = 0.f;
};