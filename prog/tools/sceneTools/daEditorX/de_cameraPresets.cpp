// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include "de_cameraPresets.h"

#include <EditorCore/ec_interface.h>
#include <propPanel/propPanel.h>
#include <propPanel/colors.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/control/spinEditStandalone.h>
#include <propPanel/control/filteredComboBoxStandalone.h>

#include <math/dag_mathBase.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>

#include <dag/dag_vector.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <libTools/util/undo.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/string_view.h>
#include <EASTL/unordered_set.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <ioSys/dag_dataBlock.h>

#include <stdio.h>
#include <string.h>

using PresetId = CameraPresetsManager::PresetId;
using VpBinding = CameraPresetsManager::VpBinding;
using CameraPose = CameraPresetsManager::CameraPose;
using Preset = CameraPresetsManager::Preset;

static constexpr float rowExtraPad = 4.0f;
static constexpr float rowGapPx = 1.0f;

static constexpr float highlightBlinkFade = 0.3f;
static constexpr float highlightBlinkDuration = 0.08f;
static constexpr float highlightBlinkTotal = 2 * highlightBlinkFade + highlightBlinkDuration;

static void cameraToPose(IGenViewportWnd &vp, CameraPose &out)
{
  TMatrix tm;
  vp.getCameraTransform(tm);
  out.pos = tm.getcol(3);
  const Point3 fwd = tm.getcol(2);
  const Point3 right = tm.getcol(0);
  out.pitchDeg = -safe_asin(-fwd.y) * RAD_TO_DEG;
  out.yawDeg = safe_atan2(-fwd.x, -fwd.z) * RAD_TO_DEG + 180.0f;
  const float rh = sqrtf(fwd.x * fwd.x + fwd.z * fwd.z);
  out.rollDeg =
    rh > 1e-5f ? floorf(safe_atan2(-right.y, right.x * fwd.z - right.z * fwd.x) * RAD_TO_DEG * 100.0f + 0.5f) / 100.0f : 0.0f;
  out.fov = vp.getFov() * RAD_TO_DEG;
}

static void poseToCamera(IGenViewportWnd &vp, const CameraPose &p)
{
  TMatrix yawTm, pitchTm, rollTm;
  yawTm.rotyTM(-p.yawDeg * DEG_TO_RAD);
  pitchTm.rotxTM(p.pitchDeg * DEG_TO_RAD);
  rollTm.rotzTM(p.rollDeg * DEG_TO_RAD);
  TMatrix tm = yawTm * pitchTm * rollTm;
  tm.setcol(3, p.pos);
  vp.setCameraTransform(tm);
  vp.setFov(p.fov * DEG_TO_RAD);
}

static bool matchesFilterCI(const char *str, const char *filter)
{
  if (!filter[0])
    return true;
  String s(str);
  s.toLower();
  String f(filter);
  f.toLower();
  return strstr(s.c_str(), f.c_str()) != nullptr;
}

static void trimView(eastl::string_view &v)
{
  while (!v.empty() && v.front() == ' ')
    v.remove_prefix(1);
  while (!v.empty() && v.back() == ' ')
    v.remove_suffix(1);
}


// --- Manager::State ---

struct CameraPresetsManager::State
{
  dag::Vector<Preset> presets; // real presets only, kept sorted by name
  CameraPose pseudoPose[4];
  PresetId nextId = FIRST_REAL_ID;

  bool highlightBlink = false;
  bool highlightBlinkButton = true;
  float highlightBlinkTimer = 0.0f;
  VpBinding highlightBlinkVp = VpBinding::None;

  struct VpRect
  {
    ImVec2 min = {}, max = {};
    bool valid = false;
  };
  VpRect vpRects[4];

  PropPanel::FilteredComboBoxStandalone combo[4];

  PropPanel::IconId addIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId cameraEditIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId sharedIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId unsharedIcon = PropPanel::IconId::Invalid;
  PropPanel::IconId vpIcon[4] = {};

  void loadIcons()
  {
    if (addIcon != PropPanel::IconId::Invalid)
      return;
    addIcon = PropPanel::load_icon("camera_add");
    cameraEditIcon = PropPanel::load_icon("camera_edit");
    sharedIcon = PropPanel::load_icon("share");
    unsharedIcon = PropPanel::load_icon("share_disabled");
    vpIcon[0] = PropPanel::load_icon("screen_active_1");
    vpIcon[1] = PropPanel::load_icon("screen_active_2");
    vpIcon[2] = PropPanel::load_icon("screen_active_3");
    vpIcon[3] = PropPanel::load_icon("screen_active_4");
  }

  void sortPresets()
  {
    eastl::sort(presets.begin(), presets.end(),
      [](const Preset &a, const Preset &b) { return strcmp(a.name.c_str(), b.name.c_str()) < 0; });
  }

  Preset *findById(PresetId id)
  {
    for (Preset &ps : presets)
      if (ps.id == id)
        return &ps;
    return nullptr;
  }

  const Preset *findById(PresetId id) const
  {
    for (const Preset &ps : presets)
      if (ps.id == id)
        return &ps;
    return nullptr;
  }

  int findIdxForViewport(int vi) const
  {
    for (int i = 0; i < (int)presets.size(); ++i)
      if ((int)presets[i].boundVp == vi)
        return i;
    return -1;
  }

  PresetId findIdForViewport(int vi) const
  {
    for (const Preset &ps : presets)
      if ((int)ps.boundVp == vi)
        return ps.id;
    return INVALID_ID;
  }

  bool isNameAvailable(eastl::string_view name, bool is_shared, PresetId except_id = INVALID_ID) const
  {
    for (const Preset &ps : presets)
      if (ps.id != except_id && ps.isShared == is_shared && name == ps.name.c_str())
        return false;
    return true;
  }

  void applyToViewport(const Preset &ps) const
  {
    if (ps.boundVp == VpBinding::None || ps.linkEnabled)
      return;
    IGenViewportWnd *vp = EDITORCORE->getViewport((int)ps.boundVp);
    if (vp)
      poseToCamera(*vp, ps.pose);
  }

  void triggerLinkBlink(int vpIdx, bool isBtn = true)
  {
    highlightBlink = true;
    highlightBlinkButton = isBtn;
    highlightBlinkTimer = 0.0f;
    highlightBlinkVp = (vpIdx >= 0 && vpIdx < 4) ? (VpBinding)vpIdx : VpBinding::None;
  }
};


// --- Manager public API ---

const dag::Vector<Preset> &CameraPresetsManager::getPresets() const { return state->presets; }

const Preset *CameraPresetsManager::getPresetById(PresetId id) const
{
  if (isPseudo(id))
    return nullptr;
  return state->findById(id);
}

CameraPose *CameraPresetsManager::getCameraPose(PresetId id) const
{
  if (isPseudo(id))
    return &state->pseudoPose[(int)id];
  Preset *ps = state->findById(id);
  return ps ? &ps->pose : nullptr;
}

