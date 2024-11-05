// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_startdlg.h"
#include "de_screenshotMetaInfoLoader.h"

#include <oldEditor/de_workspace.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <de3_huid.h>

#include <generic/dag_sort.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/containers/dag_PathMap.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <debug/dag_debug.h>


#define HOR_INDENT  3
#define VERT_INDENT 5

#define BUTTON_WIDTH  100
#define BUTTON_HEIGHT 28

#define LABEL_HEIGHT 20
#define EDIT_HEIGHT  23
#define EDIT_WIDTH   400
#define EDIT_LEN     128

#define WORKSPACE_DIALOG_HEIGHT 540


typedef IGenEditorPlugin *PluginPtr;

static int sortPlugins(const PluginPtr *a, const PluginPtr *b)
{
  return ::strcmp((*a)->getMenuCommandName(), (*b)->getMenuCommandName());
}


//==============================================================================
StartupDlg::StartupDlg(const char *caption, DeWorkspace &wsp, const char *wsp_blk, const char *select_wsp) :

  EditorStartDialog(caption, wsp, wsp_blk, select_wsp), exporters(tmpmem), mPanel(NULL), mSelected(-1)
{
  mPanel = getPanel();
  G_ASSERT(mPanel && "StartupDlg::StartupDlg: NO PANEL FOUND");

  mPanel->createStatic(ID_SDK_DIR, "");
  mPanel->createStatic(ID_GAME_DIR, "");
  mPanel->createStatic(ID_LIB_DIR, "");

  mPanel->createSeparator(0);

  mPanel->createRadioGroup(ID_GROUP, "Select action:");

  reloadWsp();
}


//==============================================================================
StartupDlg::~StartupDlg() {}


//==============================================================================

void StartupDlg::onAddWorkspace()
{
  ((DeWorkspace &)wsp).clear();

  EditorStartDialog::onAddWorkspace();
}

//==============================================================================
void StartupDlg::onChangeWorkspace(const char *name)
{
  EditorStartDialog::onChangeWorkspace(name);

  String caption;

  caption.printf(512, "Develop:\t%s", wsp.getSdkDir());
  mPanel->setText(ID_SDK_DIR, caption);

  caption.printf(512, "Game:\t\t%s", wsp.getGameDir());
  mPanel->setText(ID_GAME_DIR, caption);

  caption.printf(512, "Libraries:\t%s", wsp.getLibDir());
  mPanel->setText(ID_LIB_DIR, caption);

  Tab<String> &recent = ((DeWorkspace &)wsp).getRecents();
  int recentCnt = (recent.size() > VISIBLE_RECENT_COUNT) ? VISIBLE_RECENT_COUNT : recent.size();

  int selIdx = mPanel->getInt(ID_GROUP);
  if (selIdx == PropPanel::RADIO_SELECT_NONE)
    selIdx = 2;
  else
    selIdx -= ID_CREATE_NEW;

  if (!recent.size())
    selIdx = 1;

  if (mSelected != -1)
    return;

  PropPanel::ContainerPropertyControl *_group = mPanel->getById(ID_GROUP)->getContainer();
  G_ASSERT(_group && "StartupDlg::onChangeWorkspace: NO RADIO GROUP FOUND");

  _group->clear();

  //_group->createRadio(ID_CREATE_NEW, "Create new project...");
  _group->createRadio(ID_OPEN_PROJECT, "Open project...");

  for (int i = 0; i < recentCnt; ++i)
  {
    caption.printf(512, "Open recent: %s", (const char *)recent[i]);
    _group->createRadio(ID_RECENT_FIRST + i, caption);
  }

  mPanel->setInt(ID_GROUP, selIdx + ID_CREATE_NEW);
}

//==============================================================================

void StartupDlg::onCustomFillPanel(PropPanel::ContainerPropertyControl &panel)
{
  EditorStartDialog::onCustomFillPanel(panel);

  fillExportPluginsGrp(panel);
}


//==============================================================================
bool StartupDlg::onCustomSettings(PropPanel::ContainerPropertyControl &panel)
{
  EditorStartDialog::onCustomSettings(panel);

  DeWorkspace &deWsp = (DeWorkspace &)wsp;

  // default export
  Tab<String> &ignore = deWsp.getExportIgnore();
  ignore.clear();

  int _count = (exporters.size() > PID_EXPORTER_LAST) ? PID_EXPORTER_LAST : exporters.size();

  for (int i = 0; i < _count; ++i)
    if (!panel.getBool(PID_EXPORTER_FIRST + i))
      ignore.push_back() = exporters[i]->getMenuCommandName();

  return true;
}


//==============================================================================
void StartupDlg::fillExportPluginsGrp(PropPanel::ContainerPropertyControl &panel)
{
  exporters.clear();

  int i;

  for (i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plug = DAGORED2->getPlugin(i);

    if (plug)
      if (plug->queryInterfacePtr(HUID_IBinaryDataBuilder))
        exporters.push_back(plug);
  }

  if (!exporters.size())
    return;

  PropPanel::ContainerPropertyControl *maxGrp = panel.createGroup(PID_EXPORT_PLUGINS_GRP, "Default export");
  G_ASSERT(maxGrp);

  DeWorkspace &deWsp = (DeWorkspace &)wsp;
  const Tab<String> &ignore = deWsp.getExportIgnore();

  sort(exporters, &sortPlugins);

  int _exp_count = (exporters.size() > PID_EXPORTER_LAST) ? PID_EXPORTER_LAST : exporters.size();

  for (i = 0; i < _exp_count; ++i)
  {
    bool check = true;
    const char *plugName = exporters[i]->getMenuCommandName();

    for (int j = 0; j < ignore.size(); ++j)
      if (!::stricmp(plugName, ignore[j]))
      {
        check = false;
        break;
      }

    maxGrp->createCheckBox(PID_EXPORTER_FIRST + i, plugName, check);
  }
}


//==============================================================================

int StartupDlg::getSelected() const { return mSelected; }

//==============================================================================

void StartupDlg::onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (mPanel->getInt(ID_GROUP) != PropPanel::RADIO_SELECT_NONE)
    clickDialogButton(PropPanel::DIALOG_ID_OK);
}


bool StartupDlg::onOk()
{
  mSelected = mPanel->getInt(ID_GROUP);

  if (!EditorStartDialog::onOk())
  {
    mScreenshotMetaInfo.reset();
    mFilePathFromScreenshotMetaInfo.clear();
    return false;
  }

  if (!mFilePathFromScreenshotMetaInfo.empty())
    mSelected = ID_OPEN_DRAG_AND_DROPPED_SCREENSHOT;

  return true;
}


bool StartupDlg::onDropFiles(const dag::Vector<String> &files)
{
  if (files.empty())
    return false;

  String errorMessage;
  if (!ScreenshotMetaInfoLoader::loadMetaInfo(files[0], mScreenshotMetaInfo, errorMessage) ||
      !ScreenshotMetaInfoLoader::getProjectPath(mScreenshotMetaInfo, wsp.getAppDir(), wsp.getLevelsDir(),
        mFilePathFromScreenshotMetaInfo, errorMessage))
  {
    wingw::message_box(wingw::MBS_EXCL, "Error", "Error loading camera settings from screenshot\n\"%s\"\n\n%s", files[0].c_str(),
      errorMessage.c_str());
    return true;
  }

  clickDialogButton(PropPanel::DIALOG_ID_OK);

  return true;
}
