// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "objPropDlg.h"
#include <generic/dag_tabUtils.h>
#include <propPanel/control/container.h>
#include <string>
#include <sstream>
#include <unordered_map>

enum
{
  ID_SPLITTER = 1,
  ID_TREE,
  ID_PROPS,
};

float ObjPropDialog::lastUsedSplitRatio = 0.5f;

ObjPropDialog::ObjPropDialog(const char *caption, hdpi::Px width, hdpi::Px height, const ObjPropDialogNode &root_node) :
  DialogWindow(nullptr, width, height, caption), rootNode(root_node)
{
  PropPanel::ContainerPropertyControl *panel = getPanel();
  panel->setEventHandler(this);

  PropPanel::ContainerPropertyControl *splitter = panel->createVerticalSplitter(ID_SPLITTER);
  splitter->setSplitRatios(make_span_const<float>(&lastUsedSplitRatio, 1));

  tree = splitter->createMultiSelectTree(ID_TREE, "Nodes", hdpi::Px(0));
  splitter->createEditBox(ID_PROPS, "Parameters", "", true, false, true, PropPanel::Constants::EDITBOX_MULTILINE_FULL_HEIGHT);

  fillTree(nullptr, rootNode);
  updateTreeImages(nullptr);

  setDialogButtonText(PropPanel::DIALOG_ID_OK, "Save");
  setDialogButtonTooltip(PropPanel::DIALOG_ID_OK, "Save the changes to the physical DAG files.");
  updateSaveButtonState();
}

void ObjPropDialog::fillTree(PropPanel::TLeafHandle tree_node, const ObjPropDialogNode &dialog_node)
{
  for (const ObjPropDialogNode &childDialogNode : dialog_node.children)
  {
    PropPanel::TLeafHandle childTreeNode = tree->createTreeLeaf(tree_node, childDialogNode.name, "");
    tree->setUserData(childTreeNode, &childDialogNode);

    fillTree(childTreeNode, childDialogNode);

    if (tree->getChildCount(childTreeNode) != 0)
    {
      tree->setImageState(childTreeNode, "folder");
      tree->setExpanded(childTreeNode, true);
    }
  }
}

void ObjPropDialog::updateTreeImages(PropPanel::TLeafHandle tree_node)
{
  const int childCount = tree->getChildCount(tree_node);
  for (int i = 0; i < childCount; ++i)
  {
    PropPanel::TLeafHandle childTreeNode = tree->getChildLeaf(tree_node, i);
    ObjPropDialogNode *childDialogNode = getObjPropDialogNode(childTreeNode);
    if (childDialogNode->isScriptNode())
      tree->setButtonPictures(childTreeNode, childDialogNode->script.empty() ? "filter_unselected" : "filter_selected");

    updateTreeImages(childTreeNode);
  }
}

void ObjPropDialog::applyChanges(dag::ConstSpan<PropPanel::TLeafHandle> script_nodes)
{
  if (!changed)
    return;

  const SimpleString newScript = getPanel()->getText(ID_PROPS);
  for (PropPanel::TLeafHandle treeNode : script_nodes)
  {
    ObjPropDialogNode *dialogNode = getObjPropDialogNode(treeNode);
    G_ASSERT(dialogNode->isScriptNode());
    dialogNode->script = newScript;
  }

  shouldSave = true;
  changed = false;
  updateSaveButtonState();
  updateTreeImages(nullptr);
}

ObjPropDialogNode *ObjPropDialog::getObjPropDialogNode(PropPanel::TLeafHandle tree_node)
{
  return static_cast<ObjPropDialogNode *>(tree->getUserData(tree_node));
}

const ObjPropDialogNode *ObjPropDialog::getObjPropDialogNode(PropPanel::TLeafHandle tree_node) const
{
  return const_cast<ObjPropDialog *>(this)->getObjPropDialogNode(tree_node);
}

dag::Vector<PropPanel::TLeafHandle> ObjPropDialog::getSelectedScriptNodes() const
{
  dag::Vector<PropPanel::TLeafHandle> scriptNodes;
  tree->getSelectedLeafs(scriptNodes, true, true);
  tabutils::erase_if(scriptNodes,
    [this](PropPanel::TLeafHandle tree_node) { return !getObjPropDialogNode(tree_node)->isScriptNode(); });
  return scriptNodes;
}

String ObjPropDialog::getCommonScriptLines(dag::ConstSpan<PropPanel::TLeafHandle> script_nodes) const
{
  std::unordered_map<std::string, int> lines;
  for (PropPanel::TLeafHandle treeNode : script_nodes)
  {
    const ObjPropDialogNode *dialogNode = getObjPropDialogNode(treeNode);
    G_ASSERT(dialogNode->isScriptNode());
    std::stringstream ss(dialogNode->script.c_str());
    std::string to;
    while (std::getline(ss, to))
      ++lines[to];
  }

  String commonLines;
  for (const auto &line : lines)
    if (line.second == script_nodes.size())
    {
      commonLines += line.first.c_str();
      commonLines += "\n";
    }

  return commonLines;
}

void ObjPropDialog::updateSaveButtonState() { setDialogButtonEnabled(PropPanel::DIALOG_ID_OK, changed || shouldSave); }

void ObjPropDialog::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) {}

void ObjPropDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_TREE)
  {
    applyChanges(prevSel);

    prevSel = getSelectedScriptNodes();
    panel->setText(ID_PROPS, getCommonScriptLines(prevSel));
    panel->setEnabledById(ID_PROPS, !prevSel.empty());
  }
  else if (pcb_id == ID_PROPS)
  {
    changed = true;
    updateSaveButtonState();
  }
}

int ObjPropDialog::showDialog()
{
  const int result = DialogWindow::showDialog();

  applyChanges(getSelectedScriptNodes());

  dag::ConstSpan<float> splitRatios = getPanel()->getContainerById(ID_SPLITTER)->getSplitRatios();
  G_ASSERT(splitRatios.size() == 1);
  lastUsedSplitRatio = splitRatios[0];

  return result;
}