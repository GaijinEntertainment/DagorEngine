// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <dag/dag_vector.h>
#include <math/dag_Point3.h>
#include <EASTL/string_view.h>
#include <stddef.h>

class CameraPresetsPanel;

class CameraPresetsManager
{
public:
  using PresetId = size_t;

  static const PresetId INVALID_ID = -1;
  static const PresetId PSEUDO_VP1 = 0;
  static const PresetId PSEUDO_VP2 = 1;
  static const PresetId PSEUDO_VP3 = 2;
  static const PresetId PSEUDO_VP4 = 3;
  static const PresetId FIRST_REAL_ID = 4;

  enum class VpBinding : int
  {
    None = -1,
    VP1 = 0,
    VP2 = 1,
    VP3 = 2,
    VP4 = 3
  };

  struct CameraPose
  {
    Point3 pos = {0, 0, 0};
    float pitchDeg = 0, yawDeg = 0, rollDeg = 0, fov = 60.0f;
  };

  struct Preset
  {
    PresetId id = INVALID_ID;
    SimpleString name;
    bool isShared = false;
    VpBinding boundVp = VpBinding::None;
    bool linkEnabled = false;
    CameraPose pose;

    Preset() = default;
    Preset(PresetId id_, const char *n, bool shared) : id(id_), isShared(shared) { name = SimpleString(n); }
    Preset(PresetId id_, eastl::string_view n, bool shared) : id(id_), isShared(shared) { name = SimpleString(n.data(), n.size()); }
  };

  explicit CameraPresetsManager(const String &ps_path);
  ~CameraPresetsManager();

  const dag::Vector<Preset> &getPresets() const;

  const Preset *getPresetById(PresetId id) const;

  CameraPose *getCameraPose(PresetId id) const;

  int getPseudoVpIndex(PresetId pseudo_id) const;

  PresetId pseudoIdForVp(int vp_index) const;

  bool isPseudo(PresetId id) const;

  const char *getPresetName(PresetId id) const;

  bool isNameAvailable(eastl::string_view name, bool is_shared, PresetId except_id = INVALID_ID) const;

  String generateAvailableName(eastl::string_view name, bool is_shared = false);

  bool setNameAndShared(PresetId id, eastl::string_view name, bool is_shared);

  void setVpBinding(PresetId id, int vp_index);

  void setLinkEnabled(PresetId id, bool link);

  PresetId addPresetFromCamera(eastl::string_view name, int vp_index, bool is_shared = false);

  void deletePresets(const dag::Vector<PresetId> &ids);

  PresetId findPresetByVpBinding(VpBinding vp);

  void captureViewportRect(int vp_index);
  void renderAllViewportOverlays();

  void showPanel();
  void hidePanel();
  bool isPanelVisible() const;
  CameraPresetsPanel *getPanel() const;

  struct State;
  State *getState() const;

private:
  State *state = nullptr;
  const String localPsBlkPath;
  const String sharedPsBlkPath;
  CameraPresetsPanel *panel = nullptr;
};

class CameraPresetsPanel
{
public:
  explicit CameraPresetsPanel(CameraPresetsManager &manager);
  ~CameraPresetsPanel();

  void updateImgui();
  void setEditingPreset(CameraPresetsManager::PresetId id);

private:
  struct State;
  friend struct CameraPresetsPanel::State;
  State *state;
  CameraPresetsManager &mgr;
};
