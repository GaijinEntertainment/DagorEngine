// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <locale>
#include <sstream>

#include "rolluppanel.h"
#include "datablk.h"
#include "resource.h"
#include "enumnode.h"
#include "dlg_dim.h"
#include "common.h"

#include "mater.h"
#include "debug.h"

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

static const char *DEFAULT_CFG_GROUP = "general";

static bool is_parameter_block(const DataBlock &blk) { return iequal(blk.getBlockName(), "parameter"); }

static SchemeType get_scheme_type(const DataBlock &param_blk)
{
  const char *type = param_blk.getStr("type", "string");

  if (iequal(type, "bool"))
    return SchemeType::Bool;
  if (iequal(type, "int"))
    return SchemeType::Int;
  if (iequal(type, "real"))
    return SchemeType::Real;
  if (iequal(type, "p3"))
    return SchemeType::Point3;
  if (iequal(type, "combo"))
    return SchemeType::Combo;

  return SchemeType::String;
}

static int get_scheme_type_id_count(SchemeType type) { return type == SchemeType::Point3 ? 3 : 1; }

template <typename Pred>
static const DataBlock *find_scheme_param(Pred pred)
{
  const DataBlock &scheme = RollupPanel::getTemplateBlk();

  for (int i = 0; i < scheme.blockCount(); i++)
  {
    const DataBlock *groupBlk = scheme.getBlock(i);
    for (int j = 0; j < groupBlk->blockCount(); j++)
    {
      const DataBlock *paramBlk = groupBlk->getBlock(j);
      if (is_parameter_block(*paramBlk) && pred(*paramBlk))
        return paramBlk;
    }
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// callBacks
//////////////////////////////////////////////////////////////////////////////
class EDataBlockCB
{
public:
  virtual ~EDataBlockCB() = default;
  virtual int procCheck(bool val) = 0;
  virtual int procInt(int val) = 0;
  virtual int procCombo(const char *val, const std::vector<std::string> &items) = 0;
  virtual int procReal(real val) = 0;
  virtual int procStr(const char *val) = 0;
  virtual int procPoint3(const Point3 &val) = 0;
  int groupId;
  int paramId;
  const char *name;
};

int enum_params(const DataBlock *blk, EDataBlockCB *cb);
bool find_param(const char *group, const char *name, int &group_id, int &param_id)
{
  const DataBlock &blk = RollupPanel::getTemplateBlk();
  group_id = -1;
  param_id = -1;

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock *groupBlk = blk.getBlock(i);
    if (!iequal(groupBlk->getBlockName(), group))
      continue;

    int paramId = 0;
    for (int j = 0; j < groupBlk->blockCount(); j++)
    {
      const DataBlock *paramBlk = groupBlk->getBlock(j);
      if (!is_parameter_block(*paramBlk))
        continue;

      if (strcmp(paramBlk->getParamName(1), name) == 0)
      {
        group_id = i;
        param_id = paramId;
        return true;
      }

      paramId += get_scheme_type_id_count(get_scheme_type(*paramBlk));
    }
    break;
  }
  return false;
}
bool find_param(int group_id, int param_id, std::string &group, std::string &name, SchemeType &type)
{
  const DataBlock *groupBlk = RollupPanel::getTemplateBlk().getBlock(group_id);
  if (!groupBlk)
    return false;

  int paramId = 0;
  for (int j = 0; j < groupBlk->blockCount(); j++)
  {
    const DataBlock *paramBlk = groupBlk->getBlock(j);
    if (!is_parameter_block(*paramBlk))
      continue;

    const SchemeType paramType = get_scheme_type(*paramBlk);
    const int idCount = get_scheme_type_id_count(paramType);

    if (param_id >= paramId && param_id < paramId + idCount)
    {
      group = groupBlk->getBlockName();
      name = paramBlk->getParamName(1);
      type = paramType;
      return true;
    }

    paramId += idCount;
  }
  return false;
}
const char *find_info_by_name(const char *info, const char *name, const char *def)
{
  const DataBlock *paramBlk = find_scheme_param([&](const DataBlock &blk) { return strcmp(blk.getParamName(1), name) == 0; });
  return paramBlk ? paramBlk->getStr(info, def) : def;
}
const char *find_name_by_info(const char *info, const char *command)
{
  const DataBlock *paramBlk = find_scheme_param([&](const DataBlock &blk) { return strcmp(blk.getStr(info, ""), command) == 0; });
  return paramBlk ? paramBlk->getParamName(1) : "";
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
  int procCombo(const char *val, const std::vector<std::string> &items) override
  {
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
    const char *caption = find_info_by_name("caption", name, name);
    for (int i = 0; i < 3; i++)
    {
      std::string label = std::string(caption) + "." + "xyz"[i];
      panel->addRealInput(hwnd, (paramId + i) * PARAM_IDC_COUNT, label.c_str(), val[i], enable);
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
  UpdateCB(IRollupWindow *i_roll, const DataBlock *node_blk) : nodeBlk(node_blk), iRoll(i_roll) {}
  int procCheck(bool val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ::CheckDlgButton(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC, nodeBlk->getBool(name, false));
    return ECB_CONT;
  }
  int procCombo(const char *val, const std::vector<std::string> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    for (int i = 0; i < int(items.size()); i++)
    {
      ControlPtr<ICustButton> iEdit(GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i)));
      iEdit->SetCheck(nodeBlk->getStr(name, "") == items[i]);
      iEdit->Enable();
    }
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC)));
    iEdit->SetText(nodeBlk->getInt(name, 0));
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC)));
    iEdit->SetText(nodeBlk->getReal(name, 0), 3);
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    clearParam(hwnd, paramId);
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC)));

    char *sz = (char *)nodeBlk->getStr(name, "");

    iEdit->SetText((TCHAR *)strToWide(sz).c_str());

    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    Point3 val1 = nodeBlk->getPoint3(name, val);
    for (int i = 0; i < 3; i++)
    {
      clearParam(hwnd, paramId + i);
      ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, (paramId + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC)));

      iEdit->SetText(val1[i], 3);
    }
    return ECB_CONT;
  }

