#pragma once

#include "../av_plugin.h"
// #include "../av_interface.h"

#include <EditorCore/ec_interface_ex.h>
#include <propPanel2/c_control_event_handler.h>

#include <libTools/containers/dag_DynMap.h>
#include <libTools/staticGeomUi/nodeFlags.h>

#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>


class OLAppWindow;
class GeomObject;
// class NodeFlagsModfier;


class PrefabsPlugin : public IGenEditorPlugin, public INodeModifierClient, public ControlEventHandler
{
public:
  PrefabsPlugin();
  ~PrefabsPlugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "prefabsEditor"; }

  virtual void registered();
  virtual void unregistered();

  virtual bool begin(DagorAsset *asset);
  virtual bool end();

  virtual void clearObjects();
  virtual void onSaveLibrary();
  virtual void onLoadLibrary();

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects();
  virtual void renderGeometry(Stage stage);

  virtual bool supportAssetType(const DagorAsset &asset) const;

  virtual void fillPropPanel(PropertyContainerControlBase &panel);
  virtual void postFillPropPanel() {}

  // IGenEventHandler
  virtual void handleViewportPaint(IGenViewportWnd *wnd) {}

  // ControlEventHandler
  virtual void onChange(int pid, PropertyContainerControlBase *panel);
  virtual void onClick(int pid, PropertyContainerControlBase *panel);

  // INodeModifierClient
  virtual void onNodeFlagsChanged(int node_idx, int or_flags, int and_flags);
  virtual void onVisRangeChanged(int node_idx, real vis_range);
  virtual void onLightingChanged(int node_idx, StaticGeometryNode::Lighting light);
  virtual void onLtMulChanged(int node_idx, real lt_mul);
  virtual void onVltMulChanged(int node_idx, int vlt_mul);
  virtual void onLinkedResChanged(int node_idx, const char *res_name);
  virtual void onTopLodChanged(int node_idx, const char *top_lod_name);
  virtual void onLodRangeChanged(int node_idx, int lod_range);
  virtual void onUseDefaultChanged(bool use_default) {}

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
};
