// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <windows.h>

class PropertyDialog
{
public:
  void fillFromBlk(const DataBlock &blk, HWND &hWnd, Tab<const char *> &area_names);
  void setToBlk(const DataBlock &blk, HWND &hWnd, DataBlock &res_blk);
  void setToSquirrel(const DataBlock &blk, HWND &hWnd, String &script, bool in_line);
  static int getItemH();
  friend class FillCB;

private:
  void addComboInput(const HWND &group_hwnd, int idc, const char *name, const char *val, bool enable, Tab<String> &items);
  void addIntInput(const HWND &group_hwnd, int idc, const char *name, int val, bool enable);
  void addRealInput(const HWND &group_hwnd, int idc, const char *name, real val, bool enable);
  void addStrInput(const HWND &group_hwnd, int idc, const char *name, const char *val, bool enable);
  void addCheck(const HWND &group_hwnd, int idc, const char *name, bool val, bool enable);
};
