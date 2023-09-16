// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_panel_base.h>

#include <scriptPanelWrapper/spw_main.h>
#include <scriptPanelWrapper/spw_param.h>

#include <sqplus.h>
#include <sqstdio.h>
#include <osApiWrappers/dag_direct.h>

#include <debug/dag_debug.h>
#include <sqModules/sqModules.h>
#include <bindQuirrelEx/bindQuirrelEx.h>

#include "spw_script.inc.cpp"

enum
{
  START_PID = 100,
};


Tab<CSQPanelWrapper::GlobalConst> CSQPanelWrapper::globalConsts(midmem);


CSQPanelWrapper::CSQPanelWrapper(PropPanel2 *panel) :
  mPanel(panel), mScriptFilename(""), mDataBlock(NULL), mPanelContainer(NULL), objCB(NULL), mHandler(NULL), mPostEventCounter(0)
{
  sysVM = createCleanVM();
  curVM = createCleanVM();
  init();
}

void CSQPanelWrapper::init()
{
  if (mPanel)
  {
    mPanel->clear();
    mPanel->setEventHandler(this);
  }

  if (SquirrelVM::IsInitialized())
    SquirrelVM::GetVMSys(*sysVM);

  SquirrelVM::SetVMSys(*curVM);
  SquirrelVM::Init();

  setSystemVM();
  if (HSQUIRRELVM v = SquirrelVM::GetVMPtr())
  {
    // TODO: is this is foreign VM owned by another code, we
    // probably don't want to modify it!
    sq_pushroottable(v);
    sqstd_register_iolib(v);
    sq_pop(v, 1);
  }

  setCurrentVM();
  if (HSQUIRRELVM v = SquirrelVM::GetVMPtr())
  {
    sq_pushroottable(v);
    sqstd_register_iolib(v);
    sq_pop(v, 1);

    moduleMgr = new SqModules(v);
    moduleMgr->registerMathLib();
    moduleMgr->registerStringLib();
    moduleMgr->registerIoStreamLib();
    moduleMgr->registerIoLib();
    moduleMgr->registerSystemLib();
    moduleMgr->registerDateTimeLib();

    bindquirrel::register_reg_exp(moduleMgr);
    bindquirrel::register_utf8(moduleMgr);

    bindquirrel::sqrat_bind_dagor_math(moduleMgr);
    bindquirrel::bind_dagor_workcycle(moduleMgr, true);
    bindquirrel::bind_dagor_time(moduleMgr);
    bindquirrel::register_iso8601_time(moduleMgr);
    bindquirrel::register_platform_module(moduleMgr);

    bindquirrel::register_dagor_fs_module(moduleMgr);
    bindquirrel::register_dagor_clipboard(moduleMgr);
    bindquirrel::sqrat_bind_datablock(moduleMgr);
  }

  setSystemVM();
}


int CSQPanelWrapper::getPidByName(const char *name) { return mPanelContainer->findPidByName(name); }

void CSQPanelWrapper::sendMessage(int pid, int msg, void *param)
{
  setCurrentVM();
  mPanelContainer->sendMessage(pid, msg, param);
  setSystemVM();
}

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

  setCurrentVM();
  delete moduleMgr;
  SquirrelVM::Shutdown();
  SquirrelVM::SetVMSys(*sysVM);
  delete sysVM;
  delete curVM;
}


SquirrelVMSys *CSQPanelWrapper::createCleanVM()
{
  SquirrelVMSys *vm = new SquirrelVMSys();
  vm->_VM = NULL;
  vm->_root = NULL;
  vm->_nativeClassesTable = NULL;
  return vm;
}


void CSQPanelWrapper::setSystemVM()
{
  SquirrelVM::GetVMSys(*curVM);
  SquirrelVM::SetVMSys(*sysVM);
}


