// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>

// ne emits nodes at 1:1 and scales the vertices afterwards, so a 1x glyph bake is magnified. Canvas
// only: a screen-space overlay must not take the canvas zoom.

// Measure and draw through the pair, never one of each, or their densities differ.
void add_canvas_text(ImDrawList *draw_list, ImFont &font, float font_size_px, const ImVec2 &pos, ImU32 col, const char *text,
  const char *text_end = nullptr);

ImVec2 calc_canvas_text_size(ImFont &font, float font_size_px, const char *text, const char *text_end = nullptr);

// The power-of-two scale a raster drawn size_px tall on the canvas needs at the current zoom, capped
// as the text bakes are; an icon texture loaded at size_px times this stays sharp. Needs the editor current.
float canvas_raster_density(float size_px);

// For ImGui widgets and multi-call measures. Pass the largest size drawn inside; must not span an
// ImGui::Begin, which resets the density.
//
// font is the one the covered calls draw with. ImGui only ever applies the density to the current
// font, and AddText takes its own, so a font that is not current has to be set here or it keeps the
// density it was created with.
class RasterizerDensityScope
{
public:
  RasterizerDensityScope(ImFont &font, float max_font_size_px);
  ~RasterizerDensityScope();

  RasterizerDensityScope(const RasterizerDensityScope &) = delete;
  RasterizerDensityScope &operator=(const RasterizerDensityScope &) = delete;

private:
  ImFont &font;
  const float prevContextDensity;
  float prevFontDensity;
};
