// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <de3_objEntity.h>
#include <propPanel/c_control_event_handler.h>
#include <assets/asset.h>


class ObjectPropertyEditor : public PropPanel::ControlEventHandler
{
public:
  void begin(DagorAsset *asset, IObjEntity *asset_entity);
  void fillPropPanel(int pid, PropPanel::ContainerPropertyControl &panel);

  void onClick();

private:
  struct Data
  {
    DagData dagData;
    String lod;
    struct NodeData
    {
      DagData::Node::NodeBlock &nodeBlock;
      String name;
      SimpleString props;
      NodeData(DagData::Node::NodeBlock &nodeBlock, const String &name, const SimpleString &props) :
        nodeBlock(nodeBlock), name(name), props(props)
      {}
    };
    eastl::vector<NodeData> nodes;
    void buildProps(Tab<DagData::Node> &nodes, const String &lod);
    void loadFromDag(const char *dag_file_name, const String &lod);
    void saveToDag(const char *dag_file_name);
  };

  void getData(Tab<String> &nodes, Tab<String> &props);
  void loadProps();

  void saveChanges(const Tab<String> &props);
  eastl::vector<Data> data;
  eastl::string assetSrcFolderPath;
  eastl::string assetName;

  IObjEntity *entity;
};
