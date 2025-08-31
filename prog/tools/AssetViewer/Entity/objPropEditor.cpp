// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "objPropEditor.h"
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

void ObjectPropertyEditor::getData(Tab<String> &node_names, Tab<String> &props)
{
  for (const ObjectPropertyEditor::Data &data : data)
    for (const ObjectPropertyEditor::Data::NodeData &node : data.nodes)
    {
      node_names.push_back(node.name);
      props.push_back(String(node.props));
    }
}

void ObjectPropertyEditor::Data::buildProps(Tab<DagData::Node> &nodes, const String &lod)
{
  for (DagData::Node &node : nodes)
  {
    SimpleString name;
    for (DagData::Node::NodeBlock &block : node.blocks)
      if (block.blockType == DAG_NODE_DATA)
        name = block.name;
      else if (block.blockType == DAG_NODE_SCRIPT)
        this->nodes.emplace_back(block, name + lod, block.script);
    if (!node.children.empty())
      buildProps(node.children, lod);
  }
}

void ObjectPropertyEditor::onClick()
{
  loadProps();
  Tab<String> nodes, props;
  getData(nodes, props);
  ObjPropDialog dlg("Object properties", hdpi::_pxScaled(300), hdpi::_pxScaled(450), nodes, props);
  int dialogResult = wingw::MB_ID_CANCEL;
  while (dialogResult == wingw::MB_ID_CANCEL)
  {
    if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
    {
      dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Object properties",
        "You have changed Object properties. Do you want to save the "
        "changes to the physical DAGs?");
      if (dialogResult == wingw::MB_ID_YES)
        saveChanges(dlg.getScripts());
    }
    else
      break;
  }
}

void ObjectPropertyEditor::Data::loadFromDag(const char *dag_file_name, const String &lod)
{
  DagData newDagData;
  load_scene(dag_file_name, newDagData, true);
  dagData = newDagData;
  this->lod = lod;
  nodes.clear();
  buildProps(dagData.nodes, lod);
}

void ObjectPropertyEditor::saveChanges(const Tab<String> &props)
{
  size_t i = 0;
  for (Data &d : data)
  {
    for (Data::NodeData &node : d.nodes)
      node.nodeBlock.script = props[i++];
    d.saveToDag(String(0, "%s/%s%s", assetSrcFolderPath.c_str(), assetName.c_str(), d.lod).c_str());
  }
}

void ObjectPropertyEditor::Data::saveToDag(const char *dag_file_name) { write_dag(dag_file_name, dagData); }
