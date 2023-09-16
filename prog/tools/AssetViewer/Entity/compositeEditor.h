#pragma once
#include "compositeEditorGizmoClient.h"
#include "compositeEditorRefreshType.h"
#include "compositeEditorToolbar.h"
#include "compositeEditorTreeData.h"
#include <EASTL/unique_ptr.h>
#include <EditorCore/ec_interface.h>
#include <propPanel2/comWnd/treeview_panel.h>
#include <propPanel2/c_control_event_handler.h>
#include <sepGui/wndMenuInterface.h>

class CompositeEditorTree;
class CompositeEditorPanel;
class DagorAsset;
class IEntityViewPluginInterface;

class CompositeEditor : public ControlEventHandler, public IMenuEventHandler, public ITreeViewEventHandler
{
public:
  bool begin(DagorAsset *asset, IObjEntity *entity);
  bool end();
  bool expectingAssetReload() const { return assetExpectedToReload.length() > 0; }
  void onCompositeEditorVisibilityChanged(bool shown);
  void fillCompositeTree();

  const CompositeEditorTreeDataNode *getSelectedTreeDataNode() const;
  unsigned getSelectedTreeNodeDataBlockId() const;
  void selectTreeNodeByDataBlockId(unsigned dataBlockId);
  void deleteSelectedNode(bool needsConfirmation = true);
  void updateSelectedNodeTransform(const TMatrix &tm);
  void cloneSelectedNode();

  void setGizmo(IEditorCoreEngine::ModeType mode);
  bool getPreventUiUpdatesWhileUsingGizmo() const { return preventUiUpdatesWhileUsingGizmo; }
  void setPreventUiUpdatesWhileUsingGizmo(bool prevent, bool wasCloning = false);

  void setEntityViewPluginInterface(IEntityViewPluginInterface &inEntityViewPluginInterface);
  IEntityViewPluginInterface &getEntityViewPluginInterface() const;

  void beginUndo();
  void endUndo(const char *operation_name, bool accept = true);
  void saveForUndo(DataBlock &dataBlock);
  void loadFromUndo(const DataBlock &dataBlock);

  eastl::unique_ptr<CompositeEditorTree> compositeTreeView;
  eastl::unique_ptr<CompositeEditorPanel> compositePropPanel;
  bool autoShow = false;
  int treeLastHeight = 0;
  int propPanelLastHeight = 0;

private:
  // This is mostly for sanity check to ensure that after requesting (or expecting) a reload it really happens.
  class ReloadHelper
  {
  public:
    ReloadHelper(CompositeEditor &editor, const DagorAsset &asset, CompositeEditorRefreshType inRefreshType);
    ~ReloadHelper();

  private:
    CompositeEditor &compositeEditor;
    const String assetName;
    const CompositeEditorRefreshType refreshType;
  };

  // ControlEventHandler
  virtual void onChange(int pcb_id, PropPanel2 *panel) override;
  virtual void onClick(int pcb_id, PropPanel2 *panel) override;

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id) override;

  // ITreeViewEventHandler
  virtual void onTvSelectionChange(TreeBaseWindow &tree, TLeafHandle new_sel) override;
  virtual bool onTvContextMenu(TreeBaseWindow &tree, TLeafHandle under_mouse, IMenu &menu) override;

  void fillCompositeTreeInternal(bool keepExpansionState);
  void fillCompositePropPanel();
  bool saveComposit();
  void revertChanges();
  bool canSwitchToAnotherAsset();
  void updateAssetFromTree(CompositeEditorRefreshType refreshType);
  void updateGizmo();
  void updateToolbarVisibility();

  void onDelayedRefresh(const DagorAsset *asset, CompositeEditorRefreshType refreshType);
  static void onDelayedRefreshEntity(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndTransformation(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndCompositEditor(const DagorAsset *asset);
  static void onDelayedRevert(const DagorAsset *asset);
  static void onDelayedSelectAsset(const DagorAsset *asset);

  static void focusViewport();

  IEntityViewPluginInterface *entityViewPluginInterface = nullptr;
  DagorAsset *editedAsset = nullptr;
  CompositeEditorTreeData treeData;
  CompositeEditorTreeDataNode *selectedTreeDataNode = nullptr;
  String assetExpectedToReload;
  CompositeEditorRefreshType reloadRefreshType = CompositeEditorRefreshType::Unset;
  bool ignoreTreeSelectionChangePanelRefresh = false;
  bool modified = false;

  CompositeEditorGizmoClient gizmoClient;
  CompositeEditorToolbar toolbar;
  IEditorCoreEngine::ModeType lastSelectedGizmoMode = IEditorCoreEngine::ModeType::MODE_None;
  bool preventUiUpdatesWhileUsingGizmo = false;
};
