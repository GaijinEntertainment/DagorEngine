//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_Point3.h>


namespace rendinstgenrender
{

// RiShaderConstBuffers can only be used while "render instancing" is on.
// Start before using this class and end after you are done.
void startRenderInstancing();
void endRenderInstancing();

inline constexpr uint32_t PER_DRAW_VECS_COUNT = 14;


struct RiShaderConstBuffers
{
  // shader variables
  // box (x3)
  // float4 rendinst_opacity   - rendinst_opacity_const_no
  // float4 bounding_sphere    - bounding_sphere_const_no
  // float2 ao_mul__inv_height - bounding_sphere_const_no+1 = rendinst_ao_mul
  // float4 deltas0 (opt) - imp_size deltas for slices 0,1
  // float4 deltas1 (opt) - imp_size deltas for slices 2,3
  // float4 deltas2 (opt) - imp_size deltas for slices 4,5
  // float4 deltas3 (opt) - imp_size deltas for slices 6,7
  // float4 rendinst_bbox; - bounding box is used for vegetation interactions
  // float4 color_from - first edge random color
  // float4 color_to - second edge random color
  // float4 rendinst_interaction_params - for shader interactions with other objects
  union
  {
    struct
    {
      vec4f bbox[3];
      // vec4i inst;
      vec4f perDraw[11];
    };
    vec4f consts[PER_DRAW_VECS_COUNT];
  };


  RiShaderConstBuffers();

  // ofs and stride in typed vec4 (not in bytes)
  void setInstancing(uint32_t ofs, uint32_t stride, uint32_t impostor_data_offset, bool global_per_draw_buffer = false);
  void setBBoxZero();
  void setBBoxNoChange();
  void setBBox(vec4f p[2]);
  void setOpacity(float p0, float p1, float p2 = 0.f, float p3 = 0.f);
  void setBoundingSphere(float p0, float p1, float sphereRadius, float cylinderRadius, float sphereCenterY);
  void setBoundingBox(const vec4f &bbox);
  void setImpostorMultiWidths(float widths[], float heights[]);
  void setImpostorLocalView(const Point3 &view_x, const Point3 &view_y);
  void setRadiusFade(float radius, float drown_scale);
  void setInteractionParams(float hardness, float rendinst_height, float center_x, float center_z);
  void flushPerDraw();
  void setRandomColors(const E3DCOLOR *colors);

  static void setInstancePositions(const float *data, int vec4_count);
};

} // namespace rendinstgenrender