int CameraPresetsManager::getPseudoVpIndex(PresetId id) const { return isPseudo(id) ? (int)id : -1; }

PresetId CameraPresetsManager::pseudoIdForVp(int vp_index) const
{
  if (vp_index >= 0 && vp_index < 4)
    return (PresetId)vp_index;
  return INVALID_ID;
}

bool CameraPresetsManager::isPseudo(PresetId id) const { return id < FIRST_REAL_ID; }

const char *CameraPresetsManager::getPresetName(PresetId id) const
{
  if (isPseudo(id))
  {
    const int vi = (int)id;
    if (EDITORCORE->getViewportCount() > 1 || vi > 0)
    {
      if (vi == 0)
        return "Current camera view 01";
      if (vi == 1)
        return "Current camera view 02";
      if (vi == 2)
        return "Current camera view 03";
      if (vi == 3)
        return "Current camera view 04";
    }
    return "Current camera view";
  }
  const Preset *ps = state->findById(id);
  return ps ? ps->name.c_str() : "";
}

bool CameraPresetsManager::isNameAvailable(eastl::string_view name, bool is_shared, PresetId except_id) const
{
  return state->isNameAvailable(name, is_shared, except_id);
}

String CameraPresetsManager::generateAvailableName(eastl::string_view name, bool is_shared)
{
  if (isNameAvailable(name, is_shared))
    return String(name.data(), name.size());

  size_t i = state->presets.size();
  String candidate;
  do
  {
    i += 1;
    candidate = String(name.data(), name.size()) + String(0, " %zu", i);
  } while (!isNameAvailable(candidate.c_str(), is_shared));
  return candidate;
}

bool CameraPresetsManager::setNameAndShared(PresetId id, eastl::string_view name, bool is_shared)
{
  if (isPseudo(id))
    return false;
  Preset *ps = state->findById(id);
  if (!ps)
    return false;
  eastl::string_view trimmed = name;
  trimView(trimmed);
  if (trimmed.empty())
    return false;
  if (ps->isShared == is_shared && ps->name.c_str() == trimmed)
    return true;
  if (!state->isNameAvailable(trimmed, is_shared, id))
    return false;

  struct RenameUndo : UndoRedoObject
  {
    CameraPresetsManager::State &ms;
    PresetId id;
    SimpleString prevName;
    SimpleString newName;
    bool prevIsShared;
    bool newIsShared;

    RenameUndo(CameraPresetsManager::State &s, PresetId id, eastl::string_view name, bool isShared) :
      ms(s), id(id), newName(name.data(), name.size()), newIsShared(isShared)
    {
      for (Preset &ps : ms.presets)
        if (ps.id == id)
        {
          prevName = ps.name;
          prevIsShared = ps.isShared;
          break;
        }
    }

    void restore(bool) override
    {
      for (Preset &ps : ms.presets)
        if (ps.id == id)
        {
          ps.name = prevName;
          ps.isShared = prevIsShared;
          break;
        }
    }
    void redo() override
    {
      for (Preset &ps : ms.presets)
        if (ps.id == id)
        {
          ps.name = newName;
          ps.isShared = newIsShared;
          break;
        }
      ms.sortPresets();
    }
    size_t size() override { return sizeof(Preset); }
    void accepted() override {}
    void get_description(String &s) override { s = String("Create camera preset"); }
  };

  UndoSystem *us = EDITORCORE->getUndoSystem();
  us->begin();
  us->put(new RenameUndo(*state, ps->id, trimmed, is_shared));
  us->accept("Rename camera preset");

  ps->name = SimpleString(trimmed.data(), trimmed.size());
  ps->isShared = is_shared;
  state->sortPresets();
  return true;
}

void CameraPresetsManager::setVpBinding(PresetId id, int vp_index)
{
  if (isPseudo(id))
  {
    if (vp_index == (int)id)
    {
      PresetId existing = state->findIdForViewport(vp_index);
      if (existing != INVALID_ID)
      {
        Preset *ps = state->findById(existing);
        if (ps)
        {
          ps->boundVp = VpBinding::None;
          ps->linkEnabled = false;
        }
      }
    }
    return;
  }

  Preset *ps = state->findById(id);
  if (!ps)
    return;

  if (vp_index < 0 || vp_index >= 4)
  {
    ps->boundVp = VpBinding::None;
    ps->linkEnabled = false;
    return;
  }

  if ((int)ps->boundVp == vp_index)
    return;

  for (Preset &other : state->presets)
    if (&other != ps && (int)other.boundVp == vp_index)
    {
      other.boundVp = VpBinding::None;
      other.linkEnabled = false;
    }

  ps->boundVp = (VpBinding)vp_index;
  ps->linkEnabled = false;
  state->applyToViewport(*ps);
}

void CameraPresetsManager::setLinkEnabled(PresetId id, bool link)
{
  if (isPseudo(id))
    return;
  Preset *ps = state->findById(id);
  if (ps)
    ps->linkEnabled = link;
}

PresetId CameraPresetsManager::addPresetFromCamera(eastl::string_view name, int vp_index, bool is_shared)
{
  if (!state->isNameAvailable(name, is_shared))
    return INVALID_ID;

  IGenViewportWnd *vp = EDITORCORE->getViewport(vp_index);
  if (!vp)
    return INVALID_ID;

  const PresetId newId = state->nextId++;

  struct CreateUndo : UndoRedoObject
  {
    CameraPresetsManager::State &ms;
    Preset saved;

    CreateUndo(CameraPresetsManager::State &s, Preset preset) : ms(s), saved(eastl::move(preset))
    {
      saved.boundVp = VpBinding::None;
      saved.linkEnabled = false;
    }

    void restore(bool) override
    {
      Preset *found = ms.findById(saved.id);
      if (found)
        ms.presets.erase(ms.presets.begin() + (found - ms.presets.begin()));
      ms.sortPresets();
    }
    void redo() override
    {
      ms.presets.push_back(saved);
      ms.sortPresets();
    }
    size_t size() override { return sizeof(Preset); }
    void accepted() override {}
    void get_description(String &s) override { s = String("Create camera preset"); }
  };


  Preset ps(newId, name, is_shared);
  cameraToPose(*vp, ps.pose);

  UndoSystem *us = EDITORCORE->getUndoSystem();
  us->begin();
  us->put(new CreateUndo(*state, ps));
  us->accept("Create camera preset");

  state->presets.push_back(eastl::move(ps));
  state->sortPresets();
  return newId;
}

