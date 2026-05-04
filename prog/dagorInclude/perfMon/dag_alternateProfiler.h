//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/vector.h>
#include <util/dag_console.h>

struct AlternatingProfilerResult
{
  int64_t totalRunTimeUs = 0;
  int numFrames = 0;

  [[nodiscard]] float getFrameTimeMs() const;
  [[nodiscard]] float getFps() const;
};

/*!
 * This is a simple profiling tool, which is capable of giving precise measurement comparison between different configurations without
 * needing to restart the game or reset the measurement environment.
 *
 * The idea:
 * Create a callback that switches between configs that we want to measure.
 * Periodically (at high frequency) call this cb to change the config at runtime and accumulate performance data during a sufficiently
 * long time. At the end, compute avg and compare. For simplicity and robustness, measure the frame time only.
 *
 * Advantages:
 * * Handle heating of the device (throttling) thanks to the high frequency switching rate
 * * Works on any device, no need for GPU markers
 * * Precise if used well
 * * The whole frame time is measured, so all changes are considered between configs (not just specific markers)
 * * No need to recreate the same environment in the game between measurements. Everything is measured once.
 *
 * Disadvantages:
 * * Need to write a custom callback function every time when it's used
 * * Can't measure single markers, so it's somewhat sensitive to noise in frame time
 * * It can only measure things that are really easy to switch at runtime
 *
 * Requirements for a good measurement:
 * * FPS should be as stable as possible
 * * If the GPU perf is measured, the game has to be GPU limited in all configs to produce meaningful measurements (same with CPU
 * limit)
 * * Set a good period for switching (around 300ms works well usually)
 * * Give it sufficient time to accumulate data. Print the results periodically to see if the data is converging.
 *
 * Usage:
 * The interface is based on console variables:
 * alternating_profiler.period:
 *  0  -> Turn off the measurements and clear the results
 *  >0 -> Turn on the measurements and configs are switched periodically at this interval (in milliseconds)
 * alternating_profiler.print:
 *  use this to print the results so far without clearing them.
 */
class AlternatingConfigProfiler
{
public:
  /*!
   * Call this one per frame to try the example
   * It will sleep every frame and measure its effect on the framerate
   */
  static void example_update();

  /*!
   * In order to use this profiler, call this static function exactly once per frame.
   * The first config (id=0) is meant to be the original code. The rest of the variants are compared to the first value.
   * @tparam N Number of config variants
   * @param switch_config_cb Callback that receives the config id (0..N-1) and it's expected to apply the specified variant config
   * @return Reference to the profiler object
   */
  template <int N, typename F>
  static AlternatingConfigProfiler &update(const F &switch_config_cb)
  {
    static AlternatingConfigProfiler profiler(N);
    bool hasPeriodChanged = false;
    int periodMs = get_current_period_ms(&hasPeriodChanged);
    profiler.update(periodMs, switch_config_cb);
    int newVariantId;
    if (pop_new_variant_id(newVariantId))
      switch_config_cb(newVariantId);
    else if (profiler.getCurrentConfigId() != -1)
      set_auto_variant_id(profiler.getCurrentConfigId());
    bool shouldClear = hasPeriodChanged && periodMs <= 0;
    if (hasPeriodChanged && periodMs > 0 && periodMs < 100)
    {
      static bool warningLogged = false;
      if (!warningLogged)
      {
        warningLogged = true;
        console::warning("Warning: Switching period is low, results might be inaccurate.");
      }
    }
    if (pop_should_print() || shouldClear)
    {
      console::print(
        "Alternating profiler results (fps; frame time; perf savings per frame; perf saving as percentage of the total frame):");
      console::print("[0] (Reference): %f fps (%fms)", profiler.getResult(0).getFps(), profiler.getResult(0).getFrameTimeMs());
      for (int i = 1; i < N; ++i)
      {
        const float diff = profiler.getResult(0).getFrameTimeMs() - profiler.getResult(i).getFrameTimeMs();
        console::print("[%d]: %f fps (%fms), performance savings ([0]-[%d]): %fus, %f%%", i, profiler.getResult(i).getFps(),
          profiler.getResult(i).getFrameTimeMs(), i, roundf(diff * 1000),
          roundf(diff / profiler.getResult(0).getFrameTimeMs() * 1000) / 10.0f);
      }
    }
    if (shouldClear)
    {
      profiler.clear();
      console::print("Alternating profiler results cleared");
    }

    return profiler;
  }

  void clear();
  [[nodiscard]] const AlternatingProfilerResult &getResult(int index) const;
  [[nodiscard]] int getCurrentConfigId() const;

private:
  int numVariants;
  eastl::vector<AlternatingProfilerResult> results;
  unsigned int lastFrameNo = 0;
  int64_t lastMeasurementTimeRef = -1;
  int64_t lastSwitchTimeMs = -1;
  int lastConfig = -1;

  explicit AlternatingConfigProfiler(int num_variants);

  template <typename F>
  void update(int switch_period_ms, const F &switch_config)
  {
    unsigned int frameNo = ::dagor_frame_no();
    int numFrames = (int)frameNo - (int)lastFrameNo;
    lastFrameNo = frameNo;

    if (switch_period_ms <= 0)
    {
      lastConfig = -1;
      return;
    }

    int timeMs = ::get_time_msec();
    if (lastConfig == -1 || timeMs - lastSwitchTimeMs > switch_period_ms)
    {
      if (lastConfig == -1)
        lastConfig = 0;
      else
        lastConfig = (lastConfig + 1) % numVariants;
      switch_config(lastConfig);
      lastSwitchTimeMs = timeMs;
      lastMeasurementTimeRef = -1;
    }
    else
    {
      if (lastMeasurementTimeRef != -1)
      {
        int64_t deltaTimeUs = ::get_time_usec(lastMeasurementTimeRef);
        results[lastConfig].numFrames += numFrames;
        results[lastConfig].totalRunTimeUs += deltaTimeUs;
      }

      lastMeasurementTimeRef = ::ref_time_ticks();
    }
  }

  static bool pop_new_variant_id(int &new_value);
  static void set_auto_variant_id(int value);
  static int get_current_period_ms(bool *has_changed = nullptr);
  static bool pop_should_print();
};
