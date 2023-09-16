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

  for (int i = 0; i < points.size(); i++)
    points[i] = offset + Point2(points[i].x * scale.x, points[i].y * scale.y);

  ctx->render_line_aa(points.data(), points.size(), /*is_closed*/ false, lineWidth, line_indent, color);
}


static void render_dashed_line(StdGuiRender::GuiContext *ctx, const Point2 &p0, const Point2 &p1, const float dash, const float space,
  const float line_width, E3DCOLOR color)
{
  const float invLength = safeinv((p0 - p1).length());

  const float x1 = p0.x;
  const float y1 = p0.y;
  const float kx = p1.x - p0.x;
  const float ky = p1.y - p0.y;
  float t = 0.f;

  const float tDash = dash * invLength;
  const float tSpace = space * invLength;

  Tab<Point2> dashPoints(framemem_ptr());
  dashPoints.resize(2);
  while (t <= 1.f)
  {
    dashPoints[0].x = x1 + kx * t;
    dashPoints[0].y = y1 + ky * t;
    t += tDash;
    if (t > 1.f)
      t = 1.f;
    dashPoints[1].x = x1 + kx * t;
    dashPoints[1].y = y1 + ky * t;
    t += tSpace;

    ctx->render_line_aa(dashPoints, /*is_closed*/ false, line_width, ZERO<Point2>(), color);

    if (t < 1e-4f) // max = 10000 dashes
      return;
  }
}


void RenderCanvasContext::renderLineDashed(const Sqrat::Array &cmd) const
{
  bool isValidParams = cmd.Length() == 7;

  if (!isValidParams)
  {
    darg_assert_trace_var("invalid number of parameters for VECTOR_LINE", cmd, 0);
    return;
  }

  Tab<Point2> points(framemem_ptr());
  int numPts = (cmd.Length() - 3) / 2;
  if (numPts != 2)
  {
    darg_assert_trace_var("VECTOR_LINE_DASHED requires 2 points", cmd, 0);
    return;
  }

  points.resize(numPts);

  if (sq_ext_get_array_floats(cmd.GetObject(), 1, numPts * 2, &points[0].x) != SQ_OK)
  {
    darg_assert_trace_var("cannot read params of VECTOR_LINE", cmd, 0);
    return;
  }

  for (int i = 0; i < numPts; i++)
    points[i] = offset + Point2(points[i].x * scale.x, points[i].y * scale.y);

  const float dash = cmd[cmd.Length() - 2].Cast<float>();
  const float space = cmd[cmd.Length() - 1].Cast<float>();

  render_dashed_line(ctx, points[0], points[1], dash, space, lineWidth, color);
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

  for (int i = 0; i < points.size(); i++)
    points[i] = offset + Point2(points[i].x * scale.x, points[i].y * scale.y);

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

  for (int idx = 0; idx < count; idx++)
    points[idx] = offset + Point2(points[idx].x * scale.x, points[idx].y * scale.y);

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

  render_dashed_line(rctx->ctx, points[0], points[1], dash, space, lineWidth, color);
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

/*
Script API:

  line([x0, y0, x1, y1, ...],
    line_width,
    line_color)

  line_dashed([x0, y0, x1, y1],
    dash, space,
    line_width,
    line_color)

  fill_poly([x0, y0, x1, y1, ...],
    line_width,
    line_color, fill_color)

  fill_inverse_poly([x0, y0, x1, y1, ...],
    line_width,
    line_color, fill_color)

  ellipse(x, y, radius_x, radius_y,
    line_width,
    line_color, mid_color, fill_color)

  sector(x, y, radius_x, radius_y, angle_0, angle_1,
    line_width,
    line_color, mid_color, fill_color)

  rect(x0, y0, x1, y1,
    line_width,
    line_color, mid_color, fill_color)
*/

/** @page renderCanvas for render canvas

use it to draw in canvas
*/

void RenderCanvasContext::bind_script_api(Sqrat::Table &api)
{
  G_ASSERT(api.Length() == 0);

  api.Clear();
  api
    .SquirrelFunc("line", line_sq, 4, "u a n .")
    /// function comment
    .SquirrelFunc("line_dashed", line_dashed_sq, 6, "u a nnn .")
    .SquirrelFunc("fill_poly", fill_poly_sq, 5, "u a n ..")
    .SquirrelFunc("fill_inverse_poly", fill_inverse_poly_sq, 5, "u a n ..")
    .SquirrelFunc("ellipse", ellipse_sq, 9, "u nn nn n ...")
    .SquirrelFunc("sector", sector_sq, 11, "u nn nn nn n ...")
    .SquirrelFunc("rect", rect_sq, 9, "u nn nn n ...");
}

} // namespace darg