private:
  const DataBlock *nodeBlk;
  IRollupWindow *iRoll;
};


class UpdateNCCB : public EDataBlockCB
{
public:
  UpdateNCCB(IRollupWindow *i_roll, const DataBlock *node_blk) : iRoll(i_roll), nodeBlk(node_blk) {}
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
  int procCombo(const char *val, const std::vector<std::string> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valNCIdc = paramId * PARAM_IDC_COUNT + PARAM_NC_IDC;
    bool nc = false;
    for (int i = 0; i < int(items.size()); i++)
    {
      ControlPtr<ICustButton> iEdit(GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i)));
      if (iEdit->IsChecked() != BOOL(nodeBlk->getStr(name, "") == items[i]))
        nc = true;
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
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valEditIdc)));
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
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valEditIdc)));
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
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valEditIdc)));

    WStr text;
    iEdit->GetText(text);
    std::string val1 = wideToStr(text.data());


    bool nc = strcmp(nodeBlk->getStr(name, ""), val1.c_str()) != 0;
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
      ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valEditIdc)));
      bool nc = iEdit->GetFloat() != val1[i];
      if (nc)
        ::SetDlgItemText(hwnd, valNCIdc, nc ? _T("-NC") : _T(""));
    }
    return ECB_CONT;
  }

private:
  IRollupWindow *iRoll;
  const DataBlock *nodeBlk;
};

