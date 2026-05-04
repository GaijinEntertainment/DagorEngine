// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/container.h>

#include <scriptPanelWrapper/spw_main.h>
#include <scriptPanelWrapper/spw_param.h>

#include <sqrat.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>
#include <sqmodules/sqmodules.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/sqModulesDagor.h>

#include "spw_script.inc.cpp"

enum
{
  START_PID = 100,
};


Tab<CSQPanelWrapper::GlobalConst> CSQPanelWrapper::globalConsts(midmem);

static void script_print_func(HSQUIRRELVM /*v*/, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(_MAKE4C('SQRL'), s, vl);
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM v, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  cvlogmessage(LOGLEVEL_ERR, s, vl);
  va_end(vl);
}


CSQPanelWrapper::CSQPanelWrapper(PropPanel::ContainerPropertyControl *panel) :
  mPanel(panel), mScriptFilename(""), mDataBlock(NULL), mPanelContainer(NULL), objCB(NULL), mHandler(NULL), mPostEventCounter(0)
{
  init();
}

void CSQPanelWrapper::init()
{
  if (mPanel)
  {
    mPanel->clear();
    mPanel->setEventHandler(this);
  }

  HSQUIRRELVM v = sq_open(1280);
  sqstd_seterrorhandlers(v);
  sq_setprintfunc(v, script_print_func, script_err_print_func);

  // Bind std libs to root table for compatibility only, would better use modules
  sq_pushroottable(v);
  sqstd_register_iolib(v);
  sqstd_register_mathlib(v);
  sqstd_register_stringlib(v);
  sq_pop(v, 1);

  moduleMgr = new SqModules(v, &sq_modules_dagor_file_access);
  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerIoStreamLib();
  moduleMgr->registerIoLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();

  bindquirrel::register_reg_exp(moduleMgr);
  bindquirrel::register_utf8(moduleMgr);

  bindquirrel::sqrat_bind_dagor_math(moduleMgr);
  bindquirrel::bind_dagor_workcycle(moduleMgr, true, "scriptPanelWrapper");
  bindquirrel::bind_dagor_time(moduleMgr);
  bindquirrel::register_iso8601_time(moduleMgr);
  bindquirrel::register_platform_module(moduleMgr);

  bindquirrel::register_dagor_fs_module(moduleMgr);
  bindquirrel::register_dagor_clipboard(moduleMgr);
  bindquirrel::sqrat_bind_datablock(moduleMgr);
}


int CSQPanelWrapper::getPidByName(const char *name) { return mPanelContainer->findPidByName(name); }

void CSQPanelWrapper::sendMessage(int pid, int msg, void *param) { mPanelContainer->sendMessage(pid, msg, param); }

CSQPanelWrapper::~CSQPanelWrapper()
{
  if (mPanel)
    mPanel->setEventHandler(NULL);
  if (mPanelContainer)
  {
    delete mPanelContainer;
    mPanelContainer = NULL;
  }
  if (mPanel)
    mPanel->clear();

  HSQUIRRELVM vm = moduleMgr->getVM();
  delete moduleMgr;
  sq_close(vm);
}


bool CSQPanelWrapper::bindScript(const char script_fn[])
{
  setDataBlock(NULL);
  if (mPanel)
    mPanel->clear();
  mPostEventCounter = 0;

  mScriptFilename = script_fn;
  G_ASSERT(::dd_file_exist(mScriptFilename) && "CSQPanelWrapper: no script found!");
  setFile(mScriptFilename);

  if (mPanelContainer)
    delete mPanelContainer;

  mPanelContainer = new ScriptPanelContainer(this, NULL, "", "");

  // generate, build and run init script
  {
    String initScript = prepareVariables() + defScript;
    Sqrat::Table bindingsTbl(moduleMgr->getVM());
    HSQOBJECT bindings = bindingsTbl;
    moduleMgr->bindBaseLib(bindings);
    moduleMgr->bindRequireApi(bindings);

    bool initScriptSucceeded = false;
    HSQUIRRELVM vm = moduleMgr->getVM();
    if (SQ_FAILED(sq_compile(vm, initScript.str(), initScript.length(), "<init_script>", true, &bindings)))
      logerr("Panel init script compilation failed");
    else
    {
      sq_pushroottable(vm); // for compatibility, would better use null here
      if (SQ_SUCCEEDED(sq_call(vm, 1, false, true)))
        initScriptSucceeded = true;
      else
        logerr("Panel script failed to execute");
      sq_pop(vm, 1);
    }

    if (!initScriptSucceeded)
      return false;
  }

  Sqrat::Object exports;
  Sqrat::string errMsg;
  if (!moduleMgr->requireModule(mScriptFilename, true, mScriptFilename, exports, errMsg))
  {
    logerr("Panel script error: %s [in %s]", errMsg.c_str(), script_fn);
    return false;
  }

  Sqrat::RootTable root(moduleMgr->getVM());
  Sqrat::Object root_item = root.GetSlot("scheme");

  if (mPanelContainer && root_item.GetType() == OT_TABLE)
  {
    mPid = START_PID;
    Sqrat::Object controls = root_item.GetSlot("controls");
    mPanelContainer->fillParams(mPanel, mPid, controls);
    mPanelContainer->callChangeScript(false);
    setScriptUpdateFlag();
    return true;
  }

  logerr("missing ::%s in %s (or bad mPanelContainer=%p)", "scheme", script_fn, mPanelContainer);
  return false;
}


