// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "toastManagerInternal.h"

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndPublic.h>

#include <propPanel/imguiHelper.h>
#include <propPanel/colors.h>

#include <perfMon/dag_cpuFreq.h>

#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

static const ImGuiWindowFlags DEFAULT_TOAST_FLAGS = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs |
                                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
                                                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing;

void ToastMessage::setPosition(const IPoint2 &position, const Point2 &pivot)
{
  moveRequested = true;
  moveRequestPosition = Point2(position.x, position.y);
  moveRequestPivot = pivot;
}

void ToastMessage::setToMousePos(const Point2 &pivot)
{
  moveRequested = true;
  moveRequestPosition = ImGui::GetMousePos();
  moveRequestPivot = pivot;
}

void ToastMessage::setToCenterPos(const Point2 &pivot)
{
  moveRequested = true;
  moveRequestPosition = ImGui::GetMainViewport()->GetCenter();
  moveRequestPivot = pivot;
}

void ToastMessage::set(uint64_t _id, uint64_t set_time_ms)
{
  G_ASSERT(id == -1);
  id = _id;
  setTimeMs = set_time_ms;
}

void ToastMessage::startFadeout(uint64_t fadeout_time_ms)
{
  G_ASSERT(fadeoutTimeMs == 0);
  fadeoutTimeMs = fadeout_time_ms;
  fadeoutOnlyOnMouseLeave = false;
}

void ToastMessage::setSizeHint(Point2 size_hint) { sizeHint = size_hint; }

bool ToastMessage::beforeToast(uint64_t time_ms, float &alpha)
{
  G_ASSERT(id != -1);

  // No toast message should be around 10 sec or more. That seems pretty long...
  uint64_t elapsedTimeMs = time_ms - setTimeMs;
  if (elapsedTimeMs > 10000)
    return false;

  if (isFadeoutStarted())
  {
    // Remove the toast if it's invisible by now...
    const float fadingTimeMs = time_ms - fadeoutTimeMs;
    if (fadingTimeMs >= fadeoutIntervalMs)
      return false;

    float progress = fadingTimeMs / (float)fadeoutIntervalMs;
    alpha = max(0.0f, min(1.0f, 1.0f - progress));
    ImGui::SetNextWindowBgAlpha(alpha);
  }

  if (moveRequested)
  {
    // ImGui needs two frames to handle auto sizing,
    // but pos is enforced in the first two frames too, so "pop-in" is minimized.
    Point2 position = moveRequestPosition;
    if (sizeHint != Point2::ZERO)
    {
      ImGuiViewportP *mainViewport = (ImGuiViewportP *)ImGui::GetMainViewport();
      if (mainViewport)
      {
        // Check if the toast would be only partially visible and clamp its position request!
        const Point2 pivot = mul(sizeHint, moveRequestPivot);
        const Point2 min = position - pivot;
        const Point2 max = min + sizeHint;
        const ImRect viewportWorkRect = mainViewport->GetWorkRect();

        if (min.x < viewportWorkRect.Min.x)
          position.x += (viewportWorkRect.Min.x - min.x);
        if (min.y < viewportWorkRect.Min.y)
          position.y += (viewportWorkRect.Min.y - min.y);
        if (max.x > viewportWorkRect.Max.x)
          position.x -= (max.x - viewportWorkRect.Max.x);
        if (max.y > viewportWorkRect.Max.y)
          position.y -= (max.y - viewportWorkRect.Max.y);
      }
    }
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, moveRequestPivot);
    if (updates > 1)
      moveRequested = false;
  }

  return true;
}

void ToastMessage::updateToast(uint64_t time_ms)
{
  G_ASSERT(id != -1);

  if (!isFadeoutStarted())
  {
    if (fadeoutOnlyOnMouseLeave)
    {
      if (!moveRequested)
      {
        ImRect rect = ImGui::GetCurrentWindow()->Rect();
        rect.Expand(2.0f); // Safety pixels for clamped position requests
        if (!ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
          startFadeout(time_ms);
      }
    }
    else
    {
      uint64_t elapsedTimeMs = time_ms - setTimeMs;
      if (elapsedTimeMs >= opaqueIntervalMs)
        startFadeout(time_ms);
    }
  }

  updates++;
}

static ImVec4 getColorForToastType(ToastType type)
{
  int overrideId = PropPanel::ColorOverride::TOAST_TINT_NONE;
  if (type == ToastType::Alert)
    overrideId = PropPanel::ColorOverride::TOAST_TINT_ALERT;
  else if (type == ToastType::Success)
    overrideId = PropPanel::ColorOverride::TOAST_TINT_SUCCESS;

  return PropPanel::getOverriddenColor(overrideId);
}

PropPanel::IconId ToastManager::Icons::getForType(ToastType type)
{
  if (type == ToastType::Alert)
    return alert;
  else if (type == ToastType::Success)
    return success;

  return PropPanel::IconId::Invalid;
}

ToastManager::ToastManager() : debugMessage("Default Toast Message!"), debugPos(0, 0) {}

void ToastManager::loadIcons()
{
  icons.alert = PropPanel::load_icon("alert");
  icons.success = PropPanel::load_icon("success");
}

void ToastManager::updateImgui()
{
  const int timeMs = get_time_msec();
  for (auto it = messages.begin(); it != messages.end();)
  {
    ToastMessage &message = *it;

    float alpha = 1.0f;
    const bool isAlive = message.beforeToast(timeMs, alpha);
    if (!isAlive)
    {
      it = messages.erase(it);
      continue;
    }

    it++;

    ImGui::PushID(message.text.c_str());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.0f);
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    padding.x += 4.0f;
    padding.y += 2.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TOAST_BACKGROUND));
    ImVec4 color = getColorForToastType(message.type);
    color.w = alpha;
    ImGui::PushStyleColor(ImGuiCol_Border, color);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_Text, alpha));

    char windowName[15 + 20]{};
    snprintf(windowName, sizeof(windowName), "##toastMessage%" PRIu64, message.getId());

    ImGui::Begin(windowName, nullptr, DEFAULT_TOAST_FLAGS);

    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

    message.updateToast(timeMs);

    PropPanel::IconId icon = icons.getForType(message.type);
    if (icon != PropPanel::IconId::Invalid)
    {
      const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();
      ImGui::ImageWithBg(PropPanel::get_im_texture_id_from_icon_id(icon), fontSizedIconSize, ImVec2(0, 0), ImVec2(1, 1),
        ImVec4(0, 0, 0, 0), color);
      ImGui::SameLine();
    }
    ImGui::TextUnformatted(message.text.c_str(), message.text.c_str() + message.text.length());

    ImVec2 size = ImGui::GetCurrentWindow()->Size;
    message.setSizeHint(Point2(size.x, size.y));

    ImGui::End();

    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar(3);

    ImGui::PopID();
  }
}