class UserPropToBlkCB : public EDataBlockCB
{
public:
  UserPropToBlkCB(INode *n_, DataBlock *node_blk) : n(n_), nodeBlk(node_blk) {}
  int procCheck(bool val) override
  {
    std::wstring old = getName(name);
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
    std::wstring old = getName(name);
    if (!old.empty())
    {
      int i;
      if (n->GetUserPropInt(old.c_str(), i))
        nodeBlk->setInt(name, i);
    }
    return ECB_CONT;
  }
  int procCombo(const char *val, const std::vector<std::string> &items) override
  {
    std::wstring old = getName(name);
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
    std::wstring old = getName(name);
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
    std::wstring old = getName(name);
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
    std::wstring old = getName(name);
    if (!old.empty())
    {
      Point3 p(0, 0, 0);
      for (int i = 0; i < 3; i++)
      {
        std::wstring key = old + L"." + L"XYZ"[i];
        n->GetUserPropFloat(key.c_str(), p[i]);
      }
      if (p)
        nodeBlk->setPoint3(name, p);
    }
    return ECB_CONT;
  }

private:
  std::wstring getName(const char *name)
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
  int procCombo(const char *val, const std::vector<std::string> &items) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    for (int i = 0; i < int(items.size()); i++)
    {
      ControlPtr<ICustButton> iEdit(GetICustButton(GetDlgItem(hwnd, paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC + i)));
      if (iEdit->IsChecked())
        rezBlk->addStr(name, items[i].data());
    }
    return ECB_CONT;
  }
  int procInt(int val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valControlIdc)));
    rezBlk->addInt(name, iEdit->GetInt());
    return ECB_CONT;
  }
  int procReal(real val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valControlIdc)));
    rezBlk->addReal(name, iEdit->GetFloat());
    return ECB_CONT;
  }
  int procStr(const char *val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    const int valControlIdc = paramId * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valControlIdc)));

    WStr text;
    iEdit->GetText(text);

    rezBlk->addStr(name, wideToStr(text.data()).c_str());
    return ECB_CONT;
  }
  int procPoint3(const Point3 &val) override
  {
    HWND hwnd = iRoll->GetPanelDlg(groupId);
    Point3 p3;
    for (int i = 0; i < 3; i++)
    {
      const int valControlIdc = (paramId + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
      ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(hwnd, valControlIdc)));
      p3[i] = iEdit->GetFloat();
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
    const DataBlock *paramBlk = blk->getBlock(j);
    if (!is_parameter_block(*paramBlk))
      continue;

    const SchemeType type = get_scheme_type(*paramBlk);
    int result = ECB_SKIP;
    cb->name = paramBlk->getParamName(1);
    cb->paramId = id;
    switch (type)
    {
      case SchemeType::Bool: result = cb->procCheck(paramBlk->getBool(1)); break;
      case SchemeType::Int: result = cb->procInt(paramBlk->getInt(1)); break;
      case SchemeType::Real: result = cb->procReal(paramBlk->getReal(1)); break;
      case SchemeType::String: result = cb->procStr(paramBlk->getStr(1)); break;
      case SchemeType::Point3: result = cb->procPoint3(paramBlk->getPoint3(1)); break;
      case SchemeType::Combo:
      {
        std::vector<std::string> items;
        for (int i = 0; i < paramBlk->paramCount(); i++)
          if (iequal(paramBlk->getParamName(i), "item") && paramBlk->getParamType(i) == DataBlock::ParamType::TYPE_STRING)
            items.emplace_back(paramBlk->getStr(i));
        result = cb->procCombo(paramBlk->getStr(1), items);
      }
      break;
    }
    id += get_scheme_type_id_count(type);
    if (result == ECB_STOP)
      return 0;
  }
  return 1;
}

class SyncMaxParams : public ENodeCB
{
public:
  SyncMaxParams(RollupPanel *panel_, const char *group_, SchemeType type_, const char *name_) :
    panel(panel_), group(group_), name(name_), type(type_)
  {}
  ~SyncMaxParams() override = default;
  int proc(INode *n) override
  {
    if (n->Selected())
    {
      RollupPanel::UserProp prop;
      RollupPanel::loadUserProp(n, prop);

      switch (type)
      {
        case SchemeType::String: prop.blk.setStr(name, panel->getInput(group, name).c_str()); break;
        case SchemeType::Combo: prop.blk.setStr(name, panel->getCombo(group, name).c_str()); break;
        case SchemeType::Int: prop.blk.setInt(name, (int)panel->getRealInput(group, name)); break;
        case SchemeType::Real: prop.blk.setReal(name, panel->getRealInput(group, name)); break;
        case SchemeType::Bool: prop.blk.setBool(name, panel->getCheck(group, name)); break;
        case SchemeType::Point3: prop.blk.setPoint3(name, panel->getPoint3Input(group, name)); break;
      }

      panel->bindCommand(n, name, prop.blk);

      RollupPanel::saveBlkToUserPropBuffer(prop.blk, n, prop.nonBlkText);

      panel->setNotCommon(group, name, false);
    }
    return ECB_CONT;
  }

private:
  RollupPanel *panel;
  const char *group, *name;
  SchemeType type;
};

