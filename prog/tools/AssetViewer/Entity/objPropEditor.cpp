// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "objPropEditor.h"
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include "objPropDlg.h"

void ObjectPropertyEditor::begin(DagorAsset *asset, IObjEntity *asset_entity)
{
  if (!asset)
    return;

  assetSrcFolderPath = asset->getFolderPath();
  assetName = asset->getName();
  entity = asset_entity;
}

void ObjectPropertyEditor::fillPropPanel(int pid, PropPanel::ContainerPropertyControl &panel)
{
  panel.createButton(pid, "Object properties");
}

void ObjectPropertyEditor::loadProps()
{
  data.clear();

  for (int lod = 0;; ++lod)
  {
    String dagFileName(0, "%s/%s.lod%02d.dag", assetSrcFolderPath.c_str(), assetName.c_str(), lod);
    if (!dd_file_exists(dagFileName.c_str()))
      break;

    data.push_back().loadFromDag(dagFileName.c_str(), String(0, ".lod%02d.dag", lod));
  }
}

void ObjectPropertyEditor::makeObjPropDialogNodes(ObjPropDialogNode &dialog_node, const Data::NodeData &node_data)
{
  for (const Data::NodeData &childNode : node_data.children)
  {
    ObjPropDialogNode &childDialogNode = dialog_node.children.push_back();
    childDialogNode.name = childNode.name;
    childDialogNode.script = childNode.props;
    childDialogNode.nodeData = &childNode;
    makeObjPropDialogNodes(childDialogNode, childNode);
  }
}

void ObjectPropertyEditor::Data::buildProps(NodeData &parent, Tab<DagData::Node> &nodes, const char *dag_file_name)
{
  for (DagData::Node &node : nodes)
  {
    String name;
    bool foundScript = false;
    for (DagData::Node::NodeBlock &block : node.blocks)
    {
      if (block.blockType == DAG_NODE_DATA)
        name = block.name;
      else if (block.blockType == DAG_NODE_SCRIPT)
      {
        if (foundScript)
          logerr("Multiple script nodes found in \"%s\". The hierarchy will not be displayed correctly.", dag_file_name);
        foundScript = true;

        if (name.empty())
          name = "[No block name]";
        parent.children.emplace_back(&block, name, block.script);
        name.clear();
      }
    }

    if (!node.children.empty())
    {
      if (foundScript)
        buildProps(parent.children[0], node.children, dag_file_name);
      else
        buildProps(parent, node.children, dag_file_name);
    }
  }
}

void ObjectPropertyEditor::onClick()
{
  loadProps();

  ObjPropDialogNode dialogRootNode;
  for (const ObjectPropertyEditor::Data &current_data : data)
  {
    ObjPropDialogNode &childDialogNode = dialogRootNode.children.push_back();
    childDialogNode.name = current_data.filename;
    makeObjPropDialogNodes(childDialogNode, current_data.rootNode);
  }

  ObjPropDialog dlg("Object properties", hdpi::_pxScaled(320), hdpi::_pxScaled(200), dialogRootNode);
  dlg.setManualModalSizingEnabled();
  if (!dlg.hasEverBeenShown())
    dlg.setWindowSize(IPoint2((int)(ImGui::GetIO().DisplaySize.x * 0.5f), (int)(ImGui::GetIO().DisplaySize.y * 0.75f)));

  if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
    saveChanges(dlg.getRootDialogNode());
}

void ObjectPropertyEditor::Data::loadFromDag(const char *dag_file_name, const String &lod)
{
  DagData newDagData;
  load_scene(dag_file_name, newDagData, true);
  dagData = newDagData;
  filename = get_file_name(dag_file_name);
  rootNode.children.clear();
  buildProps(rootNode, dagData.nodes, dag_file_name);
}

void ObjectPropertyEditor::saveChangesInternal(const ObjPropDialogNode &dialog_node)
{
  for (const ObjPropDialogNode &childDialogNode : dialog_node.children)
  {
    saveChangesInternal(childDialogNode);

    if (childDialogNode.isScriptNode())
    {
      const Data::NodeData *nodeData = static_cast<const Data::NodeData *>(childDialogNode.nodeData);
      nodeData->nodeBlock->script = childDialogNode.script;
    }
  }
}

void ObjectPropertyEditor::saveChanges(const ObjPropDialogNode &dialog_node)
{
  saveChangesInternal(dialog_node);

  for (Data &d : data)
  {
    d.saveToDag(String(0, "%s/%s", assetSrcFolderPath.c_str(), d.filename).c_str());
  }
}

void ObjectPropertyEditor::Data::saveToDag(const char *dag_file_name) { write_dag(dag_file_name, dagData); }
