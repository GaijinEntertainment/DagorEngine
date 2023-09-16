//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <gui/dag_stdGuiRender.h>
#include <dasModules/dasShaders.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::FontFxType, FontFxType);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::BlendMode, BlendMode);

MAKE_TYPE_FACTORY(StdGuiShader, StdGuiRender::StdGuiShader);

namespace bind_dascript
{
inline BBox2 get_str_bbox(const char *str) { return StdGuiRender::get_str_bbox(str); }

inline void render_rect(real left, real top, real right, real bottom) { StdGuiRender::render_rect(left, top, right, bottom); }

inline void draw_line(real left, real top, real right, real bottom, real thickness)
{
  StdGuiRender::draw_line(left, top, right, bottom, thickness);
}

inline Point2 screen_size() { return StdGuiRender::screen_size(); }

inline StdGuiRender::StdGuiShader *StdGuiShader_ctor() { return new StdGuiRender::StdGuiShader; }

inline void StdGuiShader_dtor(StdGuiRender::StdGuiShader *shader) { delete shader; }

inline bool StdGuiShader_init(StdGuiRender::StdGuiShader &shader, const char *name) { return shader.init(name); }

inline void StdGuiShader_close(StdGuiRender::StdGuiShader &shader) { shader.close(); }

inline void StdGuiRender_set_shader(StdGuiRender::StdGuiShader *shader) { StdGuiRender::set_shader(shader); }
} // namespace bind_dascript
