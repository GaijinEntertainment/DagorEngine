#include <render/dynamicQuality.h>

#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <3d/dag_drv3dCmd.h>
#include <ecs/core/entityManager.h>
#include <util/dag_delayedAction.h>

#if DAGOR_DBGLEVEL > 0
#include <gui/dag_imgui.h>
#include <imgui/implot.h>
#include <util/dag_convar.h>

static DynamicQuality *dynamic_quality_instance = nullptr;
CONSOLE_INT_VAL("render", dynQualityOffset, 0, -1000, 1000);
#endif

ECS_REGISTER_EVENT(DynamicQualityChangeEvent)


DynamicQuality::DynamicQuality(const DataBlock *cfg)
{
  GPUWatchMs::init_freq();
  reset(cfg);

#if DAGOR_DBGLEVEL > 0
  // be aware that debug stuff does not work for multiple instances
  G_ASSERT(!dynamic_quality_instance);
  dynamic_quality_instance = this;
#endif
}

DynamicQuality::~DynamicQuality()
{
#if DAGOR_DBGLEVEL > 0
  if (dynamic_quality_instance == this)
    dynamic_quality_instance = nullptr;
#endif
}

void DynamicQuality::reset(const DataBlock *cfg)
{
  // go back to default range if config changes
  if (currentRange != defaultRange)
  {
    currentRange = defaultRange;
    broadcastEvent();
  }

  balance = 0;
  ranges.clear();
#if DAGOR_DBGLEVEL > 0
  rangeBarsX.clear();
  rangeBarsY.clear();
#endif

  if (!cfg)
    return;

  defaultRange = cfg->getInt("defaultRangeIndex", 0);
  currentRange = defaultRange;
  ranges.resize(cfg->blockCount());
  for (int i = 0; i < cfg->blockCount(); ++i)
  {
    const DataBlock *rangeCfg = cfg->getBlock(i);

    ranges[i].frameCountThreshold = rangeCfg->getInt("frameCountThreshold", 200);
    ranges[i].frameTimeMs = rangeCfg->getIPoint2("frameTimeMs", IPoint2(0, 0));
    ranges[i].name = rangeCfg->getStr("name", "<unknown>");
  }

#if DAGOR_DBGLEVEL > 0
  rangeBarsX.reserve(ranges.size() * 2);
  rangeBarsY.reserve(ranges.size() * 2);
  for (const QualityRange &i : ranges)
  {
    rangeBarsX.push_back(i.frameTimeMs.x);
    rangeBarsY.push_back(i.frameCountThreshold * 2);

    rangeBarsX.push_back(i.frameTimeMs.y);
    rangeBarsY.push_back(i.frameCountThreshold * 2);
  }
#endif
}

void DynamicQuality::broadcastEvent()
{
  add_delayed_action(make_delayed_action(
    [rangeName = ranges[currentRange].name] { g_entity_mgr->broadcastEventImmediate(DynamicQualityChangeEvent(rangeName)); }));
  balance = 0;
}

void DynamicQuality::processTimingRecord(uint64_t gpu_time_ms)
{
#if DAGOR_DBGLEVEL > 0
  gpu_time_ms += dynQualityOffset.get();
#endif
  QualityRange &range = ranges[currentRange];
  if (abs(balance) > range.frameCountThreshold)
  {
    bool changed = (balance < 0 && currentRange > 0);
    if (changed)
      --currentRange;
    else
    {
      changed = (balance > 0 && currentRange < (ranges.size() - 1));
      if (changed)
        ++currentRange;
    }

    if (changed)
      broadcastEvent();
  }
  else
  {
    if (gpu_time_ms < range.frameTimeMs.x)
      --balance;
    else if (gpu_time_ms > range.frameTimeMs.y)
      ++balance;
    else if (balance != 0)
      balance += (balance > 0) ? -1 : 1;
  }
}

bool DynamicQuality::allowTracking() { return ranges.size() && GPUWatchMs::available(); }

void DynamicQuality::onFrameStart()
{
  if (!allowTracking())
    return;

  uint64_t gpuTimeMs;
  timingIdx = (timingIdx + 1) % timings.size();
  if (timings[timingIdx].read(gpuTimeMs))
  {
    processTimingRecord(gpuTimeMs);
#if DAGOR_DBGLEVEL > 0
    lastGpuTime = gpuTimeMs;
    timingLatency = 0;
  }
  else
  {
    ++timingLatency;
    if (timingLatency > timings.size())
      logerr("DynamicQuality::onFrameStart is wrongly placed(multiple calls per frame) or GPU latency is too big");
#endif
  }

  timings[timingIdx].start();
}

void DynamicQuality::onFrameEnd()
{
  if (!allowTracking())
    return;

  timings[timingIdx].stop();
}

#if DAGOR_DBGLEVEL > 0

void DynamicQuality::debugUI()
{
  if (!ranges.size())
  {
    ImGui::Text("Quality ranges are not found in supplied blk");
    return;
  }

  ImGui::Text("Range %s(%u), balance %i", ranges[currentRange].name.c_str(), currentRange, balance);

  uint32_t xMark = (ranges[currentRange].frameTimeMs.x + ranges[currentRange].frameTimeMs.y) >> 1;
  uint32_t yMark = balance + ranges[currentRange].frameCountThreshold;

  if (ImPlot::BeginPlot("##Dynamic quality trace", "GPU frame time, ms", "Balance"))
  {
    for (int i = 0; i < ranges.size(); ++i)
    {
      ImPlot::SetNextFillStyle(ImPlot::GetColormapColor(i));
      ImPlot::PlotBars(ranges[i].name, rangeBarsX.data() + i * 2, rangeBarsY.data() + i * 2, 2, 0.5f);
    }
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 10, ImVec4(0, 1, 0, 1));
    ImPlot::PlotLine("##Active quality", &xMark, &yMark, 1);
    ImPlot::EndPlot();
  }
}

static void dynamic_quality_imgui()
{
  if (dynamic_quality_instance)
    dynamic_quality_instance->debugUI();
  else
    ImGui::Text("Dynamic quality disabled");
}
REGISTER_IMGUI_WINDOW("Render", "Dynamic Quality", dynamic_quality_imgui);
#endif