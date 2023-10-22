#ifndef __GAIJIN_DAGORED_DE_START_DLG__
#define __GAIJIN_DAGORED_DE_START_DLG__
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

  PID_EXPORT_PLUGINS_GRP,
  PID_EXPORTER_FIRST,
  PID_EXPORTER_LAST = PID_EXPORTER_FIRST + 500,
};

class StartupDlg : public EditorStartDialog
{
public:
  StartupDlg(void *phandle, const char *caption, DeWorkspace &wsp, const char *wsp_blk, const char *select_wsp);

  ~StartupDlg();

  int getSelected() const;

  virtual void onCustomFillPanel(PropPanel2 &panel);
  virtual bool onCustomSettings(PropPanel2 &panel);
  const DataBlock &getScreenshotMetaInfo() const { return mScreenshotMetaInfo; }
  const String &getFilePathFromScreenshotMetaInfo() const { return mFilePathFromScreenshotMetaInfo; }

protected:
  virtual void onAddWorkspace();
  virtual void onChangeWorkspace(const char *name);

  virtual void onDoubleClick(int pcb_id, PropPanel2 *panel);
  virtual bool onOk();
  virtual bool onDropFiles(const dag::Vector<String> &files) override;

private:
  Tab<class IGenEditorPlugin *> exporters;

  void fillExportPluginsGrp(PropPanel2 &panel);

  PropertyContainerControlBase *mPanel;
  int mSelected;
  DataBlock mScreenshotMetaInfo;
  String mFilePathFromScreenshotMetaInfo;
};


#endif //__GAIJIN_DAGORED_DE_START_DLG__