void CameraPresetsManager::deletePresets(const dag::Vector<PresetId> &ids)
{
  if (ids.empty())
    return;

  dag::Vector<Preset> removed;
  for (PresetId bid : ids)
  {
    Preset *ps = state->findById(bid);
    if (ps)
      removed.push_back(*ps);
  }
  if (removed.empty())
    return;

  struct DeleteUndo : UndoRedoObject
  {
    CameraPresetsManager::State &ms;
    dag::Vector<Preset> saved;

    DeleteUndo(CameraPresetsManager::State &s, dag::Vector<Preset> &&pss) : ms(s), saved(eastl::move(pss))
    {
      for (Preset &ps : saved)
      {
        ps.boundVp = VpBinding::None;
        ps.linkEnabled = false;
      }
    }

    void restore(bool) override
    {
      for (const Preset &ps : saved)
        ms.presets.push_back(ps);
      ms.sortPresets();
    }
    void redo() override
    {
      for (const Preset &ps : saved)
      {
        Preset *found = ms.findById(ps.id);
        if (found)
          ms.presets.erase(ms.presets.begin() + (found - ms.presets.begin()));
      }
      ms.sortPresets();
    }
    size_t size() override { return saved.size() * sizeof(Preset); }
    void accepted() override {}
    void get_description(String &s) override
    {
      s = saved.size() == 1 ? String("Delete camera preset") : String("Delete camera presets");
    }
  };

  UndoSystem *us = EDITORCORE->getUndoSystem();
  us->begin();
  us->put(new DeleteUndo(*state, eastl::move(removed)));
  us->accept(ids.size() == 1 ? "Delete camera preset" : "Delete camera presets");

  for (PresetId bid : ids)
  {
    Preset *ps = state->findById(bid);
    if (ps)
      state->presets.erase(state->presets.begin() + (ps - state->presets.begin()));
  }
  state->sortPresets();
}

void CameraPresetsManager::captureViewportRect(int vp_index)
{
  if (vp_index >= 0 && vp_index < 4)
    state->vpRects[vp_index] = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true};
}

bool CameraPresetsManager::isPanelVisible() const { return panel != nullptr; }
CameraPresetsPanel *CameraPresetsManager::getPanel() const { return panel; }
CameraPresetsManager::State *CameraPresetsManager::getState() const { return state; }

void CameraPresetsManager::showPanel()
{
  if (!panel)
    panel = new CameraPresetsPanel(*this);
}

void CameraPresetsManager::hidePanel()
{
  delete panel;
  panel = nullptr;
}

PresetId CameraPresetsManager::findPresetByVpBinding(VpBinding vp)
{
  for (std::size_t i = 0; i < state->presets.size(); ++i)
  {
    const Preset &ps = state->presets[i];

    if (ps.boundVp == vp)
      return ps.id;
  }

  return INVALID_ID;
}


// --- Panel::State ---

struct CameraPresetsPanel::State
{
  CameraPresetsManager &mgr;

  eastl::unordered_set<PresetId> psSelected;

  PresetId settingsPsId = CameraPresetsManager::PSEUDO_VP1;
  char settingsName[128] = {};
  bool settingsIsShared = false;
  bool settingsNameError = false;         // persistent: true if last commit attempt failed
  bool pendingSettingsNameCommit = false; // reset every frame
  PresetId pendingNextId = CameraPresetsManager::INVALID_ID;
  bool requestAutoScroll = false; // auto-scroll to settingsPsId in the panel, auto-reset on scroll

  float splitRatio = 0.45f;

  String searchText;
  const int searchInputFocusId = 0;

  PropPanel::IconId searchIconId = PropPanel::IconId::Invalid;
  PropPanel::IconId clearIconId = PropPanel::IconId::Invalid;

  PropPanel::WindowControlEventHandler spinHandler;
  PropPanel::SpinEditControlStandalone spinX{-1e6f, 1e6f, 1.0f, 1};
  PropPanel::SpinEditControlStandalone spinY{-1e6f, 1e6f, 1.0f, 1};
  PropPanel::SpinEditControlStandalone spinZ{-1e6f, 1e6f, 1.0f, 1};
  PropPanel::SpinEditControlStandalone spinRX{-90.0f, 90.0f, 0.5f, 1};
  PropPanel::SpinEditControlStandalone spinRY{0.0f, 360.0f, 0.5f, 1};
  PropPanel::SpinEditControlStandalone spinRZ{-180.0f, 180.0f, 0.5f, 1};
  PropPanel::SpinEditControlStandalone spinFov{1.0f, 179.0f, 0.5f, 1};

  dag::Vector<PresetId> visibleIds;

  explicit State(CameraPresetsManager &m) : mgr(m) {}

  void loadPanelIcons()
  {
    if (searchIconId != PropPanel::IconId::Invalid)
      return;
    searchIconId = PropPanel::load_icon("search");
    clearIconId = PropPanel::load_icon("close_editor");
  }

  int countSelected() const { return (int)psSelected.size(); }

  PresetId onlySelectedId() const
  {
    if (psSelected.size() == 1)
      return *psSelected.begin();
    return CameraPresetsManager::INVALID_ID;
  }

  void requestNameEditCommit() { pendingSettingsNameCommit = true; }

  void setSettingsPreset(PresetId id)
  {
    requestNameEditCommit();
    pendingNextId = id;
  }

  void deleteSelected()
  {
    dag::Vector<PresetId> ids(psSelected.begin(), psSelected.end());
    if (ids.empty())
      return;
    psSelected.clear();
    mgr.deletePresets(ids);
    resetSettingsPresetSelection();
  }

  // --- Presets list rendering ---

  void renderPseudoPresetRow(int vi)
  {
    const int vpCount = EDITORCORE->getViewportCount();
    if (vi >= vpCount)
      return;

    const PresetId pid = CameraPresetsManager::PSEUDO_VP1 + (PresetId)vi;

    const float internalH = ImGui::GetFrameHeight();
    const float rowH = internalH + rowExtraPad * 2.0f;
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax = {rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + rowH};
    const ImVec2 textPos = {rowMin.x + rowExtraPad, rowMin.y + (rowH - ImGui::GetTextLineHeight()) * 0.5f};
    const float iconDim = internalH * 0.9f;
    const ImVec2 iconSz = {iconDim, iconDim};
    const float iconY = rowMin.y + (rowH - iconDim) * 0.5f;
    const float vpIconX = rowMax.x - iconDim - rowExtraPad;
    const float textMaxX = vpIconX - rowExtraPad;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImGui::PushStyleColor(ImGuiCol_Header, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TREE_SELECTION_BACKGROUND));

    const bool isSettings = (settingsPsId == pid);
    ImGui::PushID(-(vi + 1));

