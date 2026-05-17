// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <scriptPanelWrapper/spw_main.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/blkUtil.h>
#include <EditorCore/ec_wndGlobal.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <debug/dag_debug.h>

#include "script_panel.h"

using hdpi::_pxScaled;

const unsigned UPDATE_TIME_SHIFT_MS = 300;

ScriptPanelEditor::ScriptPanelEditor(BlkHandler *handler) :
  scriptPanelWrap(NULL), blkHandler(handler), manualSetCnt(0), needCallChange(false)
{

  scriptPath = "";
  scriptsDir = ::make_full_path(sgg::get_exe_path_full(), "../commonData/scripts/");
  ::dd_simplify_fname_c(scriptsDir.str());
  CSQPanelWrapper::addGlobalConst("APP_DIR", sgg::get_exe_path_full());
  updateTimer = new WinTimer(this, UPDATE_TIME_SHIFT_MS, true);
}


ScriptPanelEditor::~ScriptPanelEditor()
{
  if (updateTimer)
    del_it(updateTimer);
  if (scriptPanelWrap)
    del_it(scriptPanelWrap);
}


const char *ScriptPanelEditor::getScriptFn()
{
  static String script_fn;
  script_fn = "";
  SimpleString _fn(propsBlk.getStr("__scheme", ""));
  if (_fn.length() > 0)
    script_fn = ::make_full_path(scriptsDir, _fn.str());
  return script_fn.str();
}

bool ScriptPanelEditor::hasScript() { return ::dd_file_exist(scriptPath.str()); }

void ScriptPanelEditor::updateScript()
{
  if (scriptPanelWrap && ::dd_file_exist(scriptPath.str()))
  {
    scriptPanelWrap->bindScript(scriptPath.str());
    scriptPanelWrap->setDataBlock(&propsBlk);
  }
}


void ScriptPanelEditor::createPanel()
{
  if (scriptPanelWrap)
    del_it(scriptPanelWrap);

  if (!::dd_file_exist(scriptPath.str()))
    return;

  if (blkHandler && blkHandler->getBlkPanel())
  {
    scriptPanelWrap = new CSQPanelWrapper(blkHandler->getBlkPanel());
    scriptPanelWrap->setObjectCB(this);
    scriptPanelWrap->setEventHandler(this);
    updateScript();
  }
}


void ScriptPanelEditor::destroyPanel()
{
  if (scriptPanelWrap)
  {
    scriptPanelWrap->updateDataBlock();
    del_it(scriptPanelWrap);
  }
  manualSetCnt = 0;
}

const char *ScriptPanelEditor::getTarget(const char *old_choise, const char *type, const char filter[])
{
  return selectText(old_choise);
}


const char *ScriptPanelEditor::selectText(const char *old_choise)
{
  static String result;
  PropPanel::DialogWindow *dialog = new PropPanel::DialogWindow(NULL, _pxScaled(250), _pxScaled(130), "Select text");
  dialog->getPanel()->createEditBox(0, "Text", old_choise);

  if (dialog->showDialog() == PropPanel::DIALOG_ID_OK)
    result = dialog->getPanel()->getText(0);
  delete dialog;
  return result.length() ? result.str() : old_choise;
}


const char *ScriptPanelEditor::validateTarget(const char *name, const char *type) { return NULL; }


const char *ScriptPanelEditor::getBlkText()
{
  static SimpleString value;
  propsBlk.parseIncludesAsParams = true;
  propsBlk.parseCommentsAsParams = true;
  value = blk_util::blkTextData(propsBlk);
  propsBlk.parseIncludesAsParams = false;
  propsBlk.parseCommentsAsParams = false;
  return value.str();
}


void ScriptPanelEditor::setBlkText(const char *text, int size)
{
  propsBlk.parseIncludesAsParams = true;
  propsBlk.parseCommentsAsParams = true;
  propsBlk.loadText(text, size);
  propsBlk.parseIncludesAsParams = false;
  propsBlk.parseCommentsAsParams = false;
  updateFromDataBlock();
}


void ScriptPanelEditor::setScriptPath(const char *path)
{
  propsBlk.setStr("__scheme", ::make_path_relative(path, getScriptsDir()));
  updateFromDataBlock();
}


void ScriptPanelEditor::updateFromDataBlock()
{
  String new_script_fn(getScriptFn());
  if (scriptPanelWrap && new_script_fn == scriptPath)
  {
    ++manualSetCnt;
    scriptPanelWrap->setDataBlock(&propsBlk);
    return;
  }
  scriptPath = new_script_fn;
  if (!::dd_file_exist(scriptPath.str()))
  {
    destroyPanel();
    return;
  }
  ++manualSetCnt;
  if (scriptPanelWrap)
    updateScript();
  else
    createPanel();
}


void ScriptPanelEditor::load(const char *fn)
{
  propsBlk.parseIncludesAsParams = true;
  propsBlk.parseCommentsAsParams = true;
  propsBlk.load(fn);
  propsBlk.parseIncludesAsParams = false;
  propsBlk.parseCommentsAsParams = false;
  scriptPath = getScriptFn();
  if (!::dd_file_exist(scriptPath.str()))
    destroyPanel();
  else
  {
    manualSetCnt = 1;
    if (scriptPanelWrap)
      updateScript();
    else
      createPanel();
  }
  if (!scriptPanelWrap)
    onScriptPanelChange();
}


void ScriptPanelEditor::save(const char *fn)
{
  if (scriptPanelWrap)
    scriptPanelWrap->updateDataBlock();
  propsBlk.parseIncludesAsParams = true;
  propsBlk.parseCommentsAsParams = true;
  propsBlk.saveToTextFile(fn);
  propsBlk.parseIncludesAsParams = false;
  propsBlk.parseCommentsAsParams = false;
}


void ScriptPanelEditor::onScriptPanelChange()
{
  if (manualSetCnt == 0)
  {
    if (scriptPanelWrap)
      scriptPanelWrap->updateDataBlock();
    needCallChange = true;
  }
  else if (manualSetCnt > 0)
    --manualSetCnt;
}


void ScriptPanelEditor::update()
{
  if (needCallChange)
  {
    if (blkHandler)
      blkHandler->onBlkChange();
    needCallChange = false;
  }
}