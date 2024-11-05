// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <gui/dag_stdGuiRender.h>
#include <math/dag_Matrix3.h>
#include <daRg/dag_uiShader.h>


namespace darg
{

namespace uishader
{

static TEXTUREID maskTexId = BAD_TEXTUREID;
static d3d::SamplerHandle maskSamplerId = d3d::INVALID_SAMPLER_HANDLE;
// mask matrix is applied to GuiVertex, i.e. GUI_POS_SCALE scaled screen coords
static Point3 maskMatrix0(0, 0, 0);
static Point3 maskMatrix1(0, 0, 0);


static void set_states(StdGuiRender::GuiContext &ctx) { ctx.set_mask_texture(maskTexId, maskSamplerId, maskMatrix0, maskMatrix1); }


void set_mask(StdGuiRender::GuiContext &ctx, TEXTUREID texture_id, d3d::SamplerHandle sampler_id, const Point3 &matrix_line_0,
  const Point3 &matrix_line_1, const Point2 &tc0, const Point2 &tc1)
{
  maskTexId = texture_id;
  maskSamplerId = sampler_id;

  Point3 matrixLine0 = matrix_line_0 * (tc1.x - tc0.x);
  Point3 matrixLine1 = matrix_line_1 * (tc1.y - tc0.y);
  matrixLine0.z += tc0.x;
  matrixLine1.z += tc0.y;

  maskMatrix0 = matrixLine0;
  maskMatrix1 = matrixLine1;

  set_states(ctx);
}


void set_mask(StdGuiRender::GuiContext &ctx, TEXTUREID texture_id, d3d::SamplerHandle sampler_id, const Point2 &center_pos,
  float angle, const Point2 &scale, const Point2 &tc0, const Point2 &tc1)
{
  const GuiVertexTransform &itm = ctx.getVertexTransformInverse();
  Matrix3 vtm;
  vtm.setcol(0, itm.vtm[0][0], itm.vtm[1][0], 0.f);
  vtm.setcol(1, itm.vtm[0][1], itm.vtm[1][1], 0.f);
  vtm.setcol(2, itm.vtm[0][2], itm.vtm[1][2], 1.f);

  float sx = 2.0f / StdGuiRender::screen_width();
  float sy = 2.0f / StdGuiRender::screen_height();
  Matrix3 scaleAndOffset;
  scaleAndOffset.setcol(0, sx, 0.f, 0.f);
  scaleAndOffset.setcol(1, 0.f, sy, 0.f);
  scaleAndOffset.setcol(2, -center_pos.x * sx, -center_pos.y * sy, 1.f);

  Matrix3 texCenter;
  texCenter.setcol(0, 1.f / scale.x, 0.f, 0.f);
  texCenter.setcol(1, 0.f, 1.f / scale.y, 0.f);

  Matrix3 matrix = scaleAndOffset * vtm;
  if (fabsf(angle) < 1e-6)
  {
    texCenter.setcol(2, 0.5f, 0.5f, 1.f);
    matrix = texCenter * matrix;
  }
  else
  {
    texCenter.setcol(2, 0.f, 0.f, 1.f);

    Matrix3 rotation;
    rotation.setcol(0, cos(angle), sin(angle), 0.f);
    rotation.setcol(1, -sin(angle), cos(angle), 0.f);
    rotation.setcol(2, 0.5f, 0.5f, 1.f);

    matrix = rotation * texCenter * matrix;
  }

  set_mask(ctx, texture_id, sampler_id, Point3(matrix[0][0], matrix[1][0], matrix[2][0]),
    Point3(matrix[0][1], matrix[1][1], matrix[2][1]), tc0, tc1);
}


} // namespace uishader

} // namespace darg