class SyncPanelParams : public ENodeCB
{
public:
  SyncPanelParams(RollupPanel *panel_) : panel(panel_), found(false) {}
  ~SyncPanelParams() override = default;
  int proc(INode *n) override
  {
    if (!n->Selected())
      return ECB_CONT;

    RollupPanel::UserProp prop;
    bool updated;

    if (found)
      updated = panel->updateNCFromUserPropBuffer(n, prop);
    else
    {
      panel->iRoll->Enable(true);
      found = true;
      updated = panel->updateFromUserPropBuffer(n, prop);
    }

    if (!updated)
    {
      panel->fillFromBlk(panel->getTemplateBlk(), true);
      panel->saveToUserPropBuffer(n, prop.nonBlkText);
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
std::unique_ptr<DataBlock> RollupPanel::templateBlk;

RollupPanel::RollupPanel(Interface *ip_, const HWND dlg_hwnd) : ip(ip_)
{
  iRoll.reset(GetIRollup(GetDlgItem(dlg_hwnd, IDC_ROLLUPWINDOW)));
}

DataBlock &RollupPanel::getTemplateBlk()
{
  if (!templateBlk)
  {
    debug("load defparams.blk");
    templateBlk = std::make_unique<DataBlock>(std::make_shared<NameMap>());
    templateBlk->load(get_cfg_filename(L"defparams.blk"));
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
    int rowCount = 0;
    for (int j = 0; j < groupBlk->blockCount(); j++)
    {
      const DataBlock *paramBlk = groupBlk->getBlock(j);
      if (is_parameter_block(*paramBlk))
        rowCount += get_scheme_type_id_count(get_scheme_type(*paramBlk));
    }
    HWND hGroupNew = addGroup(iRoll.get(), rowCount, groupBlk->getBlockName());
    FillCB cb(hGroupNew, this, enable);
    enum_params(groupBlk, &cb);
  }
  iRoll->Show();
  iRoll->SetScrollPos(spos);
}


void RollupPanel::updateFromBlk(const DataBlock &blk)
{
  UpdateCB cb(iRoll.get(), &blk);
  enum_groups(&cb);
}

void RollupPanel::updateNCFromBlk(const DataBlock &blk)
{
  UpdateNCCB cb(iRoll.get(), &blk);
  enum_groups(&cb);
}


static std::string_view trim_spaces(std::string_view s)
{
  const size_t begin = s.find_first_not_of(" \t");
  if (begin == std::string_view::npos)
    return std::string_view();
  return s.substr(begin, s.find_last_not_of(" \t") - begin + 1);
}

static std::string_view strip_quotes(std::string_view s)
{
  if (s.length() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
    return s.substr(1, s.length() - 2);
  return s;
}

static std::string_view take_line(std::string_view &text)
{
  const size_t eol = text.find_first_of("\r\n");
  const std::string_view line = text.substr(0, eol);

  const size_t nextLine = text.find_first_not_of("\r\n", eol);
  text = nextLine == std::string_view::npos ? std::string_view() : text.substr(nextLine);

  return line;
}

static const DataBlock *find_cfg_param(const char *group, const char *name)
{
  return find_scheme_param([&](const DataBlock &blk) {
    const char *cfgName = blk.getStr("cfg_name", NULL);
    return cfgName && iequal(cfgName, name) && iequal(blk.getStr("cfg_group", DEFAULT_CFG_GROUP), group);
  });
}

static void set_cfg_param(DataBlock &blk, const DataBlock &param_blk, const char *value)
{
  const char *paramName = param_blk.getParamName(1);

  switch (get_scheme_type(param_blk))
  {
    case SchemeType::Bool:
    {
      const CaseInsensitiveEqual eq;
      blk.setBool(paramName, eq(value, "yes") || eq(value, "on") || eq(value, "true") || eq(value, "1"));
    }
    break;
    case SchemeType::Int:
    {
      int i = 0;
      parse_nums(value, i);
      blk.setInt(paramName, i);
    }
    break;
    case SchemeType::Real:
    {
      real r = 0;
      parse_nums(value, r);
      blk.setReal(paramName, r);
    }
    break;
    case SchemeType::Point3:
    {
      Point3 p(0, 0, 0);
      (void)parse_nums(value, p.x, p.y, p.z);
      blk.setPoint3(paramName, p);
    }
    break;
    case SchemeType::String:
    case SchemeType::Combo: blk.setStr(paramName, value); break;
  }
}

static bool take_cfg_params(DataBlock &blk, std::string &text)
{
  std::string group = DEFAULT_CFG_GROUP;
  std::string rest;
  bool migrated = false;

  for (std::string_view left = text; !left.empty();)
  {
    const std::string_view rawLine = take_line(left);
    const std::string_view line = trim_spaces(rawLine);
    const size_t eq = line.find('=');

    if (line.starts_with('['))
    {
      const size_t groupEnd = line.find(']');
      if (groupEnd != std::string_view::npos)
        group = trim_spaces(line.substr(1, groupEnd - 1));
    }
    else if (!line.starts_with(';') && eq != std::string_view::npos)
    {
      const std::string name(trim_spaces(line.substr(0, eq)));

      if (const DataBlock *paramBlk = find_cfg_param(group.c_str(), name.c_str()))
      {
        const std::string value(strip_quotes(trim_spaces(line.substr(eq + 1))));
        if (!value.empty() && !blk.paramExists(paramBlk->getParamName(1)))
          set_cfg_param(blk, *paramBlk, value.c_str());
        migrated = true;
        continue;
      }
    }

    rest += rawLine;
    rest += "\r\n";
  }

  text = std::move(rest);
  return migrated;
}


bool RollupPanel::loadUserProp(INode *n, UserProp &prop)
{
  TSTR s;
  n->GetUserPropBuffer(s);
  const std::string text = wideToStr(s);

  for (std::string_view left = text; !left.empty();)
  {
    const std::string_view line = take_line(left);
    std::string &dest = line.find(':') != std::string_view::npos ? prop.blkText : prop.nonBlkText;

    dest += line;
    dest += "\r\n";
  }

  if (prop.blkText.empty() && prop.nonBlkText.empty())
    return false;

  prop.blk.loadText(prop.blkText);
  prop.cfgMigrated = take_cfg_params(prop.blk, prop.nonBlkText);

  return true;
}


void RollupPanel::saveCorrectedUserProp(INode *n, const UserProp &prop)
{
  if (prop.nonBlkText.empty() && !prop.cfgMigrated)
    n->SetUserPropBuffer(strToWide(prop.blkText).c_str());
  else
    saveBlkToUserPropBuffer(prop.blk, n, prop.nonBlkText);
}


void RollupPanel::correctUserProp(INode *n)
{
  UserProp prop;

  if (loadUserProp(n, prop))
    saveCorrectedUserProp(n, prop);
}


void RollupPanel::saveBlkToUserPropBuffer(const DataBlock &blk, INode *n, std::string_view additional)
{
  std::ostringstream os;
  os.imbue(std::locale::classic());
  blk.saveToTextStream(os);

  std::string script = os.str();

  if (!additional.empty())
  {
    script += "\r\n\r\n";
    script += additional;
  }

  n->SetUserPropBuffer(strToWide(script).c_str());
}


bool RollupPanel::updateNCFromUserPropBuffer(INode *n, UserProp &prop)
{
  if (!loadUserProp(n, prop))
    return false;

  saveCorrectedUserProp(n, prop);

  bindCommands(n, prop.blk);
  updateNCFromBlk(prop.blk);

  return true;
}

bool RollupPanel::updateFromUserPropBuffer(INode *n, UserProp &prop)
{
  if (!loadUserProp(n, prop) || !prop.blk.paramCount())
    return false;

  saveCorrectedUserProp(n, prop);

  bindCommands(n, prop.blk);
  updateFromBlk(prop.blk);

  return true;
}

void RollupPanel::saveToUserPropBuffer(INode *n, std::string_view additional)
{
  DataBlock blk(std::make_shared<NameMap>());
  UserPropCB cb(iRoll.get(), &blk);
  enum_groups(&cb);
  saveBlkToUserPropBuffer(blk, n, additional);
}

void RollupPanel::onPPChange(const char *group, SchemeType type, const char *name)
{
  SyncMaxParams cb(this, group, type, name);
  enum_nodes(ip->GetRootNode(), &cb);
}

void RollupPanel::fillPanel()
{
  fillFromBlk(getTemplateBlk(), false);
  iRoll->Enable(false);
  SyncPanelParams cb(this);
  enum_nodes(ip->GetRootNode(), &cb);
}

RollupPanel::~RollupPanel()
{
  instance = NULL;
  // dropped so that reopening the panel picks up edits to defparams.blk
  templateBlk.reset();
}

BOOL RollupPanel::generalRollupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG: return TRUE;

    case WM_DESTROY: instance = NULL; return FALSE;

    case WM_COMMAND:
    {
      if (!instance)
        break;

      int nameID = (LOWORD(wParam) - PARAM_EDIT_IDC) / PARAM_IDC_COUNT;
      int groupID = instance->iRoll->GetPanelIndex(hWnd);
      std::string group, name;
      SchemeType type = SchemeType::String;
      if (!find_param(groupID, nameID, group, name, type))
        break;

      if (type == SchemeType::Bool || type == SchemeType::String)
        instance->onPPChange(group.c_str(), type, name.c_str());
      else if (type == SchemeType::Combo && HIWORD(wParam) == BN_BUTTONUP)
      {
        const int valControlIdc = LOWORD(wParam) - (LOWORD(wParam) % PARAM_IDC_COUNT) + PARAM_EDIT_IDC;

        for (int i = 0; i < PARAM_EDITS_COUNT; i++)
        {
          const int btnId = valControlIdc + i;
          ControlPtr<ICustButton> iEdit(GetICustButton(GetDlgItem(hWnd, btnId)));
          if (!iEdit)
            break;

          iEdit->SetCheck(btnId == LOWORD(wParam));
        }
        instance->onPPChange(group.c_str(), type, name.c_str());
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
        std::string group, name;
        SchemeType type = SchemeType::String;
        if (find_param(groupID, nameID, group, name, type))
          instance->onPPChange(group.c_str(), type, name.c_str());
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

void RollupPanel::addButtons(const HWND group_hwnd, int idc, const char *name, const char *val, bool enable,
  const std::vector<std::string> &items)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);
  HWND hStaticNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W / 3, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_LABEL_IDC), ::hInstance, NULL);
  ::EnableWindow(hStaticNew, enable);
  HGDIOBJ hFont = GetStockObject(DEFAULT_GUI_FONT);
  SendMessage(hStaticNew, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SetDlgItemText(group_hwnd, idc + PARAM_LABEL_IDC, strToWide(name).c_str());

  if (!items.empty())
  {
    const int w = int((PARAM_CTRL_W + PARAM_CTRL_H + 2 * PARAM_CTRL_CAPTION_W / 3) / items.size());
    for (int i = 0; i < int(items.size()); i++)
    {
      HWND hInputNew = ::CreateWindowEx(0, _T("CustButton"), _T(""), SS_RIGHT | WS_VISIBLE | WS_CHILD,
        PARAM_CTRL_LEFT1 - 2 * PARAM_CTRL_CAPTION_W / 3 + i * w, top, w, PARAM_CTRL_H, group_hwnd,
        (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC + i), ::hInstance, NULL);
      ControlPtr<ICustButton> iEdit(GetICustButton(hInputNew));
      iEdit->SetText((TCHAR *)strToWide(items[i]).c_str());
      iEdit->SetType(CBT_CHECK);
      iEdit->SetCheck(val == items[i]);
      iEdit->Enable(enable);
      iEdit->SetButtonDownNotify(true);
    }
  }
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
  ControlPtr<ICustEdit> iEdit(GetICustEdit(hInputNew));
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText(val);

  HWND hSpinNew = ::CreateWindowEx(0, _T("SpinnerControl"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT4, top, PARAM_CTRL_H,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_SPIN_IDC), ::hInstance, NULL);
  ::EnableWindow(hSpinNew, enable);
  ControlPtr<ISpinnerControl> iSpin(GetISpinner(hSpinNew));
  iSpin->SetLimits(-MAX_REAL, MAX_REAL, FALSE);
  iSpin->LinkToEdit(hInputNew, EDITTYPE_INT);
  iSpin->SetValue(val, FALSE);
  iSpin->SetAutoScale(true);

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
  ControlPtr<ICustEdit> iEdit(GetICustEdit(hInputNew));
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText(val, 3);

  HWND hSpinNew = ::CreateWindowEx(0, _T("SpinnerControl"), _T(""), WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT4, top, PARAM_CTRL_H,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_SPIN_IDC), ::hInstance, NULL);
  ::EnableWindow(hSpinNew, enable);
  ControlPtr<ISpinnerControl> iSpin(GetISpinner(hSpinNew));
  iSpin->SetLimits(-MAX_REAL, MAX_REAL, FALSE);
  iSpin->LinkToEdit(hInputNew, EDITTYPE_FLOAT);
  iSpin->SetValue(val, FALSE);
  iSpin->SetScale(0.1f);

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
  ControlPtr<ICustEdit> iEdit(GetICustEdit(hInputNew));
  iEdit->SetNotifyOnKillFocus(true);
  iEdit->SetText((TCHAR *)strToWide(val).c_str());

  HWND hNCNew = ::CreateWindowEx(0, _T("STATIC"), _T(""), SS_LEFT | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT3, top, PARAM_CTRL_W,
    PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_NC_IDC), ::hInstance, NULL);
  SendMessage(hNCNew, WM_SETFONT, (WPARAM)hFont, TRUE);
}


void RollupPanel::addCheck(const HWND group_hwnd, int idc, const char *name, bool val, bool enable)
{
  int top = (PARAM_CTRL_H + PARAM_CTRL_GAP) * (idc / PARAM_IDC_COUNT + 1);

  HWND hCheckNew = ::CreateWindowEx(0, _T("BUTTON"), _T(""), BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD, PARAM_CTRL_LEFT, top,
    PARAM_CTRL_CAPTION_W, PARAM_CTRL_H, group_hwnd, (HMENU)(intptr_t)(idc + PARAM_EDIT_IDC), ::hInstance, NULL);
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
  ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc)));
  if (!iEdit)
    return 0;

  return iEdit->GetFloat();
}

