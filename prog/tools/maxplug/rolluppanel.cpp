// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rolluppanel.h"
#include "datablk.h"
#include "resource.h"
#include "enumnode.h"
#include "cfg.h"
#include "dlg_dim.h"

#include <stdio.h>
#include <io.h>
#include <stdlib.h>

#include "mater.h"


M_STD_STRING strToWide(const char *sz);
std::string wideToStr(const TCHAR *sw);


#define GENERAL_GROUP_NAME "[general]"

enum
{
  PARAM_EDITS_COUNT = 100,
  PARAM_LABEL_IDC = 0,
  PARAM_EDIT_IDC,
  PARAM_SPIN_IDC = PARAM_EDIT_IDC + PARAM_EDITS_COUNT,
  PARAM_NC_IDC,
  PARAM_IDC_COUNT,


  PARAM_CTRL_LEFT = 10,
  PARAM_CTRL_GAP = 4,
  PARAM_CTRL_H = 18,
  PARAM_CTRL_W = OBJ_PROP_ROLLUP_WIDTH - PARAM_CTRL_LEFT * 8,
  PARAM_CTRL_W2 = PARAM_CTRL_W * 3 / 5,
  PARAM_CTRL_CAPTION_W = PARAM_CTRL_W - PARAM_CTRL_LEFT,
  PARAM_CTRL_LEFT1 = PARAM_CTRL_LEFT + PARAM_CTRL_GAP + PARAM_CTRL_CAPTION_W,
  PARAM_CTRL_LEFT2 = PARAM_CTRL_LEFT1 + PARAM_CTRL_W,
  PARAM_CTRL_LEFT3 = PARAM_CTRL_LEFT2 + PARAM_CTRL_H,
  PARAM_CTRL_LEFT4 = PARAM_CTRL_LEFT1 + PARAM_CTRL_W2,
};

//////////////////////////////////////////////////////////////////////////////
// callBacks
//////////////////////////////////////////////////////////////////////////////
class EDataBlockCB
{
public:
  virtual ~EDataBlockCB() {}
  virtual int procCheck(bool val) = 0;
  virtual int procInt(int val) = 0;
  virtual int procCombo(const char *val, Tab<String> &items) = 0;
  virtual int procReal(real val) = 0;
  virtual int procStr(const char *val) = 0;
  virtual int procPoint3(const Point3 &val) = 0;
  int groupId;
  int paramId;
  char *name;
};

int enum_params(const DataBlock *blk, EDataBlockCB *cb);
bool find_param(const char *group, const char *name, int &group_id, int &param_id)
{
  const DataBlock *blk = &RollupPanel::getTemplateBlk();
  group_id = -1;
  param_id = -1;
  int paramId = 0;
  DataBlock *groupBlk = blk->getBlockByName(group);
  if (!groupBlk)
    return false;
  for (int i = 0; i < blk->blockCount(); i++)
  {
    if (groupBlk == blk->getBlock(i))
      for (int j = 0; j < groupBlk->blockCount(); j++)
      {
        DataBlock *paramBlk = groupBlk->getBlock(j);
        if (strcmp(paramBlk->getBlockName(), "parameter") == NULL)
        {
          if (strcmp(paramBlk->getParamName(1), name) == NULL)
          {
            group_id = i;
            param_id = paramId;
            return true;
          }
          if (strcmp(paramBlk->getStr(0), "p3") == NULL)
            paramId += 2;
          paramId++;
        }
      }
  }
  return false;
}
bool find_param(int group_id, int param_id, char *group, char *name, char *type)
{
  const DataBlock *blk = &RollupPanel::getTemplateBlk();
  int paramId = 0;
  DataBlock *groupBlk = blk->getBlock(group_id);
  if (!groupBlk)
    return false;

  for (int j = 0; j < groupBlk->blockCount(); j++)
  {
    DataBlock *paramBlk = groupBlk->getBlock(j);
    if (strcmp(paramBlk->getBlockName(), "parameter") == NULL)
    {
      if (strcmp(paramBlk->getStr(0), "p3") == NULL)
      {
        for (int i = 0; i < 3; i++)
        {
          if (paramId + i == param_id)
          {
            strcpy(group, groupBlk->getBlockName());
            strcpy(name, paramBlk->getParamName(1));
            strcpy(type, paramBlk->getStr(0));
            return true;
          }
        }
        paramId += 3;
        continue;
      }
      if (paramId == param_id)
      {
        strcpy(group, groupBlk->getBlockName());
        strcpy(name, paramBlk->getParamName(1));
        strcpy(type, paramBlk->getStr(0));
        return true;
      }
      paramId++;
    }
  }
  return false;
}
const char *find_info_by_name(const char *info, const char *name, const char *def)
{
  const DataBlock *blk = &RollupPanel::getTemplateBlk();
  for (int i = 0; i < blk->blockCount(); i++)
  {
    DataBlock *groupBlk = blk->getBlock(i);
    for (int j = 0; j < groupBlk->blockCount(); j++)
    {
      DataBlock *paramBlk = groupBlk->getBlock(j);
      if (strcmp(paramBlk->getBlockName(), "parameter") == NULL && strcmp(paramBlk->getParamName(1), name) == NULL)
        return paramBlk->getStr(info, def);
    }
  }
  return def;
}
const char *find_name_by_info(const char *info, const char *command)
{
  const DataBlock *blk = &RollupPanel::getTemplateBlk();
  for (int i = 0; i < blk->blockCount(); i++)
  {
    DataBlock *groupBlk = blk->getBlock(i);
    for (int j = 0; j < groupBlk->blockCount(); j++)
    {
      DataBlock *paramBlk = groupBlk->getBlock(j);
      if (strcmp(paramBlk->getBlockName(), "parameter") == NULL && strcmp(paramBlk->getStr(info, ""), command) == NULL)
        return paramBlk->getParamName(1);
    }
  }
  return "";
}

