// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceMetrics.h"

#if DAGOR_DBGLEVEL > 0

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <string.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>

namespace voicechat::metrics
{

static const char overlay_group[] = "Sound";
static const char overlay_name[] = "Voice metrics";
// constraint: imgui calls a registered window function only while the window is open and not collapsed
static constexpr unsigned overlay_hidden_after_ms = 500;
static constexpr unsigned published_stale_after_ms = 1000;
static constexpr float bar_floor_db = -60.0f;

// constraint: only the main thread writes these, from the imgui draw and the console
static volatile int overlay_session = 0;
static volatile int overlay_drawn_msec = 0;
static int overlay_opened_msec = 0;
static int overlay_drawn_frame = -1;
static bool imgui_turned_on_by_command = false;

struct PublishedCapture
{
  Capture capture;
  int session = 0;
  int msec = 0;
};

struct PublishedPlayback
{
  const void *communicator = nullptr;
  eastl::vector<VoiceSourceControl::Metrics> sources;
  int session = 0;
  int msec = 0;
};

static WinCritSec published_mutex;
static PublishedCapture published_capture;
static eastl::vector<PublishedPlayback> published_playback;

static constexpr unsigned level_history_ms = 10000;
static constexpr int level_history_points = 1000;

struct LevelPoint
{
  int msec = 0;
  MicLevel level;
};

static eastl::vector<LevelPoint> level_history;
static int level_history_next = 0;
static int level_history_frames = 0;

static unsigned elapsed_ms(int since_msec, int now_msec)
{
  const int elapsed = int(unsigned(now_msec) - unsigned(since_msec));
  return elapsed > 0 ? unsigned(elapsed) : 0;
}

int active_session()
{
  const int session = interlocked_acquire_load(overlay_session);
  if (elapsed_ms(interlocked_acquire_load(overlay_drawn_msec), get_time_msec()) >= overlay_hidden_after_ms)
    return 0;
  return session;
}

void publish_capture(int session, const Capture &capture)
{
  const int now = get_time_msec();
  WinAutoLock lock(published_mutex);
  const int framesProcessed = capture.sessionCounts.processing.framesProcessed;
  if (published_capture.session != session)
  {
    level_history.clear();
    level_history_next = 0;
    level_history_frames = framesProcessed;
  }
  published_capture.capture = capture;
  published_capture.session = session;
  published_capture.msec = now;

  if (!capture.recording || framesProcessed == level_history_frames)
    return;
  level_history_frames = framesProcessed;
  if (level_history.size() < level_history_points)
    level_history.push_back({now, capture.level});
  else
  {
    level_history[level_history_next] = {now, capture.level};
    level_history_next = (level_history_next + 1) % level_history_points;
  }
}

void publish_playback(const void *communicator, int session, dag::ConstSpan<VoiceSourceControl::Metrics> sources)
{
  const int now = get_time_msec();
  WinAutoLock lock(published_mutex);
  auto it = eastl::find_if(published_playback.begin(), published_playback.end(),
    [communicator](const PublishedPlayback &entry) { return entry.communicator == communicator; });
  if (it == published_playback.end())
  {
    published_playback.push_back().communicator = communicator;
    it = published_playback.end() - 1;
  }
  it->sources.assign(sources.begin(), sources.end());
  it->session = session;
  it->msec = now;
}

static int begin_overlay_frame(int now)
{
  int session = interlocked_acquire_load(overlay_session);
  const int frame = ImGui::GetFrameCount();
  const bool reopened =
    frame != overlay_drawn_frame + 1 || elapsed_ms(interlocked_acquire_load(overlay_drawn_msec), now) >= overlay_hidden_after_ms;
  overlay_drawn_frame = frame;
  if (session == 0 || reopened)
  {
    ++session;
    overlay_opened_msec = now;
    interlocked_release_store(overlay_session, session);
  }
  interlocked_release_store(overlay_drawn_msec, now);
  return session;
}

static void level_bar(const char *label, float db)
{
  const float fraction = eastl::clamp((db - bar_floor_db) / -bar_floor_db, 0.0f, 1.0f);
  ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0), String(0, "%s %.1f dBFS", label, db).str());
}

