// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pbrValidation_api.h"

#if DAGOR_DBGLEVEL > 0

#include "shaders/debug_pbr_validation.hlsli"
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <EASTL/array.h>
#include <debug/dag_gbufferColorLegends.h>
#include <debug/dag_textMarks.h>
#include <math/dag_color.h>
#include <math/dag_mathUtils.h>
#include <util/dag_convar.h>
#include <util/dag_globDef.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <stdio.h>

namespace render::pbr_validation
{
CONSOLE_BOOL_VAL("render", show_pbr_validation, false);

static Settings g_settings;
static int mode_var_id = -1;
static int albedo_range_var_id = -1;
static int smoothness_range_var_id = -1;
static int overlay_opacity_var_id = -1;
static int neutral_base_var_id = -1;
static int neutral_base_level_var_id = -1;
static int crosshair_var_id = -1;
static bool vars_resolved = false;

static constexpr int PROBE_UPDATE_INTERVAL_FRAMES = 15;
static constexpr int PROBE_FLOAT_COUNT = PBR_PROBE_SLOT_COUNT * 4;

static constexpr float PROBE_SWATCH_SIZE = 24.0f;
static constexpr float PROBE_TEXT_GAP = 8.0f;
static constexpr float PROBE_FIRST_LINE_OFS = 4.0f;
static constexpr float PROBE_LINE_OFS_STEP = 2.0f;

static const E3DCOLOR MARK_DEFAULT_FRAME_COLOR = E3DCOLOR_MAKE(0, 0, 0, 160);
static const E3DCOLOR NO_DATA_FRAME_COLOR = E3DCOLOR_MAKE(160, 160, 160, 255);
static const E3DCOLOR STATUS_PASS_FRAME_COLOR = E3DCOLOR_MAKE(50, 255, 50, 255);
static const E3DCOLOR STATUS_FAIL_FRAME_COLOR = E3DCOLOR_MAKE(255, 50, 50, 255);
static const E3DCOLOR STATUS_WARN_FRAME_COLOR = E3DCOLOR_MAKE(255, 255, 0, 255);
static constexpr float COLOR_TO_BYTE = 255.0f;


struct ProbeData
{
  Color3 albedoLinear = {0, 0, 0};
  float ao = 0;
  float metalness = 0;
  float smoothness = 0;
  float reflectance = 0;
  int material = 0;
  bool valid = false;
};

static ProbeData g_probe;
static UniqueBuf g_probe_buf;
static int probe_output_var_id = -1;
static int probe_write_var_id = -1;
static int probe_frame_counter = 0;
static d3d::EventQuery *g_probe_query = nullptr;
static bool probe_write_pending = false;

static constexpr int HIST_BINS = 256;
static constexpr int HIST_UPDATE_INTERVAL_FRAMES = 15;
static constexpr int HIST_SKY_LUMA_SUM_MAX = 30;
static constexpr int HIST_SAMPLE_GRID_W = 320;
static constexpr int HIST_SAMPLE_GRID_H = 180;
static constexpr int HIST_MIN_ALBEDO_BYTES = 3;
static constexpr int HIST_DEPTH_BYTES_MAX = 4;

static constexpr float HIST_PLOT_HEIGHT = 120.0f;
static constexpr float HIST_PLOT_INNER_PAD = 4.0f;
static constexpr float HIST_PLOT_BOTTOM_PAD = 16.0f;
static constexpr float HIST_PLOT_ROUNDING = 2.0f;
static constexpr float HIST_LEGEND_WIDTH = 120.0f;
static constexpr ImU32 HIST_BG_COLOR = IM_COL32(0, 0, 0, 40);
static constexpr ImU32 HIST_R_COLOR = IM_COL32(255, 50, 50, 140);
static constexpr ImU32 HIST_G_COLOR = IM_COL32(50, 255, 50, 140);
static constexpr ImU32 HIST_B_COLOR = IM_COL32(70, 70, 255, 140);
static constexpr ImU32 HIST_LUM_COLOR = IM_COL32(255, 255, 255, 200);
static constexpr ImU32 HIST_AXIS_COLOR = IM_COL32(120, 120, 120, 180);
static constexpr ImU32 HIST_NOTE_COLOR = IM_COL32(120, 120, 120, 150);

struct HistogramData
{
  eastl::array<float, HIST_BINS> r = {};
  eastl::array<float, HIST_BINS> g = {};
  eastl::array<float, HIST_BINS> b = {};
  eastl::array<float, HIST_BINS> lum = {};
};

static HistogramData g_hist;
static bool histogram_enabled = false; // opt-in, the readback can stall the frame
static bool crosshair_enabled = false;
static int gbuf_albedo_w = 0, gbuf_albedo_h = 0;
static int gbuf_depth_w = 0, gbuf_depth_h = 0;
static IPoint2 display_res = {0, 0};
static bool hist_valid = false;
static int hist_frame_counter = 0;
static d3d::EventQuery *g_hist_query = nullptr;
static bool hist_read_pending = false;

static bool is_depth_zero(const uint8_t *depth_ptr, int bpp)
{
  for (int i = 0; i < bpp && i < HIST_DEPTH_BYTES_MAX; i++)
    if (depth_ptr[i] != 0)
      return false;
  return true;
}

static float luminance_bt709(const Color3 &c) { return PBR_LUMA_BT709_R * c.r + PBR_LUMA_BT709_G * c.g + PBR_LUMA_BT709_B * c.b; }

static float linear_to_srgb(float v) { return powf(v, 1.0f / PBR_SRGB_GAMMA); }

static const char *material_type_name(int m)
{
  switch (m)
  {
    case SHADING_NORMAL: return "Ordinary material";
    case SHADING_SUBSURFACE: return "Subsurface";
    case SHADING_FOLIAGE: return "Foliage";
    case SHADING_SELFILLUM: return "Selfillum";
    default: return "Unknown";
  }
}

Settings &settings() { return g_settings; }

bool is_enabled() { return show_pbr_validation.get(); }
void set_enabled(bool on) { show_pbr_validation.set(on); }

void apply_settings()
{
  if (!vars_resolved)
  {
    mode_var_id = get_shader_variable_id("pbr_validation_mode", true);
    albedo_range_var_id = get_shader_variable_id("pbr_albedo_range", true);
    smoothness_range_var_id = get_shader_variable_id("pbr_smoothness_range", true);
    overlay_opacity_var_id = get_shader_variable_id("pbr_overlay_opacity", true);
    neutral_base_var_id = get_shader_variable_id("pbr_neutral_base", true);
    neutral_base_level_var_id = get_shader_variable_id("pbr_neutral_base_level", true);
    crosshair_var_id = get_shader_variable_id("pbr_crosshair", true);
    vars_resolved = true;
  }
  ShaderGlobal::set_int(mode_var_id, g_settings.mode);
  ShaderGlobal::set_float4(albedo_range_var_id, g_settings.nonMetalMin, g_settings.nonMetalMax, g_settings.metalMin,
    g_settings.metalMax);
  ShaderGlobal::set_float4(smoothness_range_var_id, g_settings.smoothnessMin, g_settings.smoothnessMax, 0.0f, 0.0f);
  ShaderGlobal::set_float(overlay_opacity_var_id, g_settings.overlayOpacity);
  ShaderGlobal::set_int(neutral_base_var_id, g_settings.neutralBase ? 1 : 0);
  ShaderGlobal::set_float(neutral_base_level_var_id, g_settings.neutralBaseLevel);
  ShaderGlobal::set_int(crosshair_var_id, crosshair_enabled ? 1 : 0);
}

bool bind_probe()
{
  if (!g_probe_buf)
  {
    probe_output_var_id = get_shader_variable_id("pbr_probe_output", true);
    probe_write_var_id = get_shader_variable_id("pbr_write_probe", true);
    g_probe_buf = dag::create_sbuffer(sizeof(float) * 4, PBR_PROBE_SLOT_COUNT, SBCF_UA_STRUCTURED_READBACK, 0, "pbr_probe_output");
  }
  if (!g_probe_buf || probe_output_var_id < 0)
    return false;
  ShaderGlobal::set_buffer(probe_output_var_id, g_probe_buf);
  // pause GPU probe writes while a readback is in flight, the fenced buffer must hold one frame
  ShaderGlobal::set_int(probe_write_var_id, probe_write_pending ? 0 : 1);
  return true;
}

static void read_probe_from_buffer()
{
  float *mapped = nullptr;
  if (!g_probe_buf->lock(0, sizeof(float) * PROBE_FLOAT_COUNT, (void **)&mapped, VBLOCK_READONLY))
  {
    g_probe.valid = false;
    return;
  }
  const float *albedoAo = mapped ? mapped + PBR_PROBE_SLOT_ALBEDO_AO * 4 : nullptr;
  const float *params = mapped ? mapped + PBR_PROBE_SLOT_PARAMS * 4 : nullptr;
  const float *flags = mapped ? mapped + PBR_PROBE_SLOT_FLAGS * 4 : nullptr;
  if (!mapped || flags[0] < PBR_PROBE_VALID_FLAG_MIN)
    g_probe.valid = false;
  else
  {
    g_probe.albedoLinear = Color3(albedoAo[0], albedoAo[1], albedoAo[2]);
    g_probe.ao = albedoAo[3];
    g_probe.smoothness = params[0];
    g_probe.metalness = params[1];
    g_probe.reflectance = params[2];
    g_probe.material = (int)params[3];
    g_probe.valid = true;
  }
  g_probe_buf->unlock();
}

static const char *evaluate_pbr_status()
{
  if (!g_probe.valid)
    return "No data";
  const bool doAlbedo = g_settings.mode == PBR_VALIDATION_MODE_ALBEDO || g_settings.mode == PBR_VALIDATION_MODE_ALL;
  const bool doSmoothness = g_settings.mode == PBR_VALIDATION_MODE_SMOOTHNESS || g_settings.mode == PBR_VALIDATION_MODE_ALL;
  // mirror the shader: highest severity wins so the status matches the overlay color
  float severity = 0.0f;
  const char *status = "PASS";
  if (doAlbedo)
  {
    float lum = luminance_bt709(g_probe.albedoLinear);
    bool isMetal = g_probe.metalness > PBR_METAL_METALNESS_MIN && g_probe.material == SHADING_NORMAL;
    float sev = 0.0f;
    const char *msg = nullptr;
    if (isMetal)
    {
      if (lum < g_settings.metalMin)
      {
        sev = clamp((g_settings.metalMin - lum) / g_settings.metalMin, 0.0f, 1.0f);
        msg = "FAIL: Metal albedo too dark";
      }
      else if (lum > g_settings.metalMax)
      {
        sev = clamp((lum - g_settings.metalMax) / (1.0f - g_settings.metalMax), 0.0f, 1.0f);
        msg = "FAIL: Metal albedo too bright";
      }
    }
    else if (lum < g_settings.nonMetalMin)
    {
      sev = clamp((g_settings.nonMetalMin - lum) / g_settings.nonMetalMin, 0.0f, 1.0f);
      msg = "FAIL: Albedo too dark";
    }
    else if (lum > g_settings.nonMetalMax)
    {
      sev = clamp((lum - g_settings.nonMetalMax) / (1.0f - g_settings.nonMetalMax), 0.0f, 1.0f);
      msg = "FAIL: Albedo too bright";
    }
    if (msg && sev > severity)
    {
      severity = sev;
      status = msg;
    }
  }
  if (doSmoothness)
  {
    float sev = 0.0f;
    const char *msg = nullptr;
    if (g_probe.smoothness >= g_settings.smoothnessMax)
    {
      sev = clamp((g_probe.smoothness - g_settings.smoothnessMax) / (1.0f - g_settings.smoothnessMax), 0.0f, 1.0f);
      msg = "WARN: Smoothness too high (near 1.0)";
    }
    else if (g_probe.smoothness <= g_settings.smoothnessMin)
    {
      sev = clamp((g_settings.smoothnessMin - g_probe.smoothness) / g_settings.smoothnessMin, 0.0f, 1.0f);
      msg = "WARN: Smoothness too low (near 0.0)";
    }
    if (msg && sev > severity)
    {
      severity = sev;
      status = msg;
    }
  }
  return status;
}

static void draw_viewport_probe()
{
  if (!crosshair_enabled)
    return;

  if (display_res.x <= 0 || display_res.y <= 0)
    return;

  float cx = display_res.x / 2;
  float cy = display_res.y / 2;

  float line = PROBE_FIRST_LINE_OFS;
  if (!g_probe.valid)
  {
    add_debug_text_mark(cx, cy, "No data (no surface at center)", -1, line, NO_DATA_FRAME_COLOR);
    return;
  }

  float sR = linear_to_srgb(g_probe.albedoLinear.r);
  float sG = linear_to_srgb(g_probe.albedoLinear.g);
  float sB = linear_to_srgb(g_probe.albedoLinear.b);
  float lumLinear = luminance_bt709(g_probe.albedoLinear);

  const char *status = evaluate_pbr_status();
  E3DCOLOR statusColor = STATUS_PASS_FRAME_COLOR;
  if (status[0] == 'F')
    statusColor = STATUS_FAIL_FRAME_COLOR;
  else if (status[0] == 'W')
    statusColor = STATUS_WARN_FRAME_COLOR;

  add_debug_text_mark(cx, cy, status, -1, line, statusColor);
  line += PROBE_LINE_OFS_STEP;

  add_debug_text_mark(cx, cy, material_type_name(g_probe.material), -1, line, MARK_DEFAULT_FRAME_COLOR);
  line += PROBE_LINE_OFS_STEP;

  char buf[128];

  snprintf(buf, sizeof(buf), "Lin: %.3f %.3f %.3f", g_probe.albedoLinear.r, g_probe.albedoLinear.g, g_probe.albedoLinear.b);
  add_debug_text_mark(cx, cy, buf, -1, line, MARK_DEFAULT_FRAME_COLOR);
  line += PROBE_LINE_OFS_STEP;

  snprintf(buf, sizeof(buf), "sRGB: %.0f %.0f %.0f", sR * COLOR_TO_BYTE, sG * COLOR_TO_BYTE, sB * COLOR_TO_BYTE);
  add_debug_text_mark(cx, cy, buf, -1, line,
    E3DCOLOR_MAKE((int)(sR * COLOR_TO_BYTE), (int)(sG * COLOR_TO_BYTE), (int)(sB * COLOR_TO_BYTE), 255));
  line += PROBE_LINE_OFS_STEP;

  snprintf(buf, sizeof(buf), "Lum: %.3f (sRGB: %.0f)", lumLinear, linear_to_srgb(lumLinear) * COLOR_TO_BYTE);
  add_debug_text_mark(cx, cy, buf, -1, line, MARK_DEFAULT_FRAME_COLOR);
  line += PROBE_LINE_OFS_STEP;

  if (g_probe.material == SHADING_NORMAL && g_probe.metalness > 0.0f)
    snprintf(buf, sizeof(buf), "Met: %.2f  Smooth: %.2f", g_probe.metalness, g_probe.smoothness);
  else
    snprintf(buf, sizeof(buf), "Met: -     Smooth: %.2f", g_probe.smoothness);
  add_debug_text_mark(cx, cy, buf, -1, line, MARK_DEFAULT_FRAME_COLOR);
  line += PROBE_LINE_OFS_STEP;

  snprintf(buf, sizeof(buf), "Refl: %.3f  AO: %.2f", g_probe.reflectance, g_probe.ao);
  add_debug_text_mark(cx, cy, buf, -1, line, MARK_DEFAULT_FRAME_COLOR);
}

static void render_legend()
{
  static const Tab<eastl::pair<const char *, E3DCOLOR>> legend = {
    {"albedo too dark (non-metal)",
      E3DCOLOR_MAKE((int)(PBR_COLOR_ALBEDO_DARK_R * COLOR_TO_BYTE), (int)(PBR_COLOR_ALBEDO_DARK_G * COLOR_TO_BYTE),
        (int)(PBR_COLOR_ALBEDO_DARK_B * COLOR_TO_BYTE), 255)},
    {"albedo too bright", E3DCOLOR_MAKE((int)(PBR_COLOR_ALBEDO_BRIGHT_R * COLOR_TO_BYTE),
                            (int)(PBR_COLOR_ALBEDO_BRIGHT_G * COLOR_TO_BYTE), (int)(PBR_COLOR_ALBEDO_BRIGHT_B * COLOR_TO_BYTE), 255)},
    {"metal albedo too dark", E3DCOLOR_MAKE((int)(PBR_COLOR_METAL_LOW_R * COLOR_TO_BYTE), (int)(PBR_COLOR_METAL_LOW_G * COLOR_TO_BYTE),
                                (int)(PBR_COLOR_METAL_LOW_B * COLOR_TO_BYTE), 255)},
    {"smoothness too high", E3DCOLOR_MAKE((int)(PBR_COLOR_SMOOTH_HIGH_R * COLOR_TO_BYTE),
                              (int)(PBR_COLOR_SMOOTH_HIGH_G * COLOR_TO_BYTE), (int)(PBR_COLOR_SMOOTH_HIGH_B * COLOR_TO_BYTE), 255)},
    {"smoothness too low", E3DCOLOR_MAKE((int)(PBR_COLOR_SMOOTH_LOW_R * COLOR_TO_BYTE), (int)(PBR_COLOR_SMOOTH_LOW_G * COLOR_TO_BYTE),
                             (int)(PBR_COLOR_SMOOTH_LOW_B * COLOR_TO_BYTE), 255)},
    {"NaN in gbuffer (checker)", E3DCOLOR_MAKE(255, 255, 255, 255)},
  };
  gbuffer_debug_color_legends::renderColorLegend(legend);
}

static void compute_histogram(BaseTexture *albedo_tex, BaseTexture *depth_tex)
{
  if (!histogram_enabled || !albedo_tex)
  {
    hist_valid = false;
    hist_read_pending = false;
    return;
  }

  // fence a frame, run the blocking lockimg only once the GPU finished, skip the sample otherwise
  if (!hist_read_pending)
  {
    if (hist_frame_counter++ < HIST_UPDATE_INTERVAL_FRAMES)
      return;
    hist_frame_counter = 0;
    if (!g_hist_query)
      g_hist_query = d3d::create_event_query();
    if (g_hist_query && d3d::issue_event_query(g_hist_query))
      hist_read_pending = true;
    return;
  }
  if (g_hist_query && !d3d::get_event_query_status(g_hist_query, false))
    return;
  hist_read_pending = false;
  hist_valid = false;

  HistogramData hist;

  int aStride = 0;
  uint8_t *aData = nullptr;
  if (!albedo_tex->lockimg((void **)&aData, aStride, 0, TEXLOCK_READ))
    return;
  if (!aData || aStride <= 0)
  {
    albedo_tex->unlockimg();
    return;
  }

  TextureInfo ti;
  albedo_tex->getinfo(ti);
  // texel size from the format, the lock stride is row pitch and may be padded
  int aBpp = get_tex_format_desc(ti.cflg).bytesPerElement;
  if (aBpp < HIST_MIN_ALBEDO_BYTES || aStride < (int)ti.w * aBpp)
  {
    albedo_tex->unlockimg();
    return;
  }

  int dStride = 0;
  uint8_t *dData = nullptr;
  int dBpp = 0;
  bool hasDepth = false;
  if (depth_tex && depth_tex->lockimg((void **)&dData, dStride, 0, TEXLOCK_READ))
  {
    TextureInfo dti;
    depth_tex->getinfo(dti);
    if (dData && dStride > 0 && dti.w == ti.w && dti.h == ti.h)
    {
      dBpp = get_tex_format_desc(dti.cflg).bytesPerElement;
      hasDepth = dStride >= (int)dti.w * dBpp;
    }
    if (!hasDepth)
    {
      depth_tex->unlockimg();
      dData = nullptr;
    }
  }

  int stepX = max(1, (int)ti.w / HIST_SAMPLE_GRID_W);
  int stepY = max(1, (int)ti.h / HIST_SAMPLE_GRID_H);

  for (uint32_t y = 0; y < ti.h; y += stepY)
  {
    const uint8_t *aRow = aData + y * aStride;
    const uint8_t *dRow = hasDepth ? dData + y * dStride : nullptr;
    for (uint32_t x = 0; x < ti.w; x += stepX)
    {
      if (dRow && is_depth_zero(dRow + x * dBpp, dBpp))
        continue;
      const uint8_t *p = aRow + x * aBpp;
      // ALBEDO_AO rt is TEXFMT_DEFAULT, BGRA in memory
      uint8_t b = p[0], g = p[1], r = p[2];
      // luminance sky filter only without depth: dark geometry must stay visible
      if (!hasDepth && r + g + b <= HIST_SKY_LUMA_SUM_MAX)
        continue;
      hist.r[r]++;
      hist.g[g]++;
      hist.b[b]++;
      int lumBin = clamp((int)(PBR_LUMA_BT709_R * r + PBR_LUMA_BT709_G * g + PBR_LUMA_BT709_B * b), 0, HIST_BINS - 1);
      hist.lum[lumBin]++;
    }
  }

  albedo_tex->unlockimg();
  if (hasDepth)
    depth_tex->unlockimg();

  g_hist = hist;
  hist_valid = true;
}

static void draw_histogram()
{
  if (!hist_valid)
  {
    ImGui::TextDisabled("Histogram computing...");
    return;
  }

  float availW = ImGui::GetContentRegionAvail().x;
  float plotW = availW - HIST_PLOT_INNER_PAD * 2;

  ImVec2 cursor = ImGui::GetCursorScreenPos();
  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(cursor, ImVec2(cursor.x + availW, cursor.y + HIST_PLOT_HEIGHT), HIST_BG_COLOR, HIST_PLOT_ROUNDING);

  // skip bins 0 and 255 for the normalization peak, they often spike
  float maxVal = 1.0f;
  for (int i = 1; i < HIST_BINS - 1; i++)
  {
    maxVal = max(maxVal, g_hist.r[i]);
    maxVal = max(maxVal, g_hist.g[i]);
    maxVal = max(maxVal, g_hist.b[i]);
    maxVal = max(maxVal, g_hist.lum[i]);
  }

  struct ChannelStyle
  {
    const eastl::array<float, HIST_BINS> &data;
    ImU32 col;
  };
  ChannelStyle channels[] = {
    {g_hist.r, HIST_R_COLOR},
    {g_hist.g, HIST_G_COLOR},
    {g_hist.b, HIST_B_COLOR},
    {g_hist.lum, HIST_LUM_COLOR},
  };

  float plotLeft = cursor.x + HIST_PLOT_INNER_PAD;
  float plotBottom = cursor.y + HIST_PLOT_HEIGHT - HIST_PLOT_ROUNDING;

  for (const auto &ch : channels)
  {
    ImVec2 prev;
    for (int i = 0; i < HIST_BINS; i++)
    {
      float x = plotLeft + (float)i / (HIST_BINS - 1) * plotW;
      float h = (ch.data[i] / maxVal) * (HIST_PLOT_HEIGHT - HIST_PLOT_INNER_PAD);
      ImVec2 cur(x, plotBottom - h);
      if (i > 0)
        dl->AddLine(prev, cur, ch.col, 1.0f);
      prev = cur;
    }
  }

  float axisY = cursor.y + HIST_PLOT_HEIGHT + HIST_PLOT_ROUNDING;
  const struct
  {
    float pos;
    const char *label;
  } axisMarks[] = {{0.0f, "0"}, {0.25f, "64"}, {0.5f, "128"}, {0.75f, "192"}, {1.0f, "255"}};
  for (const auto &mark : axisMarks)
    dl->AddText(ImVec2(plotLeft + plotW * mark.pos - (mark.pos > 0.0f ? PROBE_TEXT_GAP : 0.0f), axisY), HIST_AXIS_COLOR, mark.label);

  float legY = cursor.y + HIST_PLOT_ROUNDING;
  float legX = plotLeft + plotW - HIST_LEGEND_WIDTH;
  dl->AddText(ImVec2(legX, legY), HIST_LUM_COLOR, "Lum");
  dl->AddText(ImVec2(legX + PROBE_SWATCH_SIZE + PROBE_TEXT_GAP, legY), HIST_R_COLOR, "R");
  dl->AddText(ImVec2(legX + PROBE_SWATCH_SIZE * 2, legY), HIST_G_COLOR, "G");
  dl->AddText(ImVec2(legX + PROBE_SWATCH_SIZE * 2 + PROBE_TEXT_GAP * 2, legY), HIST_B_COLOR, "B");
  dl->AddText(ImVec2(plotLeft, legY), HIST_NOTE_COLOR, "no sky");

  ImGui::Dummy(ImVec2(availW, HIST_PLOT_HEIGHT + HIST_PLOT_BOTTOM_PAD));
}

void after_render(BaseTexture *albedo_tex, BaseTexture *depth_tex, const IPoint2 &display_resolution)
{
  display_res = display_resolution;

  TextureInfo ti;
  if (albedo_tex && albedo_tex->getinfo(ti))
  {
    gbuf_albedo_w = ti.w;
    gbuf_albedo_h = ti.h;
  }
  if (depth_tex && depth_tex->getinfo(ti))
  {
    gbuf_depth_w = ti.w;
    gbuf_depth_h = ti.h;
  }

  compute_histogram(albedo_tex, depth_tex);
  // lock only after the GPU signalled the fenced frame, never stall the render thread
  if (probe_write_pending && g_probe_query && d3d::get_event_query_status(g_probe_query, false))
  {
    read_probe_from_buffer();
    probe_write_pending = false;
  }
  if (g_probe_buf && !probe_write_pending && ++probe_frame_counter >= PROBE_UPDATE_INTERVAL_FRAMES)
  {
    probe_frame_counter = 0;
    if (!g_probe_query)
      g_probe_query = d3d::create_event_query();
    if (g_probe_query && d3d::issue_event_query(g_probe_query))
      probe_write_pending = true;
  }
  draw_viewport_probe();
  render_legend();
}

void close()
{
  if (probe_output_var_id >= 0)
    ShaderGlobal::set_buffer(probe_output_var_id, BAD_D3DRESID);
  g_probe_buf.close();
  if (g_probe_query)
  {
    d3d::release_event_query(g_probe_query);
    g_probe_query = nullptr;
  }
  probe_write_pending = false;
  probe_frame_counter = 0;
  probe_output_var_id = -1;
  probe_write_var_id = -1;
  g_probe = ProbeData{};
  if (g_hist_query)
  {
    d3d::release_event_query(g_hist_query);
    g_hist_query = nullptr;
  }
  hist_read_pending = false;
  hist_valid = false;
  hist_frame_counter = 0;
  mode_var_id = -1;
  albedo_range_var_id = -1;
  smoothness_range_var_id = -1;
  overlay_opacity_var_id = -1;
  neutral_base_var_id = -1;
  neutral_base_level_var_id = -1;
  crosshair_var_id = -1;
  vars_resolved = false;
}

static void after_device_reset(bool) { close(); }
REGISTER_D3D_AFTER_RESET_FUNC(after_device_reset);

static void imgui_window()
{
  bool enabled = is_enabled();
  if (ImGui::Checkbox("Enable PBR validation overlay", &enabled))
    set_enabled(enabled);
  if (!enabled)
  {
    ImGui::TextDisabled("Enable the overlay to see validation results");
    return;
  }

  ImGui::Separator();

  const char *modeNames[] = {"Albedo", "Smoothness", "All checks"};
  ImGui::Combo("Validation mode", &g_settings.mode, modeNames, (int)countof(modeNames));
  ImGui::SliderFloat("Overlay opacity", &g_settings.overlayOpacity, 0.0f, 1.0f, "%.2f");

  ImGui::Text("display %dx%d  albedo %dx%d  depth %dx%d", display_res.x, display_res.y, gbuf_albedo_w, gbuf_albedo_h, gbuf_depth_w,
    gbuf_depth_h);

  ImGui::Separator();
  ImGui::Checkbox("Crosshair probe readout", &crosshair_enabled);
  ImGui::Checkbox("Darken base color on passing pixels", &g_settings.neutralBase);
  if (g_settings.neutralBase)
    ImGui::SliderFloat("Base brightness", &g_settings.neutralBaseLevel, 0.0f, 1.0f, "%.2f");

  ImGui::Separator();
  ImGui::Checkbox("Histogram (synchronous CPU readback, may cost perf)", &histogram_enabled);
  if (histogram_enabled)
    draw_histogram();

  ImGui::SliderFloat("Non-metal min", &g_settings.nonMetalMin, 0.01f, 1.0f, "%.3f");
  ImGui::SliderFloat("Non-metal max", &g_settings.nonMetalMax, 0.0f, 0.99f, "%.3f");
  ImGui::SliderFloat("Metal min", &g_settings.metalMin, 0.01f, 1.0f, "%.3f");
  ImGui::SliderFloat("Metal max", &g_settings.metalMax, 0.0f, 0.99f, "%.3f");

  ImGui::SliderFloat("Smoothness min", &g_settings.smoothnessMin, 0.01f, 1.0f, "%.3f");
  ImGui::SliderFloat("Smoothness max", &g_settings.smoothnessMax, 0.0f, 0.99f, "%.3f");

  if (ImGui::TreeNode("Albedo reference values (linear luminance)"))
  {
    ImGui::BulletText("Non-metal: 60-240 sRGB (0.04-0.8 linear)");
    ImGui::BulletText("Metal: 180-250 sRGB (0.5-0.98 linear)");
    ImGui::BulletText("Iron: 198,198,200  Gold: 255,226,155");
    ImGui::BulletText("Copper: 250,208,192  Aluminium: 245,246,246");
    ImGui::BulletText("Silver: 252,250,245  Titanium: 195,186,177");
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Color legend", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::ColorButton("##dark", ImVec4(PBR_COLOR_ALBEDO_DARK_R, PBR_COLOR_ALBEDO_DARK_G, PBR_COLOR_ALBEDO_DARK_B, 1.0f));
    ImGui::SameLine();
    ImGui::Text("Albedo too dark (non-metal)");
    ImGui::ColorButton("##bright", ImVec4(PBR_COLOR_ALBEDO_BRIGHT_R, PBR_COLOR_ALBEDO_BRIGHT_G, PBR_COLOR_ALBEDO_BRIGHT_B, 1.0f));
    ImGui::SameLine();
    ImGui::Text("Albedo too bright");
    ImGui::ColorButton("##metallow", ImVec4(PBR_COLOR_METAL_LOW_R, PBR_COLOR_METAL_LOW_G, PBR_COLOR_METAL_LOW_B, 1.0f));
    ImGui::SameLine();
    ImGui::Text("Metal albedo too dark");
    ImGui::ColorButton("##smoothhigh", ImVec4(PBR_COLOR_SMOOTH_HIGH_R, PBR_COLOR_SMOOTH_HIGH_G, PBR_COLOR_SMOOTH_HIGH_B, 1.0f));
    ImGui::SameLine();
    ImGui::Text("Smoothness too high");
    ImGui::ColorButton("##smoothlow", ImVec4(PBR_COLOR_SMOOTH_LOW_R, PBR_COLOR_SMOOTH_LOW_G, PBR_COLOR_SMOOTH_LOW_B, 1.0f));
    ImGui::SameLine();
    ImGui::Text("Smoothness too low");
    ImGui::Text("NaN in gbuffer (checker)");
    ImGui::TreePop();
  }
}

REGISTER_IMGUI_WINDOW("Debug", "PBR Validation", imgui_window);
} // namespace render::pbr_validation

#else

namespace render::pbr_validation
{
static Settings g_settings;
Settings &settings() { return g_settings; }
bool is_enabled() { return false; }
void set_enabled(bool) {}
void apply_settings() {}
bool bind_probe() { return true; }
void after_render(BaseTexture *, BaseTexture *, const IPoint2 &) {}
void close() {}
} // namespace render::pbr_validation

#endif
