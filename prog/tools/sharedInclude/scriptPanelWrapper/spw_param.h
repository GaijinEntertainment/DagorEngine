//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>
#include <sqplus.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>

class DataBlock;
class CSQPanelWrapper;
class ScriptPanelTargetCB;
class ScriptPanelContainer;

namespace SqPlus
{
template <typename RT>
struct SquirrelFunction;
}

class ScriptPanelParam
{
public:
  ScriptPanelParam(ScriptPanelContainer *parent, const char *name, const char *caption) :
    mParent(parent),
    paramName(name),
    mCaption(caption),
    mPid(-1),
    mPanel(NULL),
    mVisible(false),
    mEnabled(true),
    onChangeCallback(NULL)
  {}

  virtual ~ScriptPanelParam();

  void fillParameter(PropPanel2 *panel, int &pid, SquirrelObject param);
  void getFromScript();

  virtual void setToScript(SquirrelObject &param);
  virtual void callChangeScript(bool script_update_flag);
  virtual void updateParams() {}
  virtual void validate() {}
  virtual void getTargetList(Tab<SimpleString> &list) {}

  virtual long onChanging(int pid, PropPanel2 &panel) { return 0; }
  virtual void onChange(int pid, PropPanel2 &panel) = 0;
  virtual void onClick(int pid, PropPanel2 &panel) {}
  virtual void onPostEvent(int pcb_id, PropPanel2 &panel) {}

  virtual void save(DataBlock &blk) = 0;
  virtual void load(const DataBlock &blk) = 0;

  int getPid() { return mPid; }
  virtual int getFirstPid() { return getPid(); } // for ext controls
  bool getVisible() { return mVisible; }
  void setParamName(const char *name) { paramName = name; }
  const char *getParamName() { return paramName.str(); }

  virtual void sendMessage(int pid, int message, void *arg) {}
  virtual bool onMessage(int pid, int message, void *arg) { return false; }

  void setTooltip(const char *text);

protected:
  virtual void createControl() {}
  virtual void getValueFromScript(SquirrelObject param);

  virtual void setupParameter(PropPanel2 *panel, int &pid);
  virtual void removeControl();
  virtual void setVisible(bool visible);
  virtual void setEnabled(bool enabled);

  SimpleString paramName, mCaption;
  int mPid, mPriorPid;
  bool mVisible, mEnabled;
  PropPanel2 *mPanel;
  ScriptPanelContainer *mParent;
  SqPlus::SquirrelFunction<void> *onChangeCallback;
  SquirrelObject mParam;
};


class ScriptPanelContainer : public ScriptPanelParam
{
public:
  ScriptPanelContainer(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption);
  virtual ~ScriptPanelContainer();

  virtual void fillParams(PropPanel2 *panel, int &pid, SquirrelObject param);
  virtual void callChangeScript(bool script_update_flag);
  virtual void updateParams();
  virtual void validate();
  virtual void getTargetList(Tab<SimpleString> &list);

  virtual long onChanging(int pid, PropPanel2 &panel);
  virtual void onChange(int pid, PropPanel2 &panel);
  virtual void onClick(int pid, PropPanel2 &panel);
  virtual void onPostEvent(int pid, PropPanel2 &panel);

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);

  virtual void clear();
  int getNextVisiblePid(int pid);
  CSQPanelWrapper *getWrapper() { return mPanelWrapper; }

  const char *getParamType(SquirrelObject param);
  virtual bool getParamVisible(SquirrelObject param);

  int findPidByName(const char *name);
  virtual void sendMessage(int pid, int message, void *arg);

protected:
  void scriptControlFactory(PropPanel2 *panel, int &pid, SquirrelObject param);
  virtual bool scriptExtFactory(PropPanel2 *panel, int &pid, SquirrelObject param);

  const char *getParamName(SquirrelObject param);
  const char *getParamCaption(SquirrelObject param, const char *name);
  const char *getTooltipLocalizationKey(SquirrelObject param);
  const char *getDefaultTooltip(SquirrelObject param);

  virtual void removeControl();

  Tab<ScriptPanelParam *> mItems;
  CSQPanelWrapper *mPanelWrapper;
};
