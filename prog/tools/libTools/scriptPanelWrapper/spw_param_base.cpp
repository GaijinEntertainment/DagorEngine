// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <sqplus.h>
#include <propPanel2/c_panel_base.h>

#include <scriptPanelWrapper/spw_main.h>
#include <scriptPanelWrapper/spw_param.h>
#include <scriptPanelWrapper/spw_ext.h>

#include <debug/dag_debug.h>

//=============================================================================

static const char *TOOLTIP_LOCALIZATION_KEY_PARAM = "tooltip_loc";
static const char *TOOLTIP_DEFAULT_TEXT_PARAM = "tooltip";

ScriptPanelParam::~ScriptPanelParam()
{
  if (onChangeCallback)
  {
    delete onChangeCallback;
    onChangeCallback = NULL;
  }
}


void ScriptPanelParam::setupParameter(PropPanel2 *panel, int &pid)
{
  mPid = pid;
  mPanel = panel;
}


void ScriptPanelParam::fillParameter(PropPanel2 *panel, int &pid, SquirrelObject so)
{
  setupParameter(panel, pid);
  mParam = so;
  getFromScript();
}


void ScriptPanelParam::removeControl()
{
  if (mPanel)
    mPanel->removeById(mPid);
}


void ScriptPanelParam::getFromScript()
{
  getValueFromScript(mParam);

  bool visible = true;
  if (mParent)
    visible = mParent->getParamVisible(mParam);

  bool enabled = true;
  if (mParam.Exists("disabled"))
    enabled = !mParam.GetBool("disabled");
  else
    mParam.SetValue("disabled", !enabled);


  if (mVisible != visible)
    setVisible(visible);
  else
    updateParams();

  if (mEnabled != enabled && mPanel)
    setEnabled(enabled);

  if (!onChangeCallback)
  {
    onChangeCallback = new SqPlus::SquirrelFunction<void>(mParam, "onChange");
    if (onChangeCallback->func.IsNull())
    {
      delete onChangeCallback;
      onChangeCallback = NULL;
    }
  }
}


void ScriptPanelParam::setVisible(bool visible)
{
  mVisible = visible;
  if (visible)
  {
    createControl();

    int next_pid = -1;
    if (mPanel && mParent && (next_pid = mParent->getNextVisiblePid(mPid)) != -1)
      mPanel->moveById(mPid, next_pid);
  }
  else
    removeControl();
}


void ScriptPanelParam::setEnabled(bool enabled) { mPanel->setEnabledById(mPid, mEnabled = enabled); }


void ScriptPanelParam::callChangeScript(bool script_update_flag)
{
  if (onChangeCallback)
  {
    try
    {
      (*onChangeCallback)();

      CSQPanelWrapper *wrp = mParent->getWrapper();
      if (script_update_flag && wrp)
        wrp->setScriptUpdateFlag();
    }
    catch (SquirrelError e)
    {
      logerr("Panel script error( %s::onChange() ): %s", paramName.str(), e.desc);
    }
  }
}

void ScriptPanelParam::setTooltip(const char *text)
{
  if (mPanel && mPid >= 0)
  {
    mPanel->setTooltipId(mPid, text);
  }
}

void ScriptPanelParam::setToScript(SquirrelObject &param) {}
void ScriptPanelParam::getValueFromScript(SquirrelObject param) {}


//=============================================================================
// base container without GUI


ScriptPanelContainer::ScriptPanelContainer(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name,
  const char *caption) :
  ScriptPanelParam(parent, name, caption), mItems(midmem), mPanelWrapper(wrapper)
{}


ScriptPanelContainer::~ScriptPanelContainer() { clear(); }


void ScriptPanelContainer::fillParams(PropPanel2 *panel, int &pid, SquirrelObject so)
{
  clear();
  if (panel)
    panel->clear();

  if (!so.IsNull() && so.GetType() == OT_ARRAY && so.BeginIteration())
  {
    SquirrelObject param, key;

    while (so.Next(key, param))
    {
      if (param.GetType() == OT_TABLE)
        scriptControlFactory(panel, ++pid, param);
    }
    so.EndIteration();
  }
}


const char *ScriptPanelContainer::getParamType(SquirrelObject param)
{
  return (param.Exists("type") && param.GetString("type")) ? param.GetString("type") : "group";
}


const char *ScriptPanelContainer::getParamName(SquirrelObject param)
{
  return (param.Exists("name") && param.GetString("name")) ? param.GetString("name") : "noname";
}


const char *ScriptPanelContainer::getParamCaption(SquirrelObject param, const char *name)
{
  return (param.Exists("caption") && param.GetString("caption")) ? param.GetString("caption") : name;
}


