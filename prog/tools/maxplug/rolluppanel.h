// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include "datablk.h"

class IRollupWindow;
class DataBlock;

enum class SchemeType
{
  Bool,
  Int,
  Real,
  String,
  Point3,
  Combo,
};

class RollupPanel
{
public:
  RollupPanel(Interface *ip, const HWND dlg_hwnd);
  ~RollupPanel();
  void onPPChange(const char *group, SchemeType type, const char *name);
  void fillPanel();

  static void correctUserProp(INode *n);

  static DataBlock &getTemplateBlk();

private:
  friend class FillCB;
  friend class SyncMaxParams;
  friend class SyncPanelParams;

  Interface *ip;
  static RollupPanel *instance;
  ControlPtr<IRollupWindow> iRoll;
  static std::unique_ptr<DataBlock> templateBlk;

  void addButtons(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable,
    const std::vector<std::string> &items);
  void addIntInput(const HWND group_hwnd, int idc, const char *name, int val, bool enable);
  void addRealInput(const HWND group_hwnd, int idc, const char *name, real val, bool enable);
  void addStrInput(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable);
  void addCheck(const HWND group_hwnd, int idc, const char *name, bool val, bool enable);
  void setNotCommon(const char *group, const char *name, bool nc);
  bool getCheck(const char *group, const char *name);
  std::string getInput(const char *group, const char *name);
  std::string getCombo(const char *group, const char *name);
  Point3 getPoint3Input(const char *group, const char *name);
  real getRealInput(const char *group, const char *name);
  void bindCommand(INode *n, const char *name, const DataBlock &blk);

  void fillFromBlk(const DataBlock &blk, bool enable);

  struct UserProp
  {
    DataBlock blk{std::make_shared<NameMap>()};
    std::string blkText;
    std::string nonBlkText;
    bool cfgMigrated = false;
  };

  static bool loadUserProp(INode *n, UserProp &prop);
  static void saveCorrectedUserProp(INode *n, const UserProp &prop);
  static void saveBlkToUserPropBuffer(const DataBlock &blk, INode *n, std::string_view additional = {});

  void saveToUserPropBuffer(INode *n, std::string_view additional = {});
  bool updateNCFromUserPropBuffer(INode *n, UserProp &prop);
  bool updateFromUserPropBuffer(INode *n, UserProp &prop);

  void bindCommands(INode *n, DataBlock &blk);
  HWND addGroup(IRollupWindow *roll, int count, const char *name);
  void updateNCFromBlk(const DataBlock &blk);
  void updateFromBlk(const DataBlock &blk);

  static BOOL CALLBACK generalRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

const char *find_info_by_name(const char *info, const char *name, const char *def);
const char *find_name_by_info(const char *info, const char *command);
