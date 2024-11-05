//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint3.h>
#include <3d/dag_resPtr.h>

class ComputeShaderElement;
class TextureIDPair;
class TextureIDHolder;
class TextureIDHolderWithVar;

/* Compute shader and pixel shader fallback
Key idea is using compute or pixel shader to render values to voltex.
- Compute-vbariant must have name shader_prefix##_cs, pixel-variant
  shader_prefix##_ps.
- Pixel-variant should use macros WRITE_TO_VOLTEX_TC.
- Voltex must be created with VoltexRenderer::initVoltex
- Voltex binds to register(u0) for compute shader and to render target
  for pixel shader.
*/
class VoltexRenderer
{
public:
  void init(const char *compute_shader, const char *pixel_shader);
  // Use this if not all threads write a voxel
  void init(const char *compute_shader, const char *pixel_shader, int voxel_per_group_x, int voxel_per_group_y, int voxel_per_group_z);

  static bool is_compute_supported();
  bool isComputeLoaded() const;

  void render(const ManagedTex &voltex, int mip_level = 0, IPoint3 shape = IPoint3(-1, -1, -1), IPoint3 offset = IPoint3(0, 0, 0));

  template <typename TexHolderType>
  bool initVoltex(TexHolderType &voltex, int w, int h, int d, int flags, int levels, const char *texname)
  {
    voltex.close();
    voltex = dag::create_voltex(w, h, d, flags | (shaderCs ? TEXCF_UNORDERED : TEXCF_RTARGET), levels, texname);
    return (bool)voltex;
  }

private:
  eastl::unique_ptr<ComputeShaderElement> shaderCs;
  PostFxRenderer shaderPs;
  int voxelPerGroupX = 0;
  int voxelPerGroupY = 0;
  int voxelPerGroupZ = 0;
};