struct LevelPlot
{
  eastl::vector<float> seconds, rmsDb, peakDb, gateDb;
};

static void draw_level_plot(const LevelPlot &plot)
{
  if (!ImPlot::BeginPlot("##mic level", ImVec2(-1, 200), ImPlotFlags_NoMouseText))
    return;
  ImPlot::SetupAxes("s", "dBFS");
  ImPlot::SetupAxesLimits(-(level_history_ms / 1000.0), 0, bar_floor_db, 0, ImPlotCond_Always);
  const int count = plot.seconds.size();
  if (count > 0)
  {
    ImPlot::PlotShaded("gate open", plot.seconds.data(), plot.gateDb.data(), count, bar_floor_db);
    ImPlot::PlotLine("RMS", plot.seconds.data(), plot.rmsDb.data(), count);
    ImPlot::PlotLine("peak", plot.seconds.data(), plot.peakDb.data(), count);
  }
  ImPlot::EndPlot();
}

static void draw_capture(const Capture &capture, const LevelPlot &plot, unsigned seconds)
{
  ImGui::Text("mic: %s device %d, record volume %.2f", capture.recording ? "recording on" : "not recording on", capture.recordDeviceId,
    capture.recordVolume);
  level_bar("RMS", capture.level.rmsDb);
  level_bar("peak", capture.level.peakDb);
  ImGui::Text("speech gate: %s", capture.level.gateOpen ? "open" : "closed");
  draw_level_plot(plot);

  ImGui::SeparatorText(String(0, "capture chain, %u s since the overlay opened", seconds).str());
  const ProcessingStats &proc = capture.sessionCounts.processing;
  ImGui::Text("frames processed: %d", proc.framesProcessed);
  ImGui::Text("dropped by the speech gate: %d of %d", proc.gateDroppedFrames, proc.gateFrames);
  ImGui::Text("without echo reference: %d of %d", proc.framesWithoutEcho, proc.echoFrames);
  ImGui::Text("APM errors: %d", proc.apmErrors);
  if (proc.gainFrames > 0)
    ImGui::Text("AGC2 gain: %.1f to %.1f dB over %d frames", proc.minGainDb, proc.maxGainDb, proc.gainFrames);
  else
    ImGui::TextUnformatted("AGC2 gain: not measured");
  if (proc.timedFrames > 0)
    ImGui::Text("chain per frame: %d us mean, %d us max; RNNoise %s", int(proc.chainUsec / proc.timedFrames), proc.chainMaxUsec,
      proc.denoiseFrames > 0 ? String(0, "%d us mean", int(proc.denoiseUsec / proc.denoiseFrames)).str() : "not run");
  else
    ImGui::TextUnformatted("chain time: not measured");
  ImGui::Text("record ring lags: %d, max %d ms", capture.sessionCounts.lags, capture.sessionCounts.maxLagMs);
  ImGui::Text("dropped for late readers: %d ms", capture.sessionLateReaderDroppedMs);
}


static void draw_playback(const PublishedPlayback &entry)
{
  ImGui::Text("communicator %p", entry.communicator);
  if (entry.sources.empty())
  {
    ImGui::TextUnformatted("  no voice sources");
    return;
  }
  if (!ImGui::BeginTable(String(0, "sources##%p", entry.communicator).str(), 4, ImGuiTableFlags_Borders))
    return;
  ImGui::TableSetupColumn("source");
  ImGui::TableSetupColumn("ring fill, ms");
  ImGui::TableSetupColumn("underruns");
  ImGui::TableSetupColumn("dropped blocks");
  ImGui::TableHeadersRow();
  for (const VoiceSourceControl::Metrics &src : entry.sources)
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%p", src.source);
    ImGui::TableNextColumn();
    ImGui::Text("%d of %d", src.ringQueuedMs, src.ringCapacityMs);
    ImGui::TableNextColumn();
    ImGui::Text("%d", src.underruns);
    ImGui::TableNextColumn();
    if (src.blocksDropped < 0)
      ImGui::TextUnformatted("not counted");
    else
      ImGui::Text("%d", src.blocksDropped);
  }
  ImGui::EndTable();
}