class FillCB : public EDataBlockCB
{
public:
  FillCB(HWND hwnd_, RollupPanel *panel_, bool enable_) : hwnd(hwnd_), panel(panel_), enable(enable_) {}
  int procCheck(bool val) override
  {
    panel->addCheck(hwnd, paramId * PARAM_IDC_COUNT, find_info_by_name("caption", name, name), val, enable);
    return ECB_CONT;
  }
  int procCombo(const char *val, Tab<String> &items) override
  {
    // panel->addComboInput( hwnd, paramId*PARAM_IDC_COUNT, name, val, enable, items );
    panel->addButtons(hwnd, paramId * PARAM_IDC_COUNT, find_info_by_name("caption", name, name), val, enable, items);
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    panel->addIntInput(hwnd, paramId * PARAM_IDC_COUNT, find_info_by_name("caption", name, name), val, enable);
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    panel->addRealInput(hwnd, paramId * PARAM_IDC_COUNT, find_info_by_name("caption", name, name), val, enable);
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    panel->addStrInput(hwnd, paramId * PARAM_IDC_COUNT, find_info_by_name("caption", name, name), val, enable);
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    const char *xyz = "xyz";
    const char *caption = find_info_by_name("caption", name, name);
    for (int i = 0; i < 3; i++)
    {
      char buf[32];
      sprintf(buf, "%s.%c", caption, xyz[i]);
      panel->addRealInput(hwnd, (paramId + i) * PARAM_IDC_COUNT, buf, val[i], enable);
    }
    return ECB_CONT;
  }

private:
  HWND hwnd;
  RollupPanel *panel;
  bool enable;
};

void clearParam(HWND hwnd, int param_id)
{
  ::EnableWindow(GetDlgItem(hwnd, param_id * PARAM_IDC_COUNT + PARAM_LABEL_IDC), true);
  ::EnableWindow(GetDlgItem(hwnd, param_id * PARAM_IDC_COUNT + PARAM_EDIT_IDC), true);
  ::EnableWindow(GetDlgItem(hwnd, param_id * PARAM_IDC_COUNT + PARAM_SPIN_IDC), true);
  ::SetDlgItemText(hwnd, param_id * PARAM_IDC_COUNT + PARAM_NC_IDC, _T(""));
}

class UpdateCB : public EDataBlockCB
{
public:
  UpdateCB(IRollupWindow *i_roll, RollupPanel *panel_, const DataBlock *node_blk) : iRoll(i_roll), panel(panel_), nodeBlk(node_blk) {}
  int procCheck(bool val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ::CheckDlgButton(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC, nodeBlk->getBool(name, false));
    return ECB_CONT;
  }
  int procCombo(const char *val, Tab<String> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    for (int i = 0; i < items.Count(); i++)
    {
      ICustButton *iEdit = GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i));
      iEdit->SetCheck(strcmp((char *)nodeBlk->getStr(name, ""), items[i]) == NULL);
      iEdit->Enable();
      ReleaseICustButton(iEdit);
    }
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC));
    iEdit->SetText(nodeBlk->getInt(name, 0));
    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC));
    iEdit->SetText(nodeBlk->getReal(name, 0), 3);
    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC));

    char *sz = (char *)nodeBlk->getStr(name, "");

    iEdit->SetText((TCHAR *)strToWide(sz).c_str());

    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    Point3 val1 = nodeBlk->getPoint3(name, val);
    for (int i = 0; i < 3; i++)
    {
      clearParam(hwnd, paramId + i);
      ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, (paramId + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC));

      iEdit->SetText(val1[i], 3);
      ReleaseICustEdit(iEdit);
    }
    return ECB_CONT;
  }

private:
  RollupPanel *panel;
  const DataBlock *nodeBlk;
  IRollupWindow *iRoll;
};


class UpdateNCCB : public EDataBlockCB
{
public:
  UpdateNCCB(IRollupWindow *i_roll, RollupPanel *panel_, const DataBlock *node_blk) : iRoll(i_roll), panel(panel_), nodeBlk(node_blk)
  {}
  int procCheck(bool val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valEditIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    bool nc = (::IsDlgButtonChecked(hwnd, valEditIdc) ? 1 : 0) != nodeBlk->getBool(name, false);
    if (nc)
      ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    return ECB_CONT;
  }
  int procCombo(const char *val, Tab<String> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valEditIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    bool nc = false;
    for (int i = 0; i < items.Count(); i++)
    {
      ICustButton *iEdit = GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i));
      if (iEdit->IsChecked() != (strcmp((char *)nodeBlk->getStr(name, ""), items[i]) == NULL))
        nc = true;
      ReleaseICustButton(iEdit);
    }
    if (nc)
      ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valEditIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valEditIdc));
    bool nc = iEdit->GetInt() != nodeBlk->getInt(name, 0);
    if (nc)
      ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valEditIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valEditIdc));
    bool nc = iEdit->GetFloat() != nodeBlk->getReal(name, 0);
    if (nc)
      ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valEditIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valEditIdc));

    TCHAR val1_sw[MAX_PATH];
    iEdit->GetText(val1_sw, 32);
    std::string val1 = wideToStr(val1_sw);


    bool nc = strcmp(nodeBlk->getStr(name, ""), val1.c_str()) != NULL;
    if (nc)
      ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    Point3 val1 = nodeBlk->getPoint3(name, val);
    for (int i = 0; i < 3; i++)
    {
      const int valEditIdc = (paramId + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
      const int valNCIdc = (paramId + i) * PARAM_IDC_COUNT + PARAM_NC_IDC;
      ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valEditIdc));
      bool nc = iEdit->GetFloat() != val1[i];
      if (nc)
        ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    }
    return ECB_CONT;
  }

