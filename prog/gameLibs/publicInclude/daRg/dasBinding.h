#pragma once

#include <gui/dag_stdGuiRender.h>
#include <daScript/daScript.h>
#include <daRg/dag_renderState.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_element.h>

MAKE_TYPE_FACTORY(GuiContext, StdGuiRender::GuiContext)
MAKE_TYPE_FACTORY(RenderState, darg::RenderState)
MAKE_TYPE_FACTORY(ElemRenderData, darg::ElemRenderData)
MAKE_TYPE_FACTORY(Element, darg::Element)
MAKE_TYPE_FACTORY(Properties, darg::Properties)

namespace darg
{
E3DCOLOR props_get_color(const ::darg::Properties &props, const char *key, E3DCOLOR def);
bool props_get_bool(const ::darg::Properties &props, const char *key, bool def);
float props_get_float(const ::darg::Properties &props, const char *key, float def);
int props_get_int(const ::darg::Properties &props, const char *key, int def);

void render_line_aa(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, bool is_closed, float line_width,
  const Point2 line_indent, E3DCOLOR color);
void render_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, E3DCOLOR fill_color);
void render_inverse_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points_ccw, E3DCOLOR fill_color,
  const Point2 &left_top, const Point2 &right_bottom);

} // namespace darg