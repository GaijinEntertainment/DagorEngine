//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>

class ScriptExtContainer : public ScriptPanelContainer
{
public:
  ScriptExtContainer(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption);

  virtual void updateParams();

  virtual void onChange(int pid, PropPanel::ContainerPropertyControl &panel);
  virtual void onClick(int pid, PropPanel::ContainerPropertyControl &panel);
  virtual void onPostEvent(int pid, PropPanel::ContainerPropertyControl &panel);

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);

  virtual void clear();
  virtual int getFirstPid();
  virtual bool getParamVisible(SquirrelObject param);

  virtual bool onMessage(int pid, int msg, void *arg);

protected:
  virtual void createControl();
  virtual void getValueFromScript(SquirrelObject param);
  virtual void setVisible(bool visible);
  virtual void setEnabled(bool enabled);
  void saveExt(DataBlock &blk);

  bool scriptExtFactory(PropPanel::ContainerPropertyControl *panel, int &pid, SquirrelObject param);
  int searchPidIndex(int pid);
  void setDefValues(SquirrelObject so);

  virtual void appendExBlock();
  virtual bool insertExBlock(int pid, int ind = -1);
  virtual int getItemPid(int pid) { return pid + 1; }
  void removeExBlock(int pid, int ind);
  void moveBlockExUp(int pid, int ind);
  void moveBlockExDown(int pid, int ind);
  void updateMenuFlags();

  Tab<int> mExtItemPids;
  int itemsCountMin, itemsCountMax;
  bool removeConfirm;
  DataBlock invisibleBlk;
  bool isRenameble;
};


class ScriptExtGroup : public ScriptExtContainer
{
public:
  ScriptExtGroup(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption);

protected:
  virtual void getValueFromScript(SquirrelObject param);
  virtual void appendExBlock();
  virtual bool insertExBlock(int pid, int ind = -1);
  virtual int getItemPid(int pid) { return pid; }
};