private:
  IRollupWindow *iRoll;
  RollupPanel *panel;
  const DataBlock *nodeBlk;
};

class UserPropToBlkCB : public EDataBlockCB
{
public:
  UserPropToBlkCB(INode *n_, DataBlock *node_blk) : n(n_), nodeBlk(node_blk) {}
  int procCheck(bool val) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      BOOL i;
      if (n->GetUserPropBool(old.c_str(), i))
        nodeBlk->setBool(name, i ? 1 : 0);
    }
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      int i;
      if (n->GetUserPropInt(old.c_str(), i))
        nodeBlk->setInt(name, i);
    }
    return ECB_CONT;
  }
  int procCombo(const char *val, Tab<String> &items) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      TSTR i;
      if (n->GetUserPropString(old.c_str(), i))
      {
        nodeBlk->setStr(name, wideToStr(i).c_str());
      }
    }
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      real i;
      if (n->GetUserPropFloat(old.c_str(), i))
        nodeBlk->setReal(name, i);
    }
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      TSTR i;

      if (n->GetUserPropString(old.c_str(), i))
      {
        nodeBlk->setStr(name, wideToStr(i).c_str());
      }
    }
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    M_STD_STRING old = getName(name);
    if (!old.empty())
    {
      Point3 p(0, 0, 0);
      const TCHAR *xyz = _T("XYZ");
      for (int i = 0; i < 3; i++)
      {
        TCHAR buf[32];
        _stprintf(buf, _T("%s.%c"), old.c_str(), xyz[i]);
        n->GetUserPropFloat(buf, p[i]);
      }
      if (p)
        nodeBlk->setPoint3(name, p);
    }
    return ECB_CONT;
  }

private:
  /* const char *getName(const char* name)
   {
     return find_info_by_name( "prop_name", name, "");
   }*/

  M_STD_STRING getName(const char *name)
  {
    const char *sz = find_info_by_name("prop_name", name, "");
    return strToWide(sz);
  }

  INode *n;
  DataBlock *nodeBlk;
};

class UserPropCB : public EDataBlockCB
{
public:
  UserPropCB(IRollupWindow *i_roll, DataBlock *rez_blk) : iRoll(i_roll), rezBlk(rez_blk) {}
  int procCheck(bool val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    rezBlk->addBool(name, ::IsDlgButtonChecked(hwnd, valControlIdc) ? 1 : 0);
    return ECB_CONT;
  }
  int procCombo(const char *val, Tab<String> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    for (int i = 0; i < items.Count(); i++)
    {
      ICustButton *iEdit = GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i));
      if (iEdit->IsChecked())
        rezBlk->addStr(name, items[i]);
      ReleaseICustButton(iEdit);
    }
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valControlIdc));
    rezBlk->addInt(name, iEdit->GetInt());
    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valControlIdc));
    rezBlk->addReal(name, iEdit->GetFloat());
    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valControlIdc));

    TCHAR wbuf[32];
    iEdit->GetText(wbuf, 32);

    rezBlk->addStr(name, wideToStr(wbuf).c_str());
    ReleaseICustEdit(iEdit);
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    Point3 p3;
    for (int i = 0; i < 3; i++)
    {
      const int valControlIdc = (paramId + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
      ICustEdit *iEdit = GetICustEdit(GetDlgItem(hwnd, valControlIdc));
      p3[i] = iEdit->GetFloat();
      ReleaseICustEdit(iEdit);
    }
    rezBlk->addPoint3(name, p3);
    return ECB_CONT;
  }

private:
  IRollupWindow *iRoll;
  DataBlock *rezBlk;
};

void enum_groups(EDataBlockCB *cb)
{
  for (int i = 0; i < RollupPanel::getTemplateBlk().blockCount(); i++)
  {
    DataBlock *groupBlk = RollupPanel::getTemplateBlk().getBlock(i);
    cb->groupId = i;
    enum_params(groupBlk, cb);
  }
}

