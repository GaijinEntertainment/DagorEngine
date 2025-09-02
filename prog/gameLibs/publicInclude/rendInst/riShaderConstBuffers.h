//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>
#include <EASTL/array.h>
#include <dag/dag_relocatable.h>


namespace rendinst::render
{
// RiShaderConstBuffers can only be used while "render instancing" is on.
// Start before using this class and end after you are done.
void startRenderInstancing();
void endRenderInstancing();

inline constexpr uint32_t PER_DRAW_VECS_COUNT = 14;

using SizePerRotationArr = eastl::array<Point2, 8>;

struct RiShaderConstBuffers
{
  struct Consts
  {
    vec4f bbox[3];
    vec4f rendinst_opacity;   // rendinst_opacity_const_no
    vec4f bounding_sphere;    // bounding_sphere_const_no
    vec4f ao_mul__inv_height; // bounding_sphere_const_no+1 = rendinst_ao_mul
    union
    {
      struct
      {
        vec4f radius; // PLOD poisson radius to determine quad size
        vec4f unused[3];
      } plod_data;
      vec4f shadow_impostor_sizes[4]; // size of shadows corresponding to rotations
    } optional_data;
    vec4f rendinst_bbox__cross_dissolve_range; // bounding box is used for vegetation interactions AND cross dissolve range
    vec4f color_from;                          // first edge random color
    vec4f color_to;                            // second edge random color
    vec4f rendinst_interaction_params;         // for shader interactions with other objects
  };

  Consts consts;

  RiShaderConstBuffers();

  // ofs and stride in typed vec4 (not in bytes)
  void setInstancing(uint32_t ofs, uint32_t stride, uint32_t flags, uint32_t impostor_data_offset);
  void setBBoxZero();
  void setBBoxNoChange();
  void setBBox(vec4f p[2]);
  void setOpacity(float p0, float p1, float p2 = 0.f, float p3 = 0.f);
  void setBoundingSphere(float p0, float p1, float sphereRadius, float cylinderRadius, float sphereCenterY);
  void setBoundingBox(const vec4f &bbox);
  void setCrossDissolveRange(float crossDissolveRange);
  void setShadowImpostorSizes(const SizePerRotationArr &sizes);
  void setImpostorLocalView(const Point3 &view_x, const Point3 &view_y);
  void setRadiusFade(float radius, float drown_scale);
  void setInteractionParams(float softness, float rendinst_height, float center_x, float center_z);
  void flushPerDraw() const;
  void setRandomColors(const E3DCOLOR colors[2]);
  void setPLODRadius(float radius);

  static void setInstancePositions(const float *data, int vec4_count);
};

inline void RiShaderConstBuffers::setRandomColors(const E3DCOLOR colors[2])
{
  vec4f i255 = v_splats(1.f / 255.f);
#if _TARGET_SIMD_SSE >= 4 // Note: _mm_cvtepu8_epi32 is SSE4.1
  vec4f c0 = v_mul(v_cvti_vec4f(_mm_cvtepu8_epi32(v_seti_x(colors[0].u))), i255);
  vec4f c1 = v_mul(v_cvti_vec4f(_mm_cvtepu8_epi32(v_seti_x(colors[1].u))), i255);
  consts.color_from = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 0, 1, 2)); // BGRA -> RGBA
  consts.color_to = _mm_shuffle_ps(c1, c1, _MM_SHUFFLE(3, 0, 1, 2));
#else
  consts.color_from = v_mul(v_cvti_vec4f(v_make_vec4i(colors[0].r, colors[0].g, colors[0].b, colors[0].a)), i255);
  consts.color_to = v_mul(v_cvti_vec4f(v_make_vec4i(colors[1].r, colors[1].g, colors[1].b, colors[1].a)), i255);
#endif
}


} // namespace rendinst::render

DAG_DECLARE_RELOCATABLE(rendinst::render::RiShaderConstBuffers);
