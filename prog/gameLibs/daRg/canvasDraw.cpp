// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "canvasDraw.h"

#include <sqext.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include "dargDebugUtils.h"
#include "scriptUtil.h"
#include <daRg/dag_helpers.h>

namespace darg
{

int RenderCanvasContext::type_tag_dummy = 0;

SQUserPointer RenderCanvasContext::get_type_tag() { return &type_tag_dummy; }

template <size_t N>
using TmpPoint2Vector = dag::RelocatableFixedVector<Point2, N, true, framemem_allocator, uint32_t, false>;


inline void RenderCanvasContext::convertToScreenCoordinates(Point2 *points, int count) const
{
  if (lineAlign == 0.0f)
  {
    for (int i = 0; i < count; i++)
      points[i] = offset + Point2(points[i].x * scale.x, points[i].y * scale.y);
  }
  else
  {
    float d = -max(lineWidth, 0.0f) * lineAlign;
    Point2 dOffset = offset + Point2(d, d);
    Point2 dScale = scale - Point2(d * 0.02f, d * 0.02f);
    for (int i = 0; i < count; i++)
      points[i] = dOffset + Point2(points[i].x * dScale.x, points[i].y * dScale.y);
  }
}

void RenderCanvasContext::renderLine(const Sqrat::Array &cmd, const Point2 &line_indent) const
{
  bool isValidParams = cmd.Length() > 3 && cmd.Length() % 2 == 1;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_LINE", cmd, 0);
    return;
  }

  TmpPoint2Vector<8> points((cmd.Length() - 1) / 2);
  if (sq_ext_get_array_floats(cmd.GetObject(), 1, points.size() * 2, &points[0].x) != SQ_OK)
  {
    darg_assert_trace_var("cannot read params of VECTOR_LINE", cmd, 0);
    return;
  }

  convertToScreenCoordinates(&points[0], points.size());

  ctx->render_line_aa(points.data(), points.size(), /*is_closed*/ false, lineWidth, line_indent, color);
}


void RenderCanvasContext::renderLineDashed(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() == 7;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_LINE_DASHED. Expected 7 ([VECTOR_LINE_DASHED, p1.x, p1.y, p2.x, "
                          "p2.y, dash_length, space_length])",
      cmd, 0);
    return;
  }

  Tab<Point2> points(framemem_ptr());
  const int numPts = 2;

  points.resize(numPts);

  if (sq_ext_get_array_floats(cmd.GetObject(), 1, numPts * 2, &points[0].x) != SQ_OK)
  {
    darg_assert_trace_var("cannot read params of VECTOR_LINE_DASHED", cmd, 0);
    return;
  }

  convertToScreenCoordinates(&points[0], points.size());

  const float dash = cmd[cmd.Length() - 2].Cast<float>();
  const float space = cmd[cmd.Length() - 1].Cast<float>();

  ctx->render_dashed_line(points[0], points[1], dash, space, lineWidth, color);
}


void RenderCanvasContext::renderFillPoly(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() > 3 && cmd.Length() % 2 == 1;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_POLY", cmd, 0);
    return;
  }

  if (cmd.Length() < 7)
    return;

  Tab<Point2> points(framemem_ptr());
  points.resize((cmd.Length() - 1) / 2);

  if (sq_ext_get_array_floats(cmd.GetObject(), 1, points.size() * 2, &points[0].x) != SQ_OK)
  {
    darg_assert_trace_var("cannot read params of VECTOR_POLY", cmd, 0);
    return;
  }

  convertToScreenCoordinates(&points[0], points.size());

  ctx->render_poly(points, fillColor);
  ctx->render_line_aa(points, true, lineWidth, Point2(0, 0), color);
}