HSQUIRRELVM CSQPanelWrapper::getScriptVm() const { return moduleMgr->getVM(); }


void CSQPanelWrapper::addGlobalConst(const char *_name, const char *_value, bool is_str)
{
  globalConsts.push_back(GlobalConst());
  GlobalConst &pvar = globalConsts.back();
  pvar.name = _name;
  pvar.value = _value;
  pvar.isStr = is_str;
}


String CSQPanelWrapper::buildConstScriptCode(const CSQPanelWrapper::GlobalConst &gconst)
{
  String buffer(512, gconst.isStr ? "global const %s = \"%s\"" : "global const %s = %s", gconst.name.str(), gconst.value.str());
  return buffer;
}


String CSQPanelWrapper::prepareVariables()
{
  String buffer("\r\n");

  for (int i = 0; i < globalConsts.size(); ++i)
    buffer += buildConstScriptCode(globalConsts[i]) + "\r\n";

  return buffer;
}


void CSQPanelWrapper::UpdateFile()
{
  if (mDataBlock)
    mPanelContainer->save(*mDataBlock);

  DataBlock *blk = mDataBlock;
  bindScript(mScriptFilename);
  setDataBlock(blk);
}


bool CSQPanelWrapper::setDataBlock(DataBlock *blk)
{
  mDataBlock = blk;
  if (mDataBlock && mPanelContainer)
  {
    mPanelContainer->load(*mDataBlock);
    mPanelContainer->callChangeScript(false);
    setScriptUpdateFlag();
    mPanelContainer->save(*mDataBlock);
    return true;
  }

  return false;
}


void CSQPanelWrapper::setScriptUpdateFlag()
{
  if (mPanel)
  {
    mPanel->setPostEvent(START_PID);
    ++mPostEventCounter;
  }
  else
  {
    mPanelContainer->updateParams();
    if (mHandler)
      mHandler->onScriptPanelChange();
  }
}


void CSQPanelWrapper::updateDataBlock()
{
  if (mDataBlock && mPanelContainer)
    mPanelContainer->save(*mDataBlock);
}


void CSQPanelWrapper::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (mPanelContainer)
    mPanelContainer->onChange(pcb_id, *panel);

  if (mPostEventCounter == 0 && mHandler)
    mHandler->onScriptPanelChange();
}


long CSQPanelWrapper::onChanging(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  long result = 0;
  if (mPanelContainer)
    result = mPanelContainer->onChanging(pcb_id, *panel);
  return result;
}


void CSQPanelWrapper::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (mPanelContainer)
    mPanelContainer->onClick(pcb_id, *panel);

  if (mPostEventCounter == 0 && mHandler)
    mHandler->onScriptPanelChange();
}


void CSQPanelWrapper::onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == START_PID)
  {
    mPanelContainer->updateParams();
    --mPostEventCounter;
  }
  else if (mPanelContainer && panel)
    mPanelContainer->onPostEvent(pcb_id, *panel);

  if (mHandler)
    mHandler->onScriptPanelChange();
}


void CSQPanelWrapper::validateTargets()
{
  mPanelContainer->validate();

  if (mHandler)
    mHandler->onScriptPanelChange();
}

void CSQPanelWrapper::getTargetNames(Tab<SimpleString> &list)
{
  clear_and_shrink(list);
  mPanelContainer->getTargetList(list);
}
