//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <gui/dag_stdGuiRender.h>
#include <dasModules/dasShaders.h>
#include <daScript/daScriptBind.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::FontFxType, FontFxType);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::BlendMode, BlendMode);

MAKE_TYPE_FACTORY(GuiContext, StdGuiRender::GuiContext);
MAKE_TYPE_FACTORY(StdGuiShader, StdGuiRender::StdGuiShader);

namespace bind_dascript
{

BlendMode get_alpha_blend(::StdGuiRender::GuiContext &ctx);
void set_alpha_blend(::StdGuiRender::GuiContext &ctx, BlendMode mode);
void render_line_aa(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, bool is_closed, float line_width,
  const Point2 line_indent, E3DCOLOR color);
void render_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, E3DCOLOR fill_color);
void render_inverse_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points_ccw, E3DCOLOR fill_color, Point2 left_top,
  Point2 right_bottom);
void draw_char_u(::StdGuiRender::GuiContext &ctx, uint16_t ch);
inline BBox2 get_str_bbox(const char *str) { return StdGuiRender::get_str_bbox(str); }

inline void render_rect(real left, real top, real right, real bottom) { StdGuiRender::render_rect(left, top, right, bottom); }

inline void draw_line(real left, real top, real right, real bottom, real thickness)
{
  StdGuiRender::draw_line(left, top, right, bottom, thickness);
}

inline StdGuiRender::StdGuiShader *StdGuiShader_ctor() { return new StdGuiRender::StdGuiShader; }

inline void StdGuiShader_dtor(StdGuiRender::StdGuiShader *shader) { delete shader; }

inline bool StdGuiShader_init(StdGuiRender::StdGuiShader &shader, const char *name) { return shader.init(name); }

inline void StdGuiShader_close(StdGuiRender::StdGuiShader &shader) { shader.close(); }

inline void StdGuiRender_set_shader(StdGuiRender::StdGuiShader *shader) { StdGuiRender::set_shader(shader); }
} // namespace bind_dascript
