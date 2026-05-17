// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <de3_objEntity.h>
#include <propPanel/c_control_event_handler.h>
#include <assets/asset.h>

struct ObjPropDialogNode;

class ObjectPropertyEditor : public PropPanel::ControlEventHandler
{
public:
  void begin(DagorAsset *asset, IObjEntity *asset_entity);
  void fillPropPanel(int pid, PropPanel::ContainerPropertyControl &panel);

  void onClick();

private:
  struct Data
  {
    struct NodeData
    {
      DagData::Node::NodeBlock *nodeBlock;
      String name;
      SimpleString props;
      eastl::vector<NodeData> children;

      NodeData() : nodeBlock(nullptr) {}

      NodeData(DagData::Node::NodeBlock *nodeBlock, const String &name, const SimpleString &props) :
        nodeBlock(nodeBlock), name(name), props(props)
      {}
    };

    DagData dagData;
    String filename;
    NodeData rootNode;

    void buildProps(NodeData &parent, Tab<DagData::Node> &nodes, const char *dag_file_name);
    void loadFromDag(const char *dag_file_name, const String &lod);
    void saveToDag(const char *dag_file_name);
  };

  void makeObjPropDialogNodes(ObjPropDialogNode &dialog_node, const Data::NodeData &node_data);
  void loadProps();

  void saveChangesInternal(const ObjPropDialogNode &dialog_node);
  void saveChanges(const ObjPropDialogNode &dialog_node);

  eastl::vector<Data> data;
  eastl::string assetSrcFolderPath;
  eastl::string assetName;

  IObjEntity *entity;
};
