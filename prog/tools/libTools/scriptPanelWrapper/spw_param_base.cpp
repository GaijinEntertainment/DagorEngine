// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <sqrat.h>
#include <propPanel/control/container.h>

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


void ScriptPanelParam::setupParameter(PropPanel::ContainerPropertyControl *panel, int &pid)
{
  mPid = pid;
  mPanel = panel;
}


void ScriptPanelParam::fillParameter(PropPanel::ContainerPropertyControl *panel, int &pid, Sqrat::Table so)
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
  if (mParam.HasKey("disabled"))
    enabled = !mParam.GetSlotValue<bool>("disabled", false);
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
    Sqrat::Object onChange = mParam.GetSlot("onChange");
    if (onChange.GetType() == OT_CLOSURE)
      onChangeCallback = new Sqrat::Function(mParam.GetVM(), mParam.GetObject(), onChange.GetObject());
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
    if (onChangeCallback->Execute())
    {
      CSQPanelWrapper *wrp = mParent->getWrapper();
      if (script_update_flag && wrp)
        wrp->setScriptUpdateFlag();
    }
    else
      logerr("Panel script error( %s::onChange() )", paramName.str());
  }
}

void ScriptPanelParam::setTooltip(const char *text)
{
  if (mPanel && mPid >= 0)
  {
    mPanel->setTooltipId(mPid, text);
  }
}

void ScriptPanelParam::setToScript(Sqrat::Table &param) {}
void ScriptPanelParam::getValueFromScript(Sqrat::Table param) {}


//=============================================================================
// base container without GUI


ScriptPanelContainer::ScriptPanelContainer(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name,
  const char *caption) :
  ScriptPanelParam(parent, name, caption), mItems(midmem), mPanelWrapper(wrapper)
{}


ScriptPanelContainer::~ScriptPanelContainer() { clear(); }


void ScriptPanelContainer::fillParams(PropPanel::ContainerPropertyControl *panel, int &pid, Sqrat::Table so)
{
  clear();
  if (panel)
    panel->clear();

  if (so.GetType() == OT_ARRAY)
  {
    Sqrat::Array arr(so);
    SQInteger len = arr.Length();
    for (SQInteger i = 0; i < len; ++i)
    {
      Sqrat::Table param = arr.GetSlot(i);
      if (param.GetType() == OT_TABLE)
        scriptControlFactory(panel, ++pid, param);
    }
  }
}


const char *ScriptPanelContainer::getParamType(Sqrat::Table param)
{
  Sqrat::Object tp = param.GetSlot("type");
  return tp.GetType() == OT_STRING ? tp.GetVar<const char *>().value : "group";
}


const char *ScriptPanelContainer::getParamName(Sqrat::Table param)
{
  Sqrat::Object name = param.GetSlot("name");
  return name.GetType() == OT_STRING ? name.GetVar<const char *>().value : "noname";
}


const char *ScriptPanelContainer::getParamCaption(Sqrat::Table param, const char *name)
{
  Sqrat::Object caption = param.GetSlot("caption");
  return caption.GetType() == OT_STRING ? caption.GetVar<const char *>().value : name;
}


bool ScriptPanelContainer::scriptExtFactory(PropPanel::ContainerPropertyControl *panel, int &pid, Sqrat::Table param)
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

long ScriptPanelContainer::onChanging(int pid, PropPanel::ContainerPropertyControl &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
  {
    int _value = mItems[i]->onChanging(pid, panel);
    if (_value != 0)
      return _value;
  }
  return 0;
}


void ScriptPanelContainer::onChange(int pid, PropPanel::ContainerPropertyControl &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->onChange(pid, panel);
}


void ScriptPanelContainer::onClick(int pid, PropPanel::ContainerPropertyControl &panel)
{
  for (int i = 0; i < mItems.size(); ++i)
    mItems[i]->onClick(pid, panel);
}


void ScriptPanelContainer::onPostEvent(int pid, PropPanel::ContainerPropertyControl &panel)
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


bool ScriptPanelContainer::getParamVisible(Sqrat::Table param)
{
  bool visible = true;
  if (param.HasKey("visible"))
    visible = param.GetSlotValue<bool>("visible", false);
  else
    param.SetValue("visible", visible);

  return visible;
}

const char *ScriptPanelContainer::getTooltipLocalizationKey(Sqrat::Table param)
{
  Sqrat::Object str = param.GetSlot(TOOLTIP_LOCALIZATION_KEY_PARAM);
  return str.GetType() == OT_STRING ? str.GetVar<const char *>().value : "";
}

const char *ScriptPanelContainer::getDefaultTooltip(Sqrat::Table param)
{
  Sqrat::Object str = param.GetSlot(TOOLTIP_DEFAULT_TEXT_PARAM);
  return str.GetType() == OT_STRING ? str.GetVar<const char *>().value : "";
}

//=============================================================================