std::string RollupPanel::getCombo(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return std::string();

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;

  std::string val;
  for (int i = 0; i < PARAM_EDITS_COUNT; i++)
  {
    ControlPtr<ICustButton> iEdit(GetICustButton(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc + i)));
    if (!iEdit)
      break;

    if (iEdit->IsChecked())
    {
      WStr text;
      iEdit->GetText(text);
      val = wideToStr(text.data());
    }
  }
  return val;
}

std::string RollupPanel::getInput(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return std::string();

  const int valControlIdc = paramID * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
  ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc)));
  if (!iEdit)
    return std::string();

  WStr text;
  iEdit->GetText(text);

  return wideToStr(text.data());
}

Point3 RollupPanel::getPoint3Input(const char *group, const char *name)
{
  int groupID, paramID;
  if (!find_param(group, name, groupID, paramID))
    return Point3(0, 0, 0);

  Point3 val(0, 0, 0);
  for (int i = 0; i < 3; i++)
  {
    const int valControlIdc = (paramID + i) * PARAM_IDC_COUNT + PARAM_EDIT_IDC;
    ControlPtr<ICustEdit> iEdit(GetICustEdit(GetDlgItem(iRoll->GetPanelDlg(groupID), valControlIdc)));
    if (!iEdit)
      continue;

    val[i] = iEdit->GetFloat();
  }
  return val;
}

