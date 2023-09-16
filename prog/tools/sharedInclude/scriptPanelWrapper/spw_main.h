//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <propPanel2/c_control_event_handler.h>
#include <winGuiWrapper/wgw_file_update.h>

#include "spw_interface.h"

class SquirrelObject;
class DataBlock;
class ScriptPanelContainer;
struct SquirrelVMSys;
class SqModules;

enum
{
  MSG_SET_TEXT,
  MSG_APPEND_ITEM,
};


class CSQPanelWrapper : public ControlEventHandler, public FileUpdateCallback
{
public:
  CSQPanelWrapper(PropPanel2 *panel);
  virtual ~CSQPanelWrapper();

  bool bindScript(const char script[]);
  bool setDataBlock(DataBlock *blk); // call it AFTER bindScript
  void updateDataBlock();

  void setObjectCB(IScriptPanelTargetCB *object_cb) { objCB = object_cb; }
  IScriptPanelTargetCB *getObjectCB() { return objCB; }
  void validateTargets();
  void getTargetNames(Tab<SimpleString> &list);

  void setEventHandler(IScriptPanelEventHandler *handler) { mHandler = handler; }

  int &getFreePid() { return mPid; }
  void setScriptUpdateFlag();

  // variables interface
  static void addGlobalConst(const char *_name, const char *_value, bool is_str = true); // call it BEFORE bindScript
  int getPidByName(const char *name);
  void sendMessage(int pid, int msg, void *arg);

  void setSystemVM();
  void setCurrentVM();

protected:
  void init();

  // ControlEventHandler
  virtual long onChanging(int pcb_id, PropPanel2 *panel);
  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);
  virtual void onPostEvent(int pcb_id, PropPanel2 *panel);

  // FileUpdateCallback
  virtual void UpdateFile();

  SquirrelVMSys *createCleanVM();

private:
  PropPanel2 *mPanel;
  SimpleString mScriptFilename;
  DataBlock *mDataBlock;
  ScriptPanelContainer *mPanelContainer;
  IScriptPanelTargetCB *objCB;
  int mPid, mPostEventCounter;

  SquirrelVMSys *sysVM, *curVM;
  IScriptPanelEventHandler *mHandler;
  SqModules *moduleMgr = nullptr;

  struct GlobalConst
  {
    GlobalConst()
    {
      name = "";
      value = "";
      isStr = true;
    }

    String name, value;
    bool isStr;
  };

  String buildConstScriptCode(const GlobalConst &);
  String prepareVariables();

  static Tab<GlobalConst> globalConsts;
};
