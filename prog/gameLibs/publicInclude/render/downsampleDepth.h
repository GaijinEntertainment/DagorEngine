//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>

class TextureIDPair;

namespace downsample_depth
{
enum
{
  HYPER_Z = 0,
  LINEAR_Z = 1,
  RAW_Z = 2,
  HYPER_Z_MSAA = 3
};

void close();
void init(const char *ps_name, const char *wave_cs_name = nullptr, const char *cs_name = nullptr);

// NOTE: If checkerboard_depth is used, `far_normals` match checkerboard_depth.
// NOTE: external_barriers flag is used for resource management via
// framegraph and should only be used if you know what you are doing
// resource barriers wise.

void downsample(const TextureIDPair &from_depth, int w, int h, const TextureIDPair &far_depth, const TextureIDPair *close_depth,
  const TextureIDPair *far_normals, const TextureIDPair *motion_vectors = nullptr, const TextureIDPair *checkerboard_depth = nullptr,
  bool external_barriers = false);

// ResPtr version
void downsample(const ManagedTex &from_depth, int w, int h, const ManagedTex &far_depth, const ManagedTex &close_depth,
  const ManagedTex &far_normals, const ManagedTex &motion_vectors, const ManagedTex &checkerboard_depth,
  bool external_barriers = false);

// Downsample depth in pixel shader using chain of FS draw passes or in compute shader if available.
void downsamplePS(const TextureIDPair &from_depth, int w, int h, const TextureIDPair *far_depth, const TextureIDPair *close_depth,
  const TextureIDPair *far_normals, const TextureIDPair *motion_vectors = nullptr, const TextureIDPair *checkerboard_depth = nullptr,
  bool external_barriers = false, const Point4 &source_uv_transform = Point4(1, 1, 0, 0));

void downsamplePS(const TextureIDPair &from_depth, int w, int h, const TextureIDPair *far_depth_mips, int far_depth_mip_count,
  const TextureIDPair *close_depth, const TextureIDPair *far_normals, const TextureIDPair *motion_vectors = nullptr,
  const TextureIDPair *checkerboard_depth = nullptr, bool external_barriers = false,
  const Point4 &source_uv_transform = Point4(1, 1, 0, 0));

void generate_depth_mips(const TextureIDPair &tex);

void generate_depth_mips(const TextureIDPair *depth_mips, int depth_mip_count);

// Downsample depth using single compute shader pass
void downsampleWithWaveIntin(const TextureIDPair &from_depth, int w, int h, const TextureIDPair &far_depth,
  const TextureIDPair *close_depth, const TextureIDPair *far_normals, const TextureIDPair *motion_vectors = nullptr,
  const TextureIDPair *checkerboard_depth = nullptr, bool external_barriers = false);
}; // namespace downsample_depth
