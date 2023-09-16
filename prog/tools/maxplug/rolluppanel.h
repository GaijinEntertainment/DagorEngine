// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __MAXPLUG_ROLLUP_PANEL_H
#define __MAXPLUG_ROLLUP_PANEL_H
#pragma once

#include "datablk.h"

class IRollupWindow;
class DataBlock;

class RollupPanel
{
public:
  RollupPanel(Interface *ip, const HWND dlg_hwnd);
  ~RollupPanel();
  void onPPChange(const char *group, const char *type, const char *name);
  void fillPanel();

  static bool saveUserPropBufferToBlk(DataBlock &blk, INode *n, int &blk_param_count);
  static void correctUserProp(INode *n);
  static void getBlkFromUserProp(INode *n, CStr &blk_string, CStr &non_blk_str);

  static DataBlock &getTemplateBlk();

private:
  friend class FillCB;
  friend class SyncMaxParams;
  friend class SyncPanelParams;

  Interface *ip;
  static RollupPanel *instance;
  IRollupWindow *iRoll;
  static DataBlock *templateBlk;

  void addButtons(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable, Tab<String> &items);
  void addComboInput(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable, Tab<String> &items);
  void addIntInput(const HWND group_hwnd, int idc, const char *name, int val, bool enable);
  void addRealInput(const HWND group_hwnd, int idc, const char *name, real val, bool enable);
  void addStrInput(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable);
  void addCheck(const HWND group_hwnd, int idc, const char *name, bool val, bool enable);
  void setNotCommon(const char *group, const char *name, bool nc);
  bool getCheck(const char *group, const char *name);
  void getInput(const char *group, const char *name, char *val);
  void getCombo(const char *group, const char *name, char *val);
  Point3 getPoint3Input(const char *group, const char *name);
  real getRealInput(const char *group, const char *name);
  void bindCommand(INode *n, const char *name, const DataBlock &blk);

  void fillFromBlk(const DataBlock &blk, bool enable);

  static bool saveBlkToUserPropBuffer(const DataBlock &blk, INode *n, const char *additional = NULL);
  static void getMaxBlkFileName(const char *blk, char *file_name);
  static bool getBlkInString(const DataBlock &blk, CStr &out);

  void saveToUserPropBuffer(INode *n, const char *additional = NULL);
  bool updateNCFromUserPropBuffer(INode *n, int &blk_param_count);
  bool updateFromUserPropBuffer(INode *n, int &blk_param_count);

  void bindCommands(INode *n, DataBlock &blk);
  HWND addGroup(IRollupWindow *roll, int count, const char *name);
  void updateNCFromBlk(const DataBlock &blk);
  void updateFromBlk(const DataBlock &blk);

  static void analyzeCfg(DataBlock &blk, CStr &source);

  static bool loadStrFromFile(const char *fname, CStr &str);

  static BOOL CALLBACK generalRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

const char *find_info_by_name(const char *info, const char *name, const char *def);
const char *find_name_by_info(const char *info, const char *command);

#endif
