#pragma once
#include <propPanel2/c_panel_base.h>
#include <EASTL/string.h>

class IGPUGrassService;
class DataBlock;
struct GPUGrassType;
struct GPUGrassDecal;
class GPUGrassPanel
{
  PropertyContainerControlBase *grassPanel = nullptr;
  IGPUGrassService *srv = nullptr;
  DataBlock *blk = nullptr;
  bool showNameDialog(const char *title, eastl::string &res, const eastl::function<bool(const char *)> &findName);
  bool showMultiListDialog(const char *title, const Tab<String> &entries, Tab<int> &res);
  bool showListDialog(const char *title, const Tab<String> &entries, int &res);
  void addGrassType(const GPUGrassType &type);
  void addGrassDecal(const GPUGrassDecal &decal);
  Tab<String> getGrassTypeNames() const;
  Tab<String> getGrassDecalNames() const;
  void reload();

public:
  void fillPanel(IGPUGrassService *service, DataBlock *grass_blk, PropPanel2 &panel);
  void onChange(int pcb_id, PropPanel2 *panel);
  void onClick(int pcb_id, PropPanel2 *panel, const eastl::function<void()> &loadGPUGrassFromLevelBlk,
    const eastl::function<void()> &fillPanel);
  bool isGrassValid() const;
};