    if (isSettings)
      dl->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImGuiCol_Header));

    ImGui::SetCursorScreenPos(rowMin);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("##pseudo", {rowMax.x - rowMin.x, rowH});
    if (ImGui::IsItemHovered())
    {
      dl->AddRectFilled(rowMin, rowMax, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (ImGui::IsItemClicked())
      setSettingsPreset(pid);
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      mgr.setVpBinding(pid, vi);

    const CameraPresetsManager::State *ms = mgr.getState();
    if (ms->findIdxForViewport(vi) == -1)
    {
      const ImVec2 iMin = {vpIconX, iconY};
      ImGui::SetCursorScreenPos(iMin);
      ImGui::Image(PropPanel::get_im_texture_id_from_icon_id(ms->vpIcon[vi]), iconSz);
      if (ImGui::IsMouseHoveringRect(iMin, iMin + iconSz))
        ImGui::SetTooltip("Active in Viewport %d", vi + 1);
    }

    ImGui::SetCursorScreenPos(textPos);
    ImGui::PushClipRect(textPos, {textMaxX, rowMax.y}, true);
    ImGui::TextUnformatted(mgr.getPresetName(pid));
    ImGui::PopClipRect();

    ImGui::PopID();
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos({rowMin.x, rowMax.y + rowGapPx});
    ImGui::Dummy({0, 0});
  }

  void renderRealPresetRow(int visPos, PresetId bid)
  {
    const Preset *ps = mgr.getPresetById(bid);
    if (!ps)
      return;
    const CameraPresetsManager::State *ms = mgr.getState();
    const int vpCount = EDITORCORE->getViewportCount();

    const float internalH = ImGui::GetFrameHeight();
    const float rowH = internalH + rowExtraPad * 2.0f;
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax = {rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + rowH};
    const ImVec2 textPos = {rowMin.x + rowExtraPad, rowMin.y + (rowH - ImGui::GetTextLineHeight()) * 0.5f};
    const float iconDim = internalH * 0.9f;
    const ImVec2 iconSz = {iconDim, iconDim};
    const float iconY = rowMin.y + (rowH - iconDim) * 0.5f;
    const float vpIconX = rowMax.x - iconDim - rowExtraPad;
    const float sharedIconX = vpIconX - iconDim - rowExtraPad;
    const float textMaxX = sharedIconX - rowExtraPad;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImGui::PushStyleColor(ImGuiCol_Header, PropPanel::getOverriddenColor(PropPanel::ColorOverride::TREE_SELECTION_BACKGROUND));

    const bool isSel = psSelected.count(bid) > 0;
    const bool isEdited = settingsPsId == bid;

    ImGui::SetNextItemSelectionUserData(visPos);
    ImGui::SetCursorScreenPos(rowMin);
    ImGui::Selectable("##sel", isSel, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
      ImVec2(rowMax.x - rowMin.x, rowH));

    if (isEdited && requestAutoScroll)
    {
      const float windowMinY = ImGui::GetWindowPos().y;
      const float windowMaxY = windowMinY + ImGui::GetWindowHeight();
      if (rowMin.y < windowMinY)
        ImGui::SetScrollHereY(0.0f);
      else if (rowMax.y > windowMaxY)
        ImGui::SetScrollHereY(1.0f);
      requestAutoScroll = false;
    }

    const bool hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked())
      setSettingsPreset(bid);
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      mgr.setVpBinding(bid, 0);
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
      mgr.setVpBinding(bid, -1);
    if (ImGui::IsItemToggledSelection())
      setSettingsPreset(bid);

    const int bvp = (int)ps->boundVp;
    if (bvp >= 0 && bvp < 4)
    {
      const ImVec2 iMin = {vpIconX, iconY};
      dl->AddImage(PropPanel::get_im_texture_id_from_icon_id(ms->vpIcon[bvp]), iMin, {iMin.x + iconSz.x, iMin.y + iconSz.y},
        ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);
      if (hovered && ImGui::IsMouseHoveringRect(iMin, {iMin.x + iconSz.x, iMin.y + iconSz.y}))
        ImGui::SetTooltip("Active in Viewport %d", bvp + 1);
    }

    if (ps->isShared)
    {
      const ImVec2 iMin = {sharedIconX, iconY};
      dl->AddImage(PropPanel::get_im_texture_id_from_icon_id(ms->sharedIcon), iMin, {iMin.x + iconSz.x, iMin.y + iconSz.y},
        ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);
      if (hovered && ImGui::IsMouseHoveringRect(iMin, {iMin.x + iconSz.x, iMin.y + iconSz.y}))
      {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Shared camera view");
        ImGui::TextDisabled("Saved in the project (commit changes to CVS)");
        ImGui::EndTooltip();
      }
    }

    if (isEdited)
    {
      const float rounding = ImGui::GetStyle().FrameRounding;
      const float thickness = 2.0f;
      const ImU32 col = ImGui::GetColorU32(ImGuiCol_NavCursor);
      dl->AddRect(rowMin, rowMax, col, rounding, 0, thickness);
    }

    const float textClipX = ps->isShared ? (sharedIconX - rowExtraPad) : textMaxX;
    dl->PushClipRect(textPos, {textClipX, rowMax.y}, true);
    dl->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), ps->name.c_str());
    dl->PopClipRect();

    if (ImGui::BeginPopupContextItem("##ps_ctx"))
    {
      const PresetId single = onlySelectedId();
      const Preset *ps = mgr.getPresetById(single);
      const bool canSwitchShared = mgr.isNameAvailable(ps->name.c_str(), !ps->isShared);
      if (single != CameraPresetsManager::INVALID_ID)
      {
        if (vpCount == 1)
        {
          if (ImGui::MenuItem("Set to viewport"))
            mgr.setVpBinding(single, 0);
        }
        else if (ImGui::BeginMenu("Set to viewport"))
        {
          for (int vp = 0; vp < vpCount; ++vp)
          {
            char lbl[16];
            snprintf(lbl, sizeof(lbl), "Viewport %d", vp + 1);
            if (ImGui::MenuItem(lbl))
              mgr.setVpBinding(single, vp);
          }
          ImGui::EndMenu();
        }
      }
      if (!canSwitchShared)
        ImGui::BeginDisabled();
      if (ImGui::MenuItem(ps->isShared ? "Make local" : "Make shared"))
        mgr.setNameAndShared(single, ps->name.c_str(), !ps->isShared);
      if (!canSwitchShared)
        ImGui::EndDisabled();
      if (ImGui::MenuItem("Delete"))
        deleteSelected();
      ImGui::EndPopup();
    }

    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos({rowMin.x, rowMax.y + rowGapPx});
    ImGui::Dummy({0, 0});
  }

  void renderPresetListContent()
  {
    const int vpCount = EDITORCORE->getViewportCount();
    const bool filtering = searchText.c_str()[0] != '\0';
    const dag::Vector<Preset> &allPs = mgr.getPresets();
    const int nPs = (int)allPs.size();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0));

    for (int i = 0; i < vpCount; ++i)
      renderPseudoPresetRow(i);

    ImGui::Separator();

    // Build visibleIds for this frame
    visibleIds.clear();
    for (const Preset &ps : allPs)
    {
      const bool isSettingsPs = (settingsPsId == ps.id);
      const bool isSelPs = psSelected.count(ps.id) > 0;
      if (filtering && !isSettingsPs && !isSelPs && !matchesFilterCI(ps.name.c_str(), searchText.c_str()))
        continue;
      visibleIds.push_back(ps.id);
    }

    ImGuiMultiSelectIO *msio = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d |
                                                         ImGuiMultiSelectFlags_ScopeWindow | ImGuiMultiSelectFlags_ClearOnClickVoid,
      countSelected(), (int)visibleIds.size());

    ImGuiSelectionExternalStorage selStorage;
    selStorage.UserData = this;
    selStorage.AdapterSetItemSelected = [](ImGuiSelectionExternalStorage *s, int n, bool v) {
      State *st = (State *)s->UserData;
      if (n >= 0 && n < (int)st->visibleIds.size())
      {
        const PresetId bid = st->visibleIds[n];
        if (v)
          st->psSelected.insert(bid);
        else
          st->psSelected.erase(bid);
      }
    };
    selStorage.ApplyRequests(msio);

    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    for (int i = 0; i < (int)visibleIds.size(); ++i)
    {
      ImGui::PushID(i);
      renderRealPresetRow(i, visibleIds[i]);
      ImGui::PopID();
    }

    msio = ImGui::EndMultiSelect();
    selStorage.ApplyRequests(msio);

    if (nPs == 0)
    {
      ImGui::Dummy(ImVec2(0.f, 4.f));
      ImGui::TextDisabled("  (empty)");
    }
    else if (visibleIds.empty())
    {
      ImGui::Dummy(ImVec2(0.f, 4.f));
      ImGui::TextDisabled("  (no results)");
    }

    ImGui::PopStyleVar();

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))
      deleteSelected();
  }

  // --- Settings pane rendering ---

  void renderSpinField(PropPanel::SpinEditControlStandalone &spin, float &val, float w)
  {
    spin.setValue(val);
    ImGui::SetNextItemWidth(w);
    spin.updateImgui(spinHandler);
    val = spin.getValue();
  }

  bool renderXYZRow(PropPanel::SpinEditControlStandalone &sx, float &vx, PropPanel::SpinEditControlStandalone &sy, float &vy,
    PropPanel::SpinEditControlStandalone &sz, float &vz, float spinW)
  {
    const float prev[3] = {vx, vy, vz};
    const char *labels[3] = {"X:", "Y:", "Z:"};
    for (int i = 0; i < 3; ++i)
    {
      if (i > 0)
        ImGui::SameLine(0, 10.0f);
      ImGui::PushID(i);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(labels[i]);
      ImGui::SameLine(0, 4.0f);
      float *v = i == 0 ? &vx : (i == 1 ? &vy : &vz);
      auto &sp = i == 0 ? sx : (i == 1 ? sy : sz);
      renderSpinField(sp, *v, spinW);
      ImGui::PopID();
    }
    return vx != prev[0] || vy != prev[1] || vz != prev[2];
  }

  void renderSettingsContent()
  {
    if (!mgr.isPseudo(settingsPsId) && !mgr.getPresetById(settingsPsId))
    {
      psSelected.clear();
      resetSettingsPresetSelection();
    }

    const bool isPseudo = mgr.isPseudo(settingsPsId);
    const Preset *ps = isPseudo ? nullptr : mgr.getPresetById(settingsPsId);

    CameraPose *pose = mgr.getCameraPose(settingsPsId);

    if (!pose)
    {
      ImGui::Spacing();
      ImGui::TextDisabled("Select a camera preset from the list.");
      return;
    }

    ImGui::PushID(settingsPsId);

    const float avail = ImGui::GetContentRegionAvail().x;
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    const float lW = ImGui::CalcTextSize("X:").x;
    const float spinW3 = eastl::max((avail - (lW + 4.0f) * 3.0f - 10.0f * 2.0f) / 3.0f, 40.0f);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const CameraPresetsManager::State *ms = mgr.getState();

    ImGui::Spacing();

    if (isPseudo)
    {
      ImGui::TextUnformatted(mgr.getPresetName(settingsPsId));
    }
    else
    {
      ImGui::TextUnformatted("Name:");
      const float cbW = ImGui::GetFrameHeight() + sp;

      eastl::string_view trimmed(settingsName);
      trimView(trimmed);
      const bool nameEmptyOrCollides = trimmed.empty() || !mgr.isNameAvailable(trimmed, settingsIsShared, settingsPsId);
      const bool nameError = settingsNameError | nameEmptyOrCollides;

      if (nameError)
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
          PropPanel::getOverriddenColor(PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND));
      ImGui::SetNextItemWidth(avail - cbW - sp);
      ImGui::InputText("##nm", settingsName, sizeof(settingsName));
      if (nameError)
        ImGui::PopStyleColor();

      if (ImGui::IsItemDeactivatedAfterEdit())
      {
        requestNameEditCommit();
      }

      ImGui::SameLine();
      {
        const ImVec2 sz = {ImGui::GetFrameHeight(), ImGui::GetFrameHeight()};
        if (ImGui::Button("##shared", sz))
        {
          settingsIsShared = !settingsIsShared;
          requestNameEditCommit();
        }
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          if (settingsIsShared)
          {
            ImGui::TextUnformatted("Shared preset (commit changes to CVS)");
            ImGui::TextDisabled("Left click: Change to local preset");
          }
          else
          {
            ImGui::TextUnformatted("Local preset");
            ImGui::TextDisabled("Left click: Change to shared preset");
          }
          ImGui::EndTooltip();
        }
        const ImVec2 btnMin = ImGui::GetItemRectMin();
        const ImVec2 btnMax = ImGui::GetItemRectMax();
        const ImGuiStyle &style = ImGui::GetStyle();
        dl->AddImage(PropPanel::get_im_texture_id_from_icon_id(settingsIsShared ? ms->sharedIcon : ms->unsharedIcon),
          {btnMin.x + style.FramePadding.x, btnMin.y + style.FramePadding.y},
          {btnMax.x - style.FramePadding.x, btnMax.y - style.FramePadding.y});
      }

      if (nameError)
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "The name must be unique and non-empty");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool isLinked = !isPseudo && ps && ps->linkEnabled;
    const bool pseudoBlocked = isPseudo && (ms->findIdxForViewport(mgr.getPseudoVpIndex(settingsPsId)) >= 0);
    const bool disableEdits = isLinked || pseudoBlocked;

    const ImVec2 disabledBlockStart = ImGui::GetCursorScreenPos();
    ImVec2 disabledBlockEnd = disabledBlockStart;

    if (disableEdits)
      ImGui::BeginDisabled();

    ImGui::TextUnformatted("Location, m:");
    ImGui::PushID("loc");
    {
      const float bx = pose->pos.x, by = pose->pos.y, bz = pose->pos.z;
      float x = bx, y = by, z = bz;
      renderXYZRow(spinX, x, spinY, y, spinZ, z, spinW3);
      disabledBlockEnd.x = ImGui::GetItemRectMax().x;
      if (x != bx || y != by || z != bz)
      {
        pose->pos = {x, y, z};
        if (!isPseudo && ps)
          mgr.getState()->applyToViewport(*ps);
        else if (isPseudo)
        {
          IGenViewportWnd *vp = EDITORCORE->getViewport(mgr.getPseudoVpIndex(settingsPsId));
          if (vp)
            poseToCamera(*vp, *pose);
        }
      }
    }
    ImGui::PopID();

    ImGui::Spacing();
    ImGui::TextUnformatted("Rotation, deg:");
    ImGui::PushID("rot");
    {
      const float bp = pose->pitchDeg, byw = pose->yawDeg, br = pose->rollDeg;
      float p = bp, yw = byw, r = br;
      renderXYZRow(spinRX, p, spinRY, yw, spinRZ, r, spinW3);
      if (p != bp || yw != byw || r != br)
      {
        pose->pitchDeg = p;
        pose->yawDeg = yw;
        pose->rollDeg = r;
        if (!isPseudo && ps)
          mgr.getState()->applyToViewport(*ps);
        else if (isPseudo)
        {
          IGenViewportWnd *vp = EDITORCORE->getViewport(mgr.getPseudoVpIndex(settingsPsId));
          if (vp)
            poseToCamera(*vp, *pose);
        }
      }
    }
    ImGui::PopID();

    ImGui::Spacing();
    ImGui::PushID("fov");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("FOV, deg:");
    ImGui::SameLine(0, 6);
    {
      const float bf = pose->fov;
      float f = bf;
      renderSpinField(spinFov, f, -FLT_MIN);
      if (f != bf)
      {
        pose->fov = f;
        if (!isPseudo && ps)
          mgr.getState()->applyToViewport(*ps);
        else if (isPseudo)
        {
          IGenViewportWnd *vp = EDITORCORE->getViewport(mgr.getPseudoVpIndex(settingsPsId));
          if (vp)
            vp->setFov(f * DEG_TO_RAD);
        }
      }
    }
    ImGui::PopID();

    if (disableEdits)
      ImGui::EndDisabled();

    if (disableEdits)
    {
      disabledBlockEnd.y = ImGui::GetItemRectMax().y;
      const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                           ImGui::IsMouseHoveringRect(disabledBlockStart, disabledBlockEnd);
      if (hovered)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        ImGui::BeginTooltip();
        if (isLinked)
        {
          ImGui::TextUnformatted("Currently edited through viewport");
          ImGui::TextDisabled("Left click: Highlight");
          ImGui::TextDisabled("Double click: Disable editing");
        }
        else
        {
          ImGui::TextUnformatted("This camera is occupied by preset");
          ImGui::TextDisabled("Left click: Highlight");
          ImGui::TextDisabled("Double click: Go to the preset");
        }
        ImGui::EndTooltip();
        ImGui::PopStyleVar();

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
          if (isLinked && ps)
            mgr.setLinkEnabled(settingsPsId, false);
          else if (pseudoBlocked)
          {
            const int vi = mgr.getPseudoVpIndex(settingsPsId);
            PresetId occ = mgr.getState()->findIdForViewport(vi);
            if (occ != CameraPresetsManager::INVALID_ID)
              setSettingsPreset(occ);
          }
        }
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
          const int blinkVp = isLinked ? (int)(ps->boundVp) : mgr.getPseudoVpIndex(settingsPsId);
          mgr.getState()->triggerLinkBlink(blinkVp, isLinked);
        }
      }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button(isPseudo ? "Create as a new preset" : "Create copy", {avail, 0}))
    {
      const bool newShared = isPseudo ? false : (ps ? ps->isShared : false);


      String requestedName;

      if (isPseudo)
        requestedName = "New preset";
      else
      {
        requestedName = mgr.getPresetName(settingsPsId);
        requestedName += " Copy";
      }

      String newName = mgr.generateAvailableName(requestedName.data(), newShared);

      const PresetId newId = mgr.addPresetFromCamera(newName.c_str(), isPseudo ? mgr.getPseudoVpIndex(settingsPsId) : 0, newShared);
      if (newId != CameraPresetsManager::INVALID_ID)
      {
        Preset *nb = mgr.getState()->findById(newId);
        if (nb)
          nb->pose = *pose;
        psSelected.clear();
        psSelected.insert(newId);
        setSettingsPreset(newId);
      }
    }
    ImGui::Spacing();
    ImGui::PopID();
  }

  // --- Top-level update ---

  void update()
  {
    mgr.getState()->loadIcons();
    loadPanelIcons();

    ImGui::Spacing();
    ImGui::SetNextItemWidth(-FLT_MIN);
    PropPanel::ImguiHelper::searchInput(&searchInputFocusId, "##ps_search", "Search presets...", searchText, searchIconId,
      clearIconId);
    ImGui::Spacing();

    const float totalW = ImGui::GetContentRegionAvail().x;
    const float splitterBtnW = ImGui::GetStyle().ItemSpacing.x;
    const float minPaneW = 80.0f;
    const float usable = totalW - splitterBtnW;
    float leftW = eastl::max(eastl::min(splitRatio * totalW, usable - minPaneW), minPaneW);

    {
      const float innerOff = ImGui::GetStyle().ChildBorderSize + ImGui::GetStyle().WindowPadding.x;
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(innerOff, ImGui::GetStyle().SeparatorTextPadding.y));
      if (ImGui::BeginTable("##hdr", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
      {
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, leftW);
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, splitterBtnW);
        ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::SeparatorText("Presets");
        ImGui::TableSetColumnIndex(2);
        ImGui::SeparatorText("Preset settings");
        ImGui::EndTable();
      }
      ImGui::PopStyleVar(2);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
    ImGui::BeginChild("##left_pane", {leftW, 0}, ImGuiChildFlags_Borders);
    ImGui::PopStyleColor();
    renderPresetListContent();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);
    {
      const float paneH = ImGui::GetItemRectSize().y;
      ImGui::SetNextItemAllowOverlap();
      ImGui::InvisibleButton("##split", {splitterBtnW, paneH});
      const bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
      if (hov || act)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      if (act)
      {
        leftW = eastl::max(eastl::min(leftW + ImGui::GetIO().MouseDelta.x, usable - minPaneW), minPaneW);
        splitRatio = leftW / totalW;
      }
      const ImVec2 bMin = ImGui::GetItemRectMin(), bMax = ImGui::GetItemRectMax();
      const float midX = (bMin.x + bMax.x) * 0.5f;
      const ImU32 col = ImGui::GetColorU32(act ? ImGuiCol_SeparatorActive : hov ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
      ImGui::GetWindowDrawList()->AddLine({midX, bMin.y}, {midX, bMax.y}, col, 1.0f);
    }
    ImGui::SameLine(0, 0);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
    ImGui::BeginChild("##right_pane", {0, 0}, ImGuiChildFlags_Borders);
    ImGui::PopStyleColor();
    renderSettingsContent();
    ImGui::EndChild();

    ImGui::PopStyleVar();

    if (pendingSettingsNameCommit && !mgr.isPseudo(settingsPsId) && settingsPsId != CameraPresetsManager::INVALID_ID)
    {
      eastl::string_view name(settingsName);
      trimView(name);
      settingsNameError = !mgr.setNameAndShared(settingsPsId, name, settingsIsShared);
      pendingSettingsNameCommit = false;
    }

    if (pendingNextId != CameraPresetsManager::INVALID_ID)
    {
      if (mgr.isPseudo(pendingNextId))
      {
        settingsIsShared = false;
        snprintf(settingsName, sizeof(settingsName), "%s", mgr.getPresetName(pendingNextId));
        psSelected.clear();
      }
      else
      {
        const Preset *ps = mgr.getPresetById(pendingNextId);
        if (ps)
        {
          snprintf(settingsName, sizeof(settingsName), "%s", ps->name.c_str());
          settingsIsShared = ps->isShared;
        }
      }

      settingsPsId = pendingNextId;
      pendingNextId = CameraPresetsManager::INVALID_ID;
      requestAutoScroll = true;
    }
  }

  void resetSettingsPresetSelection()
  {
    PresetId vpPreset = CameraPresetsManager::INVALID_ID;
    for (int i = 0; i < EDITORCORE->getViewportCount(); ++i)
    {
      vpPreset = mgr.findPresetByVpBinding(VpBinding(i));
      if (vpPreset != CameraPresetsManager::INVALID_ID)
      {
        settingsPsId = vpPreset;
        break;
      }
    }

    if (vpPreset == CameraPresetsManager::INVALID_ID)
    {
      settingsPsId = CameraPresetsManager::PSEUDO_VP1;
    }

    const Preset *ps = mgr.getPresetById(settingsPsId);

    settingsNameError = false;
    snprintf(settingsName, sizeof(settingsName), "%s", mgr.getPresetName(settingsPsId));
    settingsIsShared = ps ? ps->isShared : false;
  }
};


