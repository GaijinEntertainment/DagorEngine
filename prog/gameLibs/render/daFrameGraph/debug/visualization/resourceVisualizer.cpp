// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "resourceVisualizer.h"

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>

#include <math.h>


constexpr eastl::pair<ResourceBarrier, const char *> BARRIER_NAMES[]{
  {RB_RW_RENDER_TARGET, "RB_RW_RENDER_TARGET"},
  {RB_RW_UAV, "RB_RW_UAV"},
  {RB_RW_COPY_DEST, "RB_RW_COPY_DEST"},
  {RB_RW_BLIT_DEST, "RB_RW_BLIT_DEST"},
  {RB_RO_SRV, "RB_RO_SRV"},
  {RB_RO_CONSTANT_BUFFER, "RB_RO_CONSTANT_BUFFER"},
  {RB_RO_VERTEX_BUFFER, "RB_RO_VERTEX_BUFFER"},
  {RB_RO_INDEX_BUFFER, "RB_RO_INDEX_BUFFER"},
  {RB_RO_INDIRECT_BUFFER, "RB_RO_INDIRECT_BUFFER"},
  {RB_RO_VARIABLE_RATE_SHADING_TEXTURE, "RB_RO_VARIABLE_RATE_SHADING_TEXTURE"},
  {RB_RO_COPY_SOURCE, "RB_RO_COPY_SOURCE"},
  {RB_RO_BLIT_SOURCE, "RB_RO_BLIT_SOURCE"},
  {RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE, "RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE"},
  {RB_FLAG_RELEASE_PIPELINE_OWNERSHIP, "RB_FLAG_RELEASE_PIPELINE_OWNERSHIP"},
  {RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP, "RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP"},
  {RB_FLAG_SPLIT_BARRIER_BEGIN, "RB_FLAG_SPLIT_BARRIER_BEGIN"},
  {RB_FLAG_SPLIT_BARRIER_END, "RB_FLAG_SPLIT_BARRIER_END"},
  {RB_STAGE_VERTEX, "RB_STAGE_VERTEX"},
  {RB_STAGE_PIXEL, "RB_STAGE_PIXEL"},
  {RB_STAGE_COMPUTE, "RB_STAGE_COMPUTE"},
  {RB_STAGE_RAYTRACE, "RB_STAGE_RAYTRACE"},
  {RB_FLUSH_UAV, "RB_FLUSH_UAV"},
  {RB_FLAG_DONT_PRESERVE_CONTENT, "RB_FLAG_DONT_PRESERVE_CONTENT"},
  {RB_SOURCE_STAGE_VERTEX, "RB_SOURCE_STAGE_VERTEX"},
  {RB_SOURCE_STAGE_PIXEL, "RB_SOURCE_STAGE_PIXEL"},
  {RB_SOURCE_STAGE_COMPUTE, "RB_SOURCE_STAGE_COMPUTE"},
  {RB_SOURCE_STAGE_RAYTRACE, "RB_SOURCE_STAGE_RAYTRACE"},
};

