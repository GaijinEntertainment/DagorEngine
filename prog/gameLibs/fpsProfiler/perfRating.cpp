#include <fpsProfiler/dag_perfRating.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>

#include <gui/dag_imgui.h>
#include <imgui/implot.h>
#include <imgui/imgui_internal.h>

// https://info.gaijin.lan/display/CRSED/Performance+rating+statistics+panel
// Panel consists of NxM colored quads, where
// N is number of FPS groups
//(e.g. group 0: +60 fps, group 1: 60-50 fps, etc)
// where next fps is 10 units less
// M is number of averaging periods,
//(e.g. group 0: 250 ms, group 1: 500 ms, etc)
// where next period is 2 time longer than previous

// color defines 2 things:
// brightness: how much frames landed in FPS group per period
// hue: defines factor that limited frame rate (e.g. where is bottleneck)

static const int perfRatingWindow = 250 * 1000;
static const int minFrameTime = 1000;
static const int maxFramesInGroup = perfRatingWindow / minFrameTime;

const int perfRatingGroupList[fps_profile::PERF_RATING_GROUPS] = {
  16666,  // 60+ FPS
  20000,  // 60-50 FPS
  25000,  // 50-40 FPS
  33333,  // 40-30 FPS
  50000,  // 30-20 FPS
  1 << 30 // below 20 FPS
};

struct PerfRatingAcmElement
{
  void sum(PerfRatingAcmElement &b)
  {
    for (int i = 0; i != fps_profile::PERF_RATING_GROUPS; ++i)
    {
      this->rates[i].frames += b.rates[i].frames;
      this->rates[i].flags |= b.rates[i].flags;
    }
  }

  void clear()
  {
    for (int i = 0; i != fps_profile::PERF_RATING_GROUPS; ++i)
    {
      this->rates[i].frames = 0;
      this->rates[i].flags = 0;
    }
  }

  void divide(int by)
  {
    for (int i = 0; i != fps_profile::PERF_RATING_GROUPS; ++i)
    {
      this->rates[i].frames /= by;
    }
  }

  fps_profile::PerfRating rates[fps_profile::PERF_RATING_GROUPS];
};

int PerfRatingAcmCount[fps_profile::PERF_RATING_PERIODS] = {0};
PerfRatingAcmElement PerfRatingData[fps_profile::PERF_RATING_PERIODS] = {0};
int64_t lastFrameTime = 0;
int64_t accumulatedFrameTime = 0;
int64_t limitationFlags = 0;

void updatePeriodsData()
{
  for (int i = 1; i != fps_profile::PERF_RATING_PERIODS; ++i)
  {
    if (PerfRatingAcmCount[i] == 0)
      PerfRatingData[i].clear();

    ++PerfRatingAcmCount[i];

    PerfRatingData[i].sum(PerfRatingData[i - 1]);

    if (PerfRatingAcmCount[i] != 2)
      break;
    else
    {
      PerfRatingAcmCount[i] = 0;
      PerfRatingData[i].divide(2);
    }
  }

  PerfRatingData[0].clear();
  accumulatedFrameTime = 0;
}

void fps_profile::updatePerformanceRating(int frameLimitationFlags)
{
  int64_t currentTime = ref_time_ticks();
  int64_t timePassed = ref_time_delta_to_usec(currentTime - lastFrameTime);
  lastFrameTime = currentTime;
  if (timePassed < 1)
    timePassed = 1;
  limitationFlags = frameLimitationFlags;

  int frameGroup = 0;
  while ((frameGroup < fps_profile::PERF_RATING_GROUPS) && (perfRatingGroupList[frameGroup] < timePassed))
    ++frameGroup;

  int &framesTotal = PerfRatingData[0].rates[frameGroup].frames;
  framesTotal += timePassed / minFrameTime;
  framesTotal = min(framesTotal, maxFramesInGroup);

  PerfRatingData[0].rates[frameGroup].flags |= limitationFlags;

  accumulatedFrameTime += timePassed;

  if (accumulatedFrameTime > perfRatingWindow)
    updatePeriodsData();
}

fps_profile::PerfRating fps_profile::getPerformanceRating(int period, int group) { return PerfRatingData[period].rates[group]; }

void fps_profile::initPerfProfile() { debug("Init fps_profile"); }

#if DAGOR_DBGLEVEL > 0

