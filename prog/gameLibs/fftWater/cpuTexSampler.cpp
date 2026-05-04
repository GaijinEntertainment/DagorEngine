// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cpuTexSampler.h"
#include <vecmath/dag_vecMath.h>
#include "fftWater/chop_water_gen_tiling.hlsli"

float CPUTextureSampler2D::SampleLinearWrapTiled(const Point2 &uv, float overlap) const
{
  return sample_linear_wrap_tiled_impl(uv, overlap, m_data, m_res_x, m_mask_x, m_mask_y, m_res_x_f, m_res_y_f);
}

#ifdef CHOP_WATER_USE_SSE
void CPUTextureSampler2D::SampleLinearWrapTiled4(float u0, float u1, float u2, float u3, float v, float overlap, float out[4]) const
{
  sample_linear_wrap_tiled4_impl(u0, u1, u2, u3, v, overlap, out, m_data, m_res_x, m_mask_x, m_mask_y, m_res_x_f, m_res_y_f);
}
#endif