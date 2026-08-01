// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_hmapDebugShadingGradients.h>

#include <ioSys/dag_dataBlock.h>

#include <EASTL/optional.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class HeightmapDebugShadingProperties
{
public:
  void loadGradientsFile(const char *path);
  void saveGradientsFile(const char *path);

  void loadSettings(const DataBlock &blk);
  void saveSettings(DataBlock &blk) const;

  void fillPropertyPanel(PropPanel::ContainerPropertyControl &panel, int pid_start);

  // Returns true if pcb_id is a debug shading property, and the shader parameters need to be updated.
  bool onPropertyPanelChange(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start);
  bool onPropertyPanelClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start);

  HeightmapColorGradient *getSelectedGradient();
  const HeightmapColorGradient *getSelectedGradient() const;

  bool isShowingGradientColors() const { return showGradientColors; }

private:
  static eastl::optional<int64_t> getLastModificationTime(const char *path);
  const char *getOverwriteProtectedSavePath(const char *path) const;

  void updateGradientSelectorButtons(PropPanel::ContainerPropertyControl &panel, int pid_start,
    const HeightmapColorGradient *gradient);
  void updateGradientSelector(PropPanel::ContainerPropertyControl &panel, int pid_start, const HeightmapColorGradient *gradient);
  void updateGradientControl(PropPanel::ContainerPropertyControl &panel, int pid_start, const HeightmapColorGradient *gradient);
  void updateGradientKeyControlLimits(PropPanel::ContainerPropertyControl &panel, int pid_start,
    const HeightmapColorGradient *gradient);
  void updateGradientKeyControls(PropPanel::ContainerPropertyControl &panel, int pid_start, const HeightmapColorGradient *gradient);
  void onChangeGradientSelector(PropPanel::ContainerPropertyControl &panel, int pid_start);
  void onChangeGradientControl(PropPanel::ContainerPropertyControl &panel, int pid_start);
  String showNewGradientNamePicker(const char *dialog_title) const;
  void onClickCreate(PropPanel::ContainerPropertyControl &panel, int pid_start);
  void onClickClone(PropPanel::ContainerPropertyControl &panel, int pid_start);
  void onClickDelete(PropPanel::ContainerPropertyControl &panel, int pid_start);

  HeightmapColorGradients colorGradients;
  DataBlock lastLoadedColorGradientsBlk;
  eastl::optional<int64_t> lastLoadedColorGradientsBlkModificationTime;
  String selectedGradientName;
  bool showGradientColors = false;
};