namespace
{
float magnify = 0.6;
float values[fps_profile::PERF_RATING_PERIODS][fps_profile::PERF_RATING_GROUPS];

const int maxLimitingFactorValue = 8;

const unsigned int scaleMin = 0;
const unsigned int scaleMax = maxFramesInGroup * (maxLimitingFactorValue * 2 - 1);

const int plotInitSize = 500;

const char *xLabels[] = {">60", ">50", ">40", ">30", ">20", ">0"};
const char *yLabels[] = {"250ms", "500ms", "1s", "2s", "4s", "8s", "16s"};

ImPlotAxisFlags axesFlags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;

const int colorMapVecSize = 16;
const ImVec4 Black = ImVec4(0, 0, 0, 1), White = ImVec4(1, 1, 1, 1), Red = ImVec4(1, 0, 0, 1), Green = ImVec4(0, 1, 0, 1),
             Yellow = ImVec4(1, 1, 0, 1), Blue = ImVec4(0, 0, 1, 1), Magenta = ImVec4(1, 0, 1, 1), Cyan = ImVec4(0, 1, 1, 1);
const ImVec4 colorMap[colorMapVecSize] = {
  Black, White, Black, Red, Black, Green, Black, Yellow, Black, Blue, Black, Magenta, Black, Cyan, Black, White};
} // namespace

void draw_legend_rect(const ImVec2 &size, const ImVec4 &color)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = 0;

  ImVec2 pos = window->DC.CursorPos;

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ImGui::ItemSize(size, style.FramePadding.y);
  if (!ImGui::ItemAdd(bb, id))
    return;

  const ImU32 col = ImGui::GetColorU32(color);
  ImGui::RenderFrame(bb.Min, bb.Max, col, true);
}

void draw_legend_line(const ImVec2 &size, const ImVec4 &color, const char *text)
{
  draw_legend_rect(size, color);
  ImGui::SameLine();
  ImGui::Text("%s", text);
}

void draw_legend()
{
  ImVec2 size(25, 25);

  draw_legend_line(size, Red, "Replay wait (CPU bound)");
  draw_legend_line(size, Green, "Replay underfeeded (game CPU bound)");
  draw_legend_line(size, Yellow, "Replay wait & underfeed (heavy CPU bound)");
  draw_legend_line(size, Blue, "GPU overutilization (game GPU bound)");
  draw_legend_line(size, Magenta, "Replay wait on GPU (heavy GPU bound)");
  draw_legend_line(size, Cyan, "Replay underfeed & GPU overutilization (heavy game CPU bound)");
}

void perf_stat_imgui()
{
  ImGui::SliderFloat("magnify", &magnify, 0.5, 1);

  fps_profile::updatePerformanceRating(d3d::driver_command(DRV3D_COMMAND_GET_FRAMERATE_LIMITING_FACTOR, nullptr, nullptr, nullptr));

  for (size_t i = 0; i < fps_profile::PERF_RATING_PERIODS; i++)
    for (size_t j = 0; j < fps_profile::PERF_RATING_GROUPS; j++)
    {
      values[i][j] = PerfRatingData[i].rates[j].frames;

      int addon = 2 * maxFramesInGroup * (PerfRatingData[i].rates[j].flags);

      values[i][j] += addon;
    }

  if (ImPlot::GetColormapIndex("perf_colors") == -1)
    ImPlot::AddColormap("perf_colors", colorMap, colorMapVecSize);
  ImPlot::PushColormap("perf_colors");
  ImPlot::SetNextPlotTicksX(0 + 1.0 / 14.0, 1 - 1.0 / 14.0, fps_profile::PERF_RATING_GROUPS, xLabels);
  ImPlot::SetNextPlotTicksY(1 - 1.0 / 14.0, 0 + 1.0 / 14.0, fps_profile::PERF_RATING_PERIODS, yLabels);
  if (ImPlot::BeginPlot("##FPS_Heatmap1", NULL, NULL, ImVec2(plotInitSize * magnify, plotInitSize * magnify),
        ImPlotFlags_NoLegend | ImPlotFlags_NoMousePos, axesFlags, axesFlags))
  {
    ImPlot::PlotHeatmap("FPS rating", values[0], fps_profile::PERF_RATING_PERIODS, fps_profile::PERF_RATING_GROUPS, scaleMin,
      scaleMax);
    ImPlot::EndPlot();
  }
  ImPlot::PopColormap();

  draw_legend();
}
REGISTER_IMGUI_WINDOW("Render", "PerfStat", perf_stat_imgui);
#endif
