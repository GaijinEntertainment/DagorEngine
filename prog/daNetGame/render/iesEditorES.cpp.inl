// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <daECS/core/coreEvents.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <render/iesTextureManager.h>
#include <3d/dag_texMgr.h>
#include <math/integer/dag_IPoint3.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_TMatrix4D.h>
#include <sstream>

#include <render/renderLightsConsts.hlsli>

#define INSIDE_RENDERER 1
#include "world/private_worldRenderer.h"

static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }
static IesEditor *get_ies_editor()
{
  IesTextureCollection *iesTextureCollection = IesTextureCollection::getSingleton();
  if (iesTextureCollection == nullptr)
    return nullptr;
  return iesTextureCollection->requireEditor();
}

ECS_TAG(render, dev)
static void generate_ies_texture_es(const UpdateStageInfoRender &ECS_REQUIRE(ecs::Tag ies__editor__editing))
{
  IesEditor *iesEditor = get_ies_editor();
  if (iesEditor)
    iesEditor->renderIesTexture();
}

ECS_TAG(dev)
ECS_ON_EVENT(on_appear)
static void ies_edited_light_appear_es_event_handler(
  const ecs::Event &, ecs::string &light__texture_name, ecs::string &ies_editor_editing__original_ies_tex_name)
{
  IesEditor *iesEditor = get_ies_editor();
  if (!iesEditor)
    return;
  if (light__texture_name == IesTextureCollection::EDITOR_TEXTURE_NAME)
    return;
  ies_editor_editing__original_ies_tex_name = light__texture_name;
  light__texture_name = IesTextureCollection::EDITOR_TEXTURE_NAME;
}

ECS_TAG(dev)
ECS_ON_EVENT(on_disappear)
static void ies_edited_light_unedit_es_event_handler(
  const ecs::Event &, ecs::string &light__texture_name, const ecs::string &ies_editor_editing__original_ies_tex_name)
{
  IesEditor *iesEditor = get_ies_editor();
  if (!iesEditor)
    return;
  if (light__texture_name != IesTextureCollection::EDITOR_TEXTURE_NAME)
    return;
  light__texture_name = ies_editor_editing__original_ies_tex_name;
}

ECS_TAG(dev)
ECS_ON_EVENT(on_disappear)
static void ies_edited_light_unselect_es_event_handler(const ecs::Event &,
  ecs::EntityId eid ECS_REQUIRE(ecs::Tag ies__editor__editing, ecs::Tag daeditor__selected))
{
  remove_sub_template_async(eid, "ies_editor_editing");
}

template <typename Callable>
inline void selected_light_entity_ecs_query(Callable c);

void edit_selected_light_entity()
{
  selected_light_entity_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(const ecs::string &light__texture_name,
                                    ecs::Tag daeditor__selected) ECS_REQUIRE_NOT(ecs::Tag ies__editor__editing)) {
    add_sub_template_async(eid, "ies_editor_editing");
    return ecs::QueryCbResult::Continue;
  });
}

static std::string generate_ies_text(const IesEditor *ies_editor)
{
  std::stringstream ss;
  ss << "IESNA:\n";
  ss << "TILT=NONE\n";
  ss << "1 -1 1 " << ies_editor->getNumControlPoints() << " 1 1 1 0.0 0.0 0.0\n";
  ss << "1.0 1.0 0\n";
  for (int i = 0; i < ies_editor->getNumControlPoints(); ++i)
    ss << (RAD_TO_DEG * ies_editor->getPhotometryControlPoints()[i].theta) << '\n';
  ss << "0\n";
  for (int i = 0; i < ies_editor->getNumControlPoints(); ++i)
    ss << ies_editor->getPhotometryControlPoints()[i].lightIntensity << '\n';
  return ss.str();
}

static bool load_ies_text(IesEditor *ies_editor, const char *text)
{
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line) && strncmp(line.c_str(), "TILT=", 5) != 0)
    ;
  int numLamps, numVerticalAngles, numHorizontalAngles;
  float ignored;
  ss >> numLamps;
  ss >> ignored; // lumensPerLamp
  ss >> ignored; // multiplier
  ss >> numVerticalAngles;
  ss >> numHorizontalAngles;
  ss >> ignored; // photometricType
  ss >> ignored; // unitsType
  ss >> ignored; // dirX
  ss >> ignored; // dirY
  ss >> ignored; // dirZ
  ss >> ignored; // ballastFactor
  ss >> ignored; // futureUse
  ss >> ignored; // inputWatts
  if (numLamps != 1 || numVerticalAngles < 0 || numHorizontalAngles != 1)
    return false;
  eastl::vector<float> angles(numVerticalAngles);
  eastl::vector<float> intensities(numVerticalAngles);
  for (int i = 0; i < numVerticalAngles; ++i)
  {
    float angle;
    ss >> angle;
    angles[i] = angle * DEG_TO_RAD;
  }
  ss >> ignored; // the only horizontal angle
  for (int i = 0; i < numVerticalAngles; ++i)
    ss >> intensities[i];
  ies_editor->clearControlPoints();
  for (int i = 0; i < numVerticalAngles; ++i)
    ies_editor->addPointWithoutUpdate(angles[i], intensities[i]);
  ies_editor->recalcCoefficients();
  return true;
}