void RenderCanvasContext::renderFillInversePoly(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() > 3 && cmd.Length() % 2 == 1;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_INVERSE_POLY", cmd, 0);
    return;
  }

  if (cmd.Length() < 7)
    return;

  TmpPoint2Vector<64> points((cmd.Length() - 1) / 2);
  int count = points.size();

  if (sq_ext_get_array_floats(cmd.GetObject(), 1, count * 2, &points[0].x) != SQ_OK)
  {
    darg_assert_trace_var("cannot read params of VECTOR_INVERSE_POLY", cmd, 0);
    return;
  }

  const float bound = (1 << 22);

  convertToScreenCoordinates(&points[0], points.size());

  ctx->render_inverse_poly(make_span_const(points), fillColor, Point2(-bound, -bound), Point2(bound, bound));
  ctx->render_line_aa(points.data(), points.size(), true, lineWidth, Point2(0, 0), color);
}


void RenderCanvasContext::renderEllipse(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() == 4 || cmd.Length() == 5;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_ELLIPSE", cmd, 0);
    return;
  }

  Point2 pos = offset + Point2(cmd[1].Cast<float>() * scale.x, cmd[2].Cast<float>() * scale.y);

  float lineWidthLocal = max(lineWidth, 1.f);

  float d = lineWidthLocal * lineAlign;

  Point2 radius;
  radius.x = max(cmd[3].Cast<float>() * scale.x, 0.f) + d;
  radius.y = max(cmd.Length() == 4 ? radius.x : cmd[4].Cast<float>() * scale.y, 0.f) + d;

  ctx->render_ellipse_aa(pos, radius, lineWidthLocal, color, midColorUsed ? midColor : color, fillColor);
}


void RenderCanvasContext::renderSector(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() == 7;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_SECTOR (x, y, rad x, rad y, from angle, to angle)", cmd, 0);
    return;
  }

  Point2 pos = offset + Point2(cmd[1].Cast<float>() * scale.x, cmd[2].Cast<float>() * scale.y);

  float lineWidthLocal = max(lineWidth, 1.f);

  float d = lineWidthLocal * lineAlign;

  Point2 radius;
  radius.x = max(cmd[3].Cast<float>() * scale.x, 0.f) + d;
  radius.y = max(cmd[4].Cast<float>() * scale.y, 0.f) + d;

  Point2 angles;
  angles.x = cmd[5].Cast<float>() * (PI / 180.0f);
  angles.y = cmd[6].Cast<float>() * (PI / 180.0f);

  ctx->render_sector_aa(pos, radius, angles, lineWidthLocal, color, midColorUsed ? midColor : color, fillColor);
}


void RenderCanvasContext::renderRectangle(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() == 5;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_RECTANGLE", cmd, 0);
    return;
  }

  float d = lineWidth * lineAlign;

  Point2 lt = offset + Point2(cmd[1].Cast<float>() * scale.x - d, cmd[2].Cast<float>() * scale.y - d);

  Point2 rb = lt + Point2(cmd[3].Cast<float>() * scale.x + d * 2, cmd[4].Cast<float>() * scale.y + d * 2);

  ctx->render_rectangle_aa(lt, rb, lineWidth, color, midColorUsed ? midColor : color, fillColor);
}