void ToastManager::setToastMessage(ToastMessage message)
{
  static uint64_t id = 0;
  message.set(id++, get_time_msec());
  message.setSizeHint(Point2::ZERO);
  messages.push_back(message);
}

void ToastManager::updateImguiDebugPanel()
{
  const bool isDelaying = debugCreateDelayed;
  if (isDelaying)
    ImGui::BeginDisabled();

  ImGui::SliderInt("Opaque time (ms)", &debugOpaque, 500, 10000);
  ImGui::SliderInt("Fadeout time (ms)", &debugFadeout, 0, 10000);

  ImGuiDagor::InputText("Message", &debugMessage);

  dag::Vector<eastl::string_view> types = {"None", "Alert"};
  eastl::string debugInput;
  ImGuiDagor::ComboWithFilter(types[debugType].begin(), types, debugType, debugInput);

  dag::Vector<eastl::string_view> positioning = {"Pixel pos", "Mouse pos", "Center pos"};
  ImGuiDagor::ComboWithFilter(positioning[debugPosType].begin(), positioning, debugPosType, debugInput);
  IWndManager *manager = EDITORCORE->getWndManager();
  G_ASSERT(manager);
  unsigned clientWidth = 0;
  unsigned clientHeight = 0;
  manager->getWindowClientSize(manager->getMainWindow(), clientWidth, clientHeight);
  ImGui::BeginDisabled(debugPosType != 0 || isDelaying);
  ImGui::SliderInt("Pos X", &debugPos.x, 0, clientWidth);
  ImGui::SliderInt("Pos Y", &debugPos.y, 0, clientHeight);
  ImGui::EndDisabled();

  PropPanel::ImguiHelper::checkboxWithDragSelection("Center on position", &debugCenterOnPos);

  PropPanel::ImguiHelper::checkboxWithDragSelection("Fade out only on Mouse Leave", &debugFadeoutOnMouseLeave);

  PropPanel::ImguiHelper::checkboxWithDragSelection("Delay creation (by 1 Sec)", &debugDelayed);

  bool triggerDelayedCreate = false;
  if (isDelaying)
  {
    if ((get_time_msec() - debugDelayTimeMs) >= 1000)
      triggerDelayedCreate = true;
  }

  const float width = ImGui::GetContentRegionAvail().x;
  const bool create = ImGui::Button("Create", ImVec2(width, 0.0f));
  if (create || triggerDelayedCreate)
  {
    if (!debugDelayed || triggerDelayedCreate)
    {
      PropPanel::ToastMessage message;
      message.type = (PropPanel::ToastType)debugType;
      message.opaqueIntervalMs = debugOpaque;
      message.fadeoutIntervalMs = debugFadeout;
      message.text = debugMessage.c_str();
      const Point2 pivot = debugCenterOnPos ? Point2(0.5f, 0.5f) : Point2(0.0f, 0.0f);
      if (debugPosType == 0)
        message.setPosition(debugPos, pivot);
      else if (debugPosType == 1)
        message.setToMousePos(pivot);
      else
        message.setToCenterPos(pivot);
      message.fadeoutOnlyOnMouseLeave = debugFadeoutOnMouseLeave;

      PropPanel::set_toast_message(message);
      debugCreateDelayed = false;
    }
    else
    {
      debugDelayTimeMs = get_time_msec();
      debugCreateDelayed = true;
    }
  }

  if (isDelaying)
    ImGui::EndDisabled();
}

ToastManager toast_manager;

void load_toast_message_icons() { toast_manager.loadIcons(); }

void set_toast_message(const ToastMessage &message) { toast_manager.setToastMessage(message); }

void render_toast_messages() { toast_manager.updateImgui(); }

void imgui_toast_debug_panel() { toast_manager.updateImguiDebugPanel(); }

void show_toast_debug_panel() { imgui_window_set_visible("Debug", "Toast manager", true); }

} // namespace PropPanel

REGISTER_IMGUI_WINDOW("Debug", "Toast manager", PropPanel::imgui_toast_debug_panel);
