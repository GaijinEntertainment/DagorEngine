// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_confirmation_dialog.h>
#include <EditorCore/ec_cm.h>
#include <propPanel/control/container.h>
#include <imgui/imgui.h>

ViewportWindowStatSettingsDialog::ViewportWindowStatSettingsDialog(ViewportWindow &_viewport, bool *_rootEnable,
  bool root_enable_default, hdpi::Px width, hdpi::Px height) :
  DialogWindow(nullptr, width, height, "Viewport stat display settings"),
  viewport(_viewport),
  rootEnable(_rootEnable),
  rootEnableDefault(root_enable_default),
  tree(nullptr),
  root(nullptr)
{}

PropPanel::TLeafHandle ViewportWindowStatSettingsDialog::addGroup(int group, const char name[], bool enabled, bool default_val)
{
  if (!tree)
  {
    PropPanel::ContainerPropertyControl *panel = getPanel();
    G_ASSERT(panel);
    tree = panel->createTreeCheckbox(CM_STATS_SETTINGS_TREE, "", hdpi::Px(0), /*new_line*/ false, /*icons_show*/ true);
    root = tree->createTreeLeaf(nullptr, "Show stats", nullptr);
    tree->setExpanded(root, true);
    tree->setCheckboxValue(root, *rootEnable);
    defaultValues[CM_STATS_SETTINGS_TREE] = ValueInfo{.handle = root, .defaultValue = rootEnableDefault};

    G_ASSERT(tree->isImguiContainer());
    PropPanel::ContainerPropertyControl *imguiTree = static_cast<PropPanel::ContainerPropertyControl *>(tree);
    imguiTree->setTreeCheckboxIcons("eye_show", "eye_hide");

    // For some reason, setting checkbox value doesn't work immediately after creating tree widget.
    // Postpone applying of checkbox states until the next event loop run.
    getPanel()->setPostEvent(CM_STATS_SETTINGS_TREE);

    setDialogButtonText(PropPanel::DIALOG_ID_OK, "Revert to default");
    setDialogButtonText(PropPanel::DIALOG_ID_CANCEL, "Close");
  }

  PropPanel::TLeafHandle handle;
  auto it = m_groups.find(group);
  if (it != m_groups.end())
  {
    it->second.value = enabled;
    handle = it->second.handle;
  }
  else
  {
    handle = tree->createTreeLeaf(root, name, nullptr);
    tree->setExpanded(handle, true);
    m_groups[group] = Item{handle, root, group, enabled};
  }
  tree->setCheckboxValue(handle, enabled);
  defaultValues[group] = ValueInfo{.handle = handle, .defaultValue = default_val};
  updateRevertButton();

  return handle;
}

void ViewportWindowStatSettingsDialog::addOption(PropPanel::TLeafHandle group, int id, const char name[], bool value, bool default_val)
{
  PropPanel::TLeafHandle handle = tree->createTreeLeaf(group, name, nullptr);
  tree->setCheckboxValue(handle, value);
  defaultValues[id] = ValueInfo{.handle = handle, .defaultValue = default_val};
  m_options.emplace_back(Item{handle, group, id, value});
  updateRevertButton();
}

void ViewportWindowStatSettingsDialog::onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == CM_STATS_SETTINGS_TREE)
  {
    tree->setCheckboxEnable(root, true);
    tree->setCheckboxValue(root, *rootEnable);
    tree->setExpanded(root, true);

    for (auto &it : m_groups)
    {
      const Item &groupItem = it.second;

      tree->setCheckboxEnable(groupItem.handle, true);
      tree->setCheckboxValue(groupItem.handle, groupItem.value);
      tree->setExpanded(groupItem.handle, true);
    }

    for (const Item &item : m_options)
    {
      tree->setCheckboxEnable(item.handle, true);
      tree->setCheckboxValue(item.handle, item.value);
    }

    updateColors();
  }
}

bool ViewportWindowStatSettingsDialog::onOk()
{
  if (tree &&
      ConfirmationDialog("Revert to defaults", "Are you sure you want to revert to defaults?").showDialog() == PropPanel::DIALOG_ID_OK)
  {
    for (const auto &[pcb, valueInfo] : defaultValues)
    {
      if (tree->getCheckboxValue(valueInfo.handle) != valueInfo.defaultValue)
      {
        tree->setCheckboxValue(valueInfo.handle, valueInfo.defaultValue);
        onPCBChange(pcb);
      }
    }

    updateColors();
    updateRevertButton();
  }

  return false;
}

