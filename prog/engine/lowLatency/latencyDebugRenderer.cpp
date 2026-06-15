// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lowLatency.h>
#include <gui/dag_imgui.h>
#include <gui/dag_stdGuiRender.h>
#include <imgui/implot.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVar.h>
#include <util/dag_convar.h>
#include <EASTL/vector.h>

CONSOLE_BOOL_VAL("latency", debug_latency, false);
CONSOLE_INT_VAL("latency", display_refresh_time, 500, 1, 10000);
CONSOLE_INT_VAL("latency", display_max_frame_count, 32, 1, 64);

static lowlatency::LatencyData latency_data;
static uint32_t latency_display_time = 0;

static void new_line(Point2 &pos, const E3DCOLOR &color)
{
  StdGuiRender::goto_xy(pos);
  StdGuiRender::set_color(color);
  pos.y += 12;
};

static void display_value(Point2 &pos, const E3DCOLOR &color, const char *str, const lowlatency::Statistics &value)
{
  new_line(pos, color);
  StdGuiRender::draw_str(String(0, "%s: min=%3.3f max=%3.3f avg=%3.3f", str, value.min, value.max, value.avg));
};

// a,b: normalized values
static void display_interval(Point2 &pos, const E3DCOLOR &color, const char *s, float a, float b)
{
  constexpr uint32_t len = 50;
  char str[len + 1];
  str[len] = 0;
  str[0] = ':';
  for (uint32_t i = 1; i < len; ++i)
    str[i] = ' ';
  Point2 p = pos;
  new_line(pos, color);
  int aPos = static_cast<int>((len - 2) * a) + 1;
  if (aPos >= 1 && aPos <= len)
    str[aPos] = '(';
  int bPos = static_cast<int>((len - 2) * b) + 1;
  if (bPos >= 1 && bPos <= len)
    str[bPos] = aPos == bPos ? '|' : ')';
  StdGuiRender::draw_str(s);
  StdGuiRender::goto_xy(Point2(p.x + 100, p.y));
  StdGuiRender::draw_str(str);
};

static float discard_invalid_value(float val) { return val > 10 * 16.667f ? 0 : val; };

namespace
{
struct ScrollingBuffer
{
  int maxSize = 2000;
  int offset = 0;
  eastl::vector<float> xData;
  eastl::vector<float> yData;
  ScrollingBuffer()
  {
    xData.reserve(maxSize);
    yData.reserve(maxSize);
  }
  void addPoint(float x, float y)
  {
    if ((int)xData.size() < maxSize)
    {
      xData.push_back(x);
      yData.push_back(y);
    }
    else
    {
      xData[offset] = x;
      yData[offset] = y;
      offset = (offset + 1) % maxSize;
    }
  }
  int size() const { return (int)xData.size(); }
};

void latency_imgui_window()
{
  static ScrollingBuffer gameToRenderBuf;
  static ScrollingBuffer gameLatencyBuf;
  static ScrollingBuffer renderLatencyBuf;
  static ScrollingBuffer simEndBuf;
  static ScrollingBuffer renderSubmitEndBuf;
  static ScrollingBuffer gpuRenderEndBuf;
  static float t = 0;
  static float history = 10.0f;
  static int maxFrameCount = 32;

  t += ImGui::GetIO().DeltaTime;

  lowlatency::LatencyData data = lowlatency::get_statistics_since(0, maxFrameCount);
  if (data.frameCount > 0)
  {
    gameToRenderBuf.addPoint(t, data.gameToRenderLatency.avg);
    gameLatencyBuf.addPoint(t, data.gameLatency.avg);
    renderLatencyBuf.addPoint(t, data.renderLatency.avg);
    simEndBuf.addPoint(t, data.simEndTime.avg);
    renderSubmitEndBuf.addPoint(t, data.renderSubmitEndTime.avg);
    gpuRenderEndBuf.addPoint(t, data.gpuRenderEndTime.avg);
  }

  ImGui::SliderInt("Max frame count", &maxFrameCount, 1, 64);
  ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

  if (data.frameCount > 0)
  {
    ImGui::Text("Frames: %u  Latest frame ID: %u", data.frameCount, data.latestFrameId);
    ImGui::Text("Game to render: avg=%.2f min=%.2f max=%.2f ms", data.gameToRenderLatency.avg, data.gameToRenderLatency.min,
      data.gameToRenderLatency.max);
    ImGui::Text("Game latency:   avg=%.2f min=%.2f max=%.2f ms", data.gameLatency.avg, data.gameLatency.min, data.gameLatency.max);
    ImGui::Text("Render latency: avg=%.2f min=%.2f max=%.2f ms", data.renderLatency.avg, data.renderLatency.min,
      data.renderLatency.max);
  }
  else
  {
    ImGui::TextDisabled("No latency data available (Reflex not active?)");
  }

  ImPlot::SetNextAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
  float maxY = 1.0f;
  if (gameToRenderBuf.size() > 0)
    maxY = *eastl::max_element(gameToRenderBuf.yData.begin(), gameToRenderBuf.yData.end()) * 1.2f;
  maxY = eastl::max(maxY, 1.0f);
  ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, maxY, ImGuiCond_Always);