int enum_params(const DataBlock *blk, EDataBlockCB *cb)
{
  if (!blk)
    return 1;
  int id = 0;
  for (int j = 0; j < blk->blockCount(); j++)
  {
    DataBlock *paramBlk = blk->getBlock(j);
    if (stricmp(paramBlk->getBlockName(), "parameter") == NULL)
    {
      const char *type = paramBlk->getStr("type", "string");
      int result = ECB_SKIP;
      cb->name = (char *)paramBlk->getParamName(1);
      cb->paramId = id;
      if (stricmp(type, "bool") == NULL)
        result = cb->procCheck(paramBlk->getBool(1));
      else if (stricmp(type, "int") == NULL)
        result = cb->procInt(paramBlk->getInt(1));
      else if (stricmp(type, "real") == NULL)
        result = cb->procReal(paramBlk->getReal(1));
      else if (stricmp(type, "string") == NULL)
        result = cb->procStr(paramBlk->getStr(1));
      else if (stricmp(type, "p3") == NULL)
      {
        result = cb->procPoint3(paramBlk->getPoint3(1));
        id += 2;
      }
      else if (stricmp(type, "combo") == NULL)
      {
        Tab<String> items;
        for (int i = 0; i < paramBlk->paramCount(); i++)
          if (stricmp(paramBlk->getParamName(i), "item") == NULL && paramBlk->getParamType(i) == DataBlock::TYPE_STRING)
          {
            String *s = new String(paramBlk->getStr(i));
            items.Append(1, s);
          }
        cb->procCombo(paramBlk->getStr(1), items);
      }
      id++;
      switch (result)
      {
        case ECB_STOP:
          return 0;
          // case CB_CONT:
      }
    }
  }
  return 1;
}

class SyncMaxParams : public ENodeCB
{
public:
  SyncMaxParams(RollupPanel *panel_, const char *group_, const char *type_, const char *name_) :
    panel(panel_), group(group_), name(name_), type(type_)
  {}
  ~SyncMaxParams() override {}
  int proc(INode *n) override
  {
    if (n->Selected())
    {
      CStr blkStr;
      CStr nonBlkStr;

      RollupPanel::correctUserProp(n);
      RollupPanel::getBlkFromUserProp(n, blkStr, nonBlkStr);

      DataBlock blk;
      blk.loadText(STR_DEST(blkStr), blkStr.length(), NULL);

      if (stricmp(type, "string") == NULL)
      {
        static char val[0x10000];
        panel->getInput(group, name, val);
        blk.setStr(name, val);
      }

      if (stricmp(type, "combo") == NULL)
      {
        static char val[0x10000];
        panel->getCombo(group, name, val);
        blk.setStr(name, val);
      }
      else if (stricmp(type, "int") == NULL)
        blk.setInt(name, (int)panel->getRealInput(group, name));
      else if (stricmp(type, "real") == NULL)
        blk.setReal(name, panel->getRealInput(group, name));
      else if (stricmp(type, "bool") == NULL)
        blk.setBool(name, panel->getCheck(group, name));
      else if (stricmp(type, "p3") == NULL)
        blk.setPoint3(name, panel->getPoint3Input(group, name));

      panel->bindCommand(n, name, blk);

      RollupPanel::saveBlkToUserPropBuffer(blk, n, nonBlkStr);

      panel->setNotCommon(group, name, false);
    }
    return ECB_CONT;
  }

private:
  RollupPanel *panel;
  const char *group, *name, *type, *val;
};

class SyncPanelParams : public ENodeCB
{
public:
  SyncPanelParams(RollupPanel *panel_) : panel(panel_), found(false) {}
  ~SyncPanelParams() override {}
  int proc(INode *n) override
  {
    if (n->Selected())
    {
      if (found)
      {
        int paramCount = 0;
        if (!panel->updateNCFromUserPropBuffer(n, paramCount))
        {
          // panel->bindCommands(n, panel->templateBlk);
          panel->fillFromBlk(panel->getTemplateBlk(), true);
          panel->saveToUserPropBuffer(n);
        }
      }
      else
      {
        panel->iRoll->Enable(true);
        found = true;
        int paramCount = 0;
        if (!panel->updateFromUserPropBuffer(n, paramCount))
        {
          panel->fillFromBlk(panel->getTemplateBlk(), true);
          panel->saveToUserPropBuffer(n);
        }
        else if (!paramCount)
        {
          CStr blkString;
          CStr nonBlkStr;

          RollupPanel::getBlkFromUserProp(n, blkString, nonBlkStr);

          panel->fillFromBlk(panel->getTemplateBlk(), true);
          panel->saveToUserPropBuffer(n, nonBlkStr);
        }
      }
      // return ECB_STOP;
    }
    return ECB_CONT;
  }

private:
  RollupPanel *panel;
  bool found;
};

//////////////////////////////////////////////////////////////////////////////
// Panel
//////////////////////////////////////////////////////////////////////////////
RollupPanel *RollupPanel::instance = NULL;
DataBlock *RollupPanel::templateBlk = NULL;

RollupPanel::RollupPanel(Interface *ip_, const HWND dlg_hwnd) : ip(ip_)
{
  iRoll = GetIRollup(GetDlgItem(dlg_hwnd, IDC_ROLLUPWINDOW));
  char filename[1024];
  debug("load defparams.blk");
  getMaxBlkFileName("defparams.blk", filename);
  templateBlk = new DataBlock;
  templateBlk->load(filename);
}

DataBlock &RollupPanel::getTemplateBlk()
{
  if (!templateBlk)
  {
    debug("load defparams.blk");
    char filename[1024];
    getMaxBlkFileName("defparams.blk", filename);
    templateBlk = new DataBlock;
    templateBlk->load(filename);
  }
  return *templateBlk;
}

void RollupPanel::fillFromBlk(const DataBlock &blk, bool enable)
{
  int spos = iRoll->GetScrollPos();
  iRoll->DeleteRollup(0, iRoll->GetNumPanels());
  instance = this;
  for (int i = 0; i < blk.blockCount(); i++)
  {
    DataBlock *groupBlk = blk.getBlock(i);
    int psCount = 0;
    for (int j = 0; j < groupBlk->blockCount(); j++)
      if (!stricmp(groupBlk->getBlock(j)->getStr(0), "p3"))
        psCount++;
    HWND hGroupNew = addGroup(iRoll, groupBlk->blockCount() + psCount * 2, groupBlk->getBlockName());
    FillCB cb(hGroupNew, this, enable);
    enum_params(groupBlk, &cb);
  }
  iRoll->Show();
  iRoll->SetScrollPos(spos);
}