bool ScriptPanelContainer::scriptExtFactory(PropPanel2 *panel, int &pid, SquirrelObject param)
{
  ScriptPanelParam *panel_param = NULL;

  const char *type = getParamType(param);
  const char *name = getParamName(param);
  const char *caption = getParamCaption(param, name);

  if (stricmp(type, "group") == 0)
    panel_param = new ScriptExtGroup(mPanelWrapper, this, name, caption);
  else
    panel_param = new ScriptExtContainer(mPanelWrapper, this, name, caption);

  if (panel_param)
  {
    mItems.push_back(panel_param);
    panel_param->fillParameter(panel, pid, param);
    return true;
  }

  return false;
}


void ScriptPanelContainer::clear()
{
  for (int i = 0; i < mItems.size(); ++i)
    delete mItems[i];

  clear_and_shrink(mItems);
}


void ScriptPanelContainer::removeControl()
{
  clear();
  ScriptPanelParam::removeControl();
}


int ScriptPanelContainer::getNextVisiblePid(int pid)
{
  int found_index = -1;

  for (int i = 0; i < mItems.size(); ++i)
    if (mItems[i]->getPid() == pid)
    {
      found_index = i;
      break;
    }

  for (int i = found_index + 1; i < mItems.size(); ++i)
    if (mItems[i]->getVisible())
    {
      return mItems[i]->getFirstPid();
    }

  return -1;
}

int ScriptPanelContainer::findPidByName(const char *name)
{
  for (int i = 0; i < mItems.size(); i++)
    if (stricmp(mItems[i]->getParamName(), name) == 0)
      return mItems[i]->getPid();
  return -1;
}

void ScriptPanelContainer::sendMessage(int pid, int msg, void *arg)
{
  if (pid == 0 || pid == mPid)
  {
    onMessage(pid, msg, arg);
  }
  else
  {
    for (int i = 0; i < mItems.size(); i++)
    {
      if (mItems[i]->getPid() == pid)
      {
        mItems[i]->onMessage(pid, msg, arg);
        return;
      }
    }
  }
}

long ScriptPanelContainer::onChanging(int pid, PropPanel2 &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
  {
    int _value = mItems[i]->onChanging(pid, panel);
    if (_value != 0)
      return _value;
  }
  return 0;
}


void ScriptPanelContainer::onChange(int pid, PropPanel2 &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->onChange(pid, panel);
}


void ScriptPanelContainer::onClick(int pid, PropPanel2 &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->onClick(pid, panel);
}


void ScriptPanelContainer::onPostEvent(int pid, PropPanel2 &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->onPostEvent(pid, panel);
}


void ScriptPanelContainer::save(DataBlock &blk)
{
  DataBlock *cont_blk = (paramName.empty()) ? &blk : blk.addBlock(paramName);

  if (cont_blk)
  {
    for (int i = 0; i < mItems.size(); ++i)
      mItems[i]->save(*cont_blk);
  }
}


void ScriptPanelContainer::load(const DataBlock &blk)
{
  const DataBlock *cont_blk = (paramName.empty()) ? &blk : blk.getBlockByName(paramName);

  if (cont_blk)
  {
    for (int i = 0; i < mItems.size(); ++i)
      mItems[i]->load(*cont_blk);
  }
}


void ScriptPanelContainer::callChangeScript(bool script_update_flag)
{
  for (int i = 0; i < mItems.size(); ++i)
    if (mItems[i]->getVisible())
    {
      mItems[i]->callChangeScript(false);
    }
}


void ScriptPanelContainer::updateParams()
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->getFromScript();
}


void ScriptPanelContainer::validate()
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->validate();
}


void ScriptPanelContainer::getTargetList(Tab<SimpleString> &list)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->getTargetList(list);
}


bool ScriptPanelContainer::getParamVisible(SquirrelObject param)
{
  bool visible = true;
  if (param.Exists("visible"))
    visible = param.GetBool("visible");
  else
    param.SetValue("visible", visible);

  return visible;
}

const char *ScriptPanelContainer::getTooltipLocalizationKey(SquirrelObject param)
{
  return (param.Exists(TOOLTIP_LOCALIZATION_KEY_PARAM) && param.GetString(TOOLTIP_LOCALIZATION_KEY_PARAM))
           ? param.GetString(TOOLTIP_LOCALIZATION_KEY_PARAM)
           : "";
}

const char *ScriptPanelContainer::getDefaultTooltip(SquirrelObject param)
{
  return (param.Exists(TOOLTIP_DEFAULT_TEXT_PARAM) && param.GetString(TOOLTIP_DEFAULT_TEXT_PARAM))
           ? param.GetString(TOOLTIP_DEFAULT_TEXT_PARAM)
           : "";
}

//=============================================================================