const int numPoints = 1000;
static void draw_window()
{
  static int lastEditedPoint = -1;
  IesEditor *iesEditor = get_ies_editor();
  if (iesEditor == nullptr)
    return;

  edit_selected_light_entity();

  ImPlot::SetNextAxesLimits(0, 181, 0, 1.2);
  if (ImPlot::BeginPlot("Radial light intensity ", "Angle from central direction (in degrees)", "Light intensity", ImVec2(-1, 0), 0,
        ImPlotAxisFlags_Lock))
  {
    ImPlot::PlotLineG(
      "Curve",
      [](int idx, void *iesEditor) {
        float x = float(idx) / numPoints;
        float intensity = static_cast<IesEditor *>(iesEditor)->getLightIntensityAt(x * M_PI);
        return ImPlotPoint(x * 180, intensity);
      },
      iesEditor, numPoints);
    PhotometryControlPoint *controlPoints = iesEditor->getPhotometryControlPoints();
    static IPoint2 swapIndices = IPoint2(-1, -1); // handle when points cross
    if (swapIndices.x >= iesEditor->getNumControlPoints() || swapIndices.y >= iesEditor->getNumControlPoints())
      swapIndices = IPoint2(-1, -1);
    bool draggedAnyPoints = false;
    for (uint32_t i = 0; i < iesEditor->getNumControlPoints(); ++i)
    {
      int pointIndex = i;
      if (i == swapIndices.x)
        pointIndex = swapIndices.y;
      else if (i == swapIndices.y)
        pointIndex = swapIndices.x;
      double x = clamp(controlPoints[pointIndex].theta / M_PI, 0.0, 1.0) * 180;
      double y = controlPoints[pointIndex].lightIntensity;
      if (ImPlot::DragPoint(i, &x, &y, IMPLOT_AUTO_COL))
      {
        draggedAnyPoints = true;
        lastEditedPoint = pointIndex;
        controlPoints[pointIndex].theta = x * M_PI / 180;
        controlPoints[pointIndex].lightIntensity = y;
        int newIndex = pointIndex;
        while (newIndex + 1 < iesEditor->getNumControlPoints() && controlPoints[newIndex].theta > controlPoints[newIndex + 1].theta)
        {
          eastl::swap(controlPoints[newIndex], controlPoints[newIndex + 1]);
          newIndex++;
        }
        while (newIndex > 0 && controlPoints[newIndex].theta < controlPoints[newIndex - 1].theta)
        {
          eastl::swap(controlPoints[newIndex], controlPoints[newIndex - 1]);
          newIndex--;
        }
        swapIndices = IPoint2(i, newIndex);
        iesEditor->recalcCoefficients();
      }
    }
    if (!draggedAnyPoints)
      swapIndices = IPoint2(-1, -1);
    ImPlot::EndPlot();
    if (ImGui::Button("Add control point"))
    {
      float rangeStart = M_PI / 2;
      float rangeEnd = M_PI / 2;
      for (int i = 0; i <= iesEditor->getNumControlPoints(); ++i) // pick the biggest range
      {
        float prevTheta = i == 0 ? 0 : controlPoints[i - 1].theta;
        float theta = i == iesEditor->getNumControlPoints() ? M_PI : controlPoints[i].theta;
        if (theta - prevTheta > rangeEnd - rangeStart)
        {
          rangeEnd = theta;
          rangeStart = prevTheta;
        }
      }
      float midTheta = (rangeStart + rangeEnd) / 2;
      lastEditedPoint = iesEditor->addPointWithUpdate(midTheta, iesEditor->getLightIntensityAt(midTheta));
    }
    if (ImGui::Button("Clear"))
      iesEditor->clearControlPoints();
    if (lastEditedPoint != -1 && lastEditedPoint < iesEditor->getNumControlPoints())
    {
      if (ImGui::Button("Remove last modified point"))
      {
        iesEditor->removeControlPoint(lastEditedPoint);
        lastEditedPoint = -1;
      }
    }
    if (ImGui::CollapsingHeader("ies string"))
    {
      constexpr int MAX_SIZE = 1 << 13;
      static char iesText[MAX_SIZE] = {0};
      ImGui::InputTextMultiline("ies data", iesText, MAX_SIZE);
      if (ImGui::Button("import from ies text"))
      {
        if (!load_ies_text(iesEditor, iesText))
          logerr("Ies text is not valid");
      }
      if (ImGui::Button("generate ies text"))
      {
        auto text = generate_ies_text(iesEditor);
        if (text.length() < MAX_SIZE)
        {
          memcpy(iesText, text.c_str(), text.length());
          memset(iesText + text.length(), 0, MAX_SIZE - text.length());
        }
        else
          logerr("Generated ies data is too large to display. Increase buffer size...");
      }
    }
  }
}

REGISTER_IMGUI_WINDOW_EX("Editor", "IES editor", nullptr, 100, ImGuiWindowFlags_MenuBar, draw_window);