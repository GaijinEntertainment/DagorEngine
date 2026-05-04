//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <propPanel/c_control_event_handler.h>
#include <winGuiWrapper/wgw_file_update.h>

#include "spw_interface.h"

class DataBlock;
class ScriptPanelContainer;
class SqModules;
typedef struct SQVM *HSQUIRRELVM;

enum
{
  MSG_SET_TEXT,
  MSG_APPEND_ITEM,
};


class CSQPanelWrapper : public PropPanel::ControlEventHandler, public FileUpdateCallback
{
public:
  CSQPanelWrapper(PropPanel::ContainerPropertyControl *panel);
  ~CSQPanelWrapper() override;

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

  SqModules *getModuleMgr() const { return moduleMgr; }
  HSQUIRRELVM getScriptVm() const;

protected:
  void init();

  // ControlEventHandler
  long onChanging(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // FileUpdateCallback
  void UpdateFile() override;

private:
  PropPanel::ContainerPropertyControl *mPanel;
  SimpleString mScriptFilename;
  DataBlock *mDataBlock;
  ScriptPanelContainer *mPanelContainer;
  IScriptPanelTargetCB *objCB;
  int mPid, mPostEventCounter;

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