static void overlay_window()
{
  const int now = get_time_msec();
  const int session = begin_overlay_frame(now);
  auto current = [&](int published_session, int published_msec) {
    return published_session == session && elapsed_ms(published_msec, now) < published_stale_after_ms;
  };

  Capture capture;
  eastl::vector<LevelPoint> history;
  int historyNext = 0;
  bool captureCurrent = false;
  eastl::vector<PublishedPlayback> playback;
  {
    WinAutoLock lock(published_mutex);
    captureCurrent = current(published_capture.session, published_capture.msec);
    if (captureCurrent)
    {
      capture = published_capture.capture;
      history = level_history;
      historyNext = level_history_next;
    }
    published_playback.erase(eastl::remove_if(published_playback.begin(), published_playback.end(),
                               [&](const PublishedPlayback &entry) { return !current(entry.session, entry.msec); }),
      published_playback.end());
    playback = published_playback;
  }

  LevelPlot plot;
  for (int i = 0, n = history.size(); i < n; ++i)
  {
    const LevelPoint &point = history[(historyNext + i) % n];
    const unsigned age = elapsed_ms(point.msec, now);
    if (age >= level_history_ms)
      continue;
    plot.seconds.push_back(-(age / 1000.0f));
    plot.rmsDb.push_back(eastl::max(point.level.rmsDb, bar_floor_db));
    plot.peakDb.push_back(eastl::max(point.level.peakDb, bar_floor_db));
    plot.gateDb.push_back(point.level.gateOpen ? 0.0f : bar_floor_db);
  }

  if (captureCurrent)
    draw_capture(capture, plot, elapsed_ms(overlay_opened_msec, now) / 1000);
  else
    ImGui::TextUnformatted("mic: no capture runs");
  ImGui::SeparatorText("playback, underruns and dropped blocks since the overlay opened");
  if (playback.empty())
    ImGui::TextUnformatted("no communicator");
  for (const PublishedPlayback &entry : playback)
    draw_playback(entry);
}

} // namespace voicechat::metrics

REGISTER_IMGUI_WINDOW(voicechat::metrics::overlay_group, voicechat::metrics::overlay_name, voicechat::metrics::overlay_window);

static bool other_window_visible()
{
  using namespace voicechat::metrics;
  for (const ImGuiFunctionQueue *window = ImGuiFunctionQueue::windowHead; window; window = window->next)
    if ((strcmp(window->group, overlay_group) != 0 || strcmp(window->name, overlay_name) != 0) &&
        imgui_window_is_visible(window->group, window->name))
      return true;
  return false;
}

static bool voice_metrics_console_handler(const char *argv[], int argc)
{
  using namespace voicechat::metrics;
  int found = 0;
  CONSOLE_CHECK_NAME_EX("voice", "toggle_metrics", 1, 1, "shows or hides the voice metrics overlay", "")
  {
    const bool imguiOff = imgui_get_state() == ImGuiState::OFF;
    const bool shown =
      !imguiOff && imgui_window_is_visible(overlay_group, overlay_name) && !imgui_window_is_collapsed(overlay_group, overlay_name);
    if (shown)
      imgui_window_set_visible(overlay_group, overlay_name, false);
    else
      imgui_window_request_focus(overlay_group, overlay_name);
    if (shown && imgui_turned_on_by_command && imgui_get_state() == ImGuiState::OVERLAY && !other_window_visible())
      imgui_request_state_change(ImGuiState::OFF);
    if (shown)
      imgui_turned_on_by_command = false;
    else if (imguiOff)
    {
      imgui_turned_on_by_command = true;
      imgui_request_state_change(ImGuiState::OVERLAY);
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(voice_metrics_console_handler);

#endif