static eastl::string nameForBarrier(ResourceBarrier barrier)
{
  eastl::string result;
  for (auto [flag, name] : BARRIER_NAMES)
  {
    if (flag == (barrier & flag))
    {
      result += name;
      result += " | ";
    }
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

static constexpr float NODE_WINDOW_PADDING = 10.f;

constexpr ImU32 LINE_COLOR = IM_COL32(100, 100, 100, 255);

constexpr float GAP_BETWEEN_FRAMES_SIZE = 400.0f;

constexpr float STAR_RADIUS = 8.0f;
constexpr int STAR_POINTS = 5;

static void star_vertices(ImVec2 center, float outerR, float innerR, ImVec2 *out)
{
  for (int i = 0; i < STAR_POINTS * 2; ++i)
  {
    float angle = -M_PI / 2.0f + i * M_PI / STAR_POINTS;
    float r = (i % 2 == 0) ? outerR : innerR;
    out[i] = ImVec2(center.x + r * cosf(angle), center.y + r * sinf(angle));
  }
}

static void draw_star_filled(ImDrawList *draw_list, ImVec2 center, float radius, ImU32 color)
{
  ImVec2 pts[STAR_POINTS * 2];
  star_vertices(center, radius, radius * 0.382f, pts);
  draw_list->AddConcavePolyFilled(pts, STAR_POINTS * 2, color);
}

static void draw_star_outline(ImDrawList *draw_list, ImVec2 center, float radius, ImU32 color)
{
  ImVec2 pts[STAR_POINTS * 2];
  star_vertices(center, radius, radius * 0.382f, pts);
  draw_list->AddPolyline(pts, STAR_POINTS * 2, color, ImDrawFlags_Closed, 1.0f);
}

static void draw_dashed_line(ImDrawList *draw_list, ImVec2 from, ImVec2 to, ImU32 color)
{
  constexpr float DASH_LEN = 4.0f;
  constexpr float GAP_LEN = 3.0f;
  ImVec2 dir = to - from;
  float totalLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
  if (totalLen < 1.0f)
    return;
  ImVec2 norm = dir * (1.0f / totalLen);

  float d = 0.f;
  while (d < totalLen)
  {
    float dashEnd = eastl::min(d + DASH_LEN, totalLen);
    draw_list->AddLine(ImVec2(from.x + norm.x * d, from.y + norm.y * d), ImVec2(from.x + norm.x * dashEnd, from.y + norm.y * dashEnd),
      color, 1.0f);
    d = dashEnd + GAP_LEN;
  }
}

static void draw_clipped_text_wrapped(ImDrawList *draw_list, const char *text, ImVec2 rect_min, ImVec2 rect_max)
{
  constexpr float TEXT_PAD = 4.0f;
  const float maxWidth = rect_max.x - rect_min.x - TEXT_PAD;
  const float lineHeight = ImGui::GetFontSize();
  float curY = rect_min.y + TEXT_PAD;

  // Find slash-separated segment boundaries
  const char *segments[32];
  int segLengths[32];
  int segCount = 0;
  segments[0] = text;
  for (const char *c = text; *c; ++c)
  {
    if (*c == '/' && c != text && segCount + 1 < 32)
    {
      segLengths[segCount] = (int)(c - segments[segCount]);
      segCount++;
      segments[segCount] = c; // includes the leading '/'
    }
  }
  segLengths[segCount] = (int)(strlen(segments[segCount]));
  segCount++;

  draw_list->PushClipRect(rect_min, rect_max, true);

  const char *lineStart = segments[0];
  int lineLen = segLengths[0];
  for (int si = 1; si < segCount; ++si)
  {
    const float extendedWidth = ImGui::CalcTextSize(lineStart, lineStart + lineLen + segLengths[si]).x;
    if (extendedWidth <= maxWidth)
    {
      lineLen += segLengths[si];
    }
    else
    {
      draw_list->AddText(ImVec2(rect_min.x + TEXT_PAD, curY), IM_COL32(255, 255, 255, 255), lineStart, lineStart + lineLen);
      curY += lineHeight;
      lineStart = segments[si];
      lineLen = segLengths[si];
    }
  }
  draw_list->AddText(ImVec2(rect_min.x + TEXT_PAD, curY), IM_COL32(255, 255, 255, 255), lineStart, lineStart + lineLen);

  draw_list->PopClipRect();
}

using SizeFormat = dafg::visualization::ResourseVisualizer::SizeFormat;

static void format_size(char *buf, size_t buf_size, int size_bytes, SizeFormat fmt)
{
  if (fmt == SizeFormat::Hex)
  {
    snprintf(buf, buf_size, "0x%08X", size_bytes);
    return;
  }

  if (fmt == SizeFormat::Bytes)
  {
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%d", size_bytes);
    const int len = (int)strlen(tmp);
    char *dst = buf;
    const char *end = buf + buf_size - 3;
    for (int i = 0; i < len && dst < end; ++i)
    {
      if (i > 0 && (len - i) % 3 == 0)
        *dst++ = '\'';
      *dst++ = tmp[i];
    }
    *dst++ = ' ';
    *dst++ = 'B';
    *dst = '\0';
    return;
  }

  struct Unit
  {
    int64_t divisor;
    const char *suffix;
  };
  constexpr Unit DECIMAL_UNITS[] = {{1000LL * 1000LL * 1000LL, "GB"}, {1000LL * 1000LL, "MB"}, {1000LL, "KB"}};
  constexpr Unit BINARY_UNITS[] = {{1024LL * 1024LL * 1024LL, "GiB"}, {1024LL * 1024LL, "MiB"}, {1024LL, "KiB"}};
  const Unit *units = (fmt == SizeFormat::Decimal) ? DECIMAL_UNITS : BINARY_UNITS;

  for (int i = 0; i < 3; ++i)
  {
    if (size_bytes >= units[i].divisor)
    {
      const int64_t d = units[i].divisor;
      const int whole = (int)(size_bytes / d);
      const int64_t rem = size_bytes % d;

      if (whole >= 100)
        snprintf(buf, buf_size, "%d.%d %s", whole, (int)(rem * 10 / d), units[i].suffix);
      else if (whole >= 10)
        snprintf(buf, buf_size, "%d.%02d %s", whole, (int)(rem * 100 / d), units[i].suffix);
      else
        snprintf(buf, buf_size, "%d.%03d %s", whole, (int)(rem * 1000 / d), units[i].suffix);
      return;
    }
  }

  snprintf(buf, buf_size, "%d B", size_bytes);
}

constexpr float JAGGED_AMP = 5.0f;
constexpr float JAGGED_STEP = 8.0f;

static void add_corner_arc(dag::Vector<ImVec2, framemem_allocator> &pts, ImVec2 center, float radius, float startAngle,
  int segments = 4)
{
  for (int i = 0; i <= segments; ++i)
  {
    float a = startAngle + (M_PI * 0.5f) * i / segments;
    pts.push_back(ImVec2(center.x + cosf(a) * radius, center.y + sinf(a) * radius));
  }
}

static void draw_jagged_rect(ImDrawList *draw_list, ImVec2 min, ImVec2 max, ImU32 fill_color, ImU32 border_color, bool jagged_left,
  bool jagged_right, float rounding = 2.0f)
{
  float r = eastl::min(rounding, eastl::min(max.x - min.x, max.y - min.y) * 0.5f);

  dag::Vector<ImVec2, framemem_allocator> pts;
  pts.reserve(48);

  // Top-left corner
  if (jagged_left)
    pts.push_back(ImVec2(min.x, min.y));
  else
    add_corner_arc(pts, ImVec2(min.x + r, min.y + r), r, M_PI, 4);

  // Top-right corner
  if (jagged_right)
    pts.push_back(ImVec2(max.x, min.y));
  else
    add_corner_arc(pts, ImVec2(max.x - r, min.y + r), r, -M_PI * 0.5f, 4);

  const float edgeHeight = max.y - min.y;
  const int toothCount = eastl::max(1, (int)roundf(edgeHeight / JAGGED_STEP));
  const float step = edgeHeight / toothCount;

  // Right edge (top to bottom)
  if (jagged_right)
  {
    for (int i = 0; i < toothCount; ++i)
    {
      pts.push_back(ImVec2(max.x - JAGGED_AMP, min.y + step * (i + 0.5f)));
      if (i + 1 < toothCount)
        pts.push_back(ImVec2(max.x, min.y + step * (i + 1)));
    }
  }

  // Bottom-right corner
  if (jagged_right)
    pts.push_back(ImVec2(max.x, max.y));
  else
    add_corner_arc(pts, ImVec2(max.x - r, max.y - r), r, 0.f, 4);

  // Bottom-left corner
  if (jagged_left)
    pts.push_back(ImVec2(min.x, max.y));
  else
    add_corner_arc(pts, ImVec2(min.x + r, max.y - r), r, M_PI * 0.5f, 4);

  // Left edge (bottom to top)
  if (jagged_left)
  {
    for (int i = 0; i < toothCount; ++i)
    {
      pts.push_back(ImVec2(min.x + JAGGED_AMP, max.y - step * (i + 0.5f)));
      if (i + 1 < toothCount)
        pts.push_back(ImVec2(min.x, max.y - step * (i + 1)));
    }
  }

  draw_list->AddConcavePolyFilled(pts.data(), (int)pts.size(), fill_color);
  draw_list->AddPolyline(pts.data(), (int)pts.size(), border_color, ImDrawFlags_Closed, 1.0f);
}

static void draw_rotated_text(ImDrawList *draw_list, const char *text, ImVec2 pos, float angle, ImU32 color, float font_size = 0.f,
  ImFont *font = nullptr)
{
  if (!font)
    font = ImGui::GetFont();
  if (font_size <= 0.f)
    font_size = ImGui::GetFontSize();

  ImFontBaked *baked = font->GetFontBaked(font_size);
  const float scale = font_size / baked->Size;

  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  auto rotate = [&](ImVec2 pt) -> ImVec2 {
    pt.x *= scale;
    pt.y *= scale;
    return ImVec2(pos.x + pt.x * cosA - pt.y * sinA, pos.y + pt.x * sinA + pt.y * cosA);
  };

  draw_list->PushTexture(font->OwnerAtlas->TexRef);

  float cursorX = 0.f;
  for (const char *p = text; *p; ++p)
  {
    const ImFontGlyph *glyph = baked->FindGlyph((ImWchar)*p);
    if (!glyph || !glyph->Visible)
    {
      if (glyph)
        cursorX += glyph->AdvanceX;
      continue;
    }

    ImVec2 p0(cursorX + glyph->X0, glyph->Y0);
    ImVec2 p1(cursorX + glyph->X1, glyph->Y0);
    ImVec2 p2(cursorX + glyph->X1, glyph->Y1);
    ImVec2 p3(cursorX + glyph->X0, glyph->Y1);

    draw_list->PrimReserve(6, 4);
    draw_list->PrimQuadUV(rotate(p0), rotate(p1), rotate(p2), rotate(p3), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
      ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1), color);

    cursorX += glyph->AdvanceX;
  }

  draw_list->PopTexture();
}


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

namespace visualization
{

static constexpr float FRAME_LABEL_SCALE = 2.5f;

ResourseVisualizer::ResourseVisualizer(const InternalRegistry &intRegistry, const NameResolver &nameRes,
  const intermediate::Graph &irGraph) :
  registry(intRegistry), nameResolver(nameRes), intermediateGraph(irGraph)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_RES_WIN_NAME, [&]() { this->draw(); });
}

