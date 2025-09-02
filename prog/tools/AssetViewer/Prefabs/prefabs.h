// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
// #include "../av_interface.h"

#include <EditorCore/ec_interface_ex.h>
#include <propPanel/c_control_event_handler.h>

#include <libTools/containers/dag_DynMap.h>
#include <libTools/staticGeomUi/nodeFlags.h>

#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>


class OLAppWindow;
class GeomObject;
// class NodeFlagsModfier;


class PrefabsPlugin : public IGenEditorPlugin, public INodeModifierClient, public PropPanel::ControlEventHandler
{
public:
  PrefabsPlugin();
  ~PrefabsPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "prefabsEditor"; }

  void registered() override;
  void unregistered() override;

  bool begin(DagorAsset *asset) override;
  bool end() override;

  void clearObjects() override;
  void onSaveLibrary() override;
  void onLoadLibrary() override;

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override;
  void renderGeometry(Stage stage) override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  // IGenEventHandler
  void handleViewportPaint(IGenViewportWnd *wnd) override {}

  // ControlEventHandler
  void onChange(int pid, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pid, PropPanel::ContainerPropertyControl *panel) override;

  // INodeModifierClient
  void onNodeFlagsChanged(int node_idx, int or_flags, int and_flags) override;
  void onVisRangeChanged(int node_idx, real vis_range) override;
  void onLightingChanged(int node_idx, StaticGeometryNode::Lighting light) override;
  void onLtMulChanged(int node_idx, real lt_mul) override;
  void onVltMulChanged(int node_idx, int vlt_mul) override;
  void onLinkedResChanged(int node_idx, const char *res_name) override;
  void onTopLodChanged(int node_idx, const char *top_lod_name) override;
  void onLodRangeChanged(int node_idx, int lod_range) override;
  void onShowOccludersChanged(int node_idx, bool show_occluders) override;
  void onUseDefaultChanged(bool use_default) override {}

private:
  GeomObject *geom;

  const char *curAssetDagFname;

  bool showDag;
  String dagName;

  static SimpleString tempDir;

  NodeFlagsModfier *nodeModif;

  struct NodeParams
  {
    int flags;
    real visRange;
    StaticGeometryNode::Lighting light;
    real ltMul;
    int vltMul;
    int linkedRes;
    real lodRange;
    int lodName;
    bool showOccluders;
  };

  class NodeParamsTab : public Tab<NodeParams>
  {
  public:
    NodeParamsTab() : Tab<NodeParams>(tmpmem) {}
  };

  DynMap<const char *, NodeParamsTab> changedNodes;
  NameMap linkedNames;
  NameMapCI dagFilesList;

  void openDag(const char *fname);

  void centerSOObject();
  void autoAjustSOObject();
  void resetSOObjectPivot();

  void replaceStatObj();

  // void drawCenter(IGenViewportWnd *wnd, CtlDC &dc);

  bool getNodeFlags(NodeParamsTab &tab, bool &already_in_map);
  void setNodeFlags(const char *entry, const NodeParamsTab &tab, bool already_in_map);
  void saveChangedDags();

  void renderOccluders();
};
