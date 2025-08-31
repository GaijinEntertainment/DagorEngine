// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <util/dag_console.h>
#include <gui/dag_visualLog.h>
#include <util/dag_string.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui.h>
#include "../render/adaptation_manager.h"

static constexpr int HISTOGRAM_BINS = 256;

class AdaptationDebug
{
  dafg::NodeHandle readbackNode;
  RingCPUBufferLock readbackRing;
  uint32_t latestReadbackFrameRead = 0;
  uint32_t latestReadbackFrameCopied = 0;
  eastl::array<uint32_t, HISTOGRAM_BINS> lastHistogram = {};

public:
  AdaptationDebug()
  {
    readbackRing.init(sizeof(uint32_t), HISTOGRAM_BINS, 2, "adaptation_debug_readbackRing", SBCF_UA_STRUCTURED_READBACK, 0, false);
    readbackNode = dafg::register_node("adaptation_debug_readback", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      // Note: with multiplexing, we will essentially write to the readback buffer multiple times.
      // This is slightly inaccurate, but probably fine, as on average, histograms will be the same.
      registry.executionHas(dafg::SideEffects::External);
      const auto histogramBufferHandle =
        registry.read("exposure_histogram").buffer().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();

      return [this, histogramBufferHandle]() {
        const auto &histogramBuffer = histogramBufferHandle.ref();
        int unusedStride;
        if (const auto *data = reinterpret_cast<uint32_t *>(readbackRing.lock(unusedStride, latestReadbackFrameCopied, true)))
        {
          memcpy(lastHistogram.data(), data, sizeof(lastHistogram));
          readbackRing.unlock();
        }

        auto *target = static_cast<Sbuffer *>(readbackRing.getNewTarget(latestReadbackFrameCopied));
        if (target)
        {
          const_cast<Sbuffer &>(histogramBuffer).copyTo(target);
          readbackRing.startCPUCopy();
        }
      };
    });
  }

  void imgui(const ExposureBuffer &exposureBuffer, AdaptationSettings &settings)
  {
    if (!settings.isFixedExposure())
    {

      const auto getH = [](void *data, int idx) { return static_cast<float>(reinterpret_cast<uint32_t *>(data)[idx]); };
      const auto region = ImGui::GetContentRegionAvail();
      ImGui::Text("Note: values are at least 1 frame behind the actual image.");
      ImGui::PlotHistogram("##histogram", getH, lastHistogram.data(), HISTOGRAM_BINS, 0, nullptr, FLT_MAX, FLT_MAX,
        ImVec2(region.x, 256));
    }

    static bool useStops = false;
    const char *const unitsStr = useStops ? "stops" : "linear";
    ImGui::Checkbox("Display exposure as stops.", &useStops);
    ImGui::SameLine();
    ImGuiDagor::HelpMarker("A 'stop' means double the amount of light.");

    const auto linearToUnits = [&](float value) { return useStops ? log2f(value) : value; };
    const auto stopsToUnits = [&](float value) { return useStops ? value : exp2(value); };

    if (!settings.isFixedExposure())
    {
      ImGui::Text("Current values:");
      if (ImGui::BeginTable("current_values", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Target exposure (%s)", unitsStr);
        ImGui::SameLine();
        ImGuiDagor::HelpMarker("Computed from histogram.");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", linearToUnits(exposureBuffer[9]));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Actual exposure (%s)", unitsStr);
        ImGui::SameLine();
        ImGuiDagor::HelpMarker("Current value without adjustment.");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", linearToUnits(exposureBuffer[8]));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Final exposure (%s)", unitsStr);
        ImGui::SameLine();
        ImGuiDagor::HelpMarker("Current value with adjustment, clamping and fade applied.");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", linearToUnits(exposureBuffer[0]));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Histogram range (%s)", unitsStr);
        ImGui::TableNextColumn();
        ImGui::Text(useStops ? "%.2f .. %.2f" : "%f .. %f", stopsToUnits(exposureBuffer[4]), stopsToUnits(exposureBuffer[5]));

        ImGui::EndTable();
      }
    }

    ImGui::Text("Settings:");
    if (ImGui::BeginTable("settings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
      if (settings.isFixedExposure())
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Fixed exposure (%s)", unitsStr);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", linearToUnits(settings.fixedExposure));
      }
      else
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Fixed exposure");
        ImGui::TableNextColumn();
        ImGui::Text("disabled");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Exposure adjustment (%s)", unitsStr);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", linearToUnits(settings.autoExposureScale));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Min/max exposure (%s)", unitsStr);
        ImGui::TableNextColumn();
        ImGui::Text(useStops ? "%.2f .. %.2f" : "%.3f .. %.1f", linearToUnits(settings.minExposure),
          linearToUnits(settings.maxExposure));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Adapt up/down speed");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f/%.2f", settings.adaptUpSpeed, settings.adaptDownSpeed);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Brightness perception linear/power");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f/%.2f", settings.brightnessPerceptionLinear, settings.brightnessPerceptionPower);
      }

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Fade multiplier");
      ImGui::TableNextColumn();
      ImGui::Text("%.2f", settings.fadeMul);

      ImGui::EndTable();
    }
  }
};