void ResourseVisualizer::draw()
{
  if (ImGui::IsWindowCollapsed())
    return;

  showTooltips = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

  if (nodeCount == 0)
    dafg::recompile_graph.set(true);

  drawUI();

  dag::Vector<HeapIndex, framemem_allocator> gpuHeapIndices;
  dag::Vector<HeapIndex, framemem_allocator> cpuHeapIndices;
  for (auto [hi, _] : resourceHeapSizes.enumerate())
  {
    if (cpuHeapFlags.isMapped(hi) && cpuHeapFlags[hi])
      cpuHeapIndices.push_back(hi);
    else
      gpuHeapIndices.push_back(hi);
  }

  if (ImGui::BeginTabBar("HeapTypeSelector"))
  {
    if (ImGui::BeginTabItem("GPU"))
    {
      drawCanvas(gpuHeapIndices);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("CPU"))
    {
      drawCanvas(cpuHeapIndices);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  processPopup();
}

void ResourseVisualizer::drawUI()
{
  if (ImGui::Button("Recompile"))
    dafg::recompile_graph.set(true);

  ImGui::SameLine();
  ImGui::TextUnformatted("Use mouse wheel to pan/zoom. Hold LSHIFT for vertical only, LCTRL for horizontal only.");
}

// Draws heap separators and vertical column lines below the header (should be clipped to below header)
void ResourseVisualizer::drawBodyBackground(ImDrawList *drawList, eastl::span<const HeapIndex> heapIndicesToDisplay, float vScale,
  const IdIndexedMapping<HeapIndex, float> &heapVisualHeight, const IdIndexedMapping<HeapIndex, float> &heapBaseY, float header_height)
{
  const float numberRowBottom = initialPos.y + header_height;
  const float canvasBottom = initialPos.y + ImGui::GetWindowHeight() - NODE_WINDOW_PADDING;
  const float windowWidth = ImGui::GetWindowWidth();

  const auto &layout = *activeLayout;

  for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (layout.totalWidth * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE);
    for (int col = 0; col < layout.columnCount(); ++col)
    {
      const auto &column = layout.columns[col];
      const float colCenterX = viewOffset.x + (frameOffsetX + layout.columnCenterX(col) * NODE_HORIZONTAL_SIZE) * viewScaling;
      const float colHalfW = column.widthMultiplier * NODE_HORIZONTAL_HALFSIZE * viewScaling;
      const float leftEdge = colCenterX - colHalfW;

      drawList->AddLine(ImVec2(leftEdge, numberRowBottom), ImVec2(leftEdge, canvasBottom), LINE_COLOR);

      // Dashed center line for compacted (multi-node) columns
      if (column.widthMultiplier > 1.0f)
      {
        constexpr float DASH_LEN = 6.f;
        constexpr float GAP_LEN = 4.f;
        constexpr ImU32 DASH_COLOR = IM_COL32(80, 80, 80, 200);
        const float dashOffset = fmodf(viewOffset.y, DASH_LEN + GAP_LEN);
        for (float y = numberRowBottom - (DASH_LEN + GAP_LEN) + dashOffset; y < canvasBottom; y += DASH_LEN + GAP_LEN)
        {
          float y0 = eastl::max(y, numberRowBottom);
          float y1 = eastl::min(y + DASH_LEN, canvasBottom);
          if (y1 > y0)
            drawList->AddLine(ImVec2(colCenterX, y0), ImVec2(colCenterX, y1), DASH_COLOR, 2.0f);
        }
      }

      if (col == layout.columnCount() - 1)
      {
        const float rightEdge = colCenterX + colHalfW;
        drawList->AddLine(ImVec2(rightEdge, numberRowBottom), ImVec2(rightEdge, canvasBottom), LINE_COLOR);
      }
    }

    if (frame == 0)
    {
      for (HeapIndex hi : heapIndicesToDisplay)
      {
        const float heapTop = viewOffset.y + header_height + heapBaseY[hi];
        const float dataHeight = resourceHeapSizes[hi] * vScale;

        // Top separator
        drawList->AddLine(ImVec2(initialPos.x, heapTop), ImVec2(initialPos.x + windowWidth, heapTop), LINE_COLOR);

        // Data bottom separator (where the actual heap memory ends)
        drawList->AddLine(ImVec2(initialPos.x, heapTop + dataHeight), ImVec2(initialPos.x + windowWidth, heapTop + dataHeight),
          LINE_COLOR);

        // Header bottom separator (may be lower if label needs more space)
        if (heapVisualHeight[hi] > dataHeight)
          drawList->AddLine(ImVec2(initialPos.x, heapTop + heapVisualHeight[hi]),
            ImVec2(initialPos.x + windowWidth, heapTop + heapVisualHeight[hi]), LINE_COLOR);
      }
    }
  }
}

// Draws left-side heap labels with sticky scroll behavior
void ResourseVisualizer::drawHeapLabels(ImDrawList *drawList, eastl::span<const HeapIndex> heapIndicesToDisplay,
  const IdIndexedMapping<HeapIndex, float> &heapVisualHeight, const IdIndexedMapping<HeapIndex, float> &heapBaseY, float header_height)
{
  const float fontSize = ImGui::GetFontSize();
  const float panelWidth = fontSize + 2.0f * NODE_WINDOW_PADDING;
  const float headerBottom = initialPos.y + header_height;

  for (HeapIndex heapIdx : heapIndicesToDisplay)
  {
    const float heapTopY = viewOffset.y + header_height + heapBaseY[heapIdx];
    const float heapBottomY = heapTopY + heapVisualHeight[heapIdx];

    // Filled background for the label panel area (covers resources underneath)
    drawList->AddRectFilled(ImVec2(initialPos.x, eastl::max(heapTopY, headerBottom)), ImVec2(initialPos.x + panelWidth, heapBottomY),
      IM_COL32(30, 30, 30, 255));

    // Vertical separator line at the right edge of the panel
    drawList->AddLine(ImVec2(initialPos.x + panelWidth, eastl::max(heapTopY, headerBottom)),
      ImVec2(initialPos.x + panelWidth, heapBottomY), LINE_COLOR);

    // Build label text
    char sizeBuf[32];
    format_size(sizeBuf, sizeof(sizeBuf), resourceHeapSizes[heapIdx], sizeFormat);
    char labelBuf[64];
    snprintf(labelBuf, sizeof(labelBuf), "heap %i (%s)", eastl::to_underlying(heapIdx), sizeBuf);
    const float labelWidth = ImGui::CalcTextSize(labelBuf).x;

    // Sticky label Y: anchored to heap top, slides down when scrolled under header, stops at bottom.
    // After the rotated-space offset, the draw position shifts down by labelWidth,
    // so clamp max must be (heapBottomY - 2*labelWidth) to keep the text within bounds.
    const float clampTop = headerBottom + NODE_WINDOW_PADDING;
    const float clampBottom = heapBottomY - NODE_WINDOW_PADDING;
    const float labelY = eastl::min(eastl::max(heapTopY + NODE_WINDOW_PADDING, clampTop), clampBottom - labelWidth);
    const float labelX = initialPos.x + NODE_WINDOW_PADDING + fontSize;

    // Offset in rotated space: left by labelWidth, up by fontSize
    constexpr float HEAP_LABEL_ANGLE = -M_PI / 2.0f;
    const float cosA = cosf(HEAP_LABEL_ANGLE);
    const float sinA = sinf(HEAP_LABEL_ANGLE);
    const float offX = -labelWidth * cosA + fontSize * sinA;
    const float offY = -labelWidth * sinA - fontSize * cosA;

    draw_rotated_text(drawList, labelBuf, ImVec2(labelX + offX, labelY + offY), HEAP_LABEL_ANGLE, IM_COL32(200, 200, 200, 255));
  }
}

// Draws the header (angled node names, number row) -- should be drawn last, unclipped, on top of resources
void ResourseVisualizer::drawHeader(ImDrawList *drawList, float header_height)
{
  constexpr float LABEL_ANGLE = -M_PI / 4.0f;
  const float fontSize = ImGui::GetFontSize();
  const float angledAreaHeight = header_height - fontSize - NODE_WINDOW_PADDING;
  const float angledBottom = initialPos.y + angledAreaHeight;
  const float numberRowBottom = initialPos.y + header_height;
  const float windowWidth = ImGui::GetWindowWidth();

  // Filled background to cover any scrolled content underneath
  drawList->AddRectFilled(ImVec2(initialPos.x, initialPos.y), ImVec2(initialPos.x + windowWidth, numberRowBottom),
    IM_COL32(30, 30, 30, 255));

  const auto &layout = *activeLayout;

  for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (layout.totalWidth * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE);
    for (int col = 0; col < layout.columnCount(); ++col)
    {
      const auto &column = layout.columns[col];
      const float colCenterX = viewOffset.x + (frameOffsetX + layout.columnCenterX(col) * NODE_HORIZONTAL_SIZE) * viewScaling;
      const float colHalfW = column.widthMultiplier * NODE_HORIZONTAL_HALFSIZE * viewScaling;
      const float leftEdge = colCenterX - colHalfW;

      // Angled separator in the text area (bottom-left to top-right)
      drawList->AddLine(ImVec2(leftEdge, angledBottom), ImVec2(leftEdge + angledAreaHeight, initialPos.y), LINE_COLOR);

      // Vertical separator in the number row
      drawList->AddLine(ImVec2(leftEdge, angledBottom), ImVec2(leftEdge, numberRowBottom), LINE_COLOR);

      if (col == layout.columnCount() - 1)
      {
        const float rightEdge = colCenterX + colHalfW;
        drawList->AddLine(ImVec2(rightEdge, angledBottom), ImVec2(rightEdge + angledAreaHeight, initialPos.y), LINE_COLOR);
        drawList->AddLine(ImVec2(rightEdge, angledBottom), ImVec2(rightEdge, numberRowBottom), LINE_COLOR);
      }

      // Node name at 45 degrees, bottom-left to top-right
      {
        const float halfFont = fontSize * 0.5f;
        const ImVec2 verticalCenterOffset(halfFont * sinf(LABEL_ANGLE + M_PI / 2), halfFont * cosf(LABEL_ANGLE + M_PI / 2));
        const ImVec2 horOffset = 15.0f * ImVec2(cosf(LABEL_ANGLE), sinf(LABEL_ANGLE));
        const ImVec2 labelOrigin = ImVec2(colCenterX, angledBottom) - verticalCenterOffset + horOffset;
        draw_rotated_text(drawList, column.displayName.c_str(), labelOrigin, LABEL_ANGLE, IM_COL32(200, 200, 200, 255));
      }

      // Node index number centered in the number row
      {
        ImVec2 textSize = ImGui::CalcTextSize(column.indexLabel.c_str());
        const float numberY = angledBottom + (numberRowBottom - angledBottom - textSize.y) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(colCenterX - textSize.x / 2, numberY));
        ImGui::TextUnformatted(column.indexLabel.c_str());
      }

      // Tooltip listing compacted nodes when hovering a multi-node column header
      if (showTooltips && column.firstNodeIdx != column.lastNodeIdx)
      {
        const ImVec2 mousePos = ImGui::GetMousePos();
        const float rightEdge = colCenterX + colHalfW;
        bool inStrip = false;
        if (mousePos.y >= initialPos.y && mousePos.y <= numberRowBottom)
        {
          if (mousePos.y >= angledBottom)
            inStrip = mousePos.x >= leftEdge && mousePos.x <= rightEdge;
          else
          {
            const float shift = angledBottom - mousePos.y;
            inStrip = mousePos.x >= leftEdge + shift && mousePos.x <= rightEdge + shift;
          }
        }
        if (inStrip)
        {
          ImGui::BeginTooltip();
          for (int ni = column.firstNodeIdx; ni <= column.lastNodeIdx; ++ni)
          {
            auto nIdx = static_cast<intermediate::NodeIndex>(ni);
            if (nodeNames.isMapped(nIdx) && !nodeNames[nIdx].empty())
              ImGui::TextUnformatted(nodeNames[nIdx].c_str());
          }
          ImGui::EndTooltip();
        }
      }
    }

    // Frame label ("Even Frame" / "Odd Frame") to the left of node names
    if (layout.columnCount() > 0)
    {
      const char *frameLabel = (frame == 0) ? "Even Frame" : "Odd Frame";
      const float labelFontSize = fontSize * FRAME_LABEL_SCALE;
      const float firstNodeLeftEdge = viewOffset.x + (frameOffsetX + layout.columnLeftEdge(0) * NODE_HORIZONTAL_SIZE) * viewScaling;

      const ImVec2 frameLabelOrigin(firstNodeLeftEdge - 10.0f, angledBottom);

      const ImVec2 verticalCenterOffset = labelFontSize * ImVec2(sinf(LABEL_ANGLE + M_PI / 2), cosf(LABEL_ANGLE + M_PI / 2));
      const ImVec2 horOffset = 10.0f * ImVec2(cosf(LABEL_ANGLE), sinf(LABEL_ANGLE));
      draw_rotated_text(drawList, frameLabel, frameLabelOrigin - verticalCenterOffset + horOffset, LABEL_ANGLE,
        IM_COL32(200, 200, 200, 255), labelFontSize);
    }

    if (frame == 0)
    {
      drawList->AddLine(ImVec2(initialPos.x, angledBottom), ImVec2(initialPos.x + windowWidth, angledBottom), LINE_COLOR);
      drawList->AddLine(ImVec2(initialPos.x, numberRowBottom), ImVec2(initialPos.x + windowWidth, numberRowBottom), LINE_COLOR);
    }
  }
}

