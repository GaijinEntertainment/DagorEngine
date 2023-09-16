#include <render/dynamicResolution.h>
#include <3d/dag_drv3dCmd.h>

#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <gui/dag_imgui.h>
#include <imgui/implot.h>
#include <EASTL/vector.h>

#define DYNRES_DEBUG (DAGOR_DBGLEVEL > 0)

constexpr int MIN_TIMESTAMPGET_DELAY = 2;

#if DYNRES_DEBUG
static DynamicResolution *dynamic_resolution_instance = nullptr;
#endif

DynamicResolution::DynamicResolution(int target_width, int target_height)
{
  applySettings();

  targetResolutionWidth = target_width;
  targetResolutionHeight = target_height;
  currentResolutionWidth = target_width;
  currentResolutionHeight = target_height;

  timestamps.resize(MIN_TIMESTAMPGET_DELAY);
  uint64_t gpuFreq;
  G_VERIFY(d3d::driver_command(D3V3D_COMMAND_TIMESTAMPFREQ, &gpuFreq, 0, 0));
  gpuMsPerTick = 1000.0 / gpuFreq;

#if DYNRES_DEBUG
  dynamic_resolution_instance = this;
#endif
}

DynamicResolution::~DynamicResolution()
{
  for (TimestampQueries &ts : timestamps)
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &ts.beginQuery, 0, 0);
    d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &ts.endQuery, 0, 0);
  }
#if DYNRES_DEBUG
  dynamic_resolution_instance = nullptr;
#endif
}

void DynamicResolution::applySettings()
{
  const DataBlock &settingsBlk = *dgs_get_settings()->getBlockByNameEx("video")->getBlockByNameEx("dynamicResolution");
  targetFrameRate = settingsBlk.getInt("targetFPS", 60);
  if (targetFrameRate <= 0)
  {
    logerr("Invalid target frame rate for Dynamic Resolution: %d", targetFrameRate);
    autoAdjust = false;
    return;
  }
  targetMsPerFrame = 1000.0 / targetFrameRate;
  resolutionScaleStep = settingsBlk.getReal("resolutionScaleStep", 0.05);
}

void DynamicResolution::beginFrame()
{
  G_ASSERT_RETURN(gpuFrameState == GPU_FRAME_FINISHED, );
  gpuFrameState = GPU_FRAME_STARTED;
  uint32_t curIdx = frameIdx % timestamps.size();
  if (frameIdx >= timestamps.size()) //-V1051
  {
    uint64_t beginTick = 0, endTick = 0;
    if (d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, timestamps[curIdx].endQuery, &endTick, 0))
    {
      G_VERIFY(d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, timestamps[curIdx].beginQuery, &beginTick, 0));
      float timeMs = double(endTick - beginTick) * gpuMsPerTick;
      currentMsPerFrame = timeMs;
      adjustResolution();
    }
    else
    {
      // We never want to wait gpu, so increase number of timestamps
      if (curIdx == 0)
      {
        curIdx = timestamps.size();
        timestamps.emplace_back();
      }
      else
        timestamps.insert(timestamps.begin() + curIdx, TimestampQueries());
      debug("DynRes: increase number of timestamp queries to %d, frameIdx: %d", timestamps.size(), frameIdx);
      frameIdx = timestamps.size() + curIdx;
    }
  }
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &timestamps[curIdx].beginQuery, 0, 0);
}

void DynamicResolution::endFrame()
{
  G_ASSERT_RETURN(gpuFrameState == GPU_FRAME_STARTED, );
  gpuFrameState = GPU_FRAME_FINISHED;
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &timestamps[frameIdx % timestamps.size()].endQuery, 0, 0);
  frameIdx++;
}


