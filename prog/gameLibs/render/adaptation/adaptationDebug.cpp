// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/adaptationDebug.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui.h>
#include <math/dag_mathBase.h>

void draw_adaptation_imgui(const uint32_t *hist256, const ExposureBuffer &exposureBuffer, const AdaptationSettings &settings,
  bool *draw_samples_distribution)
{
  if (!settings.isFixedExposure())
  {
    const auto getH = [](void *data, int idx) { return static_cast<float>(reinterpret_cast<uint32_t *>(data)[idx]); };
    const auto region = ImGui::GetContentRegionAvail();
    ImGui::Text("Note: values are at least 1 frame behind the actual image.");
    if (hist256)
      ImGui::PlotHistogram("##histogram", getH, const_cast<uint32_t *>(hist256), ADAPTATION_HISTOGRAM_BINS, 0, nullptr, FLT_MAX,
        FLT_MAX, ImVec2(region.x, 256));

    if (draw_samples_distribution && ImGui::Button("Show samples distribution", {600, 100}))
      *draw_samples_distribution = !*draw_samples_distribution;
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