void ResourseVisualizer::drawCanvas(eastl::span<const HeapIndex> heapIndicesToDisplay)
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), ImGuiChildFlags_Borders,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
  ImGui::PushItemWidth(120.0f);

  const ImVec2 winPos = ImGui::GetWindowPos();
  const ImVec2 winSize = ImGui::GetWindowSize();
  drawList->PushClipRect(winPos, winPos + winSize, false);

  initialPos = ImVec2(winPos.x + 1.f, winPos.y + 1.f);
  viewOffset = initialPos + viewScrolling;
  const bool isCpu =
    !heapIndicesToDisplay.empty() && cpuHeapFlags.isMapped(heapIndicesToDisplay[0]) && cpuHeapFlags[heapIndicesToDisplay[0]];
  if (currentLayout.columnCount() == 0 || isCpu != layoutIsCpu || compactView != layoutCompact || showUsages != layoutUsages ||
      showBarriers != layoutBarriers || showIRResources != layoutIR)
  {
    layoutIsCpu = isCpu;
    layoutCompact = compactView;
    layoutUsages = showUsages;
    layoutBarriers = showBarriers;
    layoutIR = showIRResources;

    if (!compactView)
      currentLayout.buildFromNodes(nodeNames);
    else
    {
      const auto &interesting = isCpu ? cpuInteresting : gpuInteresting;
      const auto &feNodes = showUsages ? (showBarriers ? interesting.logicalBarriers : interesting.logical)
                                       : (showBarriers ? interesting.physicalBarriers : interesting.physical);
      const auto &irNodes = showBarriers ? interesting.irBarriers : interesting.ir;
      currentLayout.buildCompact(nodeNames, showIRResources ? irNodes : feNodes);
    }
  }
  activeLayout = &currentLayout;
  viewWidth =
    RES_RECORD_WINDOW * (activeLayout->totalWidth * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE) - GAP_BETWEEN_FRAMES_SIZE;

  // All heaps in the same tab share a single vertical scale factor
  int maxHeapSize = 0;
  for (HeapIndex hi : heapIndicesToDisplay)
    maxHeapSize = eastl::max(maxHeapSize, resourceHeapSizes[hi]);
  const float vScale = maxHeapSize > 0 ? verticalScaling / float(maxHeapSize) : 0.f;
  // Gap defined as a fraction of max heap size so it scales uniformly with content
  constexpr float HEAP_GAP_FRACTION = 0.25f;
  const float scaledHeapGap = maxHeapSize * HEAP_GAP_FRACTION * vScale;

  // Compute header height from the longest node name at 45 degrees
  constexpr float LABEL_ANGLE_ABS = M_PI / 4.0f;
  const float fontSize = ImGui::GetFontSize();
  float maxNodeNameWidth = 0.f;
  for (const auto &name : nodeNames)
    maxNodeNameWidth = eastl::max(maxNodeNameWidth, ImGui::CalcTextSize(name.c_str()).x);
  const float angledAreaHeight = maxNodeNameWidth * sinf(LABEL_ANGLE_ABS) + NODE_WINDOW_PADDING;
  const float numberRowHeight = fontSize + NODE_WINDOW_PADDING;
  nodeHeaderHeight = angledAreaHeight + numberRowHeight;

  // Compute the minimum visual height for each heap (label must fit)
  // Compute visual height per heap. The label minimum is converted to "virtual bytes"
  // so it scales uniformly with content, keeping the anchor formula correct.
  IdIndexedMapping<HeapIndex, float> heapVisualHeight(resourceHeapSizes.size(), 0.f);
  for (HeapIndex hi : heapIndicesToDisplay)
  {
    char sizeBuf[32];
    format_size(sizeBuf, sizeof(sizeBuf), resourceHeapSizes[hi], sizeFormat);
    char labelBuf[64];
    snprintf(labelBuf, sizeof(labelBuf), "heap %i (%s)", eastl::to_underlying(hi), sizeBuf);
    const float labelMinBytes = vScale > 0.f ? (ImGui::CalcTextSize(labelBuf).x + 2.0f * NODE_WINDOW_PADDING) / vScale : 0.f;
    const float effectiveHeapSize = eastl::max((float)resourceHeapSizes[hi], labelMinBytes);
    heapVisualHeight[hi] = effectiveHeapSize * vScale;
  }

  // Precompute per-heap base Y positions for the displayed heaps
  IdIndexedMapping<HeapIndex, float> heapBaseY(resourceHeapSizes.size(), 0.f);
  {
    float y = 0.f;
    for (HeapIndex heapIdx : heapIndicesToDisplay)
    {
      heapBaseY[heapIdx] = y;
      y += heapVisualHeight[heapIdx] + scaledHeapGap;
    }
  }

  // Split draw list: channel 0 = body content, channel 1 = overlays (drawn on top)
  ImDrawListSplitter splitter;
  splitter.Split(drawList, 2);
  splitter.SetCurrentChannel(drawList, 0);

  // Clip body content to below the header and to the right of the heap label panel
  const float heapLabelPanelWidth = fontSize + 2.0f * NODE_WINDOW_PADDING;
  const ImVec2 bodyClipMin(initialPos.x + heapLabelPanelWidth, initialPos.y + nodeHeaderHeight);
  const ImVec2 bodyClipMax = winPos + winSize;
  drawList->PushClipRect(bodyClipMin, bodyClipMax, true);
  ImGui::PushClipRect(bodyClipMin, bodyClipMax, true);

  drawBodyBackground(drawList, heapIndicesToDisplay, vScale, heapVisualHeight, heapBaseY, nodeHeaderHeight);

  hoveredIRResource = eastl::nullopt;
  hoveredFrontendIdx = -1;
  hoveredResourceOwnerFrame = 0;
  hoveredResourceHeap = 0;

  // Draw resource rects
  const float frameStride = activeLayout->totalWidth * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE;
  for (HeapIndex heapIdx : heapIndicesToDisplay)
  {
    if (!heapToResources.isMapped(heapIdx))
      continue;

    for (auto resIdx : heapToResources[heapIdx])
    {
      const auto &irRes = irResourceInfos[resIdx];

      for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
      {
        const auto &placement = irRes.placements[frame];
        if (placement.heapIndex < 0 || placement.heapIndex != eastl::to_underlying(heapIdx))
          continue;

        const float frameOffsetX = frame * frameStride;
        const float resOffsetY = viewOffset.y + nodeHeaderHeight + heapBaseY[heapIdx] + placement.offset * vScale;

        // Build a list of rects to draw: one per frontend resource, or one for the whole IR resource
        struct RectToDraw
        {
          const Lifetime *lifetime;
          const char *name;
          int feIdx;
          bool createdByRename;
          bool consumedByRename;
        };
        dag::Vector<RectToDraw, framemem_allocator> rectsToDraw;

        if (showIRResources)
        {
          if (!irRes.physicalLifetime.empty())
            rectsToDraw.push_back({&irRes.physicalLifetime, irRes.name.c_str(), -1, false, false});
        }
        else
        {
          for (const auto &fe : irRes.frontendResources)
          {
            if (fe.physicalLifetime.empty())
              continue;
            int feIdx = (int)(&fe - irRes.frontendResources.data());
            rectsToDraw.push_back({&fe.physicalLifetime, fe.name, feIdx, fe.createdByRename, fe.consumedByRename});
          }
        }

        for (const auto &rect : rectsToDraw)
        {
          G_ASSERT(rect.lifetime->firstNodeIdx < (int)nodeCount);
          const auto startNIdx = static_cast<intermediate::NodeIndex>(rect.lifetime->firstNodeIdx);
          const int startCol = activeLayout->nodeToColumn[startNIdx];
          const float startNodeX = activeLayout->nodeXCenter(startNIdx);
          const float startColHalfW = activeLayout->columns[startCol].widthMultiplier * 0.5f;
          constexpr float EDGE_GAP = 0.05f;
          const float startOffset = rect.createdByRename ? 0 : -(startColHalfW - EDGE_GAP);
          const auto resRectMin =
            ImVec2(viewOffset.x + viewScaling * (frameOffsetX + NODE_HORIZONTAL_SIZE * (startNodeX + startOffset)), resOffsetY);

          const int extraFrames = rect.lifetime->lastNodeIdx / nodeCount;
          const int endIdxInFrame = rect.lifetime->lastNodeIdx % nodeCount;
          const auto endNIdx = static_cast<intermediate::NodeIndex>(endIdxInFrame);
          const int endCol = activeLayout->nodeToColumn[endNIdx];
          const float endNodeX = activeLayout->nodeXCenter(endNIdx) + extraFrames * activeLayout->totalWidth;
          const float endColHalfW = activeLayout->columns[endCol].widthMultiplier * 0.5f;
          const uint32_t gapSkips = extraFrames * GAP_BETWEEN_FRAMES_SIZE;
          const float endOffset = rect.consumedByRename ? 0 : (endColHalfW - EDGE_GAP);
          const auto resRectMax =
            ImVec2(viewOffset.x + (frameOffsetX + NODE_HORIZONTAL_SIZE * (endNodeX + endOffset) + gapSkips) * viewScaling,
              resOffsetY + placement.size * vScale);

          char resLabel[128];
          snprintf(resLabel, sizeof(resLabel), "%s (%i)", rect.name ? rect.name : "?", frame);

          const int feIdx = rect.feIdx;
          auto drawResRect = [&drawList, resLabel] //
            (ImVec2 min, ImVec2 max, bool jagged_left, bool jagged_right, bool highlight) {
              const ImU32 borderColor = highlight ? IM_COL32(255, 255, 100, 255) : IM_COL32(30, 161, 161, 255);

              if (jagged_left || jagged_right)
              {
                draw_jagged_rect(drawList, min, max, IM_COL32(75, 75, 75, 128), borderColor, jagged_left, jagged_right);
              }
              else
              {
                drawList->AddRectFilled(min, max, IM_COL32(75, 75, 75, 128), 2.0f);
                drawList->AddRect(min, max, borderColor, 2.0f);
              }

              draw_clipped_text_wrapped(drawList, resLabel, min, max);
            };

          auto checkHovered = [this, frame, heapIdx, resIdx, feIdx](ImVec2 min, ImVec2 max) {
            if (ImGui::IsMouseHoveringRect(min, max))
            {
              hoveredIRResource = resIdx;
              hoveredFrontendIdx = feIdx;
              hoveredResourceOwnerFrame = frame;
              hoveredResourceHeap = eastl::to_underlying(heapIdx);
              return true;
            }
            return false;
          };

          if (frame + 1 < RES_RECORD_WINDOW || rect.lifetime->lastNodeIdx < nodeCount)
          {
            bool hovered = checkHovered(resRectMin, resRectMax);
            drawResRect(resRectMin, resRectMax, false, false, hovered);
          }
          else
          {
            const ImVec2 wrapMinA(viewOffset.x - NODE_HORIZONTAL_SIZE * viewScaling, resRectMin.y);
            const ImVec2 wrapMaxA(resRectMax.x - (viewWidth + GAP_BETWEEN_FRAMES_SIZE) * viewScaling, resRectMax.y);
            const ImVec2 wrapMinB(resRectMin.x, resRectMin.y);
            const ImVec2 wrapMaxB(viewOffset.x + viewWidth * viewScaling, resRectMax.y);
            bool hovered = checkHovered(wrapMinA, wrapMaxA) || checkHovered(wrapMinB, wrapMaxB);
            drawResRect(wrapMinA, wrapMaxA, true, false, hovered);
            drawResRect(wrapMinB, wrapMaxB, false, true, hovered);
          }
        }
      }
    }
  }

  // Draw usage stars (separate pass so they render on top of all resource rects)
  if (showUsages)
  {
    struct UsageStar
    {
      int timeIdx;
      const ResourceUsage *usage;
      intermediate::NodeIndex nodeIdx;
      int col;
      int extraFrames;
    };

    for (HeapIndex heapIdx : heapIndicesToDisplay)
    {
      if (!heapToResources.isMapped(heapIdx))
        continue;

      for (auto resIdx : heapToResources[heapIdx])
      {
        const auto &irRes = irResourceInfos[resIdx];

        for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
        {
          const auto &placement = irRes.placements[frame];
          if (placement.heapIndex < 0 || placement.heapIndex != eastl::to_underlying(heapIdx))
            continue;

          const float frameOffsetX = frame * frameStride;
          const float resOffsetY = viewOffset.y + nodeHeaderHeight + heapBaseY[heapIdx] + placement.offset * vScale;
          const float resRectHeight = placement.size * vScale;
          const float starRadius = eastl::min(STAR_RADIUS, resRectHeight * 0.45f);
          const float starY = resOffsetY + resRectHeight * 0.5f;

          // In IR mode, use IR-level lifetime and combine all frontend usages
          const char *irResName = irRes.name.c_str();

          auto processUsages = [&](const Lifetime &lifetime, const char *resName,
                                 const dag::VectorMap<int, ResourceUsage> &usageTimeline, bool createdByRename,
                                 bool consumedByRename) {
            // Compute resRectMin/Max for dashed line anchors
            const auto startNIdx = static_cast<intermediate::NodeIndex>(lifetime.firstNodeIdx);
            const int startCol = activeLayout->nodeToColumn[startNIdx];
            const float startNodeX = activeLayout->nodeXCenter(startNIdx);
            const float startColHalfW = activeLayout->columns[startCol].widthMultiplier * 0.5f;
            constexpr float EDGE_GAP = 0.05f;
            const float startOffset = createdByRename ? 0 : -(startColHalfW - EDGE_GAP);
            const float resRectMinX = viewOffset.x + viewScaling * (frameOffsetX + NODE_HORIZONTAL_SIZE * (startNodeX + startOffset));

            const int extraFrEnd = lifetime.lastNodeIdx / nodeCount;
            const int endIdxInFrame = lifetime.lastNodeIdx % nodeCount;
            const auto endNIdx = static_cast<intermediate::NodeIndex>(endIdxInFrame);
            const int endCol = activeLayout->nodeToColumn[endNIdx];
            const float endNodeX = activeLayout->nodeXCenter(endNIdx) + extraFrEnd * activeLayout->totalWidth;
            const float endColHalfW = activeLayout->columns[endCol].widthMultiplier * 0.5f;
            const uint32_t endGapSkips = extraFrEnd * GAP_BETWEEN_FRAMES_SIZE;
            const float endOffset = consumedByRename ? 0 : (endColHalfW - EDGE_GAP);
            const float resRectMaxX =
              viewOffset.x + (frameOffsetX + NODE_HORIZONTAL_SIZE * (endNodeX + endOffset) + endGapSkips) * viewScaling;

            // Group usages by (column, extraFrames) to spread evenly in compact columns
            dag::VectorMap<eastl::pair<int, int>, dag::Vector<UsageStar, framemem_allocator>, eastl::less<eastl::pair<int, int>>,
              framemem_allocator>
              columnGroups;

            for (const auto &[timeIdx, usage] : usageTimeline)
            {
              const int extraFr = timeIdx / nodeCount;
              const int inFrameIdx = timeIdx % nodeCount;
              const auto nIdx = static_cast<intermediate::NodeIndex>(inFrameIdx);
              if (!activeLayout->nodeToColumn.isMapped(nIdx) || activeLayout->nodeToColumn[nIdx] < 0)
                continue;
              const int col = activeLayout->nodeToColumn[nIdx];
              columnGroups[{col, extraFr}].push_back({timeIdx, &usage, nIdx, col, extraFr});
            }

            for (const auto &[key, stars] : columnGroups)
            {
              const auto &[col, extraFr] = key;
              const float colWidth = activeLayout->columns[col].widthMultiplier;
              const int count = (int)stars.size();

              const bool wrapsAround = frame + extraFr >= RES_RECORD_WINDOW;
              const float starFrameOffsetX = wrapsAround ? (frame + extraFr - RES_RECORD_WINDOW) * frameStride : frameOffsetX;
              const float colLeft = activeLayout->columnLeftEdge(col) + (wrapsAround ? 0.f : extraFr * activeLayout->totalWidth);
              const uint32_t gapSkips = wrapsAround ? 0 : extraFr * GAP_BETWEEN_FRAMES_SIZE;

              const float spacingPx = colWidth / count * NODE_HORIZONTAL_SIZE * viewScaling;
              const float groupStarRadius = eastl::min(starRadius, spacingPx * 0.45f);

              for (int si = 0; si < count; ++si)
              {
                const auto &star = stars[si];
                const float fraction = (si + 0.5f) / count;
                const float nodeX = colLeft + colWidth * fraction;
                const float starX = viewOffset.x + (starFrameOffsetX + NODE_HORIZONTAL_SIZE * nodeX + gapSkips) * viewScaling;
                const ImVec2 starCenter(starX, starY);

                const bool withinPhysical = star.timeIdx >= lifetime.firstNodeIdx && star.timeIdx <= lifetime.lastNodeIdx;

                if (withinPhysical)
                  draw_star_filled(drawList, starCenter, groupStarRadius, IM_COL32(255, 255, 255, 255));
                else
                {
                  draw_star_outline(drawList, starCenter, groupStarRadius, IM_COL32(255, 255, 255, 200));
                  if (star.timeIdx < lifetime.firstNodeIdx)
                    draw_dashed_line(drawList, starCenter, ImVec2(resRectMinX, starY), IM_COL32(255, 255, 255, 200));
                  else
                    draw_dashed_line(drawList, starCenter, ImVec2(resRectMaxX, starY), IM_COL32(255, 255, 255, 200));
                }

                if (showTooltips)
                {
                  const ImVec2 mousePos = ImGui::GetMousePos();
                  const float dx = mousePos.x - starCenter.x;
                  const float dy = mousePos.y - starCenter.y;
                  if (dx * dx + dy * dy <= (groupStarRadius + 2.f) * (groupStarRadius + 2.f))
                  {
                    const char *nodeName = nodeNames.isMapped(star.nodeIdx) ? nodeNames[star.nodeIdx].c_str() : "?";
                    if (withinPhysical)
                    {
                      ImGui::SetTooltip("\"%s\"\n"
                                        "Node: %s\n"
                                        "Access: %s\n"
                                        "Stage: %s\n"
                                        "Type: %s",
                        resName ? resName : "?", nodeName, dafg::to_string(star.usage->access),
                        dafg::to_string(star.usage->stage).c_str(), dafg::to_string(star.usage->type).c_str());
                    }
                    else
                    {
                      ImGui::SetTooltip("\"%s\"\n"
                                        "Node: %s\n"
                                        "Usage optimized away (no side-effects)",
                        resName ? resName : "?", nodeName);
                    }
                    hoveredIRResource = eastl::nullopt;
                  }
                }
              }
            }
          };

          if (showIRResources)
          {
            if (!irRes.physicalLifetime.empty())
              for (const auto &fe : irRes.frontendResources)
                processUsages(irRes.physicalLifetime, irResName, fe.usageTimeline, false, false);
          }
          else
          {
            for (const auto &fe : irRes.frontendResources)
            {
              if (fe.physicalLifetime.empty())
                continue;
              processUsages(fe.physicalLifetime, fe.name, fe.usageTimeline, fe.createdByRename, fe.consumedByRename);
            }
          }
        }
      }
    }
  }

  if (showBarriers)
  {
    for (HeapIndex heapIdx : heapIndicesToDisplay)
    {
      if (!heapToResources.isMapped(heapIdx))
        continue;

      for (auto resIdx : heapToResources[heapIdx])
      {
        const auto &irRes = irResourceInfos[resIdx];

        for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
        {
          const auto &placement = irRes.placements[frame];
          if (placement.heapIndex < 0 || placement.heapIndex != eastl::to_underlying(heapIdx))
            continue;

          const float resOffsetY = viewOffset.y + nodeHeaderHeight + heapBaseY[heapIdx] + placement.offset * vScale;

          for (auto [time, barrier] : irRes.barrierEvents)
          {
            const int inFrameTime = time % nodeCount;
            const int displayFrame = (frame + (time / nodeCount)) % RES_RECORD_WINDOW;
            const auto barrierNIdx = static_cast<intermediate::NodeIndex>(inFrameTime);
            const int barrierCol = activeLayout->nodeToColumn[barrierNIdx];
            const float barrierX = activeLayout->columnLeftEdge(barrierCol);

            const float frameOffsetX = displayFrame * frameStride;

            const float rectMiddle = viewOffset.x + (frameOffsetX + NODE_HORIZONTAL_SIZE * barrierX) * viewScaling;
            const float rectBottom = resOffsetY + eastl::max(4.f, placement.size * vScale);

            const auto min = ImVec2(rectMiddle - 3.f, resOffsetY);
            const auto max = ImVec2(rectMiddle + 3.f, rectBottom);

            if (showTooltips && ImGui::IsMouseHoveringRect(min, max))
            {
              hoveredIRResource = eastl::nullopt;
              ImGui::SetTooltip("%s", nameForBarrier(barrier).c_str());
            }

            drawList->AddRectFilled(min, max, IM_COL32(75, 75, 75, 128), 2.0f);
            drawList->AddRect(min, max, IM_COL32(161, 120, 30, 255), 2.0f);
          }
        }
      }
    }
  }

  // Pop body clip rects, switch to overlay channel for header/labels/controls
  ImGui::PopClipRect();
  drawList->PopClipRect();

  splitter.SetCurrentChannel(drawList, 1);

  drawHeapLabels(drawList, heapIndicesToDisplay, heapVisualHeight, heapBaseY, nodeHeaderHeight);
  drawHeader(drawList, nodeHeaderHeight);

  // Zoom control overlay (bottom-right corner)
  {
    constexpr float ZOOM_STEP = 1.3f;
    const float btnSize = ImGui::GetFrameHeight() * 1.2f;
    const float gridSize = btnSize * 3.f + ImGui::GetStyle().ItemSpacing.x * 2.f;
    const float padding = 10.f;
    const ImVec2 gridOrigin(winPos.x + padding, winPos.y + winSize.y - gridSize - padding);

    // Opaque background behind the button grid
    drawList->AddRectFilled(ImVec2(gridOrigin.x - 4.f, gridOrigin.y - 4.f),
      ImVec2(gridOrigin.x + gridSize + 4.f, gridOrigin.y + gridSize + 4.f), IM_COL32(30, 30, 30, 255), 4.f);

    ImGui::SetCursorScreenPos(gridOrigin);
    ImGui::BeginGroup();

    auto zoomButton = [&](const char *label, const char *tooltip, int row, int col) -> bool {
      ImGui::SetCursorScreenPos(ImVec2(gridOrigin.x + col * (btnSize + ImGui::GetStyle().ItemSpacing.x),
        gridOrigin.y + row * (btnSize + ImGui::GetStyle().ItemSpacing.y)));
      ImGui::PushID(label);
      bool pressed = ImGui::Button(label, ImVec2(btnSize, btnSize));
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
      ImGui::PopID();
      return pressed;
    };

    // Row 0: [   ] [+V ] [ + ]
    if (zoomButton("+V", "Zoom in vertically (Shift+Scroll Up)", 0, 1))
      verticalScaling = eastl::clamp(verticalScaling * ZOOM_STEP, 10.f, 100000.f);
    if (zoomButton("+", "Zoom in both axes (Scroll Up)", 0, 2))
    {
      verticalScaling = eastl::clamp(verticalScaling * ZOOM_STEP, 10.f, 100000.f);
      viewScaling = eastl::clamp(viewScaling * ZOOM_STEP, 0.1f, 10.0f);
    }

    // Row 1: [-H ] [ R ] [+H ]
    if (zoomButton("-H", "Zoom out horizontally (Ctrl+Scroll Down)", 1, 0))
      viewScaling = eastl::clamp(viewScaling / ZOOM_STEP, 0.1f, 10.0f);
    if (zoomButton("R", "Reset view", 1, 1))
    {
      viewScrolling = ImVec2(100.f, 100.f);
      viewScaling = 1.0f;
      verticalScaling = 400.0f;
    }
    if (zoomButton("+H", "Zoom in horizontally (Ctrl+Scroll Up)", 1, 2))
      viewScaling = eastl::clamp(viewScaling * ZOOM_STEP, 0.1f, 10.0f);

    // Row 2: [ - ] [-V ] [   ]
    if (zoomButton("-", "Zoom out both axes (Scroll Down)", 2, 0))
    {
      verticalScaling = eastl::clamp(verticalScaling / ZOOM_STEP, 10.f, 100000.f);
      viewScaling = eastl::clamp(viewScaling / ZOOM_STEP, 0.1f, 10.0f);
    }
    if (zoomButton("-V", "Zoom out vertically (Shift+Scroll Down)", 2, 1))
      verticalScaling = eastl::clamp(verticalScaling / ZOOM_STEP, 10.f, 100000.f);

    ImGui::EndGroup();
  }

  // View options overlay (top-left corner, below header)
  {
    const float padding = 10.f;
    const ImVec2 overlayPos(winPos.x + padding, initialPos.y + nodeHeaderHeight + padding);

    // Opaque background
    const float checkboxHeight = ImGui::GetFrameHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.y;
    const float overlayHeight = checkboxHeight * 5.f + spacing * 4.f;
    const float overlayWidth = 170.f;
    drawList->AddRectFilled(ImVec2(overlayPos.x - 4.f, overlayPos.y - 4.f),
      ImVec2(overlayPos.x + overlayWidth + 4.f, overlayPos.y + overlayHeight + 4.f), IM_COL32(30, 30, 30, 255), 4.f);

    ImGui::SetCursorScreenPos(overlayPos);
    ImGui::Checkbox("Compact", &compactView);
    ImGui::SetCursorScreenPos(ImVec2(overlayPos.x, overlayPos.y + checkboxHeight + spacing));
    ImGui::Checkbox("Barriers", &showBarriers);
    ImGui::SetCursorScreenPos(ImVec2(overlayPos.x, overlayPos.y + (checkboxHeight + spacing) * 2.f));
    ImGui::Checkbox("Usages", &showUsages);
    ImGui::SetCursorScreenPos(ImVec2(overlayPos.x, overlayPos.y + (checkboxHeight + spacing) * 3.f));
    ImGui::Checkbox("IR view", &showIRResources);
    ImGui::SetCursorScreenPos(ImVec2(overlayPos.x, overlayPos.y + (checkboxHeight + spacing) * 4.f));
    ImGui::SetNextItemWidth(overlayWidth);
    ImGui::Combo("##sizeformat", (int *)&sizeFormat, "Decimal (KB/MB/GB)\0Binary (KiB/MiB/GiB)\0Exact bytes\0Hex\0");
  }

  processInput();

  splitter.Merge(drawList);

  drawList->PopClipRect();
  ImGui::EndChild();
}