  if (ImPlot::BeginPlot("##Latencies", nullptr, "ms", ImVec2(-1, 200), 0, ImPlotAxisFlags_NoTickLabels, 0))
  {
    if (gameToRenderBuf.size())
      ImPlot::PlotLine("Game to Render", &gameToRenderBuf.xData[0], &gameToRenderBuf.yData[0], gameToRenderBuf.size(),
        ImPlotLineFlags_None, gameToRenderBuf.offset);
    if (gameLatencyBuf.size())
      ImPlot::PlotLine("Game", &gameLatencyBuf.xData[0], &gameLatencyBuf.yData[0], gameLatencyBuf.size(), ImPlotLineFlags_None,
        gameLatencyBuf.offset);
    if (renderLatencyBuf.size())
      ImPlot::PlotLine("Render", &renderLatencyBuf.xData[0], &renderLatencyBuf.yData[0], renderLatencyBuf.size(), ImPlotLineFlags_None,
        renderLatencyBuf.offset);
    ImPlot::EndPlot();
  }

  ImPlot::SetNextAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
  maxY = 1.0f;
  if (gpuRenderEndBuf.size() > 0)
    maxY = *eastl::max_element(gpuRenderEndBuf.yData.begin(), gpuRenderEndBuf.yData.end()) * 1.2f;
  maxY = eastl::max(maxY, 1.0f);
  ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, maxY, ImGuiCond_Always);

  if (ImPlot::BeginPlot("##Pipeline", nullptr, "ms", ImVec2(-1, 200), 0, ImPlotAxisFlags_NoTickLabels, 0))
  {
    if (simEndBuf.size())
      ImPlot::PlotLine("Sim End", &simEndBuf.xData[0], &simEndBuf.yData[0], simEndBuf.size(), ImPlotLineFlags_None, simEndBuf.offset);
    if (renderSubmitEndBuf.size())
      ImPlot::PlotLine("Render Submit End", &renderSubmitEndBuf.xData[0], &renderSubmitEndBuf.yData[0], renderSubmitEndBuf.size(),
        ImPlotLineFlags_None, renderSubmitEndBuf.offset);
    if (gpuRenderEndBuf.size())
      ImPlot::PlotLine("GPU Render End", &gpuRenderEndBuf.xData[0], &gpuRenderEndBuf.yData[0], gpuRenderEndBuf.size(),
        ImPlotLineFlags_None, gpuRenderEndBuf.offset);
    ImPlot::EndPlot();
  }
}
} // namespace

REGISTER_IMGUI_WINDOW("Render", "Low Latency Stats", latency_imgui_window);

void lowlatency::render_debug_low_latency()
{
  if (debug_latency.get())
  {
    const uint32_t currentTime = get_time_msec();
    if (currentTime - latency_display_time > display_refresh_time.get() || latency_data.frameCount == 0)
    {
      latency_display_time = currentTime;
      latency_data = lowlatency::get_statistics_since(0, display_max_frame_count.get());
    }
    if (latency_data.frameCount > 0)
    {
      IPoint2 current_block;
      current_block[0] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
      current_block[1] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
      {
        StdGuiRender::ScopeStarter strt;
        Point2 pos(StdGuiRender::screen_size().x - 500, 100);

        StdGuiRender::set_color(E3DCOLOR(0, 0, 0, 180));
        if (lowlatency::get_supported_latency_modes() > 0)
          StdGuiRender::render_rect(pos.x - 10, pos.y - 20, pos.x + 380, pos.y + 140);
        else
          StdGuiRender::render_rect(pos.x - 10, pos.y - 20, pos.x + 380, pos.y + 20);
        if (lowlatency::get_supported_latency_modes() > 0 && latency_data.frameCount > 0)
        {
          display_value(pos, 0xFFFFFFFF, "Game to render latency", latency_data.gameToRenderLatency);
          display_value(pos, 0xFFFFFFFF, "Render latency", latency_data.renderLatency);
          display_value(pos, 0xFFFFFFFF, "Game latency", latency_data.gameLatency);

          const float intervalLength = eastl::max({discard_invalid_value(latency_data.simEndTime.avg),
            discard_invalid_value(latency_data.inputSampleTime.avg), discard_invalid_value(latency_data.renderSubmitEndTime.avg),
            discard_invalid_value(latency_data.presentEndTime.avg), discard_invalid_value(latency_data.driverEndTime.avg),
            discard_invalid_value(latency_data.osRenderQueueEndTime.avg), discard_invalid_value(latency_data.gpuRenderEndTime.avg)});

          float invLength = 1.0f / intervalLength;

          new_line(pos, 0xFFFFFFFF);
          StdGuiRender::draw_str(String(0, "Displayed interval length=%3.3f", intervalLength));

          display_interval(pos, 0xFF8888FF, "Simulation", 0, latency_data.simEndTime.avg * invLength);
          display_interval(pos, 0xFFFF88FF, "Input", latency_data.inputSampleTime.avg * invLength,
            latency_data.inputSampleTime.avg * invLength);
          display_interval(pos, 0x88FF88FF, "Render submission", latency_data.renderSubmitStartTime.avg * invLength,
            latency_data.renderSubmitEndTime.avg * invLength);
          display_interval(pos, 0x88FFFFFF, "Present", latency_data.presentStartTime.avg * invLength,
            latency_data.presentEndTime.avg * invLength);
          display_interval(pos, 0x8888FFFF, "Driver", latency_data.driverStartTime.avg * invLength,
            latency_data.driverEndTime.avg * invLength);
          display_interval(pos, 0xFF88FFFF, "Os render queue", latency_data.osRenderQueueStartTime.avg * invLength,
            latency_data.osRenderQueueEndTime.avg * invLength);
          display_interval(pos, 0x888888FF, "Gpu render", latency_data.gpuRenderStartTime.avg * invLength,
            latency_data.gpuRenderEndTime.avg * invLength);
        }
      }
      ShaderGlobal::setBlock(current_block[0], ShaderGlobal::LAYER_FRAME);
      ShaderGlobal::setBlock(current_block[1], ShaderGlobal::LAYER_SCENE);
    }
  }
}