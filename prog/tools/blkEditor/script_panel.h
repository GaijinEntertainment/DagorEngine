// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <scriptPanelWrapper/spw_interface.h>
#include <propPanel/control/container.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_timer.h>

class CSQPanelWrapper;

class BlkHandler
{
public:
  virtual void onBlkChange() = 0;
  virtual PropPanel::ContainerPropertyControl *getBlkPanel() = 0;
};

class ScriptPanelEditor : public IScriptPanelEventHandler, public IScriptPanelTargetCB, public ITimerCallBack
{
public:
  ScriptPanelEditor(BlkHandler *handler);
  ~ScriptPanelEditor() override;

  void createPanel();
  void destroyPanel();

  const char *getBlkText();
  void setBlkText(const char *text, int size);

  void load(const char *fn);
  void save(const char *fn);

  const char *getScriptsDir() { return scriptsDir.str(); }
  const char *getScriptPath() { return scriptPath.str(); }
  bool hasScript();
  void setScriptPath(const char *path);

private:
  // IScriptPanelEventHandler
  void onScriptPanelChange() override;

  // IScriptPanelTargetCB
  const char *getTarget(const char *old_choise, const char *type, const char filter[]) override;
  const char *validateTarget(const char *name, const char *type) override;

  // ITimerCallBack
  void update() override;

  const char *getScriptFn();
  void updateScript();
  void updateFromDataBlock();

  const char *selectText(const char *old_choise);

  DataBlock propsBlk;
  CSQPanelWrapper *scriptPanelWrap;
  String scriptsDir, scriptPath;
  BlkHandler *blkHandler;
  int manualSetCnt;
  WinTimer *updateTimer;
  bool needCallChange;
};
