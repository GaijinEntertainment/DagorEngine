// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "canvas_text.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

namespace
{
// Pixels per em, so also a sharpness ceiling: a size that would cross it takes a lower density and
// is magnified instead. AddText bakes an unclamped size (only PushFont clamps) over an RGBA32 atlas
// that never shrinks, so without this a caption sized from a node property exhausts it and asserts.
constexpr float RASTER_CAP = 320.0f;

// NavigateToSelection sets an unsnapped zoom past the zoom table's top; each rung is a bake that stays.
constexpr float MAX_DENSITY = 8.0f;

// Powers of two: framing animations ease the zoom continuously (NavigateAction::SetViewRect), and a
// bake is cached per (font, size, density), so an exact density would bake a set per animation frame.
float density_for(float canvas_zoom, float max_font_size_px)
{
  float density = 1.0f;
  while (density < canvas_zoom && density < MAX_DENSITY && (density * 2.0f) * max_font_size_px <= RASTER_CAP)
  {
    density *= 2.0f;
  }
  return density;
}
} // namespace

float canvas_raster_density(float size_px)
{
  const float invZoom = ne::GetCurrentZoom(); // canvas units per screen pixel, the inverse of the zoom
  return density_for(invZoom > 0.0f ? 1.0f / invZoom : 1.0f, size_px);
}

RasterizerDensityScope::RasterizerDensityScope(ImFont &font_, float max_font_size_px) :
  font(font_), prevContextDensity(ImGui::GetFontRasterizerDensity())
{
  const float density = canvas_raster_density(max_font_size_px);
  ImGui::SetFontRasterizerDensity(density); // carries the ImGui widget path, which draws with the current font
  prevFontDensity = font.CurrentRasterizerDensity;
  font.CurrentRasterizerDensity = density;
}

RasterizerDensityScope::~RasterizerDensityScope()
{
  font.CurrentRasterizerDensity = prevFontDensity;
  ImGui::SetFontRasterizerDensity(prevContextDensity);
}

void add_canvas_text(ImDrawList *draw_list, ImFont &font, float font_size_px, const ImVec2 &pos, ImU32 col, const char *text,
  const char *text_end)
{
  const RasterizerDensityScope density(font, font_size_px);
  draw_list->AddText(&font, font_size_px, pos, col, text, text_end);
}

ImVec2 calc_canvas_text_size(ImFont &font, float font_size_px, const char *text, const char *text_end)
{
  const RasterizerDensityScope density(font, font_size_px);
  return font.CalcTextSizeA(font_size_px, FLT_MAX, 0.0f, text, text_end);
}