void CameraPresetsPanel::setEditingPreset(PresetId id) { state->setSettingsPreset(id); }


// --- Manager load / save ---

CameraPresetsManager::CameraPresetsManager(const String &ps_path) :
  state(new State()), localPsBlkPath(ps_path + "/camera_presets.local.blk"), sharedPsBlkPath(ps_path + "/camera_presets.blk")
{
  auto loadFromBlk = [&](const char *path, bool isShared) {
    DataBlock blk;
    if (!blk.load(path))
      return;
    dblk::iterate_blocks_by_name(blk, "preset", [&](const DataBlock &b) {
      const char *name = b.getStr("name", "");
      if (!name[0])
        return;
      Preset ps(state->nextId++, name, isShared);
      ps.pose.pos = b.getPoint3("pos", Point3());
      ps.pose.pitchDeg = b.getReal("pitch", 0.0f);
      ps.pose.yawDeg = b.getReal("yaw", 0.0f);
      ps.pose.rollDeg = b.getReal("roll", 0.0f);
      ps.pose.fov = b.getReal("fov", 60.0f);
      state->presets.push_back(eastl::move(ps));
    });
  };
  loadFromBlk(localPsBlkPath, false);
  loadFromBlk(sharedPsBlkPath, true);
  state->sortPresets();
}

