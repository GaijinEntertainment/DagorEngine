// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <fftWater/chop_water_defines.hlsli>

//=================================================================================================
class CPUTextureSampler2D
{
public:
  CPUTextureSampler2D(float *data, int32_t res_x, int32_t res_y) :
    m_data(data),
    m_res_x(res_x),
    m_res_y(res_y),
    m_mask_x(res_x - 1),
    m_mask_y(res_y - 1),
    m_res_x_f((float)res_x),
    m_res_y_f((float)res_y)
  {}

  void WritePixel(int32_t x, int32_t y, float val) { m_data[y * m_res_x + x] = val; }

  float ReadPixelWrap(int32_t x, int32_t y) const
  {
    x &= m_mask_x;
    y &= m_mask_y;
    return m_data[y * m_res_x + x];
  }

  float SampleLinearWrapTiled(const Point2 &uv, float overlap) const;
#ifdef CHOP_WATER_USE_SSE
  void SampleLinearWrapTiled4(float u0, float u1, float u2, float u3, float v, float overlap, float out[4]) const;
#endif
private:
  float *m_data = 0;
  int32_t m_res_x = 0;
  int32_t m_res_y = 0;
  int32_t m_mask_x = 0;
  int32_t m_mask_y = 0;
  float m_res_x_f = 0.0f;
  float m_res_y_f = 0.0f;
};