ViewportWindowStatSettingsDialog::Item *ViewportWindowStatSettingsDialog::getGroupByHandle(PropPanel::TLeafHandle handle)
{
  for (auto &it : m_groups)
  {
    Item &groupItem = it.second;
    if (groupItem.handle == handle)
      return &groupItem;
  }

  return nullptr;
}

void ViewportWindowStatSettingsDialog::setTreeNodeState(PropPanel::TLeafHandle node, bool item_enabled, bool parent_enabled)
{
  // Use state icons instead of check boxes for the non-interactive state.
  tree->setCheckboxEnable(node, parent_enabled);

  if (parent_enabled)
  {
    tree->setCheckboxValue(node, item_enabled);
    tree->setButtonPictures(node, nullptr);
    tree->setColor(node, E3DCOLOR(0, 0, 0));
  }
  else
  {
    tree->setButtonPictures(node, item_enabled ? "eye_disabled" : "eye_hide");
    tree->setColor(node, E3DCOLOR(192, 192, 192));
  }
}

void ViewportWindowStatSettingsDialog::updateColors()
{
  setTreeNodeState(root, *rootEnable, true);

  for (auto &it : m_groups)
  {
    const Item &groupItem = it.second;
    setTreeNodeState(groupItem.handle, groupItem.value, *rootEnable);
  }

  for (const Item &item : m_options)
  {
    bool parentEnabled = *rootEnable;
    if (parentEnabled)
    {
      const Item *groupItem = getGroupByHandle(item.group);
      G_ASSERT(groupItem);
      parentEnabled = groupItem && groupItem->value;
    }

    setTreeNodeState(item.handle, item.value, parentEnabled);
  }
}

void ViewportWindowStatSettingsDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) { onPCBChange(pcb_id); }

void ViewportWindowStatSettingsDialog::onPCBChange(int pcb_id)
{
  bool changed = false;

  G_ASSERT(tree->isCheckboxEnable(root));
  bool value = tree->getCheckboxValue(root);
  if (value != *rootEnable)
  {
    *rootEnable = value;
    viewport.handleStatSettingsDialogChange(pcb_id, value);
    changed = true;
  }

  for (auto &it : m_groups)
  {
    Item &groupItem = it.second;

    value = tree->getCheckboxValue(groupItem.handle);
    if (value == groupItem.value)
      continue;

    groupItem.value = value;
    viewport.handleStatSettingsDialogChange(groupItem.id, value);
    changed = true;

    // If all child elements are disabled, enable them
    if (value)
    {
      bool allChildDisabled = true;
      for (const Item &item : m_options)
      {
        if (item.group == groupItem.handle && item.value)
        {
          allChildDisabled = false;
          break;
        }
      }

      if (allChildDisabled)
      {
        for (Item &item : m_options)
        {
          if (item.group == groupItem.handle && !item.value)
          {
            item.value = true;
            tree->setCheckboxValue(item.handle, true);
            viewport.handleStatSettingsDialogChange(item.id, value);
          }
        }
      }
    }
  }

  for (Item &item : m_options)
  {
    value = tree->getCheckboxValue(item.handle);
    if (value != item.value)
    {
      item.value = value;
      viewport.handleStatSettingsDialogChange(item.id, value);
      changed = true;
    }
  }

  if (changed)
    updateColors();

  updateRevertButton();
}

bool ViewportWindowStatSettingsDialog::isDefault() const
{
  if (tree)
  {
    for (const auto &[_, valueInfo] : defaultValues)
    {
      if (tree->getCheckboxValue(valueInfo.handle) != valueInfo.defaultValue)
      {
        return false;
      }
    }
  }

  return true;
}

void ViewportWindowStatSettingsDialog::updateRevertButton()
{
  const bool enabled = root ? !isDefault() : false;
  setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, enabled);
  setDialogButtonTooltip(PropPanel::DIALOG_ID_OK, enabled ? "" : "All set to default");
}