void RenderCanvasContext::renderQuads(const Sqrat::Array &cmd) const
{
  if (cmd.Length() == 3)
  {
    // VECTOR_QUADS, [vertices], [quad indices]

    if (cmd[1].GetType() != OT_ARRAY || cmd[2].GetType() != OT_ARRAY)
    {
      darg_assert_trace_var("invalid parameters for VECTOR_QUADS, expected array of points and array of indices", cmd, 0);
      return;
    }

    const Sqrat::Array &sqPoints = cmd[1];
    const Sqrat::Array &sqIndices = cmd[2];

    if (sqPoints.Length() % 3 != 0)
    {
      darg_assert_trace_var("invalid size of points array for VECTOR_QUADS", cmd[1], 0);
      return;
    }

    if (sqIndices.Length() % 4 != 0)
    {
      darg_assert_trace_var("invalid size of indices array for VECTOR_QUADS", cmd[2], 0);
      return;
    }

    if (!fillColor)
      return;

    Tab<Point2> points(framemem_ptr());
    Tab<E3DCOLOR> colors(framemem_ptr());
    points.reserve(sqPoints.Length() / 3);
    colors.reserve(sqPoints.Length() / 3);

    for (int i = 0, len = sqPoints.Length(); i < len; i += 3)
      points.push_back(offset + Point2(sqPoints[i].Cast<float>() * scale.x, sqPoints[i + 1].Cast<float>() * scale.y));

    if (fillColor == 0xFFFFFFFFu)
    {
      for (int i = 0, len = sqPoints.Length(); i < len; i += 3)
        colors.push_back(script_decode_e3dcolor(sqPoints[i + 2].Cast<SQInteger>()));
    }
    else
    {
      for (int i = 0, len = sqPoints.Length(); i < len; i += 3)
        colors.push_back(e3dcolor_mul(fillColor, script_decode_e3dcolor(sqPoints[i + 2].Cast<SQInteger>())));
    }


    for (int i = 0, len = sqIndices.Length(); i < len; i += 4)
    {
      int idx[4];

      for (int k = 0; k < 4; k++)
      {
        idx[k] = sqIndices[i + k].Cast<int>();
        if (idx[k] < 0 || idx[k] >= points.size())
        {
          debug("ERROR: VECTOR_QUADS: point index = %d, points.size() = %d", idx[k], int(points.size()));
          darg_assert_trace_var("VECTOR_QUADS: point index is out of range", sqIndices, 0);
          return;
        }
      }

      ctx->render_quad_color(points[idx[0]], points[idx[1]], points[idx[2]], points[idx[3]], Point2(0, 0), Point2(0, 0), Point2(0, 0),
        Point2(0, 0), colors[idx[0]], colors[idx[1]], colors[idx[2]], colors[idx[3]]);
    }
  }
  else
  {
    // VECTOR_QUADS, (x, y, color) * 4 times, ...
    bool isValidParams = cmd.Length() > 1 && (cmd.Length() - 1) % 12 == 0;

    if (!isValidParams)
    {
      darg_assert_trace_var("invalid number of parameters for VECTOR_QUADS", cmd, 0);
      return;
    }

    if (!fillColor)
      return;

    Point2 p[4];
    E3DCOLOR colors[4];

    for (int i = 1, len = cmd.Length(); i < len; i += 12)
    {
      for (int k = 0, index = 0; k < 12; k += 3, index++) // 3 numbers per vertex - x, y, color
      {
        p[index] = offset + Point2(cmd[i + k].Cast<float>() * scale.x, cmd[i + k + 1].Cast<float>() * scale.y);
        colors[index] = e3dcolor_mul(fillColor, script_decode_e3dcolor(cmd[i + k + 2].Cast<SQInteger>()));
      }

      ctx->render_quad_color(p[0], p[1], p[2], p[3], Point2(0, 0), Point2(0, 0), Point2(0, 0), Point2(0, 0), colors[0], colors[1],
        colors[2], colors[3]);
    }
  }
}


static RenderCanvasContext *get_ctx(HSQUIRRELVM vm)
{
  SQUserPointer ptr = nullptr;
  SQUserPointer typetag = nullptr;
  if (SQ_FAILED(sq_getuserdata(vm, 1, &ptr, &typetag)) || typetag != RenderCanvasContext::get_type_tag())
  {
    sq_throwerror(vm, "Invalid canvas context handle used as 'this'");
    return nullptr;
  }
  return *((RenderCanvasContext **)ptr);
}


static SQInteger line_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Sqrat::Var<Sqrat::Array> sqPoints(vm, 2);
  SQInteger nPts = sq_getsize(vm, 2) / 2;

  Tab<Point2> points(framemem_ptr());
  points.resize(nPts);

  if (sq_ext_get_array_floats(sqPoints.value.GetObject(), 0, nPts * 2, &points[0].x) != SQ_OK)
    return sq_throwerror(vm, "Failed to parse points");

  float lineWidth;
  sq_getfloat(vm, 3, &lineWidth);

  E3DCOLOR color;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 4, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);

  rctx->ctx->render_line_aa(points, /*is_closed*/ false, lineWidth, Point2(0, 0), color);
  return 0;
}