ECS_DECLARE_BOXED_TYPE(AdaptationDebug);
ECS_REGISTER_BOXED_TYPE(AdaptationDebug, nullptr);

template <typename Callable>
static void get_adaptation_debug_ecs_query(Callable c);

template <typename Callable>
static void get_adaptation_manager_ecs_query(Callable c);

template <typename Callable>
static void adaptation_override_settings_ecs_query(Callable c);

template <typename Callable>
static void adaptation_level_settings_ecs_query(Callable c);

static void dumpComponents(ecs::Object &entity)
{
  for (const auto &components : entity)
  {
    if (components.second.is<float>())
      console::print("  %s: %d", components.first.c_str(), components.second.get<float>());
    if (components.second.is<bool>())
      console::print("  %s: %s", components.first.c_str(), components.second.get<bool>() ? "true" : "fasle");
  }
}

static bool adaptation_debug_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("postfx", "dumpExposure", 1, 1)
  {
    get_adaptation_manager_ecs_query([](AdaptationManager &adaptation__manager) { adaptation__manager.dumpExposureBuffer(); });
    console::print_d("Exposure saved to debug log");
  }
  CONSOLE_CHECK_NAME("postfx", "fixedExposure", 1, 2)
  {
    float exposure = argc > 1 ? atof(argv[1]) : -1;
    adaptation_override_settings_ecs_query([exposure](ecs::Object &adaptation_override_settings) {
      adaptation_override_settings.addMember("adaptation__on", exposure < 0);
      adaptation_override_settings.addMember("adaptation__fixedExposure", exposure);
    });
    console::print("usage: postfx.fixedExposure [number=%f]", exposure);
  }
  CONSOLE_CHECK_NAME("postfx", "dumpAdaptationSettings", 1, 1)
  {
    adaptation_override_settings_ecs_query([](ecs::Object &adaptation_override_settings) {
      console::print_d("Overrided adaptation settings:");
      dumpComponents(adaptation_override_settings);
    });
    adaptation_level_settings_ecs_query([](ecs::Object &adaptation_level_settings) {
      console::print_d("Adaptation level settings:");
      dumpComponents(adaptation_level_settings);
    });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(adaptation_debug_console_handler);

static void imgui_callback()
{
  g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("adaptation__debugger"));

  get_adaptation_debug_ecs_query([](AdaptationDebug &adaptation__debug) {
    get_adaptation_manager_ecs_query([&](AdaptationManager &adaptation__manager) {
      auto settings = adaptation__manager.getSettings();
      adaptation__debug.imgui(adaptation__manager.getLastExposureBuffer(), settings);
    });
  });
}

static constexpr auto IMGUI_WINDOW_GROUP = "Render";
static constexpr auto IMGUI_WINDOW = "Eye adaptation";
REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_WINDOW, imgui_callback);