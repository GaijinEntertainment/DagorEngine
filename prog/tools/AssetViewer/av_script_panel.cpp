// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <scriptPanelWrapper/spw_main.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/blkUtil.h>
#include <sepGui/wndGlobal.h>
#include <debug/dag_debug.h>
#include <assetsGui/av_selObjDlg.h>
#include <assets/asset.h>

#include "av_script_panel.h"
#include "av_appwnd.h"

void AVScriptPanelEditor::initVars()
{
  static bool inited = false;
  if (!inited)
  {
    String app_dir(::get_app().getWorkspace().getAppDir());
    simplify_fname(app_dir);
    CSQPanelWrapper::addGlobalConst("APP_DIR", app_dir);
    inited = true;
  }
}

const unsigned UPDATE_TIME_SHIFT_MS = 1000;


AVScriptPanelEditor::AVScriptPanelEditor(const char *script_path, const char *panel_caption) :
  scriptPanelWrap(NULL), mAsset(NULL), needCallChange(false), ignoreChangesCnt(0), panelCaption("Object script parameters")
{
  initVars();
  SimpleString scripts_dir(::get_app().getWorkspace().getAssetScriptDir());
  if (scripts_dir.empty())
    scripts_dir = ::make_full_path(sgg::get_exe_path(), "../commonData/scripts/");
  scriptPath = ::make_full_path(scripts_dir.str(), script_path);
  updateTimer = new WinTimer(this, UPDATE_TIME_SHIFT_MS, true);
  if (panel_caption)
    panelCaption = panel_caption;
}


AVScriptPanelEditor::~AVScriptPanelEditor()
{
  del_it(updateTimer);
  del_it(scriptPanelWrap);
}


bool AVScriptPanelEditor::scriptExists() { return ::dd_file_exist(scriptPath.str()); }


void AVScriptPanelEditor::createPanel(PropPanel2 &panel)
{
  if (!scriptExists() || get_app().getScriptChangeFlag())
    return;

  del_it(scriptPanelWrap);

  ignoreChangesCnt = 2;
  scriptPanelWrap = new CSQPanelWrapper(&panel);
  scriptPanelWrap->setObjectCB(this);
  scriptPanelWrap->setEventHandler(this);
  scriptPanelWrap->bindScript(scriptPath.str());
  scriptPanelWrap->setDataBlock(&propsBlk);
  if (mAsset)
    panel.setEnabled(!mAsset->isVirtual());
  panel.setCaptionValue(panelCaption.str());
}


void AVScriptPanelEditor::destroyPanel()
{
  if (scriptPanelWrap && !get_app().getScriptChangeFlag())
  {
    scriptPanelWrap->updateDataBlock();
    del_it(scriptPanelWrap);
  }
}


void AVScriptPanelEditor::load(DagorAsset *asset)
{
  mAsset = asset;
  if (!mAsset || get_app().getScriptChangeFlag())
    return;

  if (mAsset->isVirtual())
    propsBlk.setFrom(&mAsset->props);
  else if (DataBlock *src_blk = mAsset->props.getBlockByName("__src"))
    propsBlk.setFrom(src_blk);
  else
  {
    if (!::dd_file_exist(mAsset->getSrcFilePath()))
      propsBlk.reset();
    else
    {
      propsBlk.parseIncludesAsParams = true;
      propsBlk.parseCommentsAsParams = true;
      propsBlk.load(mAsset->getSrcFilePath());
      propsBlk.parseIncludesAsParams = false;
      propsBlk.parseCommentsAsParams = false;
    }
  }
}


void AVScriptPanelEditor::onScriptPanelChange()
{
  if (ignoreChangesCnt > 0)
  {
    --ignoreChangesCnt;
    return;
  }

  if (mAsset && !mAsset->isVirtual())
  {
    get_app().setScriptChangeFlag();
    needCallChange = true;
  }
}


void AVScriptPanelEditor::update()
{
  if (!mAsset || !needCallChange || !scriptPanelWrap)
    return;

  needCallChange = false;
  scriptPanelWrap->updateDataBlock();
  SimpleString str_data = blk_util::blkTextData(propsBlk);
  mAsset->props.loadText(str_data.str(), str_data.length(), mAsset->getSrcFilePath());
  DataBlock *src_blk = mAsset->props.addBlock("__src");
  if (src_blk)
    src_blk->setFrom(&propsBlk);
  get_app().getAssetMgr().callAssetChangeNotifications(*mAsset, mAsset->getNameId(), mAsset->getType());
}


const char *AVScriptPanelEditor::getTarget(const char *old_choise, const char *type, const char filter[])
{
  if (stricmp(type, "asset") == 0)
  {
    Tab<int> masks(tmpmem);
    const DagorAssetMgr *amgr = &get_app().getAssetMgr();

    const char delims[] = " ,";
    char buffer[256];
    strncpy(buffer, filter, 255);
    char *pch = strtok(buffer, delims);
    while (pch != NULL)
    {
      masks.push_back(amgr->getAssetTypeId(pch));
      pch = strtok(NULL, delims);
    }

    return selectAsset(old_choise, masks);
  }
  if (stricmp(type, "ext_param_name") == 0)
  {
    return selectGroupName(old_choise);
  }

  return NULL;
}


const char *AVScriptPanelEditor::selectAsset(const char *old_choise, dag::ConstSpan<int> masks)
{
  int _x, _y;
  unsigned _w, _h;
  static String result;
  G_ASSERT(get_app().getPropPanel() && "Plugin panel closed!");
  if (!get_app().getWndManager().getWindowPosSize(get_app().getPropPanel()->getParentWindowHandle(), _x, _y, _w, _h))
    return NULL;
  get_app().getWndManager().clientToScreen(_x, _y);
  DagorAssetMgr *amgr = const_cast<DagorAssetMgr *>(&get_app().getAssetMgr());

  SelectAssetDlg dlg(0, amgr, "Select asset", "Select asset", "Reset asset", masks, _x - _w - 5, _y, _w, _h);
  dlg.selectObj(old_choise);
  int ret = dlg.showDialog();

  if (ret == DIALOG_ID_CLOSE)
    return old_choise;
  if (ret == DIALOG_ID_OK)
  {
    result = dlg.getSelObjName();
    return result.str();
  }

  return "";
}


const char *AVScriptPanelEditor::selectGroupName(const char *old_choise)
{
  static String result;
  CDialogWindow *dialog = new CDialogWindow(NULL, hdpi::_pxScaled(250), hdpi::_pxScaled(130), "Select group name");
  dialog->getPanel()->createEditBox(0, "Name", old_choise);

  if (dialog->showDialog() == DIALOG_ID_OK)
    result = dialog->getPanel()->getText(0);
  delete dialog;
  return result.length() ? result.str() : old_choise;
}


const char *AVScriptPanelEditor::validateTarget(const char *name, const char *type) { return NULL; }
