// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

#include "buffBindPoints.h"

namespace drv3d_metal
{
enum
{
  BindlessTypeSampler = 1 << 0,
  BindlessTypeBuffer = 1 << 1,
  BindlessTypeTexture2D = 1 << 2,
  BindlessTypeTextureCube = 1 << 3,
  BindlessTypeTexture2DArray = 1 << 4,
};

class Shader
{
public:
  static const int g_max_textures_in_shader = 32;

  struct VA
  {
    int vec;
    int vsdr;
    int reg;
  };

  struct Buffers
  {
    int slot = -1;
    int remapped_slot = -1;
  };

  NSString *src;
  char entry[96];

  EncodedBufferRemap bufRemap[BUFFER_POINT_COUNT];

  Buffers buffers[BUFFER_POINT_COUNT];
  int num_buffers = 0;
  int immediate_slot = -1;
  uint64_t buffer_mask = 0;

  EncodedBufferRemap bindless_buffers[BUFFER_POINT_COUNT];
  int num_bindless_buffers = 0;
  uint64_t bindless_mask = 0;
  uint32_t bindless_type_mask = 0;

  int num_reg;
  int shd_type;

  int tgrsz_x;
  int tgrsz_y;
  int tgrsz_z;

  int num_va;
  VA va[16];

  int num_tex;
  EncodedMetalImageType tex_type[g_max_textures_in_shader];
  uint8_t tex_binding[g_max_textures_in_shader];
  uint8_t tex_remap[g_max_textures_in_shader];
  uint64_t texture_mask = 0;

  int num_samplers = 0;
  uint8_t sampler_binding[g_max_textures_in_shader];
  uint8_t sampler_remap[g_max_textures_in_shader];
  uint64_t sampler_mask = 0;

  int accelerationStructureCount = 0;
  // acceleration structures share bind points with buffers
  uint8_t acceleration_structure_remap[BUFFER_POINT_COUNT];
  uint8_t acceleration_structure_binding[BUFFER_POINT_COUNT];

  uint32_t output_mask = ~0u;
  uint64_t shader_hash = 0;

  // because of how everything is done those are embedded in vs
  Shader *mesh_shader = nullptr;
  Shader *amplification_shader = nullptr;

  Shader();
  bool compileShader(const char *source, bool async);
  void release();

private:
  bool setup(const char *data, int data_size, bool async);
  bool loadFromBinary(const char *source, bool async);
  bool loadFromSource(const char *source, bool async);
};
} // namespace drv3d_metal