void RollupPanel::updateFromBlk(const DataBlock &blk)
{
  UpdateCB cb(iRoll, this, &blk);
  enum_groups(&cb);
}

void RollupPanel::updateNCFromBlk(const DataBlock &blk)
{
  UpdateNCCB cb(iRoll, this, &blk);
  enum_groups(&cb);
}

void RollupPanel::getMaxBlkFileName(const char *blk, char *filename)
{
  ::GetModuleFileNameA(hInstance, filename, MAX_PATH);
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fName[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(filename, drive, dir, fName, ext);
  strcpy(filename, drive);
  strcat(filename, dir);
  strcat(filename, "../"); // search one dir up first
  strcat(filename, blk);
  if (FILE *fp = fopen(filename, "rb"))
    fclose(fp);
  else
  {
    // if not found then get in module dir
    strcpy(filename, drive);
    strcat(filename, dir);
    strcat(filename, blk);
  }
}


void RollupPanel::analyzeCfg(DataBlock &blk, CStr &source)
{
  CStr script = GENERAL_GROUP_NAME;
  script += "\r\n";
  script += source;

  char filename[1024];
  getMaxBlkFileName("tempcfg.ini", filename);

  FILE *h = fopen(filename, "w+b");
  fwrite(script.data(), script.length(), 1, h);
  fclose(h);


  CfgReader cfg(strToWide(filename).c_str());

  const DataBlock &scheme = RollupPanel::getTemplateBlk();
  const int nid = scheme.getNameId("parameter");

  for (int i = 0; i < scheme.blockCount(); ++i)
  {
    const DataBlock *groupBlk = scheme.getBlock(i);

    if (groupBlk)
    {
      for (int j = 0; j < groupBlk->blockCount(); ++j)
      {
        const DataBlock *paramBlk = groupBlk->getBlock(j);

        if (paramBlk && paramBlk->getBlockNameId() == nid)
        {
          const char *name = paramBlk->getStr("cfg_name", NULL);
          const char *group = paramBlk->getStr("cfg_group", NULL);

          if (name)
          {
            if (!group)
              group = "general";

            M_STD_STRING cfgName = strToWide(name).c_str();
            M_STD_STRING cfgGroup = strToWide(group).c_str();

            const char *paramName = paramBlk->getParamName(1);
            const char *paramType = paramBlk->getStr("type", NULL);

            M_STD_STRING val = cfg.GetKeyValue(cfgName.c_str(), cfgGroup.c_str());

            std::string valStr = wideToStr(val.c_str());

            if (paramType)
            {
              if (valStr.length() && paramType && !blk.paramExists(paramName))
              {
                if (!strcmp(paramType, "bool"))
                {
                  const bool val = !stricmp(valStr.c_str(), "yes") || !stricmp(valStr.c_str(), "on") ||
                                   !stricmp(valStr.c_str(), "true") || !stricmp(valStr.c_str(), "1");

                  blk.setBool(paramName, val);
                }
                else if (!strcmp(paramType, "int"))
                {
                  const int val = strtol(valStr.c_str(), NULL, 0);
                  blk.setInt(paramName, val);
                }
                else if (!strcmp(paramType, "real"))
                {
                  const real val = strtod(valStr.c_str(), NULL);
                  blk.setReal(paramName, val);
                }
                else if (!strcmp(paramType, "p3"))
                {
                  Point3 val = Point3(0, 0, 0);
                  sscanf(valStr.c_str(), " %f , %f , %f", &val.x, &val.y, &val.z);

                  blk.setPoint3(paramName, val);
                }
                else
                  blk.setStr(paramName, valStr.c_str());
              }

              cfg.WriteKeyVal(cfgName.c_str(), cfgGroup.c_str(), NULL);
            }
          }
        }
      }
    }
  }

  loadStrFromFile(filename, script);

  source = script.remove(0, (int)strlen(GENERAL_GROUP_NAME) + 2);
}


void RollupPanel::getBlkFromUserProp(INode *n, CStr &blk_string, CStr &non_blk_str)
{
  TSTR s;
  n->GetUserPropBuffer(s);
  CStr str = wideToStr(s).c_str();


  if (!str.length())
    return;

  const char *start = (const char *)str;
  const char *end = NULL;

  const char *rStr = strchr(start, '\r');
  const char *nStr = strchr(start, '\n');

  while (rStr || nStr)
  {
    if (rStr && nStr)
      end = __min(rStr, nStr);
    else if (rStr)
      end = rStr;
    else
      end = nStr;

    CStr line;
    const int strLen = end - start;

    line.Resize(strLen + 3);
    memcpy(STR_DEST(line), start, strLen);
    STR_DEST(line)[strLen] = '\r';
    STR_DEST(line)[strLen + 1] = '\n';
    STR_DEST(line)[strLen + 2] = 0;

    if (strchr((const char *)line, ':'))
      blk_string += line;
    else
      non_blk_str += line;

    while (*end == '\r' || *end == '\n')
      ++end;

    start = end;

    rStr = strchr(start, '\r');
    nStr = strchr(start, '\n');
  }

  if (*start)
  {
    CStr line;
    const char *end = str + strlen(str);
    const int strLen = end - start;

    line.Resize(strLen + 1);
    memcpy(STR_DEST(line), start, strLen);
    STR_DEST(line)[strLen] = 0;

    if (strchr((const char *)line, ':'))
      blk_string += line;
    else
      non_blk_str += line;
  }
}


bool RollupPanel::saveUserPropBufferToBlk(DataBlock &blk, INode *n, int &blk_param_count) // FAKE
{
  TSTR s;
  n->GetUserPropBuffer(s);
  CStr buf = wideToStr(s).c_str();

  if (!buf.length())
    return false;

  char filename[1024];
  getMaxBlkFileName("tempfake.blk", filename);

  FILE *h = fopen(filename, "w+b");
  fwrite(buf.data(), buf.length(), 1, h);
  fclose(h);

  blk.load(filename);
  blk_param_count = blk.paramCount();

  return true;
}


void RollupPanel::correctUserProp(INode *n)
{
  CStr newScript = "";

  if (n)
  {
    TSTR s;
    n->GetUserPropBuffer(s);
    CStr buf = wideToStr(s).c_str();

    if (buf.length())
    {
      CStr blkScript;
      CStr nonBlkScript;

      getBlkFromUserProp(n, blkScript, nonBlkScript);

      if (nonBlkScript.length())
      {
        DataBlock blk;
        blk.loadText(STR_DEST(blkScript), blkScript.length(), NULL);

        analyzeCfg(blk, nonBlkScript);
        getBlkInString(blk, blkScript);

        newScript = blkScript + "\r\n\r\n" + nonBlkScript;
      }
      else
        newScript = blkScript;
    }
  }

  n->SetUserPropBuffer(strToWide(newScript).c_str());
}


bool RollupPanel::saveBlkToUserPropBuffer(const DataBlock &blk, INode *n, const char *additional)
{
  CStr script;
  getBlkInString(blk, script);

  if (additional)
  {
    script += "\r\n\r\n";
    script += additional;
  }

  n->SetUserPropBuffer(strToWide(script).c_str());
  return true;
}


bool RollupPanel::getBlkInString(const DataBlock &blk, CStr &out)
{
  char filename[1024];
  getMaxBlkFileName("tempfake.blk", filename);

  blk.saveToTextFile(filename);

  return loadStrFromFile(filename, out);
}


bool RollupPanel::loadStrFromFile(const char *fname, CStr &str)
{
  FILE *h = fopen(fname, "r+b");

  if (!h)
    return false;

  CStr buffer;
  static char buf[0x10000 + 1];

  size_t count = fread(buf, 1, 0x10000, h);

  while (count)
  {
    buf[count] = 0;
    buffer += buf;

    count = fread(buf, 1, 0x10000, h);
  }

  fclose(h);
  str = buffer;

  return true;
}


bool RollupPanel::updateNCFromUserPropBuffer(INode *n, int &blk_param_count)
{
  correctUserProp(n);

  DataBlock blk;
  if (!saveUserPropBufferToBlk(blk, n, blk_param_count))
    return false;

  bindCommands(n, blk);
  updateNCFromBlk(blk);

  return true;
}

bool RollupPanel::updateFromUserPropBuffer(INode *n, int &blk_param_count)
{
  correctUserProp(n);

  DataBlock blk;
  if (!saveUserPropBufferToBlk(blk, n, blk_param_count))
    return false;

  bindCommands(n, blk);
  updateFromBlk(blk);

  return true;
}

void RollupPanel::saveToUserPropBuffer(INode *n, const char *additional)
{
  DataBlock blk;
  UserPropCB cb(iRoll, &blk);
  enum_groups(&cb);
  saveBlkToUserPropBuffer(blk, n, additional);
}

void RollupPanel::onPPChange(const char *group, const char *type, const char *name)
{
  SyncMaxParams cb(this, group, type, name);
  enum_nodes(ip->GetRootNode(), &cb);
}

void RollupPanel::fillPanel()
{
  fillFromBlk(*templateBlk, false);
  iRoll->Enable(false);
  SyncPanelParams cb(this);
  enum_nodes(ip->GetRootNode(), &cb);
}

RollupPanel::~RollupPanel()
{
  ReleaseIRollup(iRoll);
  instance = NULL;
  if (templateBlk)
  {
    delete templateBlk;
    templateBlk = NULL;
  }
}

BOOL RollupPanel::generalRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG: return TRUE;

    case WM_DESTROY: instance = NULL; return FALSE;

    case WM_COMMAND:
    {
      int nameID = (LOWORD(wParam) - PARAM_EDIT_IDC) / PARAM_IDC_COUNT;
      int groupID = instance->iRoll->GetPanelIndex(hWnd);
      char group[32], name[32], type[32];
      if (find_param(groupID, nameID, group, name, type))
        if (stricmp(type, "bool") == NULL || stricmp(type, "string") == NULL)
          instance->onPPChange(group, type, name);

      if ((!stricmp(type, "combo") && HIWORD(wParam) == BN_BUTTONUP))
      {
        const int valControlIdc = LOWORD(wParam) - (LOWORD(wParam) % PARAM_IDC_COUNT) + PARAM_EDIT_IDC;

        for (int i = 0; i < PARAM_EDITS_COUNT; i++)
        {
          const int btnId = valControlIdc + i;
          ICustButton *iEdit = GetICustButton(GetDlgItem(hWnd, btnId));
          if (iEdit)
          {
            OutputDebugString(_T("iEdit != NULL\n"));
            iEdit->SetCheck(btnId == LOWORD(wParam));
          }

          ReleaseICustButton(iEdit);
        }
        instance->onPPChange(group, type, name);
      }
    }
    break;

    case WM_CUSTEDIT_ENTER: break;

    case CC_SPINNER_CHANGE:
    {
      int nameID = LOWORD(wParam);
      if (nameID % PARAM_IDC_COUNT == PARAM_SPIN_IDC && instance)
      {
        nameID = (nameID - PARAM_SPIN_IDC) / PARAM_IDC_COUNT;
        int groupID = instance->iRoll->GetPanelIndex(hWnd);
        char group[32], name[32], type[32];
        if (find_param(groupID, nameID, group, name, type))
          instance->onPPChange(group, type, name);
      }
    }
      return TRUE;
      break;

    case WM_NOTIFY: break;
    default: break;
  }

  return FALSE;
}


