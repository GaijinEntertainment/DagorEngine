// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint4.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/customControl.h>
#include <propPanel/control/panelWindow.h>
#include <util/dag_string.h>

class DagorAsset;
class IModelessWindowController;
class TextEditor;

class CanopyEditorWindow final : public PropPanel::ControlEventHandler,
                                 public PropPanel::ICustomControl,
                                 public PropPanel::PanelWindowPropertyControl
{
public:
  struct ViewportCanopyParams
  {
    enum Shape
    {
      BOX,
      CONE,
      SPHEROID,
    };

    bool valid = false;
    Shape shape = BOX;
    float topOffset = 0.1f;
    float topPart = 0.4f;
    float widthPart = 0.3f;
    float opacity = 0.0f;
  };

  CanopyEditorWindow();
  ~CanopyEditorWindow();

  bool canSelectAsset(const DagorAsset *next_asset);
  bool canClose();
  void onAssetSelectionChanged(const DagorAsset *asset);
  void updateImgui() override;
  void customControlUpdate(int id) override;
  const char *getViewportFxAssetName() const;
  bool getViewportCanopyParams(ViewportCanopyParams &params) const;
  void setSelectedBlkPath(const char *path);
  int saveState(DataBlock &datablk, bool by_name = false) override;
  int loadState(DataBlock &datablk, bool by_name = false) override;

private:
  struct ControlPath
  {
    bool isBlock = false;
    int itemIndex = -1;
    eastl::vector<int> blockPath;
  };

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void reloadSelectedBlk();
  void updateCurrentAssetParameters();
  void refreshPanel(bool rebuild_parameters_panel = true);
  void rebuildParametersPanel();
  void rebuildParametersPanel(PropPanel::ContainerPropertyControl &panel, DataBlock &block, eastl::vector<int> &block_path);
  void applyEditorText(bool rebuild_parameters_panel);
  bool flushPendingEditorTextBeforeVisualChange();
  void syncTextFromRecognizedParameters();
  void updateViewportFxState();
  void discardUnsavedChanges();
  bool saveCurrentParameters();
  bool resolveUnsavedChanges();
  bool canEditParameters() const;
  void updateControlStates();
  void syncTextEditorWidget();
  void onTextEditorValueChanged(const char *text);
  void updateDirtyState();
  DataBlock *getEditableBlock(const eastl::vector<int> &path);
  const DataBlock *getEditableBlock(const eastl::vector<int> &path) const;

  String getResolvedBlkPath() const;

  PropPanel::ContainerPropertyControl *parametersGroup = nullptr;
  PropPanel::ContainerPropertyControl *textEditorGroup = nullptr;
  String selectedBlkPath;
  String loadedFileText;
  String lineEnding;
  String currentAssetName;
  String savedParametersText;
  String editorParametersText;
  String textEditorWidgetText;
  String viewportFxAssetName;
  DataBlock loadedBlk;
  DataBlock editableDmgBlock;
  eastl::vector<ControlPath> actionPaths;
  eastl::vector<ControlPath> valuePaths;
  eastl::vector<int> currentDmgBlockPath;
  bool dirty = false;
  bool showFx = false;
  bool viewportFxAvailable = false;
  bool updatingControls = false;
  bool pendingTextParse = false;
  int lastTextChangeMs = 0;
  TextEditor *parametersTextEditor = nullptr;
};

IModelessWindowController *get_canopy_editor_window_controller();
DagorAsset *resolve_canopy_fx_asset(const char *name);
void load_canopy_editor_settings(const DataBlock &blk);
void save_canopy_editor_settings(DataBlock &blk);
void load_canopy_editor_window_state(CanopyEditorWindow &window);
void save_current_canopy_editor_window_state();