template <class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void RollupPanel::bindCommand(INode *n, const char *name, const DataBlock &blk)
{
  const char *command = find_info_by_name("max_command", name, NULL);

  if (command)
  {
    if (strcmp(command, "cast_shadows") == 0)
      n->SetCastShadows(blk.getBool(name, true));
    if (strcmp(command, "recieve_shadows") == 0)
      n->SetRcvShadows(blk.getBool(name, true));
    if (strcmp(command, "renderable") == 0)
      n->SetRenderable(blk.getBool(name, true));
  }

  const char *old = find_info_by_name("prop_name", name, "");

  std::wstring w_old = strToWide(old);

  if (strcmp(old, "") && n->UserPropExists(w_old.c_str()))
  {
    auto param = blk.getParam(blk.findParam(name));
    if (!param)
      debug("error");
    else
      std::visit(overloaded{
                   [&](const std::string &v) { n->SetUserPropString(w_old.c_str(), strToWide(v.c_str()).c_str()); },
                   [&](int v) { n->SetUserPropInt(w_old.c_str(), v); },
                   [&](real v) { n->SetUserPropFloat(w_old.c_str(), v); },
                   [&](const Point3 &p) {
                     for (int i = 0; i < 3; i++)
                     {
                       std::wstring key = w_old + L"." + L"XYZ"[i];
                       n->SetUserPropFloat(key.c_str(), p[i]);
                     }
                   },
                   [&](bool v) { n->SetUserPropBool(w_old.c_str(), v); },
                   [&](auto &&) { debug("error"); },
                 },
        param->get());
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
    if (m && (_tcscmp(m->get_classname(), L"billboard_atest") == 0 || _tcscmp(m->get_classname(), L"facing_leaves") == 0))
    {
      blk.setBool(find_name_by_info("max_command", "billboard"), true);
      debug(L"find billboard material '{}'", m->get_classname());
    }
  }
  UserPropToBlkCB cb(n, &blk);
  enum_groups(&cb);
}
