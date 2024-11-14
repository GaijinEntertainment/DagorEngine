// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_cm.h>
#include <propPanel/control/container.h>
#include <imgui/imgui.h>

ViewportWindowStatSettingsDialog::ViewportWindowStatSettingsDialog(ViewportWindow &_viewport, bool *_rootEnable, hdpi::Px width,
  hdpi::Px height) :
  DialogWindow(nullptr, width, height, "Viewport stat display settings"),
  viewport(_viewport),
  rootEnable(_rootEnable),
  tree(nullptr),
  root(nullptr)
{}

PropPanel::TLeafHandle ViewportWindowStatSettingsDialog::addGroup(int group, const char name[], bool enabled)
{
  if (!tree)
  {
    PropPanel::ContainerPropertyControl *panel = getPanel();
    G_ASSERT(panel);
    tree = panel->createTreeCheckbox(CM_STATS_SETTINGS_TREE, "", hdpi::Px(0), /*new_line*/ false, /*icons_show*/ true);
    root = tree->createTreeLeaf(nullptr, "Show stats", nullptr);
    tree->setBool(root, true);
    tree->setCheckboxValue(root, *rootEnable);

    G_ASSERT(tree->isImguiContainer());
    PropPanel::ContainerPropertyControl *imguiTree = static_cast<PropPanel::ContainerPropertyControl *>(tree);
    imguiTree->setTreeCheckboxIcons("eye_show", "eye_hide");

    // For some reason, setting checkbox value doesn't work immediately after creating tree widget.
    // Postpone applying of checkbox states until the next event loop run.
    getPanel()->setPostEvent(CM_STATS_SETTINGS_TREE);
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
    tree->setBool(handle, true);
    m_groups[group] = Item{handle, root, group, enabled};
  }
  tree->setCheckboxValue(handle, enabled);
  return handle;
}

void ViewportWindowStatSettingsDialog::addOption(PropPanel::TLeafHandle group, int id, const char name[], bool value)
{
  PropPanel::TLeafHandle handle = tree->createTreeLeaf(group, name, nullptr);
  tree->setCheckboxValue(handle, value);
  m_options.emplace_back(Item{handle, group, id, value});
}

void ViewportWindowStatSettingsDialog::onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == CM_STATS_SETTINGS_TREE)
  {
    tree->setCheckboxEnable(root, true);
    tree->setCheckboxValue(root, *rootEnable);
    tree->setBool(root, true);

    for (auto &it : m_groups)
    {
      const Item &groupItem = it.second;

      tree->setCheckboxEnable(groupItem.handle, true);
      tree->setCheckboxValue(groupItem.handle, groupItem.value);
      tree->setBool(groupItem.handle, true);
    }

    for (const Item &item : m_options)
    {
      tree->setCheckboxEnable(item.handle, true);
      tree->setCheckboxValue(item.handle, item.value);
    }

    updateColors();
  }
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

void ViewportWindowStatSettingsDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
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

    if (!tree->isCheckboxEnable(groupItem.handle))
      continue;

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
    if (!tree->isCheckboxEnable(item.handle))
      continue;

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
}