static SQInteger line_dashed_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Sqrat::Var<Sqrat::Array> sqPoints(vm, 2);
  SQInteger nPts = sq_getsize(vm, 2) / 2;
  if (nPts != 2)
    return sq_throwerror(vm, "Dashed line supports only 2 points");

  Tab<Point2> points(framemem_ptr());
  points.resize(nPts);

  if (sq_ext_get_array_floats(sqPoints.value.GetObject(), 0, nPts * 2, &points[0].x) != SQ_OK)
    return sq_throwerror(vm, "Failed to parse points");

  float lineWidth, dash, space;
  sq_getfloat(vm, 3, &dash);
  sq_getfloat(vm, 4, &space);
  sq_getfloat(vm, 5, &lineWidth);

  E3DCOLOR color;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 6, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);

  rctx->ctx->render_dashed_line(points[0], points[1], dash, space, lineWidth, color);
  return 0;
}

static SQInteger fill_poly_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Sqrat::Var<Sqrat::Array> sqPoints(vm, 2);
  SQInteger nPts = sq_getsize(vm, 2) / 2;

  Tab<Point2> points(framemem_ptr());
  points.resize(nPts);

  if (sq_ext_get_array_floats(sqPoints.value.GetObject(), 0, nPts * 2, &points[0].x) != SQ_OK)
    return sq_throwerror(vm, "Failed to parse points");

  float lineWidth;
  sq_getfloat(vm, 3, &lineWidth);

  E3DCOLOR color, fillColor;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 4, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);
  if (!decode_e3dcolor(vm, 5, fillColor, &errMsg))
    return sqstd_throwerrorf(vm, "fillColor: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);
  fillColor = color_apply_opacity(fillColor, rctx->renderStateOpacity);

  rctx->ctx->render_poly(points, fillColor);
  rctx->ctx->render_line_aa(points, true, lineWidth, Point2(0, 0), color);
  return 0;
}


static SQInteger fill_inverse_poly_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Sqrat::Var<Sqrat::Array> sqPoints(vm, 2);
  SQInteger nPts = sq_getsize(vm, 2) / 2;

  Tab<Point2> points(framemem_ptr());
  points.resize(nPts);

  if (sq_ext_get_array_floats(sqPoints.value.GetObject(), 0, nPts * 2, &points[0].x) != SQ_OK)
    return sq_throwerror(vm, "Failed to parse points");

  float lineWidth;
  sq_getfloat(vm, 3, &lineWidth);

  E3DCOLOR color, fillColor;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 4, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);
  if (!decode_e3dcolor(vm, 5, fillColor, &errMsg))
    return sqstd_throwerrorf(vm, "fillColor: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);
  fillColor = color_apply_opacity(fillColor, rctx->renderStateOpacity);

  const float bound = (1 << 22);

  rctx->ctx->render_inverse_poly(points, fillColor, Point2(-bound, -bound), Point2(bound, bound));
  rctx->ctx->render_line_aa(points, true, lineWidth, Point2(0, 0), color);
  return 0;
}


static SQInteger ellipse_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Point2 pos, radius;
  sq_getfloat(vm, 2, &pos.x);
  sq_getfloat(vm, 3, &pos.y);
  sq_getfloat(vm, 4, &radius.x);
  sq_getfloat(vm, 5, &radius.y);

  float lineWidth;
  sq_getfloat(vm, 6, &lineWidth);

  E3DCOLOR color, midColor, fillColor;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 7, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);
  if (!decode_e3dcolor(vm, 8, midColor, &errMsg))
    return sqstd_throwerrorf(vm, "midColor: %s", errMsg);
  if (!decode_e3dcolor(vm, 9, fillColor, &errMsg))
    return sqstd_throwerrorf(vm, "fillColor: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);
  midColor = color_apply_opacity(midColor, rctx->renderStateOpacity);
  fillColor = color_apply_opacity(fillColor, rctx->renderStateOpacity);

  rctx->ctx->render_ellipse_aa(pos, radius, lineWidth, color, midColor, fillColor);
  return 0;
}


