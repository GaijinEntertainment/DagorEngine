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

  void updateParams() override;

  void onChange(int pid, PropPanel::ContainerPropertyControl &panel) override;
  void onClick(int pid, PropPanel::ContainerPropertyControl &panel) override;
  void onPostEvent(int pid, PropPanel::ContainerPropertyControl &panel) override;

  void save(DataBlock &blk) override;
  void load(const DataBlock &blk) override;

  void clear() override;
  int getFirstPid() override;
  bool getParamVisible(SquirrelObject param) override;

  bool onMessage(int pid, int msg, void *arg) override;

protected:
  void createControl() override;
  void getValueFromScript(SquirrelObject param) override;
  void setVisible(bool visible) override;
  void setEnabled(bool enabled) override;
  void saveExt(DataBlock &blk);

  bool scriptExtFactory(PropPanel::ContainerPropertyControl *panel, int &pid, SquirrelObject param) override;
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
  void getValueFromScript(SquirrelObject param) override;
  void appendExBlock() override;
  bool insertExBlock(int pid, int ind = -1) override;
  int getItemPid(int pid) override { return pid; }
};
