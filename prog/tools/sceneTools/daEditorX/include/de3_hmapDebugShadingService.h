//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{
class ContainerPropertyControl;
}

class DataBlock;
struct HeightmapColorGradient;

class IHmapDebugShadingService
{
public:
  virtual void loadGradientsFile(const char *path) = 0;
  virtual void saveGradientsFile(const char *path) = 0;

  virtual void loadSettings(const DataBlock &blk) = 0;
  virtual void saveSettings(DataBlock &blk) const = 0;

  virtual void fillPropertyPanel(PropPanel::ContainerPropertyControl &panel, int pid_start) = 0;

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start) = 0;
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start) = 0;

  virtual const HeightmapColorGradient *getSelectedGradient() const = 0;
  virtual bool isShowingGradientColors() const = 0;

  static constexpr unsigned HUID = 0x4C7D168Cu; // IHmapDebugShadingService

  static constexpr int REQUIRED_PROPERTY_IDS = 201; // The number of property identifiers used by fillPropertyPanel().
};