void CSQPanelWrapper::setCurrentVM()
{
  SquirrelVM::GetVMSys(*sysVM);
  SquirrelVM::SetVMSys(*curVM);
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
  setCurrentVM();
  SquirrelObject sqInst, ret;

  try
  {
    String initScript = prepareVariables() + defScript;
    Sqrat::Table bindingsTbl(moduleMgr->getVM());
    HSQOBJECT bindings = bindingsTbl;
    moduleMgr->bindBaseLib(bindings);
    moduleMgr->bindRequireApi(bindings);
    if (!SquirrelVM::CompileBuffer(initScript.str(), sqInst, &bindings))
      throw SquirrelError();
    if (!SquirrelVM::RunScript(sqInst, ret))
      throw SquirrelError();
  }
  catch (SquirrelError e)
  {
    logerr("Panel script error: %s [in %s]", e.desc, script_fn);
    setSystemVM();
    return false;
  }

  Sqrat::Object exports;
  String errMsg;
  if (!moduleMgr->requireModule(mScriptFilename, true, mScriptFilename, exports, errMsg))
  {
    logerr("Panel script error: %s [in %s]", errMsg.c_str(), script_fn);
    setSystemVM();
    return false;
  }

  SquirrelObject root = SquirrelVM::GetRootTable();
  SquirrelObject root_item = (root.Exists("scheme")) ? root.GetValue("scheme") : SquirrelObject();

  if (mPanelContainer && root_item.GetType() == OT_TABLE)
  {
    mPid = START_PID;
    SquirrelObject controls = root_item.GetValue("controls");
    mPanelContainer->fillParams(mPanel, mPid, controls);
    mPanelContainer->callChangeScript(false);
    setScriptUpdateFlag();
    setSystemVM();
    return true;
  }

  logerr("missing ::%s in %s (or bad mPanelContainer=%p)", "scheme", script_fn, mPanelContainer);
  setSystemVM();
  return false;
}


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
    setCurrentVM();
    mPanelContainer->load(*mDataBlock);
    mPanelContainer->callChangeScript(false);
    setScriptUpdateFlag();
    mPanelContainer->save(*mDataBlock);
    setSystemVM();
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
  {
    setCurrentVM();
    mPanelContainer->save(*mDataBlock);
    setSystemVM();
  }
}


void CSQPanelWrapper::onChange(int pcb_id, PropPanel2 *panel)
{
  if (mPanelContainer)
  {
    setCurrentVM();
    mPanelContainer->onChange(pcb_id, *panel);
    setSystemVM();
  }

  if (mPostEventCounter == 0 && mHandler)
    mHandler->onScriptPanelChange();
}


long CSQPanelWrapper::onChanging(int pcb_id, PropPanel2 *panel)
{
  long result = 0;
  if (mPanelContainer)
  {
    setCurrentVM();
    result = mPanelContainer->onChanging(pcb_id, *panel);
    setSystemVM();
  }
  return result;
}


void CSQPanelWrapper::onClick(int pcb_id, PropPanel2 *panel)
{
  if (mPanelContainer)
  {
    setCurrentVM();
    mPanelContainer->onClick(pcb_id, *panel);
    setSystemVM();
  }

  if (mPostEventCounter == 0 && mHandler)
    mHandler->onScriptPanelChange();
}


void CSQPanelWrapper::onPostEvent(int pcb_id, PropPanel2 *panel)
{
  setCurrentVM();
  if (pcb_id == START_PID)
  {
    mPanelContainer->updateParams();
    --mPostEventCounter;
  }
  else if (mPanelContainer && panel)
    mPanelContainer->onPostEvent(pcb_id, *panel);
  setSystemVM();

  if (mHandler)
    mHandler->onScriptPanelChange();
}


void CSQPanelWrapper::validateTargets()
{
  setCurrentVM();
  mPanelContainer->validate();
  setSystemVM();

  if (mHandler)
    mHandler->onScriptPanelChange();
}

void CSQPanelWrapper::getTargetNames(Tab<SimpleString> &list)
{
  clear_and_shrink(list);
  mPanelContainer->getTargetList(list);
}
