// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/c_common.h>
#include <dag/dag_vector.h>
#include <EASTL/vector_map.h>

class ViewportWindow;

class ViewportWindowStatSettingsDialog : public PropPanel::DialogWindow
{
public:
  ViewportWindowStatSettingsDialog(ViewportWindow &_viewport, bool *_rootEnable, bool root_enable_default, hdpi::Px width,
    hdpi::Px height);

  void onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  PropPanel::TLeafHandle addGroup(int group, const char name[], bool enabled, bool default_val = true);
  void addOption(PropPanel::TLeafHandle group, int id, const char name[], bool value, bool default_val = true);

private:
  bool onOk() override;

  struct Item
  {
    PropPanel::TLeafHandle handle;
    PropPanel::TLeafHandle group;
    int id;
    bool value;
  };

  struct ValueInfo
  {
    PropPanel::TLeafHandle handle;
    bool defaultValue;
  };

  bool *rootEnable;
  const bool rootEnableDefault;
  PropPanel::TLeafHandle root;
  PropPanel::ContainerPropertyControl *tree;
  dag::Vector<Item> m_options;
  eastl::vector_map<int, Item> m_groups;
  eastl::vector_map<int, ValueInfo> defaultValues;

  Item *getGroupByHandle(PropPanel::TLeafHandle handle);
  void setTreeNodeState(PropPanel::TLeafHandle node, bool item_enabled, bool parent_enabled);
  void updateColors();

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onPCBChange(int pcb_id);

  bool isDefault() const;
  void updateRevertButton();

  ViewportWindow &viewport;
};