void ResourseVisualizer::processPopup()
{
  if (!showTooltips || !hoveredIRResource)
    return;

  if (!irResourceInfos.isMapped(*hoveredIRResource))
    return;
  const auto &irRes = irResourceInfos[*hoveredIRResource];
  const auto &placement = irRes.placements[hoveredResourceOwnerFrame];

  const char *name = "?";
  if (hoveredFrontendIdx >= 0 && hoveredFrontendIdx < (int)irRes.frontendResources.size())
    name = irRes.frontendResources[hoveredFrontendIdx].name ? irRes.frontendResources[hoveredFrontendIdx].name : "?";
  else if (!irRes.name.empty())
    name = irRes.name.c_str();

  char sizeBuf[32];
  format_size(sizeBuf, sizeof(sizeBuf), placement.size, sizeFormat);
  ImGui::SetTooltip("\"%s\", frame %i\nHeap %i\nOffset 0x%08X\nSize %s", name, hoveredResourceOwnerFrame, placement.heapIndex,
    placement.offset, sizeBuf);
}

void ResourseVisualizer::processInput()
{
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
      viewScrolling = viewScrolling + ImGui::GetIO().MouseDelta;

    if (ImGui::GetIO().MouseWheel != 0)
    {
      constexpr float ZOOM_FACTOR = 1.1f;
      const float zoomMul = powf(ZOOM_FACTOR, ImGui::GetIO().MouseWheel);

      const bool shiftOnly = ImGui::IsKeyDown(ImGuiKey_LeftShift) && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
      const bool ctrlOnly = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_LeftShift);
      const bool zoomVertical = !ctrlOnly;
      const bool zoomHorizontal = !shiftOnly;

      if (zoomVertical)
      {
        const float prevVertScaling = verticalScaling;
        verticalScaling = eastl::clamp(verticalScaling * zoomMul, 10.f, 100000.f);

        const float mouseContentY = ImGui::GetMousePos().y - viewOffset.y - nodeHeaderHeight;
        const float prevAnchor = mouseContentY / prevVertScaling;
        const float nextAnchor = mouseContentY / verticalScaling;
        viewScrolling.y = viewScrolling.y + verticalScaling * (nextAnchor - prevAnchor);
      }
      if (zoomHorizontal)
      {
        const float prevScaling = viewScaling;
        viewScaling = eastl::clamp(viewScaling * zoomMul, 0.1f, 10.0f);

        const float prevAnchor = (ImGui::GetMousePos().x - viewOffset.x) / prevScaling;
        const float nextAnchor = (ImGui::GetMousePos().x - viewOffset.x) / viewScaling;
        viewScrolling.x = viewScrolling.x + viewScaling * (nextAnchor - prevAnchor);
      }
    }
  }
}