HWND RollupPanel::addGroup(IRollupWindow *roll, int count, const char *name)
{
  int h = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (count + 1) + PARAM_CTRL_GAP;
  int index = roll->AppendRollup(::hInstance, MAKEINTRESOURCE(IDD_MAX_GENERAL_ROLLUP), (DLGPROC)generalRollupProc,
    (TCHAR *)strToWide(name).c_str());
  roll->SetPageDlgHeight(index, h);
  return roll->GetPanelDlg(index);
}

void RollupPanel::addButtons(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable, Tab<String> &items)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W / 3, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  int w = (PARAM_CTRL_W + PARAM_CTRL_H + 2 * PARAM_CTRL_CAPTION_W / 3) / items.Count();
  for (int i = 0; i < items.Count(); i++)
  {
    HWND hInputNew = ::CreateWindowEx(0, _T("CustButton"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD,
      PARAM_CTRL_LEFT1 - 2 * PARAM_CTRL_CAPTION_W / 3 + i * w, top, w, PARAM_CTRL_H, group_hwnd,
      (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC + i), ::hInstance, NULL);
    ICustButton *iEdit = GetICustButton(hInputNew);
    iEdit->SetText((TCHAR *)strToWide(items[i]).c_str());
    iEdit->SetType(CBT_CHECK);
    iEdit->SetCheck(!strcmp(val, items[i]));
    iEdit->Enable(enable);
    iEdit->SetButtonDownNotify(true);
    ReleaseICustButton(iEdit);
  }
  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}