void DynamicResolution::adjustResolution()
{
  if (autoAdjust)
  {
    if (currentMsPerFrame > targetMsPerFrame * maxThresholdToChange)
    {
      framesAboveThreshold++;
      if (framesAboveThreshold > timestamps.size())
      {
        framesAboveThreshold = 0;
        resolutionScale = max(minResolutionScale, resolutionScale - resolutionScaleStep);
      }
    }
    else
      framesAboveThreshold = 0;

    if (currentMsPerFrame < targetMsPerFrame * minThresholdToChange)
    {
      framesUnderThreshold++;
      if (framesUnderThreshold > timestamps.size())
      {
        framesUnderThreshold = 0;
        resolutionScale = min(maxResolutionScale, resolutionScale + resolutionScaleStep);
      }
    }
    else
      framesUnderThreshold = 0;
  }

  currentResolutionWidth = targetResolutionWidth * resolutionScale;
  currentResolutionHeight = targetResolutionHeight * resolutionScale;
}


#if DYNRES_DEBUG

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
  void AddPoint(float x, float y)
  {
    if (xData.size() < maxSize)
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
};

void DynamicResolution::debugImguiWindow()
{
  static ScrollingBuffer frameMsData;
  static float t = 0;
  t += ImGui::GetIO().DeltaTime;
  frameMsData.AddPoint(t, currentMsPerFrame);

  ImGui::SliderFloat("Resolution Scale", &resolutionScale, 0.5, 1, "%.2f");
  ImGui::LabelText("Resolution", "%dx%d", currentResolutionWidth, currentResolutionHeight);
  const char *dynResModes[] = {"Auto", "Cyclic", "Manually"};
  static int currentMode = 0;
  ImGui::Combo("Mode", &currentMode, dynResModes, IM_ARRAYSIZE(dynResModes));
  if (currentMode == 0)
  {
    autoAdjust = true;
  }
  else if (currentMode == 1)
  {
    autoAdjust = false;
    int monotonuousChanges = (maxResolutionScale - minResolutionScale) / resolutionScaleStep;
    if (frameIdx % (monotonuousChanges * 2) < monotonuousChanges)
      resolutionScale = max(minResolutionScale, resolutionScale - resolutionScaleStep);
    else
      resolutionScale = min(maxResolutionScale, resolutionScale + resolutionScaleStep);
  }
  else if (currentMode == 2)
  {
    autoAdjust = false;
  }

  const char *fpsModes[] = {"30", "60", "120", "custom"};
  static int fpsMode = 3;
  ImGui::Combo("Target FPS", &fpsMode, fpsModes, IM_ARRAYSIZE(fpsModes));
  if (fpsMode < 3)
    targetFrameRate = atoi(fpsModes[fpsMode]);
  else if (fpsMode == 3)
    ImGui::SliderInt("FPS", &targetFrameRate, 15, 360);
  targetMsPerFrame = 1000.0 / targetFrameRate;

  static float history = 10.0f;
  float thresholdsX[4], thresholdsY[4];
  thresholdsX[0] = t;
  thresholdsY[0] = targetMsPerFrame * maxThresholdToChange;
  thresholdsX[1] = t - history - 0.1;
  thresholdsY[1] = targetMsPerFrame * maxThresholdToChange;
  thresholdsX[2] = t - history - 0.1;
  thresholdsY[2] = targetMsPerFrame * minThresholdToChange;
  thresholdsX[3] = t;
  thresholdsY[3] = targetMsPerFrame * minThresholdToChange;
  ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
  ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
  float maxY = max(targetMsPerFrame * 1.33f, *eastl::max_element(frameMsData.yData.begin(), frameMsData.yData.end()));
  ImPlot::SetNextPlotLimitsY(0, maxY, ImGuiCond_Always);
  if (ImPlot::BeginPlot("##GPU frame time", nullptr, "gpu time (ms)", ImVec2(0, 0), 0, ImPlotAxisFlags_NoTickLabels, 0))
  {
    ImPlot::PlotLine("current", &frameMsData.xData[0], &frameMsData.yData[0], frameMsData.xData.size(), frameMsData.offset);
    ImPlot::PlotLine("thresholds", thresholdsX, thresholdsY, 4);
    ImPlot::EndPlot();
  }
}

static void dynamic_resolution_imgui()
{
  if (dynamic_resolution_instance)
    dynamic_resolution_instance->debugImguiWindow();
}

REGISTER_IMGUI_WINDOW("Render", "Dynamic Resolution", dynamic_resolution_imgui);
#endif