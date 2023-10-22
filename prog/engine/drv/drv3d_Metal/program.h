
#pragma once

#include "shader.h"
#include "vdecl.h"

namespace drv3d_metal
{
class Program
{
public:
  enum VBStep
  {
    VBSTEP_CONSTANT = 0,
    VBSTEP_PERVERTEX,
    VBSTEP_PERINSTANCE
  };

  static constexpr int MAX_SIMRT = 8;

  // this is binary serialized. if something is changed need to bump pipeline cache version
  struct RenderState
  {
    static constexpr unsigned RasterizerStateSizeInBytes = 2 * shaders::RenderState::NumIndependentBlendParameters +
                                                           2 * shaders::RenderState::NumIndependentBlendParameters + sizeof(uint32_t) +
                                                           sizeof(uint8_t) * 4;
    union RasterizerState
    {
      // bitcounts should match what's in dag_renderStates.h
      struct
      {
        struct
        {
          uint8_t ablend : 1;
          uint8_t ablendOp : 3;
          uint8_t ablendScr : 4;
          uint8_t ablendDst : 4;
          uint8_t sepblend : 1;
          uint8_t rgbblendOp : 3;
          uint8_t rgbblendScr : 4;
          uint8_t rgbblendDst : 4;
        } blend[shaders::RenderState::NumIndependentBlendParameters];

        uint32_t writeMask;
        uint8_t a2c;
        uint8_t pad[2];
      };
      uint8_t state[RasterizerStateSizeInBytes];
    };

    RasterizerState raster_state;

    static_assert(sizeof(RasterizerState) == RasterizerStateSizeInBytes);

    MTLPixelFormat pixelFormat[MAX_SIMRT];
    MTLPixelFormat depthFormat;
    MTLPixelFormat stencilFormat;

    uint8_t vbuffer_stride[BUFFER_POINT_COUNT];
    uint8_t sample_count = 1;
    bool is_volume = false;

    RenderState()
    {
      for (int i = 0; i < MAX_SIMRT; i++)
      {
        pixelFormat[0] = MTLPixelFormatInvalid;
      }

      for (int i = 0; i < BUFFER_POINT_COUNT; i++)
      {
        vbuffer_stride[i] = 0;
      }

      depthFormat = MTLPixelFormatInvalid;
      stencilFormat = MTLPixelFormatInvalid;
    }

    friend inline bool operator==(const RenderState &left, const RenderState &right)
    {
      if (!memcmp(left.raster_state.state, right.raster_state.state, RasterizerStateSizeInBytes) &&
          left.depthFormat == right.depthFormat && left.stencilFormat == right.stencilFormat &&
          left.sample_count == right.sample_count && left.is_volume == right.is_volume)
      {
        for (int i = 0; i < MAX_SIMRT; i++)
        {
          if (left.pixelFormat[i] != right.pixelFormat[i])
          {
            return false;
          }
        }

        for (int i = 0; i < BUFFER_POINT_COUNT; i++)
        {
          if (left.vbuffer_stride[i] != right.vbuffer_stride[i])
          {
            return false;
          }
        }

        return true;
      }

      return false;
    }
  };

  VDecl *vdecl = nullptr;

  Shader *vshader;
  Shader *pshader;
  Shader *cshader;

  id<MTLComputePipelineState> csPipeline;

  static MTLRenderPipelineDescriptor *pipelineStateDescriptor;

  Program(Shader *vshdr, Shader *pshdr, VDecl *decl);
  Program(Shader *cshdr);

  void release();
};
} // namespace drv3d_metal