void ResourseVisualizer::updateVisualization()
{
  using ResourceIndex = intermediate::ResourceIndex;

  nodeCount = intermediateGraph.nodes.totalKeys();

  nodeNames.clear();
  nodeNames.resize(nodeCount);
  for (auto [nodeIdx, irNode] : intermediateGraph.nodes.enumerate())
  {
    if (irNode.frontendNode)
      nodeNames[nodeIdx] = registry.knownNames.getName(*irNode.frontendNode);
    else
      nodeNames[nodeIdx] = "<internal>";
  }

  resourceHeapSizes.clear();
  cpuHeapFlags.clear();

  // Step 1: Compute IR resource lifetimes from the IR graph
  irResourceInfos.clear();
  irResourceInfos.resize(intermediateGraph.resources.totalKeys());

  for (auto [nodeIdx, irNode] : intermediateGraph.nodes.enumerate())
  {
    const int nodeExecIdx = eastl::to_underlying(nodeIdx);

    for (const auto &req : irNode.resourceRequests)
    {
      auto &irRes = irResourceInfos[req.resource];
      const int time = req.fromLastFrame ? nodeExecIdx + (int)nodeCount : nodeExecIdx;
      irRes.physicalLifetime.record(time);
      irRes.logicalLifetime.record(time);
    }

    for (auto resIdx : irNode.supressedRequests)
      irResourceInfos[resIdx].logicalLifetime.record(nodeExecIdx);
  }

  // Step 2: Build reverse mapping ResNameId -> ResourceIndex, process placements
  ska::flat_hash_map<ResNameId, ResourceIndex> resNameToIR;
  for (auto [resIdx, res] : intermediateGraph.resources.enumerate())
    for (auto resId : res.frontendResources)
      resNameToIR[resId] = resIdx;

  for (const auto [id, frame, heap, offset, size, is_cpu] : resourcePlacemantEntries)
  {
    if (frame >= RES_RECORD_WINDOW)
      continue;

    auto it = resNameToIR.find(id);
    if (it == resNameToIR.end())
      continue;

    auto &placement = irResourceInfos[it->second].placements[frame];
    placement.heapIndex = heap;
    placement.offset = offset;
    placement.size = size;

    if (resourceHeapSizes.size() <= heap)
    {
      resourceHeapSizes.resize(heap + 1, 0);
      cpuHeapFlags.resize(heap + 1, false);
    }
    resourceHeapSizes[HeapIndex(heap)] = eastl::max(resourceHeapSizes[HeapIndex(heap)], offset + size);
    cpuHeapFlags[HeapIndex(heap)] = is_cpu;
  }

  // Process barriers
  for (const auto [res_id, res_frame, exec_time, exec_frame, barrier] : resourceBarrierEntries)
  {
    if (res_frame >= RES_RECORD_WINDOW)
      continue;
    auto it = resNameToIR.find(res_id);
    if (it == resNameToIR.end())
      continue;
    const auto frameDelta = (exec_frame - res_frame + RES_RECORD_WINDOW) % RES_RECORD_WINDOW;
    irResourceInfos[it->second].barrierEvents.emplace_back(exec_time + frameDelta * (int)nodeCount, barrier);
  }

  // Step 3: Split IR lifetimes into frontend resource lifetimes
  auto resolve = [this](ResNameId resId) -> ResNameId { return nameResolver.resolve(resId); };

  for (auto [resIdx, irRes] : irResourceInfos.enumerate())
  {
    if (!intermediateGraph.resources.isMapped(resIdx))
      continue;
    const auto &res = intermediateGraph.resources[resIdx];
    if (res.frontendResources.empty())
      continue;

    // Build IR resource name from the rename chain
    irRes.name.clear();
    for (size_t i = 0; i < res.frontendResources.size(); ++i)
    {
      if (i > 0)
        irRes.name += " -> ";
      irRes.name += registry.knownNames.getName(res.frontendResources[i]);
    }

    // Build frontend resource entries from the rename chain
    irRes.frontendResources.resize(res.frontendResources.size());
    for (size_t i = 0; i < res.frontendResources.size(); ++i)
    {
      auto &fe = irRes.frontendResources[i];
      fe.name = registry.knownNames.getName(res.frontendResources[i]);
      if (i > 0)
        fe.createdByRename = true;
      if (i < res.frontendResources.size() - 1)
        fe.consumedByRename = true;
    }

    // Compute per-frontend logical lifetimes from the registry
    for (auto [nodeIdx, irNode] : intermediateGraph.nodes.enumerate())
    {
      if (!irNode.frontendNode)
        continue;
      const int nodeExecIdx = eastl::to_underlying(nodeIdx);
      const auto &node = registry.nodes[*irNode.frontendNode];

      for (auto resId : node.createdResources)
      {
        auto it = resNameToIR.find(resId);
        if (it == resNameToIR.end() || it->second != resIdx)
          continue;
        // Find which frontend resource this is
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resId)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx);
      }

      for (auto [to, from] : node.renamedResources)
      {
        auto resolvedFrom = resolve(from);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
        {
          if (res.frontendResources[i] == resolvedFrom)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx);
          if (res.frontendResources[i] == to)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx);
        }
      }

      for (auto resId : node.readResources)
      {
        auto resolved = resolve(resId);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resolved)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx);
      }

      for (auto resId : node.modifiedResources)
      {
        auto resolved = resolve(resId);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resolved)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx);
      }

      for (auto &[resId, _] : node.historyResourceReadRequests)
      {
        auto resolved = resolve(resId);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resolved)
            irRes.frontendResources[i].logicalLifetime.record(nodeExecIdx + (int)nodeCount);
      }
    }

    // Clamp frontend physical lifetimes to the IR resource's physical lifetime
    for (auto &fe : irRes.frontendResources)
    {
      if (fe.logicalLifetime.empty() || irRes.physicalLifetime.empty())
        continue;
      fe.physicalLifetime.firstNodeIdx = eastl::max(fe.logicalLifetime.firstNodeIdx, irRes.physicalLifetime.firstNodeIdx);
      fe.physicalLifetime.lastNodeIdx = eastl::min(fe.logicalLifetime.lastNodeIdx, irRes.physicalLifetime.lastNodeIdx);
    }

    // Step 4: Populate usage timelines
    for (auto [nodeIdx, irNode] : intermediateGraph.nodes.enumerate())
    {
      if (!irNode.frontendNode)
        continue;
      const int nodeExecIdx = eastl::to_underlying(nodeIdx);
      const auto &node = registry.nodes[*irNode.frontendNode];

      for (const auto &[resId, req] : node.resourceRequests)
      {
        auto resolved = resolve(resId);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resolved)
            irRes.frontendResources[i].usageTimeline[nodeExecIdx] = req.usage;
      }

      for (const auto &[resId, req] : node.historyResourceReadRequests)
      {
        auto resolved = resolve(resId);
        for (size_t i = 0; i < res.frontendResources.size(); ++i)
          if (res.frontendResources[i] == resolved)
            irRes.frontendResources[i].usageTimeline[nodeExecIdx + (int)nodeCount] = req.usage;
      }
    }
  }

  // Build heap -> resource index lookup
  heapToResources.clear();
  heapToResources.resize(resourceHeapSizes.size());
  for (auto [resIdx, irRes] : irResourceInfos.enumerate())
    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
      if (irRes.placements[frame].heapIndex >= 0)
      {
        auto &list = heapToResources[HeapIndex(irRes.placements[frame].heapIndex)];
        if (list.empty() || list.back() != resIdx)
          list.push_back(resIdx);
      }

  // Compute which nodes are "interesting" (have a resource starting or ending), separately for GPU and CPU
  auto initInteresting = [&](InterestingNodes &n) {
    n.physical.assign(nodeCount, false);
    n.physicalBarriers.assign(nodeCount, false);
    n.logical.assign(nodeCount, false);
    n.logicalBarriers.assign(nodeCount, false);
    n.ir.assign(nodeCount, false);
    n.irBarriers.assign(nodeCount, false);
  };
  initInteresting(gpuInteresting);
  initInteresting(cpuInteresting);

  for (auto [resIdx, irRes] : irResourceInfos.enumerate())
  {
    bool isCpu = false;
    bool hasPlacement = false;
    for (int f = 0; f < RES_RECORD_WINDOW; ++f)
    {
      int hi = irRes.placements[f].heapIndex;
      if (hi >= 0)
      {
        hasPlacement = true;
        if (cpuHeapFlags.isMapped(HeapIndex(hi)) && cpuHeapFlags[HeapIndex(hi)])
          isCpu = true;
      }
    }
    if (!hasPlacement)
      continue;
    auto &interesting = isCpu ? cpuInteresting : gpuInteresting;

    for (const auto &fe : irRes.frontendResources)
    {
      if (!fe.physicalLifetime.empty())
      {
        G_ASSERT(fe.physicalLifetime.firstNodeIdx < (int)nodeCount);
        int first = fe.physicalLifetime.firstNodeIdx;
        int last = fe.physicalLifetime.lastNodeIdx % (int)nodeCount;
        interesting.physical[first] = true;
        interesting.physical[last] = true;
        interesting.physicalBarriers[first] = true;
        interesting.physicalBarriers[last] = true;
        // Logical is a superset of physical -- resource rects always use physical lifetimes
        interesting.logical[first] = true;
        interesting.logical[last] = true;
        interesting.logicalBarriers[first] = true;
        interesting.logicalBarriers[last] = true;
      }
      if (!fe.logicalLifetime.empty())
      {
        int first = fe.logicalLifetime.firstNodeIdx;
        int last = fe.logicalLifetime.lastNodeIdx % (int)nodeCount;
        interesting.logical[first] = true;
        interesting.logical[last] = true;
        interesting.logicalBarriers[first] = true;
        interesting.logicalBarriers[last] = true;
      }
    }

    // IR-level lifetimes (one per IR resource, no rename splits)
    if (!irRes.physicalLifetime.empty())
    {
      int first = irRes.physicalLifetime.firstNodeIdx;
      int last = irRes.physicalLifetime.lastNodeIdx % (int)nodeCount;
      interesting.ir[first] = true;
      interesting.ir[last] = true;
      interesting.irBarriers[first] = true;
      interesting.irBarriers[last] = true;
    }

    for (auto [time, barrier] : irRes.barrierEvents)
    {
      int t = time % (int)nodeCount;
      interesting.physicalBarriers[t] = true;
      interesting.logicalBarriers[t] = true;
      interesting.irBarriers[t] = true;
    }
  }

  // Force layout rebuild on next draw
  layoutIsCpu = false;
  layoutCompact = false;
  layoutUsages = false;
  layoutBarriers = false;
  currentLayout = ColumnLayout();
}

void ResourseVisualizer::clearResourcePlacements() { resourcePlacemantEntries.clear(); }

void ResourseVisualizer::recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size, bool is_cpu)
{
  resourcePlacemantEntries.push_back({id, frame, heap, offset, size, is_cpu});
}

void ResourseVisualizer::clearResourceBarriers() { resourceBarrierEntries.clear(); }

void ResourseVisualizer::recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  resourceBarrierEntries.push_back({res_id, res_frame, exec_time, exec_frame, barrier});
}

} // namespace visualization

} // namespace dafg