CameraPresetsManager::~CameraPresetsManager()
{
  delete panel;

  DataBlock localBlk, sharedBlk;
  for (const Preset &ps : state->presets)
  {
    DataBlock *b = (ps.isShared ? sharedBlk : localBlk).addNewBlock("preset");
    b->setStr("name", ps.name.c_str());
    b->setPoint3("pos", ps.pose.pos);
    b->setReal("pitch", ps.pose.pitchDeg);
    b->setReal("yaw", ps.pose.yawDeg);
    b->setReal("roll", ps.pose.rollDeg);
    b->setReal("fov", ps.pose.fov);
  }
  localBlk.saveToTextFile(localPsBlkPath);
  sharedBlk.saveToTextFile(sharedPsBlkPath);

  delete state;
}


// --- Panel ---

CameraPresetsPanel::CameraPresetsPanel(CameraPresetsManager &manager) : state(new State(manager)), mgr(manager)
{
  state->resetSettingsPresetSelection();
}

CameraPresetsPanel::~CameraPresetsPanel()
{
  state->requestNameEditCommit();
  delete state;
}

void CameraPresetsPanel::updateImgui() { state->update(); }


// --- Overlay / blink ---

static ImVec4 colWithBlink(bool isBlink, float time, ImVec4 origCol)
{
  const ImVec4 blinkCol =
    ImGui::ColorConvertU32ToFloat4(PropPanel::getOverriddenColorU32(PropPanel::ColorOverride::BLINK_HIGHTLIGHT_ANIMATION_COLOR));
  if (!isBlink || time < 0.f || time >= highlightBlinkTotal)
    return origCol;
  if (time < highlightBlinkFade)
    return lerp(origCol, blinkCol, time / highlightBlinkFade);
  if (time < highlightBlinkFade + highlightBlinkDuration)
    return blinkCol;
  return lerp(blinkCol, origCol, (time - highlightBlinkFade - highlightBlinkDuration) / highlightBlinkFade);
}