static SQInteger sector_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Point2 pos, radius, angles;
  sq_getfloat(vm, 2, &pos.x);
  sq_getfloat(vm, 3, &pos.y);
  sq_getfloat(vm, 4, &radius.x);
  sq_getfloat(vm, 5, &radius.y);
  sq_getfloat(vm, 6, &angles.x);
  sq_getfloat(vm, 7, &angles.y);

  float lineWidth;
  sq_getfloat(vm, 8, &lineWidth);

  E3DCOLOR color, midColor, fillColor;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 9, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);
  if (!decode_e3dcolor(vm, 10, midColor, &errMsg))
    return sqstd_throwerrorf(vm, "midColor: %s", errMsg);
  if (!decode_e3dcolor(vm, 11, fillColor, &errMsg))
    return sqstd_throwerrorf(vm, "fillColor: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);
  midColor = color_apply_opacity(midColor, rctx->renderStateOpacity);
  fillColor = color_apply_opacity(fillColor, rctx->renderStateOpacity);

  rctx->ctx->render_sector_aa(pos, radius, angles, lineWidth, color, midColor, fillColor);
  return 0;
}


static SQInteger rect_sq(HSQUIRRELVM vm)
{
  RenderCanvasContext *rctx = get_ctx(vm);
  if (!rctx)
    return SQ_ERROR;

  Point2 lt, rb;
  sq_getfloat(vm, 2, &lt.x);
  sq_getfloat(vm, 3, &lt.y);
  sq_getfloat(vm, 4, &rb.x);
  sq_getfloat(vm, 5, &rb.y);

  float lineWidth;
  sq_getfloat(vm, 6, &lineWidth);

  E3DCOLOR color, midColor, fillColor;
  const char *errMsg = nullptr;
  if (!decode_e3dcolor(vm, 7, color, &errMsg))
    return sqstd_throwerrorf(vm, "color: %s", errMsg);
  if (!decode_e3dcolor(vm, 8, midColor, &errMsg))
    return sqstd_throwerrorf(vm, "midColor: %s", errMsg);
  if (!decode_e3dcolor(vm, 9, fillColor, &errMsg))
    return sqstd_throwerrorf(vm, "fillColor: %s", errMsg);

  color = color_apply_opacity(color, rctx->renderStateOpacity);
  midColor = color_apply_opacity(midColor, rctx->renderStateOpacity);
  fillColor = color_apply_opacity(fillColor, rctx->renderStateOpacity);

  rctx->ctx->render_rectangle_aa(lt, rb, lineWidth, color, midColor, fillColor);
  return 0;
}

/* qdox @page canvasAPI

Script API:
  Use it to draw in canvas, by adding `draw` function in rendObj = ROBJ_VECTOR_CANVAS components description
  this function should accpets two arguments `draw(api, rectElem)`
  rectElem is table with {w=width, h=height} (in pixels).

  'api' is pseudo object that has following functions::

    line([x0, y0, x1, y1, ...], line_width, line_color)
    line_dashed([x0, y0, x1, y1, ...], line_width, line_color)
    fill_poly([x0, y0, x1, y1, ...], line_width, line_color, fill_color)
    fill_inverse_poly([x0, y0, x1, y1, ...], line_width,line_color, fill_color)
    ellipse(x, y, radius_x, radius_y, line_width, line_color, mid_color, fill_color)
    sector(x, y, radius_x, radius_y, angle_0, angle_1, line_width, line_color, mid_color, fill_color)
    rect(x0, y0, x1, y1, line_width, line_color, mid_color, fill_color)

*/

void RenderCanvasContext::bind_script_api(Sqrat::Table &api)
{
  G_ASSERT(api.Length() == 0);

  api.Clear();
  api //
    .SquirrelFunc("line", line_sq, 4, "u a n .")
    ///
    .SquirrelFunc("line_dashed", line_dashed_sq, 6, "u a nnn .")
    .SquirrelFunc("fill_poly", fill_poly_sq, 5, "u a n ..")
    .SquirrelFunc("fill_inverse_poly", fill_inverse_poly_sq, 5, "u a n ..")
    .SquirrelFunc("ellipse", ellipse_sq, 9, "u nn nn n ...")
    .SquirrelFunc("sector", sector_sq, 11, "u nn nn nn n ...")
    .SquirrelFunc("rect", rect_sq, 9, "u nn nn n ...")
    /**/;
}

} // namespace darg