void RollupPanel::addComboInput(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable, Tab<String> &items)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  HWND hInputNew =
    ::CreateWindowEx(0, _T("COMBOBOX"), _T(""), WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP, PARAM_CTRL_LEFT1, top,
      PARAM_CTRL_W + PARAM_CTRL_H, PARAM_CTRL_H * 9, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
  ::EnableWindow(hInputNew, enable);
  for (int i = 0; i < items.Count(); i++)
    ComboBox_AddString(hInputNew, strToWide(items[i]).c_str());
  ComboBox_SelectString(hInputNew, -1, strToWide(val).c_str());

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}


void RollupPanel::addIntInput(const HWND group_hwnd, int idc, const char *name, int val, bool enable)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  HWND hInputNew = ::CreateWindowEx(0, _T("CustEdit"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT1, top, PARAM_CTRL_W2,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
  ::EnableWindow(hInputNew, enable);
  ICustEdit *iEdit = GetICustEdit(hInputNew);
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText(val);
  ReleaseICustEdit(iEdit);

  HWND hSpinNew = ::CreateWindowEx(0, _T("SpinnerControl"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT4, top, PARAM_CTRL_H,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_SPIN_IDC), ::hInstance, NULL);
  ::EnableWindow(hSpinNew, enable);
  ISpinnerControl *iSpin = GetISpinner(hSpinNew);
  iSpin->SetLimits(-MAX_REAL, MAX_REAL, FALSE);
  iSpin->LinkToEdit(hInputNew, EDITTYPE_INT);
  iSpin->SetValue(val, FALSE);
  iSpin->SetAutoScale(true);
  ReleaseISpinner(iSpin);

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W2,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void RollupPanel::addRealInput(const HWND group_hwnd, int idc, const char *name, real val, bool enable)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  HWND hInputNew = ::CreateWindowEx(0, _T("CustEdit"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT1, top, PARAM_CTRL_W2,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
  ::EnableWindow(hInputNew, enable);
  ICustEdit *iEdit = GetICustEdit(hInputNew);
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText(val, 3);
  ReleaseICustEdit(iEdit);

  HWND hSpinNew = ::CreateWindowEx(0, _T("SpinnerControl"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT4, top, PARAM_CTRL_H,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_SPIN_IDC), ::hInstance, NULL);
  ::EnableWindow(hSpinNew, enable);
  ISpinnerControl *iSpin = GetISpinner(hSpinNew);
  iSpin->SetLimits(-MAX_REAL, MAX_REAL, FALSE);
  iSpin->LinkToEdit(hInputNew, EDITTYPE_FLOAT);
  iSpin->SetValue(val, FALSE);
  iSpin->SetScale(0.1f);
  // iSpin->SetAutoScale(true);
  ReleaseISpinner(iSpin);

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W2,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void RollupPanel::addStrInput(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  HWND hInputNew = ::CreateWindowEx(0, _T("CustEdit"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT1, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
  ::EnableWindow(hInputNew, enable);
  ICustEdit *iEdit = GetICustEdit(hInputNew);
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText((TCHAR *)strToWide(val).c_str());
  ReleaseICustEdit(iEdit);

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}


void RollupPanel::addCheck(const HWND group_hwnd, int idc, const char *name, bool val, bool enable)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);

  HWND hCheckNew = ::CreateWindowEx(0, _T("BUTTON"), _T(""), BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W /*PARAM_CTRL_W*2*/, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
  ::EnableWindow(hCheckNew, enable);

  ::SetWindowText(hCheckNew, strToWide(name).c_str());
  ::CheckDlgButton(group_hwnd, idc + PARAM_EDIT_IDC, val ? BST_CHECKED : BST_UNCHECKED);

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void RollupPanel::setNotCommon(const char *group, const char *name, bool nc)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return;

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_NC_IDC;
  ::SetDlgItemText(iRoll->GetPanelDlg(groupID), valControlIdc, nc ? _T("-NC") : _T(""));
}

bool RollupPanel::getCheck(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return false;

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
  return ::IsDlgButtonChecked(iRoll->GetPanelDlg(groupID), valControlIdc) ? 1 : 0;
}

real RollupPanel::getRealInput(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return 0;

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
  ICustEdit *iEdit = GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc));
  real val = iEdit->GetFloat();
  ReleaseICustEdit(iEdit);
  return val;
}

void RollupPanel::getCombo(const char *group, const char *name, char *val)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return;

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;

  for (int i = 0; i < 5; i++)
  {
    ICustButton *iEdit = GetICustButton(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc + i));
    if (iEdit && iEdit->IsChecked())
    {
      TCHAR sw[32];
      iEdit->GetText(sw, 32);
      strcpy(val, wideToStr(sw).c_str());
    }
    ReleaseICustButton(iEdit);
  }
}

void RollupPanel::getInput(const char *group, const char *name, char *val)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return;

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
  ICustEdit *iEdit = GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc));

  TCHAR sw[32];
  iEdit->GetText(sw, 32);
  strcpy(val, wideToStr(sw).c_str());

  ReleaseICustEdit(iEdit);
}

