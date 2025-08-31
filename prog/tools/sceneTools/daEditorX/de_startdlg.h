// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_startDlg.h>
#include <ioSys/dag_dataBlock.h>

class DeWorkspace;

enum
{
  VISIBLE_RECENT_COUNT = 10,

  ID_SDK_DIR = ID_START_DIALOG_BUTTON_EDIT + 1,
  ID_GAME_DIR,
  ID_LIB_DIR,

  ID_GROUP,
  ID_CREATE_NEW,
  ID_OPEN_PROJECT,
  ID_RECENT_FIRST,
  ID_RECENT_LAST = ID_RECENT_FIRST + VISIBLE_RECENT_COUNT,
  ID_OPEN_DRAG_AND_DROPPED_SCREENSHOT = ID_RECENT_LAST + 1,

  // only set if a workspace was specified in the command line and that workspace did not exist (and the startup dialog
  // was shown because of that)
  ID_OPEN_FROM_COMMAND_LINE,

  PID_EXPORT_PLUGINS_GRP,
  PID_EXPORTER_FIRST,
  PID_EXPORTER_LAST = PID_EXPORTER_FIRST + 500,
};

class StartupDlg : public EditorStartDialog
{
public:
  StartupDlg(const char *caption, DeWorkspace &wsp, const char *wsp_blk, const char *select_wsp,
    bool project_file_set_from_command_line);

  ~StartupDlg() override;

  int getSelected() const;

  void onCustomFillPanel(PropPanel::ContainerPropertyControl &panel) override;
  bool onCustomSettings(PropPanel::ContainerPropertyControl &panel) override;

  bool onDropFiles(const dag::Vector<String> &files);
  const DataBlock &getScreenshotMetaInfo() const { return mScreenshotMetaInfo; }
  const String &getFilePathFromScreenshotMetaInfo() const { return mFilePathFromScreenshotMetaInfo; }

protected:
  void onAddWorkspace() override;
  void onChangeWorkspace(const char *name) override;

  void onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  bool onOk() override;

private:
  Tab<class IGenEditorPlugin *> exporters;

  void fillExportPluginsGrp(PropPanel::ContainerPropertyControl &panel);

  PropPanel::ContainerPropertyControl *mPanel;
  int mSelected;
  DataBlock mScreenshotMetaInfo;
  String mFilePathFromScreenshotMetaInfo;
  bool projectFileSetFromCommandLine;
};