void CameraPresetsManager::renderAllViewportOverlays()
{
  state->loadIcons();

  const int vpCount = EDITORCORE->getViewportCount();
  const float winPad = 2.0f;
  const float margin = 6.0f;

  for (int vi = 0; vi < vpCount; ++vi)
  {
    State::VpRect &rc = state->vpRects[vi];
    if (!rc.valid)
      continue;

    IGenViewportWnd *vp = EDITORCORE->getViewport(vi);
    if (vp)
      cameraToPose(*vp, state->pseudoPose[vi]);

    const int psIdx = state->findIdxForViewport(vi);

    int vpX = 0, vpY = 0;
    if (vp)
      vp->getViewportSize(vpX, vpY);

    if (psIdx >= 0 && vp)
    {
      Preset &ps = state->presets[psIdx];
      if (ps.linkEnabled)
        ps.pose = state->pseudoPose[vi];
      else
        poseToCamera(*vp, ps.pose);
    }

    const float frameH = ImGui::GetFrameHeight();
    const float btnSz = frameH;
    const float itemGap = 2.0f;
    const float innerW = 460.0f;
    const float comboW = innerW - 2.0f * (btnSz + itemGap);
    const float winW = innerW + 2.0f * winPad;
    const float winH = frameH + 2.0f * winPad;

    if (vpX * 0.7f < winW)
      continue;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(winPad, winPad));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

    ImGui::SetNextWindowPos({rc.max.x - winW - margin, rc.min.y + margin}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({winW, winH}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    char wndName[32];
    snprintf(wndName, sizeof(wndName), "##vpps%d", vi);

    const bool visible = ImGui::Begin(wndName, nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoFocusOnAppearing);

    if (!visible)
    {
      ImGui::PopStyleVar(3);
      ImGui::End();
      continue;
    }

    const ImVec2 iconSz(btnSz - ImGui::GetStyle().FramePadding.x * 2.0f, btnSz - ImGui::GetStyle().FramePadding.y * 2.0f);

    if (ImGui::ImageButton("##add", PropPanel::get_im_texture_id_from_icon_id(state->addIcon), iconSz))
    {
      String name = generateAvailableName("New preset");
      const PresetId newId = addPresetFromCamera(name.c_str(), vi);
      if (newId != INVALID_ID)
      {
        setVpBinding(newId, vi);
        if (panel)
          panel->setEditingPreset(newId);
      }
    }
    if (ImGui::IsItemHovered())
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
      ImGui::SetTooltip("Record current camera as preset");
      ImGui::PopStyleVar();
    }

    ImGui::SameLine(0, itemGap);

    char currentViewLabel[64];
    if (vpCount > 1)
      snprintf(currentViewLabel, sizeof(currentViewLabel), "Current camera view %02d", vi + 1);
    else
      snprintf(currentViewLabel, sizeof(currentViewLabel), "Current camera view");

    const PresetId boundId = state->findIdForViewport(vi);
    const char *preview = (boundId != INVALID_ID) ? state->findById(boundId)->name.c_str() : currentViewLabel;
    const int unfilteredSel = (boundId == INVALID_ID) ? 0 : ([&]() -> int {
      for (int i = 0; i < (int)state->presets.size(); ++i)
        if (state->presets[i].id == boundId)
          return i + 1;
      return 0;
    })();

    ImGui::SetNextItemWidth(comboW);
    PropPanel::FilteredComboBoxStandalone &cmb = state->combo[vi];

    const bool isBlinkCombo = state->highlightBlink && (int)state->highlightBlinkVp == vi && !state->highlightBlinkButton;
    ImGui::PushStyleColor(ImGuiCol_FrameBg,
      colWithBlink(isBlinkCombo, state->highlightBlinkTimer, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg)));
    ImGui::PushStyleColor(ImGuiCol_Button,
      colWithBlink(isBlinkCombo, state->highlightBlinkTimer, ImGui::GetStyleColorVec4(ImGuiCol_Button)));

    if (cmb.beginCombo("##psc", preview, unfilteredSel))
    {
      const int total = 1 + (int)state->presets.size();
      if (cmb.beginFiltering(total))
      {
        const char *filter = cmb.getFilter();
        cmb.addItem(currentViewLabel, 0, 0, false);
        bool curViewSeparator = false;
        for (int i = 0; i < (int)state->presets.size(); ++i)
        {
          const char *nm = state->presets[i].name.c_str();
          if (!matchesFilterCI(nm, filter))
            continue;
          if (!curViewSeparator)
          {
            cmb.addSeparator();
            curViewSeparator = true;
          }
          cmb.addItem(nm, i + 1, 0, !filter[0] ? false : strcmp(nm, filter) == 0,
            state->presets[i].isShared ? state->sharedIcon : PropPanel::IconId::Invalid);
        }
        cmb.endFiltering();
      }
      const int chosen = cmb.endCombo();
      if (chosen == 0)
      {
        if (boundId != INVALID_ID)
          setVpBinding(boundId, -1);
      }
      else if (chosen > 0 && chosen - 1 < (int)state->presets.size())
      {
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
        {
          VpBinding currentVp = VpBinding(vi);

          if (findPresetByVpBinding(currentVp) == INVALID_ID)
          {
            Preset &ps = state->presets[chosen - 1];
            VpBinding vpBinding = ps.boundVp;
            ps.boundVp = currentVp;
            state->applyToViewport(ps);
            ps.boundVp = vpBinding;
          }
        }
        else
        {
          const PresetId newPsId = state->presets[chosen - 1].id;
          setVpBinding(newPsId, vi);
        }
      }
    }

    ImGui::PopStyleColor(2);

    if (cmb.isHovered())
    {
      if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Select preset");
        ImGui::TextDisabled("Left click: Open the list");
        ImGui::TextDisabled("Middle click: Deselect current preset");
        ImGui::EndTooltip();
        ImGui::PopStyleVar();
      }
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && boundId != INVALID_ID)
        setVpBinding(boundId, -1);
    }

    ImGui::SameLine(0, itemGap);

    if (boundId != INVALID_ID)
    {
      Preset *ps = state->findById(boundId);
      const bool wasLink = ps && ps->linkEnabled;
      const bool isBlinkBtn = state->highlightBlink && (int)state->highlightBlinkVp == vi && state->highlightBlinkButton;

      if (wasLink)
        ImGui::PushStyleColor(ImGuiCol_Button,
          colWithBlink(isBlinkBtn, state->highlightBlinkTimer, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)));
      else if (isBlinkBtn)
        ImGui::PushStyleColor(ImGuiCol_Button,
          colWithBlink(isBlinkBtn, state->highlightBlinkTimer, ImGui::GetStyleColorVec4(ImGuiCol_Button)));

      if (ImGui::ImageButton("##lnk", PropPanel::get_im_texture_id_from_icon_id(state->cameraEditIcon), iconSz))
        if (ps)
          ps->linkEnabled = !ps->linkEnabled;
      if (wasLink || isBlinkBtn)
        ImGui::PopStyleColor();

      if (ImGui::IsItemHovered())
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        ImGui::BeginTooltip();
        if (wasLink)
        {
          ImGui::TextUnformatted("Editing the preset in viewport");
          ImGui::TextDisabled("Left click: Lock the camera");
        }
        else
        {
          ImGui::TextUnformatted("Camera locked");
          ImGui::TextDisabled("Left click: Edit the preset in viewport");
        }
        ImGui::EndTooltip();
        ImGui::PopStyleVar();
      }
    }
    else
    {
      ImGui::Dummy({btnSz, btnSz});
    }

    ImGui::PopStyleVar(3);
    ImGui::End();
  }

  if (state->highlightBlink)
  {
    state->highlightBlinkTimer += ImGui::GetIO().DeltaTime;
    if (state->highlightBlinkTimer >= highlightBlinkTotal)
    {
      state->highlightBlink = false;
      state->highlightBlinkButton = true;
      state->highlightBlinkTimer = 0.0f;
      state->highlightBlinkVp = VpBinding::None;
    }
  }
}