Point3 RollupPanel::getPoint3Input(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return Point3(0, 0, 0);

  Point3 val;
  for (int i = 0; i < 3; i++)
  {
    const int valControlIdc = (paramID + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ICustEdit *iEdit = GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc));
    val[i] = iEdit->GetFloat();
    ReleaseICustEdit(iEdit);
  }
  return val;
}

void RollupPanel::bindCommand(INode *n, const char *name, const DataBlock &blk)
{
  const char *command = find_info_by_name("max_command", name, NULL);

  if (command)
  {
    if (strcmp(command, "cast_shadows") == NULL)
      n->SetCastShadows(blk.getBool(name, true));
    if (strcmp(command, "recieve_shadows") == NULL)
      n->SetRcvShadows(blk.getBool(name, true));
    if (strcmp(command, "renderable") == NULL)
      n->SetRenderable(blk.getBool(name, true));
  }

  const char *old = find_info_by_name("prop_name", name, "");

  M_STD_STRING w_old = strToWide(old);

  if (strcmp(old, "") && n->UserPropExists(w_old.c_str()))
  {

    switch (blk.getParamType(blk.findParam(name)))
    {
      case DataBlock::TYPE_STRING: n->SetUserPropString(w_old.c_str(), strToWide(blk.getStr(name, "")).c_str()); break;
      case DataBlock::TYPE_INT: n->SetUserPropInt(w_old.c_str(), blk.getInt(name, 0)); break;
      case DataBlock::TYPE_REAL: n->SetUserPropFloat(w_old.c_str(), blk.getReal(name, 0)); break;
      case DataBlock::TYPE_POINT3:
      {
        Point3 p = blk.getPoint3(name, Point3(0, 0, 0));
        const TCHAR *xyz = _T( "XYZ" );
        for (int i = 0; i < 3; i++)
        {
          TCHAR buf[32];
          _stprintf(buf, _T("%s.%c"), w_old.c_str(), xyz[i]);
          n->SetUserPropFloat(buf, p[i]);
        }
      }
      break;
      case DataBlock::TYPE_BOOL: n->SetUserPropBool(w_old.c_str(), blk.getBool(name, false)); break;
      default:
        debug("error");
        // G_ASSERT(0);
    }
  }
}

void RollupPanel::bindCommands(INode *n, DataBlock &blk)
{
  blk.setBool(find_name_by_info("max_command", "cast_shadows"), n->CastShadows() ? 1 : 0);
  blk.setBool(find_name_by_info("max_command", "recieve_shadows"), n->RcvShadows() ? 1 : 0);
  blk.setBool(find_name_by_info("max_command", "renderable"), n->Renderable() ? 1 : 0);


  if (n->GetMtl())
  {
    IDagorMat *m = (IDagorMat *)n->GetMtl()->GetInterface(I_DAGORMAT);
    if (m && (_tcscmp(m->get_classname(), _T("billboard_atest")) == NULL || _tcscmp(m->get_classname(), _T("facing_leaves")) == NULL))
    {
      blk.setBool(find_name_by_info("max_command", "billboard"), true);
      debug("find billboard material '%s'", m->get_classname());
    }
  }
  UserPropToBlkCB cb(n, &blk);
  enum_groups(&cb);
}
