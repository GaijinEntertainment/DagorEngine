// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vector>
#include <array>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <locale>

#include <max.h>
#include <stdmat.h>
#include <math.h>
#include <iparamb2.h>

#include "dagor.h"
#include "dagfmt.h"
#include "mater.h"
#include "texmaps.h"
#include "resource.h"
#include "datablk.h"
#include "debug.h"
#include "common.h"
#include "ci.h"
#include "layout.h"
#include "enumnode.h"

#include <d3dx9.h>
#include <d3d9types.h>
#include <ihardwarematerial.h>

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

//////////////////////////////////////////////////////////////////////////////////

#define TX_MODULATE   0
#define TX_ALPHABLEND 1
#define BMIDATA(x)    ((UBYTE *)((BYTE *)(x) + sizeof(BITMAPINFOHEADER)))

static const TSTR DEFAULT_SHADER_NAME(_T("gi_black"));
static const TSTR DAGOR_SHADERS_CONFIG(_T("dagorShaders.blk"));
static const TCHAR *REAL_TWO_SIDED(_T("real_two_sided"));

/////////////////////////////////////////////////////////////////////////////////

static const std::unordered_map<std::string, DataBlock::ParamType, CaseInsensitiveHash, CaseInsensitiveEqual> &type_map()
{
  // old C++ doesn't recognize the static initializer list for complex containers
  static std::unordered_map<std::string, DataBlock::ParamType, CaseInsensitiveHash, CaseInsensitiveEqual> tmap;
  if (tmap.empty())
  {
    tmap.emplace("text", DataBlock::ParamType::TYPE_STRING);
    tmap.emplace("int", DataBlock::ParamType::TYPE_INT);
    tmap.emplace("bool", DataBlock::ParamType::TYPE_BOOL);
    tmap.emplace("color", DataBlock::ParamType::TYPE_E3DCOLOR);
    tmap.emplace("real", DataBlock::ParamType::TYPE_REAL);
    //
    tmap.emplace("t", DataBlock::ParamType::TYPE_STRING);
    tmap.emplace("i", DataBlock::ParamType::TYPE_INT);
    tmap.emplace("b", DataBlock::ParamType::TYPE_BOOL);
    tmap.emplace("c", DataBlock::ParamType::TYPE_E3DCOLOR);
    tmap.emplace("r", DataBlock::ParamType::TYPE_REAL);
    tmap.emplace("m", DataBlock::ParamType::TYPE_MATRIX);
    tmap.emplace("p2", DataBlock::ParamType::TYPE_POINT2);
    tmap.emplace("p3", DataBlock::ParamType::TYPE_POINT3);
    tmap.emplace("p4", DataBlock::ParamType::TYPE_POINT4);
    tmap.emplace("ip2", DataBlock::ParamType::TYPE_IPOINT2);
    tmap.emplace("ip3", DataBlock::ParamType::TYPE_IPOINT3);
  }
  return tmap;
}

/////////////////////////////////////////////////////////////////////////////////

static const char *blk_tex_name(int i)
{
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703) // C++17
  static_assert(DAGTEXNUM == 16);
#endif
  static const char *s[DAGTEXNUM] = {"tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7", "tex8", "tex9", "tex10", "tex11",
    "tex12", "tex13", "tex14", "tex15"};
  return s[(i >= 0 && i < DAGTEXNUM) ? i : 0];
}

/////////////////////////////////////////////////////////////////////////////////

static std::wstring mangled_category_name(unsigned depth, const std::wstring &name)
{
  std::wstring prefix;
  while (depth-- > 0)
    prefix += _T("--- ");
  return prefix + name;
}

std::unique_ptr<DataBlock> get_blk()
{
  std::wstring filename = get_cfg_filename(DAGOR_SHADERS_CONFIG);
  DataBlock *dataBlk = new DataBlock(std::make_shared<NameMap>());
  dataBlk->load(filename);
  return std::unique_ptr<DataBlock>(dataBlk);
}


static std::vector<std::wstring> get_blk_shader_list_of_category(unsigned depth, const DataBlock *categoryBlk)
{
  std::vector<std::wstring> shader_list;

  if (!categoryBlk)
    return shader_list;

  int category_name_i = categoryBlk->findParam("name");
  if (category_name_i != -1 && depth)
  {
    const std::wstring category_name = mangled_category_name(depth, strToWide(categoryBlk->getStr(category_name_i)));
    shader_list.emplace_back(category_name);
  }

  for (int i = 0; i < categoryBlk->blockCount(); ++i)
  {
    DataBlock *blk = categoryBlk->getBlock(i);
    if (!blk)
      continue;

    if (!stricmp(blk->getBlockName(), "shader_category"))
    {
      std::vector<std::wstring> shaders = get_blk_shader_list_of_category(depth + 1, blk);
      shader_list.insert(shader_list.end(), shaders.begin(), shaders.end());
      continue;
    }

    int name_i = blk->findParam("name");
    if (name_i == -1)
      continue;

    const std::wstring name = strToWide(blk->getStr(name_i));
    shader_list.emplace_back(name);
  }

  return shader_list;
}


std::vector<std::wstring> get_blk_shader_list(const DataBlock *dataBlk) { return get_blk_shader_list_of_category(0, dataBlk); }


static const DataBlock *get_blk_shader(const DataBlock *dataBlk, const std::wstring &shader_name)
{
  if (!dataBlk)
    return nullptr;

  // categories
  for (int c_i = 0; c_i < dataBlk->blockCount(); ++c_i)
  {
    DataBlock *category_blk = dataBlk->getBlock(c_i);
    if (!category_blk)
      continue;

    // shaders
    for (int s_i = 0; s_i < category_blk->blockCount(); ++s_i)
    {
      DataBlock *shader_blk = category_blk->getBlock(s_i);
      if (!shader_blk)
        continue;

      int shader_name_i = shader_blk->findParam("name");
      if (shader_name_i == -1)
        continue;

      if (iequal(strToWide(shader_blk->getStr(shader_name_i)), shader_name))
        return shader_blk;
    }
  }

  return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////

static DataBlock::ParamType deserialize_param_type(const std::string &s)
{
  auto it = type_map().find(s);
  if (it == type_map().end())
    return DataBlock::ParamType::TYPE_NONE;

  return it->second;
}

/////////////////////////////////////////////////////////////////////////////////

struct ParamInfo
{
  DataBlock::ParamType type;
  std::wstring name, description, custom_ui, value, def, parent;
  float soft_min, soft_max;
  bool is_group;
  bool def_enabled;
  bool soft_min_enabled;
  bool soft_max_enabled;

  ParamInfo() :
    type(DataBlock::ParamType::TYPE_NONE),
    soft_min(0),
    soft_max(0),
    is_group(false),
    def_enabled(false),
    soft_min_enabled(false),
    soft_max_enabled(false)
  {}

  bool isInt() const
  {
    return type == DataBlock::ParamType::TYPE_INT || type == DataBlock::ParamType::TYPE_IPOINT2 ||
           type == DataBlock::ParamType::TYPE_IPOINT3;
  }
};

static const TCHAR *get_default_param_value(DataBlock::ParamType type)
{
  switch (type)
  {
    case DataBlock::ParamType::TYPE_BOOL: return L"no";

    case DataBlock::ParamType::TYPE_INT: return L"0";
    case DataBlock::ParamType::TYPE_REAL: return L"0.0";

    case DataBlock::ParamType::TYPE_IPOINT2: return L"0, 0";
    case DataBlock::ParamType::TYPE_POINT2: return L"0.0, 0.0";

    case DataBlock::ParamType::TYPE_IPOINT3: return L"0, 0, 0";
    case DataBlock::ParamType::TYPE_POINT3: return L"0.0, 0.0, 0.0";

    case DataBlock::ParamType::TYPE_E3DCOLOR: return L"0, 0, 0, 0";
    case DataBlock::ParamType::TYPE_POINT4: return L"0.0, 0.0, 0.0, 0.0";

    default: break;
  }
  return L"";
}

static std::wstring get_param_value_as_string(const DataBlock *blk, int name_id)
{
  if (!blk || name_id == -1)
    return std::wstring();

  const DataBlock::Param *p = blk->getParam(name_id);
  if (!p)
    return std::wstring();

  std::wstringstream os;
  os.imbue(std::locale::classic());

  switch (p->type)
  {
    case DataBlock::ParamType::TYPE_STRING: return strToWide(p->as_c_str());
    case DataBlock::ParamType::TYPE_BOOL: return p->as_bool() ? L"yes" : L"no";

    case DataBlock::ParamType::TYPE_INT: os << p->as_int(); break;
    case DataBlock::ParamType::TYPE_REAL: os << p->as_real(); break;

    case DataBlock::ParamType::TYPE_POINT2: os << p->as_pt2().x << ", " << p->as_pt2().y; break;
    case DataBlock::ParamType::TYPE_POINT3: os << p->as_pt3().x << ", " << p->as_pt3().y << ", " << p->as_pt3().z; break;
    case DataBlock::ParamType::TYPE_POINT4:
      os << p->as_pt4().x << ", " << p->as_pt4().y << ", " << p->as_pt4().z << ", " << p->as_pt4().w;
      break;

    case DataBlock::ParamType::TYPE_IPOINT2: os << p->as_ipt2().x << ", " << p->as_ipt2().y; break;
    case DataBlock::ParamType::TYPE_IPOINT3: os << p->as_ipt3().x << ", " << p->as_ipt3().y << ", " << p->as_ipt3().z; break;

    case DataBlock::ParamType::TYPE_E3DCOLOR:
      os << p->as_color().r << ", " << p->as_color().g << ", " << p->as_color().b << ", " << p->as_color().a;
      break;

    case DataBlock::ParamType::TYPE_MATRIX:
    {
      os << "[";
      os << "[" << p->as_tm().getcol(0).x << ", " << p->as_tm().getcol(0).y << ", " << p->as_tm().getcol(0).z << "]";
      os << "[" << p->as_tm().getcol(1).x << ", " << p->as_tm().getcol(1).y << ", " << p->as_tm().getcol(1).z << "]";
      os << "[" << p->as_tm().getcol(2).x << ", " << p->as_tm().getcol(2).y << ", " << p->as_tm().getcol(2).z << "]";
      os << "[" << p->as_tm().getcol(3).x << ", " << p->as_tm().getcol(3).y << ", " << p->as_tm().getcol(3).z << "]";
      os << "]";
      break;
    }

    default: debug("unknown type"); break;
  }

  return os.str();
}

static float get_param_value_as_float(const DataBlock *blk, int name_id, float def)
{
  if (!blk || name_id == -1)
    return def;

  const DataBlock::Param *p = blk->getParam(name_id);
  if (!p)
    return def;

  if (p->type == DataBlock::ParamType::TYPE_INT)
    return p->as_int();

  if (p->type == DataBlock::ParamType::TYPE_REAL)
    return p->as_real();

  return def;
}

static std::vector<ParamInfo> get_blk_shader_params_of_group(const DataBlock *groupBlk, const std::wstring &parent)
{
  std::vector<ParamInfo> shader_params;

  if (!groupBlk)
    return shader_params;

  for (int i = 0; i < groupBlk->blockCount(); ++i)
  {
    DataBlock *blk = groupBlk->getBlock(i);
    if (!blk)
      continue;

    if (!_stricmp(blk->getBlockName(), "parameters_group"))
    {
      ParamInfo group;
      group.parent = parent;
      group.is_group = true;
      int group_name_i = blk->findParam("name");
      if (group_name_i != -1)
        group.name = strToWide(blk->getStr(group_name_i));
      shader_params.emplace_back(group);

      std::vector<ParamInfo> params = get_blk_shader_params_of_group(blk, group.name);
      shader_params.insert(shader_params.end(), params.begin(), params.end());
      continue;
    }

    int param_name_i = blk->findParam("name");
    if (param_name_i == -1)
      continue;

    ParamInfo param;
    param.parent = parent;
    param.name = strToWide(blk->getStr(param_name_i));

    int param_type_i = blk->findParam("type");
    if (param_type_i != -1)
      param.type = deserialize_param_type(blk->getStr(param_type_i));
    else
      param.type = DataBlock::ParamType::TYPE_STRING;

    int param_description_i = blk->findParam("description");
    if (param_description_i != -1)
      param.description = strToWide(blk->getStr(param_description_i));

    int custom_ui_i = blk->findParam("custom_ui");
    if (custom_ui_i != -1)
      param.custom_ui = strToWide(blk->getStr(custom_ui_i));

    if (iequal(param.custom_ui, L"color"))
    {
      param.soft_min = 0.0;
      param.soft_max = 1.0;
      param.soft_min_enabled = param.soft_max_enabled = true;
    }

    int default_i = blk->findParam("default");
    if (default_i != -1)
    {
      param.def = param.value = get_param_value_as_string(blk, default_i);
      param.def_enabled = true;
    }
    else
      param.value = get_default_param_value(param.type);

    int soft_min_i = blk->findParam("soft_min");
    if (soft_min_i != -1)
    {
      param.soft_min = get_param_value_as_float(blk, soft_min_i, -FLT_MAX);
      param.soft_min_enabled = true;
    }

    int soft_max_i = blk->findParam("soft_max");
    if (soft_max_i != -1)
    {
      param.soft_max = get_param_value_as_float(blk, soft_max_i, +FLT_MAX);
      param.soft_max_enabled = true;
    }

    shader_params.emplace_back(param);
  }

  return shader_params;
}


static std::vector<ParamInfo> get_blk_shader_params(const DataBlock *shaderBlk)
{
  // shader is the root group
  return get_blk_shader_params_of_group(shaderBlk, std::wstring());
}


static ParamInfo get_param_info(const DataBlock *dataBlk, const std::wstring classname, const std::wstring param_name)
{
  if (!dataBlk || classname.empty() || param_name.empty())
    return ParamInfo();

  ParamInfo param;
  param.type = DataBlock::ParamType::TYPE_STRING;
  param.name = param_name;

  const DataBlock *shader_blk = get_blk_shader(dataBlk, classname);
  if (!shader_blk)
    return param;

  std::vector<ParamInfo> params = get_blk_shader_params(shader_blk);
  auto it = std::find_if(params.begin(), params.end(),
    [&param_name](const ParamInfo &p_i) { return !p_i.is_group && iequal(p_i.name, param_name); });
  if (it == params.end())
    return param;

  return *it;
}

static DataBlock::ParamType guess_blk_type_by_value(const std::wstring &value)
{
  if (iequal(value, L"yes") || iequal(value, L"true") || iequal(value, L"no") || iequal(value, L"false"))
    return DataBlock::ParamType::TYPE_BOOL;

  int ix, iy, iz, consumed;
  if (3 == swscanf(value.data(), L"%i, %i, %i%n", &ix, &iy, &iz, &consumed) && consumed == value.length())
    return DataBlock::ParamType::TYPE_IPOINT3;
  if (2 == swscanf(value.data(), L"%i, %i%n", &ix, &iy, &consumed) && consumed == value.length())
    return DataBlock::ParamType::TYPE_IPOINT2;
  if (1 == swscanf(value.data(), L"%i%n", &ix, &consumed) && consumed == value.length())
    return DataBlock::ParamType::TYPE_INT;

  float x, y, z, w;
  if (4 == swscanf(value.data(), L"%f, %f, %f, %f", &x, &y, &z, &w))
    return DataBlock::ParamType::TYPE_POINT4;
  if (3 == swscanf(value.data(), L"%f, %f, %f", &x, &y, &z))
    return DataBlock::ParamType::TYPE_POINT3;
  if (2 == swscanf(value.data(), L"%f, %f", &x, &y))
    return DataBlock::ParamType::TYPE_POINT2;
  if (1 == swscanf(value.data(), L"%f", &x))
    return DataBlock::ParamType::TYPE_REAL;

  return DataBlock::ParamType::TYPE_STRING;
}

static ParamInfo get_blk_param_info_value(const DataBlock *dataBlk, const std::wstring &line, const std::wstring &classname)
{
  ParamInfo res;

  // split the line into name and value: atest=1 --> { atest, 1 }
  std::vector<std::wstring> tokens = split(line, L'=');
  if (tokens.size() != 2)
    return res; // invalid input

  res.name = tokens[0];
  if (res.name.empty())
    return res;

  // backup value
  std::wstring value = tokens[1];

  // name itself might contain type: atest:f --> { atest, f }
  tokens = split(res.name, L':');
  if (tokens.size() == 2)
  {
    // use specified type
    res.type = DataBlock::deserialize_param_type(wideToStr(tokens[1].data()));
    res.name = tokens[0]; // type stripped
  }
  else // read type from the config
    res = get_param_info(dataBlk, classname, res.name);

  // override value
  res.value = value;

  // special case
  if (res.name == REAL_TWO_SIDED)
    res.type = DataBlock::ParamType::TYPE_BOOL;

  if (res.type == DataBlock::ParamType::TYPE_NONE)
    res.type = guess_blk_type_by_value(res.value);

  return res;
}

static std::vector<ParamInfo> get_blk_params(const DataBlock *dataBlk, const std::wstring &_script, const std::wstring &classname)
{
  std::vector<std::wstring> lines = split(simplifyRN(_script), L'\n');

  std::vector<ParamInfo> params;
  params.reserve(lines.size());

  bool has_real_two_sided = false;

  for (std::wstring &line : lines)
  {
    if (line.empty())
      continue;

    auto param = get_blk_param_info_value(dataBlk, line, classname);
    if (param.name.empty())
      continue;

    if (param.name == REAL_TWO_SIDED)
      has_real_two_sided = true;

    params.emplace_back(param);
  }

  if (!has_real_two_sided)
  {
    ParamInfo param;
    param.name = REAL_TWO_SIDED;
    param.type = DataBlock::ParamType::TYPE_BOOL;
    param.value = L"no";
    params.emplace_back(param);
  }

  return params;
}

/////////////////////////////////////////////////////////////////////////////////

class Dagormat2Dialog;

class NewParameterDialog
{
public:
  NewParameterDialog(Dagormat2Dialog *p, M_STD_STRING &shdr);
  virtual ~NewParameterDialog();

  const std::vector<std::wstring> &GetNewParamNames() const { return selected_names; }

  int DoModal();
  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  std::vector<std::wstring> GetSelectedNames() const;
  bool GetAndVerifyParamName();

  HWND hWnd;
  Dagormat2Dialog *parent;
  M_STD_STRING shader;
  std::vector<std::wstring> selected_names;
};

class ShaderClassDialog
{
public:
  ShaderClassDialog(Dagormat2Dialog *p);
  virtual ~ShaderClassDialog();

  const std::wstring &GetShaderClassName() const { return shader_class_name; }

  int DoModal();
  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  bool GetName();

  HWND hWnd;
  Dagormat2Dialog *parent;

  std::wstring shader_class_name;
};

/////////////////////////////////////////////////////////////////////////////////

class AbstractWidget : public ParamDlg
{
public:
  HWND hPanel;
  HWND hSType;
  BOOL creating;

  ParamInfo param;
  RECT screen_rc; // screen (not dialog!) coordinates relative to the parent window

  AbstractWidget(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~AbstractWidget() override;

  void ReloadDialog() override {}
  Class_ID ClassID() override { return DagorMat2_CID; }
  void SetThing(ReferenceTarget *m) override {}
  ReferenceTarget *GetThing() override { return NULL; }
  void DeleteThis() override { delete this; }
  void SetTime(TimeValue t) override {}
  void ActivateDlg(BOOL onOff) override {}

  virtual const TCHAR *GetType() const = 0;
  virtual bool IsGroup() const { return false; }

protected:
  Dagormat2Dialog *parent;
};

class WidgetText : public AbstractWidget
{
public:
  WidgetText(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetText() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("t"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING &v);

  ICustEdit *edit;
};

class WidgetColor : public AbstractWidget
{
public:
  WidgetColor(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetColor() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("c"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING &v);

  IColorSwatch *col;
};

class WidgetBool : public AbstractWidget
{
public:
  WidgetBool(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetBool() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("b"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING &v);
};

class WidgetNumeric : public AbstractWidget
{
public:
  WidgetNumeric(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetNumeric() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override
  {
    static const TCHAR *t[2][5] = {{0, L"i", L"ip2", L"ip3", L"ip4"}, {0, L"r", L"p2", L"p3", L"p4"}};
    return t[is_float][size];
  }

  bool isOutOfRange() const;

private:
  void GetValue();
  void SetValue(const M_STD_STRING &v);
  void UpdateColorSwatch(float f[4]);

  bool is_float;
  bool is_color_ui;
  bool is_spinner_used;
  int size;
  ISpinnerControl *spinner[4];
  IColorSwatch *col;
  HPEN hFramePen;
};

class WidgetGroup : public AbstractWidget
{
public:
  WidgetGroup(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetGroup() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("~"); }
  bool IsGroup() const override { return true; }
};

///////////////////////////////////////////////////////////////////////////

class Dagormat2Dialog : public ParamDlg
{
public:
  void DialogsReposition();
  HWND hwmedit;
  IMtlParams *ip;
  class DagorMat2 *theMtl;
  HWND hShader, hPhong, hTex, hParam, hProxy;
  bool valid;
  bool isActive;
  IColorSwatch *csa, *csd, *css, *cse;
  bool creating;
  TexDADMgr dadmgr;

  std::array<ICustButton *, NUMTEXMAPS> texbut;
  POINT paramOrg;
  HPEN hFramePen;

  std::vector<std::unique_ptr<AbstractWidget>> parameters;
  std::unique_ptr<DataBlock> blk;

  WStr proxyPath;
  FilterList filterList;
  static const TCHAR *defaultExtension;

  Dagormat2Dialog(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m);
  ~Dagormat2Dialog() override;

  void RestoreParams();
  void SaveParams();

  void FillSlotNames();

  M_STD_STRING GetShaderName();
  void UpdateShaderNameWidget();
  void ChangeShaderName();
  void Update2SidedWidget();
  void AddParam(ParamInfo param);
  AbstractWidget *GetParam(const M_STD_STRING &name);
  void RemParam(const M_STD_STRING &name);
  void RemParamGroup(const M_STD_STRING &group_name);
  void MarkUnknownParams(const DataBlock *dataBlk);
  std::vector<RECT> GetParamGroupRectangles() const;

  void Invalidate();
  void UpdateMtlDisplay() { ip->MtlChanged(); }
  void UpdateTexDisplay(int i);

  // methods inherited from ParamDlg:
  int FindSubTexFromHWND(HWND hw) override;
  void ReloadDialog() override;
  Class_ID ClassID() override { return DagorMat2_CID; }
  BOOL KeyAtCurTime(int id);
  void SetThing(ReferenceTarget *m) override;
  ReferenceTarget *GetThing() override { return (ReferenceTarget *)theMtl; }
  void DeleteThis() override { delete this; }
  void SetTime(TimeValue t) override { Invalidate(); }
  void ActivateDlg(BOOL onOff) override
  {
    csa->Activate(onOff);
    csd->Activate(onOff);
    css->Activate(onOff);
    cse->Activate(onOff);
  }

  HWND AppendDialog(const TCHAR *paramName, long Resource, DLGPROC lpDialogProc, LPARAM param);

  void ImportProxymat();
  void ExportProxymat();
};

static const size_t MAX_PARAM_DLGS = 20;
static const int PARAM_DLG_GAP = 7;

//////////////////////////////////////////////////////////////////////////////////

class DagorMat2PostLoadCallback : public PostLoadCallback
{
public:
  DagorMat2 *parent;

  DagorMat2PostLoadCallback() { parent = NULL; }

  void proc(ILoad *iload) override;
};


class DagorMat2 : public Mtl, public IDagorMat2
{
public:
  class Dagormat2Dialog *dlg;
  IParamBlock *pblock;
  Texmaps *texmaps;
  std::array<TexHandle *, NUMTEXMAPS> texHandle;
  Interval ivalid;
  float shin;
  float power;
  Color cola, cold, cols, cole;
  Sides twosided;
  TSTR classname, script;
  DagorMat2PostLoadCallback postLoadCallback;
  static ToolTipExtender tooltip;

  DagorMat2(BOOL loading);
  ~DagorMat2() override;
  void NotifyChanged();

  void *GetInterface(ULONG) override;
  void ReleaseInterface(ULONG, void *) override;

  // From IDagorMat
  Color get_amb() override;
  Color get_diff() override;
  Color get_spec() override;
  Color get_emis() override;
  float get_power() override;
  IDagorMat::Sides get_2sided() override;
  const TCHAR *get_classname() override;
  const TCHAR *get_script() override;
  const TCHAR *get_texname(int) override;
  float get_param(int) override;

  void set_amb(Color) override;
  void set_diff(Color) override;
  void set_spec(Color) override;
  void set_emis(Color) override;
  void set_power(float) override;
  void set_2sided(Sides) override;
  void set_classname(const TCHAR *) override;
  void set_script(const TCHAR *) override;
  void set_texname(int, const TCHAR *) override;
  void set_param(int, float) override;

  // From IDagorMat2
  void enumerate_textures(EnumTexCB &) override;
  void enumerate_parameters(EnumParamCB &) override;

  // From MtlBase and Mtl
  int NumSubTexmaps() override;
  Texmap *GetSubTexmap(int) override;
  void SetSubTexmap(int i, Texmap *m) override;

  void SetAmbient(Color c, TimeValue t) override;
  void SetDiffuse(Color c, TimeValue t) override;
  void SetSpecular(Color c, TimeValue t) override;
  void SetShininess(float v, TimeValue t) override;

  Color GetAmbient(int mtlNum = 0, BOOL backFace = FALSE) override;
  Color GetDiffuse(int mtlNum = 0, BOOL backFace = FALSE) override;
  Color GetSpecular(int mtlNum = 0, BOOL backFace = FALSE) override;
  float GetXParency(int mtlNum = 0, BOOL backFace = FALSE) override;
  float GetShininess(int mtlNum = 0, BOOL backFace = FALSE) override;
  float GetShinStr(int mtlNum = 0, BOOL backFace = FALSE) override;

  ParamDlg *CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) override;

  void Shade(ShadeContext &sc) override;
  void Update(TimeValue t, Interval &valid) override;
  void Reset() override;
  Interval Validity(TimeValue t) override;

  Class_ID ClassID() override;
  SClass_ID SuperClassID() override;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  void GetClassName(TSTR &s, bool localized) const override { s = GetString(IDS_DAGORMAT2); }
#else
  void GetClassName(TSTR &s) { s = GetString(IDS_DAGORMAT2); }
#endif

  void DeleteThis() override;

  ULONG Requirements(int subMtlNum) override;
  ULONG LocalRequirements(int subMtlNum) override;
  int NumSubs() override;
  Animatable *SubAnim(int i) override;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  MSTR SubAnimName(int i, bool localized) override;
#else
  TSTR SubAnimName(int i) override;
#endif
  int SubNumToRefNum(int subNum) override;

  // From ref
  int NumRefs() override;
  RefTargetHandle GetReference(int i) override;
  void SetReference(int i, RefTargetHandle rtarg) override;

  IOResult Save(ISave *isave) override;
  IOResult Load(ILoad *iload) override;

  RefTargetHandle Clone(RemapDir &remap) override;

#if defined(MAX_RELEASE_R17) && MAX_RELEASE >= MAX_RELEASE_R17
  RefResult NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
    BOOL propagate) override;
#else
  RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message) override;
#endif

  BOOL SupportTexDisplay() override;
  BOOL SupportsMultiMapsInViewport() override;
  void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb) override;
  void ActivateTexDisplay(BOOL onoff) override;
  void DiscardTexHandles();
  void updateViewportTexturesState();
  bool hasAlpha();
};

ToolTipExtender DagorMat2::tooltip;

static void setToolTip(HWND hWnd, const std::wstring &hint, HWND hExclude)
{
  if (hWnd == hExclude)
    return;

  DagorMat2::tooltip.SetToolTip(hWnd, hint.data());
  for (HWND hChild = GetWindow(hWnd, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
    setToolTip(hChild, hint, hExclude);
}

#define PB_REF  0
#define TEX_REF 9

enum
{
  PB_AMB,
  PB_DIFF,
  PB_SPEC,
  PB_EMIS,
  PB_SHIN,
};

class MaterClassDesc2 : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading) override { return new DagorMat2(loading); }
  const TCHAR *ClassName() override { return GetString(IDS_DAGORMAT2_LONG); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return MATERIAL_CLASS_ID; }
  Class_ID ClassID() override { return DagorMat2_CID; }
  const TCHAR *Category() override { return _T(""); }
};
static MaterClassDesc2 materCD;
ClassDesc *GetMaterCD2() { return &materCD; }

class TexmapsClassDesc2 : public ClassDesc
{
public:
  int IsPublic() override { return 0; }
  void *Create(BOOL loading) override { return new Texmaps; }
  const TCHAR *ClassName() override { return _T("DagorTexmaps"); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return TEXMAP_CONTAINER_CLASS_ID; }
  Class_ID ClassID() override { return Texmaps_CID; }
  const TCHAR *Category() override { return _T(""); }
};
static TexmapsClassDesc2 texmapsCD;
ClassDesc *GetTexmapsCD2() { return &texmapsCD; }

//--- MaterDlg2 ------------------------------------------------------

static const int texidc[NUMTEXMAPS] = {
  IDC_TEX0,
  IDC_TEX1,
  IDC_TEX2,
  IDC_TEX3,
  IDC_TEX4,
  IDC_TEX5,
  IDC_TEX6,
  IDC_TEX7,
  IDC_TEX8,
  IDC_TEX9,
  IDC_TEX10,
  IDC_TEX11,
  IDC_TEX12,
  IDC_TEX13,
  IDC_TEX14,
  IDC_TEX15,
};

static INT_PTR CALLBACK ShaderDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dagormat2Dialog *dlg = 0;
  if (msg == WM_INITDIALOG)
  {
    dlg = (Dagormat2Dialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hShader = hWnd;
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
    {
      dlg->Update2SidedWidget();
      dlg->UpdateShaderNameWidget();
      dlg->theMtl->NotifyChanged();
    }
    break;

    case WM_PAINT:
      if (!dlg->valid)
      {
        dlg->valid = TRUE;
        dlg->ReloadDialog();
      }
      return FALSE;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_BACKFACE_1, BN_CLICKED))
      {
        dlg->theMtl->twosided = IDagorMat::Sides::OneSided;
        dlg->theMtl->NotifyChanged();
        dlg->SaveParams();
        dlg->UpdateMtlDisplay();
      }

      else if (wParam == MAKEWPARAM(IDC_BACKFACE_2, BN_CLICKED))
      {
        dlg->theMtl->twosided = IDagorMat::Sides::DoubleSided;
        dlg->theMtl->NotifyChanged();
        dlg->SaveParams();
        dlg->UpdateMtlDisplay();
      }

      else if (wParam == MAKEWPARAM(IDC_BACKFACE_REAL2, BN_CLICKED))
      {
        dlg->theMtl->twosided = IDagorMat::Sides::RealDoubleSided;
        dlg->theMtl->NotifyChanged();
        dlg->SaveParams();
        dlg->UpdateMtlDisplay();
      }

      else if (wParam == MAKEWPARAM(IDC_CLASSNAME, BN_CLICKED))
      {
        ShaderClassDialog shader_class_dlg(dlg);
        if (shader_class_dlg.DoModal() == IDOK)
        {
          std::wstring name = shader_class_dlg.GetShaderClassName();
          SetWindowText(GetDlgItem(hWnd, IDC_CLASSNAME), name.c_str());
          dlg->ChangeShaderName();
        }
      }

      break;

    default: return FALSE;
  }

  return TRUE;
}

static INT_PTR CALLBACK TexDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dagormat2Dialog *dlg = 0;
  if (msg == WM_INITDIALOG)
  {
    dlg = (Dagormat2Dialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hTex = hWnd;
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
      for (size_t i = 0; i < dlg->texbut.size(); ++i)
      {
        dlg->texbut[i] = GetICustButton(GetDlgItem(hWnd, texidc[i]));
        dlg->texbut[i]->SetDADMgr(&dlg->dadmgr);
      }
      dlg->FillSlotNames();
      break;

    case WM_PAINT:
      if (!dlg->valid)
      {
        dlg->valid = TRUE;
        dlg->ReloadDialog();
      }
      return FALSE;

    case WM_COMMAND:
      for (size_t i = 0; i < NUMTEXMAPS; ++i)
        if (LOWORD(wParam) == texidc[i])
          PostMessage(dlg->hwmedit, WM_TEXMAP_BUTTON, i, (LPARAM)dlg->theMtl);
      break;

    default: return FALSE;
  }

  return TRUE;
}

static INT_PTR CALLBACK ParamDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dagormat2Dialog *dlg = 0;
  if (msg == WM_INITDIALOG)
  {
    dlg = (Dagormat2Dialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hParam = hWnd;
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
    {
      RECT rc;
      GetWindowRect(GetDlgItem(hWnd, IDC_PARAM_START), &rc);
      dlg->paramOrg.x = rc.left;
      dlg->paramOrg.y = rc.top;
      ScreenToClient(hWnd, &dlg->paramOrg);
      dlg->DialogsReposition();
    }
    break;

    case WM_SHOWWINDOW:
      if (wParam)
        dlg->RestoreParams();
      else
        dlg->SaveParams();
      break;

    case WM_PAINT:
      if (!dlg->valid)
      {
        dlg->valid = TRUE;
        dlg->ReloadDialog();
      }
      {
        std::vector<RECT> rect = dlg->GetParamGroupRectangles();
        if (!rect.empty())
        {
          PAINTSTRUCT ps;
          HDC hdc = BeginPaint(hWnd, &ps);
          float kx = float(GetDeviceCaps(hdc, LOGPIXELSX)) / 96.0f;
          float ky = float(GetDeviceCaps(hdc, LOGPIXELSY)) / 96.0f;
          HPEN hOldPen = (HPEN)SelectObject(hdc, dlg->hFramePen);
          HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
          HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

          for (RECT &rc : rect)
          {
            static const int GAP_X = 2, GAP_Y = 2;
            Rectangle(hdc, rc.left - GAP_X * kx, rc.top - GAP_Y * ky, rc.right + GAP_X * kx, rc.bottom + GAP_Y * ky);
          }

          SelectObject(hdc, hOldPen);
          SelectObject(hdc, hOldBrush);
          EndPaint(hWnd, &ps);
        }
      }
      return FALSE;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_NEW, BN_CLICKED))
      {
        M_STD_STRING shader = dlg->GetShaderName();
        NewParameterDialog new_par(dlg, shader);
        if (new_par.DoModal() == IDOK)
        {
          const auto &names = new_par.GetNewParamNames();
          for (const auto &name : names)
          {
            ParamInfo param = get_param_info(dlg->blk.get(), shader, name);
            dlg->theMtl->script += param.name.data();
            dlg->theMtl->script += L"=";
            dlg->theMtl->script += param.value.data();
            dlg->theMtl->script += L"\r\n";
          }
          dlg->RestoreParams();
        }
      }
      else if (wParam == MAKEWPARAM(IDC_PARAM_DELETE_ALL, BN_CLICKED) &&
               MessageBox(hWnd, L"Delete all parameters? Are you sure?", L"Delete all", MB_ICONQUESTION | MB_YESNO) == IDYES)
      {
        dlg->parameters.clear();
        dlg->SaveParams();
        dlg->RestoreParams();
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

static INT_PTR CALLBACK PhongDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dagormat2Dialog *dlg = 0;
  if (msg == WM_INITDIALOG)
  {
    dlg = (Dagormat2Dialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hPhong = hWnd;
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
      dlg->csa = GetIColorSwatch(GetDlgItem(hWnd, IDC_AMB), dlg->theMtl->cola, GetString(IDS_AMB_COLOR));
      dlg->csd = GetIColorSwatch(GetDlgItem(hWnd, IDC_DIFF), dlg->theMtl->cold, GetString(IDS_DIFF_COLOR));
      dlg->css = GetIColorSwatch(GetDlgItem(hWnd, IDC_SPEC), dlg->theMtl->cols, GetString(IDS_SPEC_COLOR));
      dlg->cse = GetIColorSwatch(GetDlgItem(hWnd, IDC_EMIS), dlg->theMtl->cole, GetString(IDS_EMIS_COLOR));
      break;

    case WM_PAINT:
      if (!dlg->valid)
      {
        dlg->valid = TRUE;
        dlg->ReloadDialog();
      }
      return FALSE;

    case CC_COLOR_BUTTONDOWN: theHold.Begin(); break;

    case CC_COLOR_BUTTONUP:
      if (HIWORD(wParam))
        theHold.Accept(GetString(IDS_COLOR_CHANGE));
      else
        theHold.Cancel();
      break;

    case CC_COLOR_CHANGE:
    {
      int id = LOWORD(wParam), pbid = -1;
      IColorSwatch *cs = NULL;
      switch (id)
      {
        case IDC_AMB:
          cs = dlg->csa;
          pbid = PB_AMB;
          break;

        case IDC_DIFF:
          cs = dlg->csd;
          pbid = PB_DIFF;
          break;

        case IDC_SPEC:
          cs = dlg->css;
          pbid = PB_SPEC;
          break;

        case IDC_EMIS:
          cs = dlg->cse;
          pbid = PB_EMIS;
          break;

        default: return TRUE;
      }
      int buttonUp = HIWORD(wParam);
      if (buttonUp)
        theHold.Begin();
      dlg->theMtl->pblock->SetValue(pbid, dlg->ip->GetTime(), (Color &)Color(cs->GetColor()));
      cs->SetKeyBrackets(dlg->KeyAtCurTime(pbid));
      if (buttonUp)
      {
        theHold.Accept(GetString(IDS_COLOR_CHANGE));
        dlg->UpdateMtlDisplay();
        dlg->theMtl->NotifyChanged();
      }
    }
    break;

    case CC_SPINNER_CHANGE:
      if (!theHold.Holding())
        theHold.Begin();
      if (LOWORD(wParam) == IDC_SHIN_S)
      {
        ISpinnerControl *spin = (ISpinnerControl *)lParam;
        dlg->theMtl->pblock->SetValue(PB_SHIN, dlg->ip->GetTime(), spin->GetFVal());
        spin->SetKeyBrackets(dlg->KeyAtCurTime(PB_SHIN));
      }
      break;

    case CC_SPINNER_BUTTONDOWN: theHold.Begin(); break;

    case WM_CUSTEDIT_ENTER:
    case CC_SPINNER_BUTTONUP:
      if (HIWORD(wParam) || msg == WM_CUSTEDIT_ENTER)
        theHold.Accept(GetString(IDS_PARAM_CHANGE));
      else
        theHold.Cancel();
      break;

    default: return FALSE;
  }

  return TRUE;
}

static INT_PTR CALLBACK ProxyDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dagormat2Dialog *dlg = 0;
  if (msg == WM_INITDIALOG)
  {
    dlg = (Dagormat2Dialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hProxy = hWnd;
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_PAINT:
      if (!dlg->valid)
      {
        dlg->valid = TRUE;
        dlg->ReloadDialog();
      }
      return FALSE;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PROXY_IMPORT, BN_CLICKED))
      {
        if (get_open_filename(dlg->hProxy, L"Import from proxymat...", dlg->filterList, dlg->defaultExtension, dlg->proxyPath))
          dlg->ImportProxymat();
      }
      else if (wParam == MAKEWPARAM(IDC_PROXY_EXPORT, BN_CLICKED))
      {
        if (get_save_filename(dlg->hProxy, L"Export to proxymat...", dlg->filterList, dlg->defaultExtension, dlg->proxyPath, false))
          dlg->ExportProxymat();
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

const TCHAR *Dagormat2Dialog::defaultExtension = L"proxymat.blk";

Dagormat2Dialog::Dagormat2Dialog(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m) : isActive(false), csa(0), csd(0), css(0), cse(0)
{
  dadmgr.Init(this);
  hwmedit = hwMtlEdit;
  ip = imp;
  theMtl = m;
  valid = FALSE;
  paramOrg.x = paramOrg.y = 0;

  blk = get_blk();

  filterList.Append(_T("Proxymat (proxymat.blk)"));
  filterList.Append(_T("*.proxymat.blk"));

  creating = TRUE;
  std::fill(texbut.begin(), texbut.end(), nullptr);
  hShader = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2_SHADER), ShaderDlgProc, L"Shader class", (LPARAM)this);
  hPhong = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2_PHONG), PhongDlgProc, L"Phong parameters", (LPARAM)this);
  hTex = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2_TEX), TexDlgProc, L"Texture slots", (LPARAM)this);
  hParam = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2_PARAM), ParamDlgProc, L"Shader parameters", (LPARAM)this);
  hProxy = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2_PROXY), ProxyDlgProc, L"Proxymat import/export", (LPARAM)this);
  creating = FALSE;

  hFramePen = (HPEN)CreatePen(PS_SOLID, 2, RGB(56, 56, 56));
}

Dagormat2Dialog::~Dagormat2Dialog()
{
  SaveParams();
  ReleaseIColorSwatch(csa);
  ReleaseIColorSwatch(csd);
  ReleaseIColorSwatch(css);
  ReleaseIColorSwatch(cse);
  for (ICustButton *cb : texbut)
    ReleaseICustButton(cb);
  SetWindowLongPtr(hShader, GWLP_USERDATA, NULL);
  SetWindowLongPtr(hTex, GWLP_USERDATA, NULL);
  SetWindowLongPtr(hParam, GWLP_USERDATA, NULL);
  SetWindowLongPtr(hPhong, GWLP_USERDATA, NULL);

  DeleteObject(hFramePen);
}

void Dagormat2Dialog::MarkUnknownParams(const DataBlock *dataBlk)
{
  if (!dataBlk)
    return;

  M_STD_STRING shader_name = GetShaderName();
  if (shader_name.empty())
    return;

  const DataBlock *shader_blk = get_blk_shader(dataBlk, shader_name);
  if (!shader_blk)
    return;

  std::vector<ParamInfo> params = get_blk_shader_params(shader_blk);

  for (auto &p : parameters)
  {
    HWND hDel = GetDlgItem(p->hPanel, IDC_PARAM_DELETE);

    if (p->param.is_group)
    {
      DagorMat2::tooltip.SetToolTip(hDel, (L"Delete group " + p->param.name).data());
      continue;
    }
    else
      DagorMat2::tooltip.SetToolTip(hDel, (L"Delete parameter " + p->param.name).data());

    auto it = std::find_if(params.begin(), params.end(), [&p](ParamInfo &p_i) { return iequal(p_i.name, p->param.name); });

    // known parameter
    if (it != params.end())
    {
      std::wstring hint = it->name;

      if (!it->description.empty())
      {
        hint += L"\n";
        hint += it->description;
      }

      if (it->def_enabled)
      {
        hint += L"\nDefault: ";
        hint += it->def;
      }
      else
        hint += L"\nDefault is unknown";

      if (it->soft_min_enabled || it->soft_max_enabled)
      {
        hint += L"\nExpected range: [";

        if (it->soft_min_enabled)
        {
          if (it->isInt())
            hint += std::to_wstring(int(it->soft_min));
          else
            hint += std::to_wstring(it->soft_min);
        }

        hint += L"..";

        if (it->soft_max_enabled)
        {
          if (it->isInt())
            hint += std::to_wstring(int(it->soft_max));
          else
            hint += std::to_wstring(it->soft_max);
        }

        hint += L"]";

        WidgetNumeric *wn = dynamic_cast<WidgetNumeric *>(p.get());
        if (wn && wn->isOutOfRange())
          hint += L"\nOUT OF RANGE!";
      }

      setToolTip(p->hPanel, hint, hDel);
      continue;
    }

    // unknown parameter
    if (p->hSType)
      ::SetWindowText(p->hSType, (L'*' + p->param.name).data());

    std::wstringstream ss;
    ss << p->param.name << _T("\n*not specified in the shader\n") << shader_name;
    setToolTip(p->hPanel, ss.str(), hDel);
  }
}


std::vector<RECT> Dagormat2Dialog::GetParamGroupRectangles() const
{
  std::vector<RECT> rect;
  rect.reserve(parameters.size()); // rough estimation

  RECT rc;
  bool collect_vertical_size = false;

  for (int i = 0;;)
  {
    if (collect_vertical_size)
    {
      if (i >= parameters.size())
      {
        rect.emplace_back(rc);
        break;
      }

      AbstractWidget *w = parameters[i].get();
      if (w->IsGroup())
      {
        rect.emplace_back(rc);
        collect_vertical_size = false;
      }
      else
      {
        rc.bottom = w->screen_rc.bottom;
        ++i;
      }
    }
    else // search next group widget
    {
      if (i >= parameters.size())
        break;

      AbstractWidget *w = parameters[i].get();
      if (w->IsGroup())
      {
        rc = w->screen_rc;
        collect_vertical_size = true;
      }

      ++i;
    }
  }

  return rect;
}


struct FindParamInfo
{
  ParamInfo pinfo;
  bool found;
};

static FindParamInfo find_param_info(std::vector<ParamInfo> &params, const std::wstring &name)
{
  FindParamInfo res;
  auto it = std::find_if(params.begin(), params.end(), [&name](const ParamInfo &p) { return !p.is_group && p.name == name; });
  res.found = it != params.end();
  if (res.found)
    res.pinfo = *it;
  return res;
}

void Dagormat2Dialog::RestoreParams()
{
  parameters.clear();
  DagorMat2::tooltip.RemoveToolTips();

  M_STD_STRING shader_name = theMtl->classname.data();
  if (shader_name.empty())
    return;

  // get list of shader parameters (including group names)
  const DataBlock *shader_blk = get_blk_shader(blk.get(), shader_name);
  std::vector<ParamInfo> all_params = get_blk_shader_params(shader_blk);

  // collect user-specified params
  std::vector<ParamInfo> user_params;
  M_STD_STRING script = simplifyRN(M_STD_STRING(theMtl->script));
  std::vector<std::wstring> lines = split(script, L'\n');
  for (std::wstring &line : lines)
    if (!line.empty())
      user_params.emplace_back(get_blk_param_info_value(blk.get(), line, shader_name));

  // find uncategorized parameters (those that do not have a parent group)
  std::vector<ParamInfo> uncategorized_params;
  for (const ParamInfo &param : all_params)
    if (!param.is_group && param.parent.empty())
    {
      auto res = find_param_info(user_params, param.name);
      if (res.found)
        uncategorized_params.emplace_back(res.pinfo);
    }

  if (!uncategorized_params.empty())
  {
    ParamInfo group;
    group.is_group = true;
    group.name = L"Uncategorized";
    parameters.emplace_back(std::unique_ptr<WidgetGroup>(new WidgetGroup(group, this)));
    for (ParamInfo param : uncategorized_params)
    {
      param.parent = group.name; // override empty parent group
      AddParam(param);
    }
  }

  // create widgets (if any) in the same order as defined in .blk
  for (const ParamInfo &p : all_params)
  {
    if (!p.is_group)
      continue;

    std::vector<ParamInfo> grouped_params;
    for (const ParamInfo &param : all_params)
      if (!param.is_group && param.parent == p.name)
      {
        auto res = find_param_info(user_params, param.name);
        if (res.found)
          grouped_params.emplace_back(res.pinfo);
      }

    if (!grouped_params.empty())
    {
      parameters.emplace_back(std::unique_ptr<WidgetGroup>(new WidgetGroup(p, this)));
      for (const ParamInfo &param : grouped_params)
        AddParam(param);
    }
  }

  // "real_two_sided" is treated as a radio button
  for (const ParamInfo &param : user_params)
    if (iequal(param.name, REAL_TWO_SIDED))
    {
      if (iequal(param.value, L"yes"))
        theMtl->twosided = IDagorMat::Sides::RealDoubleSided;
      Update2SidedWidget();
      break;
    }

  // collect unknown parameters
  std::vector<ParamInfo> unknown_params;
  for (const ParamInfo &param : user_params)
  {
    // skip real_two_sided
    if (iequal(param.name, REAL_TWO_SIDED))
      continue;

    // find parameter among pre-defined
    auto res = find_param_info(all_params, param.name);
    if (!res.found)
      unknown_params.emplace_back(param);
  }

  // make a new group and add the unknown parameters
  if (!unknown_params.empty())
  {
    ParamInfo group;
    group.is_group = true;
    group.name = L"Unknown";
    parameters.emplace_back(std::unique_ptr<WidgetGroup>(new WidgetGroup(group, this)));

    for (ParamInfo param : unknown_params)
    {
      param.parent = group.name; // override empty parent group
      AddParam(param);
    }
  }

  SaveParams();
  DialogsReposition();
  MarkUnknownParams(blk.get());
}

void Dagormat2Dialog::SaveParams()
{
  M_STD_STRING buffer;

  for (size_t i = 0; i < parameters.size(); ++i)
  {
    AbstractWidget *p = parameters[i].get();

    if (!p || p->param.is_group || p->param.name.empty())
      continue;

    buffer += p->param.name;
    buffer += _T('=');
    buffer += p->param.value;
    buffer += _T("\r\n");
  }

  if (IsDlgButtonChecked(hShader, IDC_BACKFACE_REAL2))
    buffer += _T("real_two_sided=yes\r\n");
  else
    buffer += _T("real_two_sided=no\r\n");

  theMtl->script = buffer.c_str();
  theMtl->NotifyChanged();
}

int Dagormat2Dialog::FindSubTexFromHWND(HWND hw)
{
  for (size_t i = 0; i < texbut.size(); ++i)
    if (texbut[i])
      if (texbut[i]->GetHwnd() == hw)
        return int(i);
  return -1;
}

M_STD_STRING Dagormat2Dialog::GetShaderName()
{
  M_STD_STRING name;
  HWND hwnd = GetDlgItem(hShader, IDC_CLASSNAME);
  long length = GetWindowTextLength(hwnd);
  if (length > 0)
  {
    name.resize(length);
    GetWindowText(hwnd, (TCHAR *)name.c_str(), length + 1);
  }
  return name;
}

void Dagormat2Dialog::UpdateShaderNameWidget() { SetWindowText(GetDlgItem(hShader, IDC_CLASSNAME), theMtl->classname.data()); }

void Dagormat2Dialog::ChangeShaderName()
{
  M_STD_STRING str = GetShaderName();
  theMtl->classname = str.c_str();
  UpdateShaderNameWidget();
  theMtl->NotifyChanged();
  SaveParams();
  RestoreParams();
  FillSlotNames();
  DialogsReposition();
}

void Dagormat2Dialog::Update2SidedWidget()
{
  CheckDlgButton(hShader, IDC_BACKFACE_1, theMtl->twosided == IDagorMat::Sides::OneSided);
  CheckDlgButton(hShader, IDC_BACKFACE_2, theMtl->twosided == IDagorMat::Sides::DoubleSided);
  CheckDlgButton(hShader, IDC_BACKFACE_REAL2, theMtl->twosided == IDagorMat::Sides::RealDoubleSided);
}

static std::wstring fix_empty_param(DataBlock::ParamType type, const std::wstring &value)
{
  return value.empty() ? get_default_param_value(type) : value;
}

void Dagormat2Dialog::AddParam(ParamInfo param)
{
  if (param.name.empty())
    return;

  param.value = fix_empty_param(param.type, param.value);

  AbstractWidget *par = nullptr;
  switch (param.type)
  {
    case DataBlock::ParamType::TYPE_BOOL: par = new WidgetBool(param, this); break;
    case DataBlock::ParamType::TYPE_E3DCOLOR: par = new WidgetColor(param, this); break;

    case DataBlock::ParamType::TYPE_INT:
    case DataBlock::ParamType::TYPE_REAL:
    case DataBlock::ParamType::TYPE_IPOINT2:
    case DataBlock::ParamType::TYPE_IPOINT3:
    case DataBlock::ParamType::TYPE_POINT2:
    case DataBlock::ParamType::TYPE_POINT3:
    case DataBlock::ParamType::TYPE_POINT4: par = new WidgetNumeric(param, this); break;

    default: par = new WidgetText(param, this); break;
  }

  parameters.emplace_back(std::unique_ptr<AbstractWidget>(par));
}

AbstractWidget *Dagormat2Dialog::GetParam(const M_STD_STRING &name)
{
  for (auto &p : parameters)
    if (iequal(p->param.name, name))
      return p.get();
  return nullptr;
}

void Dagormat2Dialog::RemParam(const M_STD_STRING &name)
{
  M_STD_STRING script = simplifyRN(M_STD_STRING(theMtl->script));

  // cut the script into lines
  std::vector<std::wstring> lines = split(script, L'\n');

  script.clear();

  for (const std::wstring &line : lines)
  {
    // fetch param name
    std::vector<std::wstring> tokens = split(line, L'=');
    if (tokens.size() != 2)
      continue; // invalid input

    std::wstring param_name = tokens[0];
    std::wstring param_value = tokens[1];
    tokens = split(param_name, L':');
    param_name = tokens[0];

    // ignore specified parameter
    if (iequal(param_name, name))
      continue;

    script += param_name;
    script += L'=';
    script += param_value;
    script += L"\r\n";
  }

  theMtl->script = script.data();
  theMtl->NotifyChanged();
  RestoreParams();
}

void Dagormat2Dialog::RemParamGroup(const M_STD_STRING &group_name)
{
  M_STD_STRING script = simplifyRN(M_STD_STRING(theMtl->script));

  // cut the script into lines
  std::vector<std::wstring> lines = split(script, L'\n');

  script.clear();

  for (const std::wstring &line : lines)
  {
    // fetch param name
    std::vector<std::wstring> tokens = split(line, L'=');
    if (tokens.size() != 2)
      continue; // invalid input

    std::wstring param_name = tokens[0];
    std::wstring param_value = tokens[1];
    tokens = split(param_name, L':');
    param_name = tokens[0];

    auto it = std::find_if(parameters.begin(), parameters.end(),
      [&param_name](const std::unique_ptr<AbstractWidget> &w) { return !w->param.is_group && w->param.name == param_name; });
    if (it != parameters.end() && (*it)->param.parent == group_name)
      continue;

    script += param_name;
    script += L'=';
    script += param_value;
    script += L"\r\n";
  }

  theMtl->script = script.data();
  theMtl->NotifyChanged();
  RestoreParams();
}

HWND Dagormat2Dialog::AppendDialog(const TCHAR *paramName, long Resource, DLGPROC lpDialogProc, LPARAM param)
{
  HWND result = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(Resource), hParam, lpDialogProc, param);

  AbstractWidget *dlg = (WidgetText *)GetWindowLongPtr(result, GWLP_USERDATA);

  ::SetWindowText(result, paramName);
  dlg->hSType = ::FindWindowEx(result, NULL, _T("STATIC"), _T("StaticType"));
  if (dlg->hSType)
    ::SetWindowText(dlg->hSType, paramName);
  ::ShowWindow(result, SW_SHOW);

  dlg->creating = false;
  return result;
}

void Dagormat2Dialog::FillSlotNames()
{
  // reserved for future update
}

void Dagormat2Dialog::Invalidate()
{
  valid = FALSE;
  isActive = FALSE;
  Rect rect;
  rect.left = rect.top = 0;
  rect.right = rect.bottom = 10;
  InvalidateRect(hParam, &rect, FALSE);
}

void Dagormat2Dialog::SetThing(ReferenceTarget *m)
{
  theMtl = (DagorMat2 *)m;
  if (theMtl)
    theMtl->dlg = this;
  ReloadDialog();
}

BOOL Dagormat2Dialog::KeyAtCurTime(int id) { return theMtl->pblock->KeyFrameAtTime(id, ip->GetTime()); }

void Dagormat2Dialog::UpdateTexDisplay(int i)
{
  Texmap *t = theMtl->texmaps->gettex(i);
  TSTR nm;
  if (t)
#if defined(MAX_RELEASE_R27) && MAX_RELEASE >= MAX_RELEASE_R27
    nm = t->GetFullName(false).data();
#else
    nm = t->GetFullName();
#endif
  else
    nm = GetString(IDS_NONE);
  texbut[i]->SetText(nm);
}

void Dagormat2Dialog::ReloadDialog()
{
  Interval valid;
  TimeValue time = ip->GetTime();
  theMtl->Update(time, valid);
  csa->SetColor(theMtl->cola);
  csa->SetKeyBrackets(KeyAtCurTime(PB_AMB));
  csd->SetColor(theMtl->cold);
  csd->SetKeyBrackets(KeyAtCurTime(PB_DIFF));
  css->SetColor(theMtl->cols);
  css->SetKeyBrackets(KeyAtCurTime(PB_SPEC));
  cse->SetColor(theMtl->cole);
  cse->SetKeyBrackets(KeyAtCurTime(PB_EMIS));
  Update2SidedWidget();
  for (int i = 0; i < NUMTEXMAPS; ++i)
    UpdateTexDisplay(i);

  UpdateShaderNameWidget();
}

void Dagormat2Dialog::ImportProxymat()
{
  DataBlock proxyBlk(std::make_shared<NameMap>());
  if (!proxyBlk.load(proxyPath.data()))
    return;

  int classname_i = proxyBlk.findParam("class");
  if (classname_i != -1)
  {
    theMtl->classname = strToWide(proxyBlk.getStr(classname_i)).data();
    UpdateShaderNameWidget();
  }

  int twosided_i = proxyBlk.findParam("twosided");
  if (twosided_i != -1)
    theMtl->twosided = proxyBlk.getBool(twosided_i, false) ? IDagorMat::Sides::DoubleSided : IDagorMat::Sides::OneSided;

  std::wstring script;
  for (int script_i = -1; (script_i = proxyBlk.findParam("script", script_i)) != -1;)
    script += (strToWide(proxyBlk.getStr(script_i)) + L"\r\n");

  if (!script.empty())
    theMtl->script = script.data();

  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    int tex_i = proxyBlk.findParam(blk_tex_name(i));
    if (tex_i != -1)
    {
      theMtl->set_texname(i, strToWide(proxyBlk.getStr(tex_i)).data());
      UpdateTexDisplay(i);
    }
  }

  theMtl->NotifyChanged();
  RestoreParams();
  UpdateMtlDisplay();
}

void Dagormat2Dialog::ExportProxymat()
{
  DataBlock proxyBlk(std::make_shared<NameMap>());

  proxyBlk.addStr("class", wideToStr(theMtl->classname.data()).data());
  proxyBlk.addBool("tex16support", true);
  proxyBlk.addBool("twosided", theMtl->twosided == IDagorMat::Sides::DoubleSided);

  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    const TCHAR *tex = theMtl->get_texname(i);
    if (tex)
      proxyBlk.addStr(blk_tex_name(i), wideToStr(tex).data());
  }

  std::vector<std::wstring> lines = split(simplifyRN(theMtl->script.data()), L'\n');
  for (auto const &s : lines)
    if (!s.empty())
      proxyBlk.addStr("script", wideToStr(s.data()).data());

  proxyBlk.saveToTextFile(proxyPath.data());
}

//--- DagorMat2 -------------------------------------------------

#define VERSION 0

static ParamBlockDescID pbdesc[] = {
  {TYPE_RGBA, NULL, FALSE, PB_AMB},
  {TYPE_RGBA, NULL, FALSE, PB_DIFF},
  {TYPE_RGBA, NULL, FALSE, PB_SPEC},
  {TYPE_RGBA, NULL, FALSE, PB_EMIS},
  {TYPE_FLOAT, NULL, FALSE, PB_SHIN},
};


void DagorMat2PostLoadCallback::proc(ILoad *iload)
{
  parent->updateViewportTexturesState();

  Texmap *tex = parent->texmaps->gettex(0);
  if (tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
  {
    BitmapTex *bitmap = (BitmapTex *)tex;

    // debug("mat=0x%08x texmap loaded '%s'", (unsigned int)parent, bitmap->GetMapName());

    bitmap->SetAlphaSource(ALPHA_FILE);
    bitmap->SetAlphaAsMono(TRUE);
  }
}


DagorMat2::DagorMat2(BOOL loading)
{
  postLoadCallback.parent = this;
  dlg = NULL;
  pblock = NULL;
  texmaps = NULL;
  ivalid.SetEmpty();
  std::fill(texHandle.begin(), texHandle.end(), nullptr);

  // debug("mat=0x%08x created", (unsigned int)this);

  Reset();
}


DagorMat2::~DagorMat2()
{
  // debug("mat=0x%08x deleted", (unsigned int)this);

  DiscardTexHandles();
}


void DagorMat2::Reset()
{
  // debug("mat=0x%08x reseted", (unsigned int)this);

  ReplaceReference(TEX_REF, (Texmaps *)CreateInstance(TEXMAP_CONTAINER_CLASS_ID, Texmaps_CID));
  ReplaceReference(PB_REF, CreateParameterBlock(pbdesc, sizeof(pbdesc) / sizeof(pbdesc[0]), VERSION));
  pblock->SetValue(PB_AMB, 0, cola = Color(1, 1, 1));
  pblock->SetValue(PB_DIFF, 0, cold = Color(1, 1, 1));
  pblock->SetValue(PB_SPEC, 0, cols = Color(1, 1, 1));
  pblock->SetValue(PB_EMIS, 0, cole = Color(0, 0, 0));
  pblock->SetValue(PB_SHIN, 0, shin = 30.0f);
  twosided = Sides::OneSided;
  classname = DEFAULT_SHADER_NAME;
  script = _T("");
  updateViewportTexturesState();
}

ParamDlg *DagorMat2::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
  dlg = new Dagormat2Dialog(hwMtlEdit, imp, this);
  return dlg;
}


static Color blackCol(0, 0, 0);
static Color whiteCol(1.0f, 1.0f, 1.0f);

void DagorMat2::Shade(ShadeContext &sc)
{
  sc.out.c = blackCol;
  sc.out.t = blackCol;

  if (gbufID)
    sc.SetGBufferID(gbufID);

  if (sc.mode == SCMODE_SHADOW)
    return;

  LightDesc *l;
  Color lightCol;
  BOOL is_shiny = (cols != Color(0, 0, 0)) ? 1 : 0;
  Color diffIllum = blackCol, specIllum = blackCol;
  Point3 V = sc.V(), N = sc.Normal();
  for (int i = 0; i < sc.nLights; i++)
  {
    l = sc.Light(i);
    float NL, diffCoef;
    Point3 L;
    if (l->Illuminate(sc, N, lightCol, L, NL, diffCoef))
    {
      if (NL <= 0.0f)
        continue;
      if (l->affectDiffuse)
        diffIllum += diffCoef * lightCol;
      if (is_shiny && l->affectSpecular)
      {
        Point3 H = FNormalize(L - V);
        float c = DotProd(N, H);
        if (c > 0.0f)
        {
          c = powf(c, power);
          specIllum += c * lightCol;
        }
      }
    }
  }
  Color dc, ac;
  if (texmaps->texmap[0])
  {
    AColor mc = texmaps->texmap[0]->EvalColor(sc);

    if (!hasAlpha())
      mc.a = 1.f;

    Color c(mc.r * mc.a, mc.g * mc.a, mc.b * mc.a);
    sc.out.c = (diffIllum * cold + cole + cola * sc.ambientLight) * c + specIllum * cols;
    sc.out.t = Color(1.f - mc.a, 1.f - mc.a, 1.f - mc.a);
  }
  else
    sc.out.c = diffIllum * cold + cole + cola * sc.ambientLight + specIllum * cols;
}


BOOL DagorMat2::SupportTexDisplay() { return TRUE; }


BOOL DagorMat2::SupportsMultiMapsInViewport() { return TRUE; }


void DagorMat2::ActivateTexDisplay(BOOL onoff)
{
  if (!onoff)
  {
    DiscardTexHandles();
  }
}


bool DagorMat2::hasAlpha() { return _tcsstr(script, _T("atest" )) || _tcsstr(classname, _T("alpha")); }


void DagorMat2::updateViewportTexturesState()
{
  if (hasAlpha())
  {
    SetMtlFlag(MTL_TEX_DISPLAY_ENABLED);
    SetMtlFlag(MTL_HW_TEX_ENABLED);
  }
  else
  {
    if (texmaps->gettex(0))
    {
      SetActiveTexmap(texmaps->gettex(0));
      SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS);
      ClearMtlFlag(MTL_TEX_DISPLAY_ENABLED);
      // GetCOREInterface()->ForceCompleteRedraw();
    }
  }
}


void DagorMat2::DiscardTexHandles()
{
  for (auto &th : texHandle)
    if (th)
    {
      th->DeleteThis();
      th = nullptr;
    }
}

void DagorMat2::SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb)
{
  if (!texmaps->gettex(0))
    return;

  if (cb.NumberTexturesSupported() < 1)
    return;

  if (!texHandle[0])
  {
    Interval valid;
    BITMAPINFO *bmiColor = texmaps->gettex(0)->GetVPDisplayDIB(t, cb, valid, FALSE, 0, 0);
    if (!bmiColor)
      return;

    BITMAPINFO *bmiAlpha = texmaps->gettex(0)->GetVPDisplayDIB(t, cb, valid, TRUE, 0, 0);
    if (!bmiAlpha)
      return;

    UBYTE *bmiPtr = BMIDATA(bmiColor);
    UBYTE *bmiAlphaPtr = BMIDATA(bmiAlpha);
    for (unsigned int pixelNo = 0; pixelNo < bmiColor->bmiHeader.biWidth * bmiColor->bmiHeader.biHeight; pixelNo++)
    {
      bmiPtr[3] = (UBYTE)(((int)bmiAlphaPtr[0] + bmiAlphaPtr[1] + bmiAlphaPtr[2]) / 3);
      bmiPtr += 4;
      bmiAlphaPtr += 4;
    }

    TexHandle *tmpTexHandle = cb.MakeHandle(bmiAlpha);
    if (tmpTexHandle)
      tmpTexHandle->DeleteThis();

    texHandle[0] = cb.MakeHandle(bmiColor);
    if (!texHandle[0])
      return;


    // debug("mat=0x%08x creates tex ('%s', '%s', '%s')",
    //   (unsigned int)this,
    //   classname,
    //   GetName(),
    //   texmaps->gettex(0)->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00) ? ((BitmapTex*)texmaps->gettex(0))->GetMapName() : "???");
  }

  IHardwareMaterial *pIHWMat = (IHardwareMaterial *)GetProperty(PROPID_HARDWARE_MATERIAL);
  if (pIHWMat)
  {
    pIHWMat->SetNumTexStages(1);
    int texOp = TX_ALPHABLEND;
    pIHWMat->SetTexture(0, texHandle[0]->GetHandle());
    mtl->texture[0].useTex = 0;
    cb.GetGfxTexInfoFromTexmap(t, mtl->texture[0], texmaps->texmap[0]);
    pIHWMat->SetTextureColorArg(0, 1, D3DTA_TEXTURE);
    pIHWMat->SetTextureColorArg(0, 2, D3DTA_CURRENT);
    pIHWMat->SetTextureAlphaArg(0, 1, D3DTA_TEXTURE);
    pIHWMat->SetTextureAlphaArg(0, 2, D3DTA_CURRENT);
    pIHWMat->SetTextureColorOp(0, D3DTOP_SELECTARG1);
    pIHWMat->SetTextureAlphaOp(0, D3DTOP_SELECTARG1);
    pIHWMat->SetTextureTransformFlag(0, D3DTTFF_COUNT2);
  }
  else
  {
    cb.GetGfxTexInfoFromTexmap(t, mtl->texture[0], texmaps->texmap[0]);

    // mtl->Ka = Point3(1., 0.1, 0.1);
    // mtl->Kd = Point3(1., 0.2, 0.5);
    // mtl->Ks = Point3(1, 0, 0);
    // mtl->shininess = 0;
    // mtl->shinStrength = 0;
    // mtl->opacity = 0.5;
    // mtl->selfIllum = 0;
    // mtl->dblSided = FALSE;
    // mtl->shadeLimit = GW_TEXTURE | GW_TRANSPARENCY;

    mtl->texture[0].textHandle = texHandle[0]->GetHandle();
    mtl->texture[0].colorOp = GW_TEX_REPLACE;
    mtl->texture[0].colorAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].colorScale = GW_TEX_SCALE_1X;
    mtl->texture[0].alphaOp = GW_TEX_REPLACE;
    mtl->texture[0].alphaAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].alphaScale = GW_TEX_SCALE_1X;

    ////mtl->texture[0].colorOp           = GW_TEX_REPLACE;
    ////mtl->texture[0].colorAlphaSource  = GW_TEX_ZERO;
    ////mtl->texture[0].colorScale        = GW_TEX_SCALE_1X;
    ////mtl->texture[0].alphaOp           = GW_TEX_LEAVE;
    ////mtl->texture[0].alphaAlphaSource  = GW_TEX_TEXTURE;
    ////mtl->texture[0].alphaScale        = GW_TEX_SCALE_1X;
  }
}


ULONG DagorMat2::Requirements(int subm)
{
  ULONG r = MTLREQ_PHONG;
  if (twosided == IDagorMat::Sides::DoubleSided)
    r |= MTLREQ_2SIDE;
  for (int i = 0; i < NUMTEXMAPS; ++i)
    if (texmaps->texmap[i])
      r |= texmaps->texmap[i]->Requirements(subm);

  Texmap *tex = texmaps->gettex(0);
  if (tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
  {
    BitmapTex *bitmap = (BitmapTex *)tex;
    if (bitmap->GetAlphaSource() != ALPHA_NONE)
      r |= MTLREQ_TRANSP | MTLREQ_TRANSP_IN_VP;
  }
  return r;
}


ULONG DagorMat2::LocalRequirements(int subMtlNum) { return Requirements(subMtlNum); }


void DagorMat2::Update(TimeValue t, Interval &valid)
{
  ivalid = FOREVER;
  pblock->GetValue(PB_AMB, t, cola, ivalid);
  pblock->GetValue(PB_DIFF, t, cold, ivalid);
  pblock->GetValue(PB_SPEC, t, cols, ivalid);
  pblock->GetValue(PB_EMIS, t, cole, ivalid);
  pblock->GetValue(PB_SHIN, t, shin, ivalid);
  power = powf(2.0f, shin / 10.0f) * 4.0f;
  valid &= ivalid;


  // Texmap *tex = texmaps->gettex(0);
  // if (tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
  //{
  //   BitmapTex *bitmap = (BitmapTex*)tex;
  //   IParamBlock2 *pb = bitmap->GetParamBlock(0);
  //   if (pb)
  //   {
  //     int val[17];
  //     BOOL res[17];
  //     for (int i = 0; i < 17; i++)
  //     {
  //       int id = pb->IndextoID(i);
  //       Interval valid = FOREVER;
  //       res[i] = pb->GetValue(id, 0, val[i], valid);
  //     }
  //     int a = 1;
  //   }
  // }
}

Interval DagorMat2::Validity(TimeValue t)
{
  Interval valid = FOREVER;
  float f;
  Color c;
  pblock->GetValue(PB_AMB, t, c, valid);
  pblock->GetValue(PB_DIFF, t, c, valid);
  pblock->GetValue(PB_SPEC, t, c, valid);
  pblock->GetValue(PB_EMIS, t, c, valid);
  pblock->GetValue(PB_SHIN, t, f, valid);
  return valid;
}

int DagorMat2::NumSubTexmaps() { return NUMTEXMAPS; }

Texmap *DagorMat2::GetSubTexmap(int i) { return texmaps->gettex(i); }

void DagorMat2::SetSubTexmap(int i, Texmap *m)
{
  if (m)
  {
    m->SetMtlFlag(MTL_HW_TEX_ENABLED);
    if (m->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
    {
      BitmapTex *bitmap = (BitmapTex *)m;
      IParamBlock2 *pb = bitmap->GetParamBlock(0);
      if (pb)
        pb->SetValue(12, 0, 0);
      bitmap->SetAlphaSource(ALPHA_FILE);
      bitmap->SetAlphaAsMono(TRUE);
    }
  }
  texmaps->settex(i, m);
  NotifyChanged();
  if (dlg)
    dlg->UpdateTexDisplay(i);
  GetCOREInterface()->ForceCompleteRedraw();
}

int DagorMat2::NumSubs() { return 10; }

int DagorMat2::SubNumToRefNum(int subNum) { return subNum; }

int DagorMat2::NumRefs() { return 10; }

Animatable *DagorMat2::SubAnim(int i)
{
  switch (i)
  {
    case PB_REF: return pblock;
    case TEX_REF: return texmaps;
    default: return NULL;
  }
}

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
MSTR DagorMat2::SubAnimName(int i, bool localized)
#else
TSTR DagorMat2::SubAnimName(int i)
#endif
{
  switch (i)
  {
    case PB_REF: return _T("Parameters");
    case TEX_REF: return _T("Maps");
    default: return _T("");
  }
}

RefTargetHandle DagorMat2::GetReference(int i)
{
  switch (i)
  {
    case PB_REF: return pblock;
    case TEX_REF: return texmaps;
    default: return NULL;
  }
}

void DagorMat2::SetReference(int i, RefTargetHandle rtarg)
{
  switch (i)
  {
    case PB_REF: pblock = (IParamBlock *)rtarg; break;
    case TEX_REF: texmaps = (Texmaps *)rtarg; break;
    default:
      if (i == 1)
        if (rtarg)
          if (rtarg->ClassID() == Texmaps_CID)
          {
            texmaps = (Texmaps *)rtarg;
            break;
          }
      if (i >= 1 && i < 1 + NUMTEXMAPS)
        if (texmaps)
          texmaps->settex(i - 1, (Texmap *)rtarg);
      break;
  }
}

RefTargetHandle DagorMat2::Clone(RemapDir &remap)
{
  DagorMat2 *mtl = new DagorMat2(FALSE);
  *((MtlBase *)mtl) = *((MtlBase *)this);
  mtl->ReplaceReference(PB_REF, remap.CloneRef(pblock));
  mtl->ReplaceReference(TEX_REF, remap.CloneRef(texmaps));
  mtl->twosided = twosided;
  mtl->classname = (!classname.isNull() ? classname : DEFAULT_SHADER_NAME);
  mtl->script = script;
#if MAX_RELEASE >= 4000
  BaseClone(this, mtl, remap);
#endif
  return mtl;
}

#if defined(MAX_RELEASE_R17) && MAX_RELEASE >= MAX_RELEASE_R17
RefResult DagorMat2::NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
  BOOL propagate)
#else
RefResult DagorMat2::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message)
#endif
{
  switch (message)
  {
    case REFMSG_CHANGE:
      if (hTarget == pblock)
        ivalid.SetEmpty();
      if (dlg && dlg->theMtl == this && !dlg->isActive)
        dlg->Invalidate();
      break;

    case REFMSG_GET_PARAM_DIM:
    {
      GetParamDim *gpd = (GetParamDim *)partID;
      switch (gpd->index)
      {
        case PB_SHIN: gpd->dim = defaultDim; break;
        case PB_AMB:
        case PB_DIFF:
        case PB_SPEC:
        case PB_EMIS: gpd->dim = stdColor255Dim; break;
      }
      DiscardTexHandles();
      return REF_STOP;
    }

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    case REFMSG_GET_PARAM_NAME_NONLOCALIZED:
#else
    case REFMSG_GET_PARAM_NAME:
#endif
    {
      GetParamName *gpn = (GetParamName *)partID;
      gpn->name.printf(_T("param#%d"), gpn->index);
      DiscardTexHandles();
      return REF_STOP;
    }
  }

  DiscardTexHandles();
  return REF_SUCCEED;
}

#define MTL_HDR_CHUNK 0x4000
enum
{
  CH_2SIDED = 1,
  CH_CLASSNAME,
  CH_SCRIPT,
};


IOResult DagorMat2::Save(ISave *isave)
{
  //      ULONG nb;
  isave->BeginChunk(MTL_HDR_CHUNK);
  IOResult res = MtlBase::Save(isave);
  if (res != IO_OK)
    return res;
  isave->EndChunk();
  if (twosided == IDagorMat::Sides::DoubleSided)
  {
    isave->BeginChunk(CH_2SIDED);
    isave->EndChunk();
  }
  isave->BeginChunk(CH_CLASSNAME);
  res = isave->WriteCString(classname);
  if (res != IO_OK)
    return res;
  isave->EndChunk();
  isave->BeginChunk(CH_SCRIPT);
  res = isave->WriteCString(script);
  if (res != IO_OK)
    return res;
  isave->EndChunk();
  return IO_OK;
}

IOResult DagorMat2::Load(ILoad *iload)
{
  //      ULONG nb;
  int id;
  IOResult res;

  twosided = Sides::OneSided;
  while (IO_OK == (res = iload->OpenChunk()))
  {
    switch (id = iload->CurChunkID())
    {
      case MTL_HDR_CHUNK:
        res = MtlBase::Load(iload);
        ivalid.SetEmpty();
        break;
      case CH_2SIDED:
        twosided = Sides::DoubleSided;
        ivalid.SetEmpty();
        break;
      case CH_CLASSNAME:
      {
        TCHAR *s;
        res = iload->ReadCStringChunk(&s);
        if (res == IO_OK)
          classname = (s ? s : DEFAULT_SHADER_NAME);
        ivalid.SetEmpty();
      }
      break;
      case CH_SCRIPT:
      {
        TCHAR *s;
        res = iload->ReadCStringChunk(&s);
        if (res == IO_OK)
          script = s;
        ivalid.SetEmpty();
      }
      break;
    }
    iload->CloseChunk();
    if (res != IO_OK)
      return res;
  }
  //      iload->RegisterPostLoadCallback(
  //              new ParamBlockPLCB(versions,NUM_OLDVERSIONS,&curVersion,this,PB_REF));

  DiscardTexHandles();
  iload->RegisterPostLoadCallback(&postLoadCallback);

  // debug("mat=0x%08x loaded '%s'", (unsigned int)this, classname);

  return IO_OK;
}

void DagorMat2::NotifyChanged()
{
  updateViewportTexturesState();
  NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
  DiscardTexHandles();
}

void DagorMat2::SetAmbient(Color c, TimeValue t)
{
  cola = c;
  pblock->SetValue(PB_AMB, t, c);
}

void DagorMat2::SetDiffuse(Color c, TimeValue t)
{
  cold = c;
  pblock->SetValue(PB_DIFF, t, c);
}

void DagorMat2::SetSpecular(Color c, TimeValue t)
{
  cols = c;
  pblock->SetValue(PB_SPEC, t, c);
}

void DagorMat2::SetShininess(float v, TimeValue t)
{
  shin = v * 100.0f;
  power = powf(2.0f, shin / 10.0f) * 4.0f;
  pblock->SetValue(PB_SHIN, t, shin);
}

Color DagorMat2::GetAmbient(int mtlNum, BOOL backFace) { return cola; }

Color DagorMat2::GetDiffuse(int mtlNum, BOOL backFace) { return cold; }

Color DagorMat2::GetSpecular(int mtlNum, BOOL backFace) { return cols; }

float DagorMat2::GetXParency(int mtlNum, BOOL backFace) { return 0; }

float DagorMat2::GetShininess(int mtlNum, BOOL backFace) { return shin / 100.0f; }

float DagorMat2::GetShinStr(int mtlNum, BOOL backFace)
{
  if (cols == Color(0, 0, 0))
    return 0;
  return 1;
}

Class_ID DagorMat2::ClassID() { return DagorMat2_CID; }

SClass_ID DagorMat2::SuperClassID() { return MATERIAL_CLASS_ID; }

void DagorMat2::DeleteThis() { delete this; }

Color DagorMat2::get_amb()
{
  Color c;
  pb_get_value(*pblock, PB_AMB, 0, c);
  return c;
}

Color DagorMat2::get_diff()
{
  Color c;
  pb_get_value(*pblock, PB_DIFF, 0, c);
  return c;
}

Color DagorMat2::get_spec()
{
  Color c;
  pb_get_value(*pblock, PB_SPEC, 0, c);
  return c;
}

Color DagorMat2::get_emis()
{
  Color c;
  pb_get_value(*pblock, PB_EMIS, 0, c);
  return c;
}

float DagorMat2::get_power()
{
  float f = 0;
  pb_get_value(*pblock, PB_SHIN, 0, f);
  return powf(2.0f, f / 10.0f) * 4.0f;
}

IDagorMat::Sides DagorMat2::get_2sided() { return twosided; }

const TCHAR *DagorMat2::get_classname() { return classname; }

const TCHAR *DagorMat2::get_script() { return script; }

const TCHAR *DagorMat2::get_texname(int i)
{
  Texmap *tex = texmaps->gettex(i);
  if (tex)
    if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
    {
      BitmapTex *b = (BitmapTex *)tex;
      return b->GetMapName();
    }
  return NULL;
}

float DagorMat2::get_param(int i) { return 0; }

void DagorMat2::enumerate_textures(EnumTexCB &cb)
{
  for (int i = 0; i < NUMTEXMAPS; ++i)
  {
    const TCHAR *texname = get_texname(i);
    if (texname && cb.proc(strToWide(blk_tex_name(i)).data(), texname) == ECB_STOP)
      return;
  }
}

void DagorMat2::enumerate_parameters(EnumParamCB &cb)
{
  M_STD_STRING shader_name = classname.data();
  if (shader_name.empty())
    return;

  auto blk = get_blk();

  // get list of shader parameters (including group names)
  const DataBlock *shader_blk = get_blk_shader(blk.get(), shader_name);
  std::vector<ParamInfo> all_params = get_blk_shader_params(shader_blk);

  // collect user-specified params
  std::vector<ParamInfo> user_params;
  M_STD_STRING s = simplifyRN(M_STD_STRING(script));
  std::vector<std::wstring> lines = split(s, L'\n');
  for (std::wstring &line : lines)
    if (!line.empty())
      user_params.emplace_back(get_blk_param_info_value(blk.get(), line, shader_name));

  // find uncategorized parameters (those that do not have a parent group)
  std::vector<ParamInfo> uncategorized_params;
  for (const ParamInfo &param : all_params)
    if (!param.is_group && param.parent.empty())
    {
      auto res = find_param_info(user_params, param.name);
      if (res.found)
        uncategorized_params.emplace_back(res.pinfo);
    }

  if (!uncategorized_params.empty())
    for (ParamInfo param : uncategorized_params)
      if (cb.proc(L"Uncategorized", param.name.data(), int(param.type), param.value.data()) == ECB_STOP)
        return;

  // create widgets (if any) in the same order as defined in .blk
  for (const ParamInfo &p : all_params)
  {
    if (!p.is_group)
      continue;

    std::vector<ParamInfo> grouped_params;
    for (const ParamInfo &param : all_params)
      if (!param.is_group && param.parent == p.name)
      {
        auto res = find_param_info(user_params, param.name);
        if (res.found)
          grouped_params.emplace_back(res.pinfo);
      }

    if (!grouped_params.empty())
      for (ParamInfo param : grouped_params)
        if (cb.proc(param.parent.data(), param.name.data(), int(param.type), param.value.data()) == ECB_STOP)
          return;
  }

  // collect unknown parameters
  std::vector<ParamInfo> unknown_params;
  for (const ParamInfo &param : user_params)
  {
    // skip real_two_sided
    if (iequal(param.name, REAL_TWO_SIDED))
      continue;

    // find parameter among pre-defined
    auto res = find_param_info(all_params, param.name);
    if (!res.found)
      unknown_params.emplace_back(param);
  }

  // make a new group and add the unknown parameters
  if (!unknown_params.empty())
  {
    for (ParamInfo param : unknown_params)
      if (cb.proc(L"Uncategorized", param.name.data(), int(param.type), param.value.data()) == ECB_STOP)
        return;
  }
}

void *DagorMat2::GetInterface(ULONG id)
{
  if (id == I_DAGORMAT)
    return (IDagorMat *)this;
  else if (id == I_DAGORMAT2)
    return (IDagorMat2 *)this;
  else
    return Mtl::GetInterface(id);
}

void DagorMat2::ReleaseInterface(ULONG id, void *i)
{
  if (id == I_DAGORMAT)
    ; // no-op
  else
    Mtl::ReleaseInterface(id, i);
}

void DagorMat2::set_amb(Color c)
{
  cola = c;
  pblock->SetValue(PB_AMB, 0, c);
}

void DagorMat2::set_diff(Color c)
{
  cold = c;
  pblock->SetValue(PB_DIFF, 0, c);
}

void DagorMat2::set_spec(Color c)
{
  cols = c;
  pblock->SetValue(PB_SPEC, 0, c);
}

void DagorMat2::set_emis(Color c)
{
  cole = c;
  pblock->SetValue(PB_EMIS, 0, c);
}

void DagorMat2::set_power(float p)
{
  if (p <= 0.0f)
    p = 32.0f;
  power = p;
  shin = float(log(double(power) / 4.0) / log(2.0) * 10.0);
  pblock->SetValue(PB_SHIN, 0, shin);
}

void DagorMat2::set_2sided(Sides b)
{
  twosided = b;
  NotifyChanged();
}

void DagorMat2::set_classname(const TCHAR *s)
{
  if (s)
    classname = s;
  else
    classname = DEFAULT_SHADER_NAME;
  NotifyChanged();
}

void DagorMat2::set_script(const TCHAR *s)
{
  if (s)
    script = s;
  else
    script = _T("");
  NotifyChanged();
}

void DagorMat2::set_texname(int i, const TCHAR *s)
{
  if (i < 0 || i >= NUMTEXMAPS)
    return;
  if (s)
    if (!*s)
      s = NULL;
  BitmapTex *bm = NULL;
  if (s)
  {
    if (*s == '\\' || *s == '/')
      ++s;
    bm = NewDefaultBitmapTex();
    assert(bm);

    M_STD_STRING path = strToWide(dagor_path);
    path += M_STD_STRING(s);

    bm->SetMapName((TCHAR *)path.c_str());
  }
  texmaps->settex(i, bm);
}

void DagorMat2::set_param(int i, float p) {}

////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK NewParameterDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  NewParameterDialog *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (NewParameterDialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (NewParameterDialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }

  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

NewParameterDialog::NewParameterDialog(Dagormat2Dialog *p, M_STD_STRING &shdr) : parent(p), hWnd(NULL), shader(shdr) {}

NewParameterDialog::~NewParameterDialog() {}

int NewParameterDialog::DoModal()
{
  return ::DialogBoxParam(hInstance, (const TCHAR *)IDD_DAGORPAR_NEW, parent->hParam, NewParameterDialogProc, (LPARAM)this);
}

static bool is_parameter_name_valid(const M_STD_STRING &name)
{
  return std::all_of(name.begin(), name.end(),
    [](wchar_t c) { return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_'); });
}

BOOL NewParameterDialog::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      attach_layout_to_dialog(this->hWnd, MAKEINTRESOURCE(IDD_DAGORPAR_NEW));

      HWND lb = ::GetDlgItem(hWnd, IDC_PARAM_NAME);
      ListBox_SetColumnWidth(lb, 512); // FIXME

      auto dataBlk = get_blk();

      const DataBlock *shader_blk = get_blk_shader(dataBlk.get(), shader);
      if (shader_blk)
      {
        std::vector<ParamInfo> shader_params = get_blk_shader_params(shader_blk);

        // root parameters
        for (const auto &param : shader_params)
          if (param.parent.empty() && !param.is_group && !parent->GetParam(param.name))
            ListBox_InsertString(lb, -1, param.name.data());

        // grouped parameters
        std::vector<std::wstring> names;
        for (const auto &param : shader_params)
          if (param.is_group)
          {
            names.clear();
            for (const auto &p : shader_params)
              if (p.parent == param.name && !parent->GetParam(p.name))
                names.emplace_back(p.name);

            if (!names.empty())
            {
              ListBox_InsertString(lb, -1, mangled_category_name(1, param.name).data());
              for (const auto &n : names)
                ListBox_InsertString(lb, -1, n.data());
            }
          }
      }
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDOK:
          if (HIWORD(wParam) == BN_CLICKED && GetAndVerifyParamName())
            ::EndDialog(hWnd, IDOK);
          break;

        case IDCANCEL:
          if (HIWORD(wParam) == BN_CLICKED)
            ::EndDialog(hWnd, IDCANCEL);
          break;

        case IDC_PARAM_NAME:
          if (HIWORD(wParam) == LBN_DBLCLK && GetAndVerifyParamName())
            ::EndDialog(hWnd, IDOK);
          break;

        default: break;
      }
    }
    break;

    case WM_SIZE: update_layout(hWnd, lParam); return 0;

    default: return FALSE;
  }

  return TRUE;
}

std::vector<std::wstring> NewParameterDialog::GetSelectedNames() const
{
  std::vector<std::wstring> selected;

  HWND lb = GetDlgItem(hWnd, IDC_PARAM_NAME);
  if (!lb)
    return selected;

  int n = ListBox_GetSelCount(lb);
  if (!n)
    return selected;

  selected.reserve(n);

  std::vector<int> sel_items;
  sel_items.resize(n);
  ListBox_GetSelItems(lb, n, &sel_items[0]);

  for (int i = 0; i < n; ++i)
  {
    static wchar_t buf[1024]; // FIXME
    ListBox_GetText(lb, sel_items[i], buf);
    selected.emplace_back(buf);
  }

  return selected;
}

bool NewParameterDialog::GetAndVerifyParamName()
{
  std::vector<std::wstring> selected = GetSelectedNames();

  if (selected.empty())
  {
    // no selection, check the edit field

    const TCHAR *err = nullptr;

    std::wstring name;
    HWND ed = GetDlgItem(hWnd, IDC_PARAM_NAME_EDIT);
    long length = GetWindowTextLength(ed);
    if (length > 0)
    {
      name.resize(length);
      GetWindowText(ed, (TCHAR *)name.c_str(), length + 1);
    }

    if (name.empty())
      err = _T("Name is empty");

    if (!is_parameter_name_valid(name))
      err = _T("Can't assign parameter,\nit contains non-supported characters.\n\nAllowed characters: ")
            _T("'A'..'Z', 'a'..'z', '_'");

    if (parent->GetParam(name))
      err = _T("Name already exists");

    if (err)
    {
      MessageBox(hWnd, err, _T("Error"), MB_OK | MB_ICONSTOP);
      return false;
    }

    selected_names.clear();
    selected_names.push_back(name);
    return true;
  }

  auto is_selected = [&selected](const std::wstring &name) -> bool {
    auto it = std::find_if(selected.begin(), selected.end(), [&name](const std::wstring &n) { return n == name; });
    return it != selected.end();
  };

  auto dataBlk = get_blk();
  const DataBlock *shader_blk = get_blk_shader(dataBlk.get(), shader);
  if (!shader_blk)
    return false;
  std::vector<ParamInfo> shader_params = get_blk_shader_params(shader_blk);

  std::vector<std::wstring> verified;
  verified.reserve(selected.size());

  // any selected parameters
  for (const auto &param : shader_params)
    if (!param.is_group && is_selected(param.name) && !parent->GetParam(param.name))
      verified.emplace_back(param.name);

  // override with grouped parameters
  for (const auto &param : shader_params)
    if (param.is_group && is_selected(mangled_category_name(1, param.name)))
      for (const auto &p : shader_params)
        if (p.parent == param.name && !parent->GetParam(p.name))
          verified.emplace_back(p.name);

  if (verified.empty())
    return true;

  selected_names.assign(verified.begin(), verified.end());
  return true;
}

////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK ShaderClassDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ShaderClassDialog *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (ShaderClassDialog *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (ShaderClassDialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }

  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

ShaderClassDialog::ShaderClassDialog(Dagormat2Dialog *p) : parent(p), hWnd(NULL) {}

ShaderClassDialog::~ShaderClassDialog() {}

int ShaderClassDialog::DoModal()
{
  return ::DialogBoxParam(hInstance, (const TCHAR *)IDD_DAGORPAR_SHADER_SELECTOR, parent->hShader, ShaderClassDialogProc,
    (LPARAM)this);
}

BOOL ShaderClassDialog::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      attach_layout_to_dialog(this->hWnd, MAKEINTRESOURCE(IDD_DAGORPAR_SHADER_SELECTOR));

      HWND hCmb = ::GetDlgItem(hWnd, IDC_SHADER_CLASS_LIST);

      std::vector<std::wstring> shader_list = get_blk_shader_list(get_blk().get());
      for (auto const &sh : shader_list)
        ComboBox_InsertString(hCmb, -1, sh.data());

      const wchar_t *name = parent->theMtl->classname.data();
      int i = ComboBox_FindStringExact(hCmb, 0, (LPARAM)name);
      if (i != CB_ERR)
        ComboBox_SetCurSel(hCmb, i);
      else
        ComboBox_SetText(hCmb, name);
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDOK:
          if (HIWORD(wParam) == BN_CLICKED && GetName())
            ::EndDialog(hWnd, IDOK);
          break;

        case IDCANCEL:
          if (HIWORD(wParam) == BN_CLICKED)
            ::EndDialog(hWnd, IDCANCEL);
          break;

        case IDC_SHADER_CLASS_LIST:
          if (HIWORD(wParam) == CBN_DBLCLK && GetName())
            ::EndDialog(hWnd, IDOK);
          break;

        default: break;
      }
    }
    break;

    case WM_SIZE: update_layout(this->hWnd, lParam); return 0;

    default: return FALSE;
  }

  return TRUE;
}

bool ShaderClassDialog::GetName()
{
  HWND hCmbName = ::GetDlgItem(hWnd, IDC_SHADER_CLASS_LIST);
  int len = ::GetWindowTextLength(hCmbName);

  if (len <= 0)
  {
    MessageBox(hWnd, _T("Empty shader class is not allowed"), _T("Class selection error"), MB_ICONERROR | MB_OK);
    return false;
  }

  std::wstring s;
  s.resize(len);
  ::GetWindowText(hCmbName, (TCHAR *)s.c_str(), len + 1);

  if (s.substr(0, 1) == _T("-"))
  {
    TSTR msg;
    msg.printf(_T("\"%s\" category cannot be set as shader class."), TSTR(s.data()));
    MessageBox(hWnd, msg, _T("Class selection error"), MB_ICONERROR | MB_OK);
    return false;
  }

  shader_class_name = s;
  return true;
}

////////////////////////////////////////////////////////////////

AbstractWidget::AbstractWidget(const ParamInfo &pinfo_, Dagormat2Dialog *p) :
  hPanel(NULL), hSType(NULL), creating(TRUE), param(pinfo_), parent(p)
{}

AbstractWidget::~AbstractWidget() { ::DestroyWindow(hPanel); }

template <typename T>
static INT_PTR CALLBACK widget_dlg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_INITDIALOG)
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  else
    lParam = GetWindowLongPtr(hWnd, GWLP_USERDATA);

  T *dlg = reinterpret_cast<T *>(lParam);
  if (!dlg)
    return FALSE;

  dlg->hPanel = hWnd;
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////

WidgetText::WidgetText(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p), edit(NULL)
{
  hPanel = p->AppendDialog(param.name.c_str(), IDD_DAGORPAR_TEXT, widget_dlg_proc<WidgetText>, (LPARAM)this);
}

WidgetText::~WidgetText() {}

INT_PTR WidgetText::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
      edit = ::GetICustEdit(GetDlgItem(hWnd, IDC_PAR_TEXT_VALUE));
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
      break;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_DELETE, BN_CLICKED))
      {
        parent->RemParam(param.name);
        return FALSE;
      }
      if (wParam == MAKEWPARAM(IDC_PAR_TEXT_VALUE, EN_CHANGE) && !creating)
        GetValue();
      break;

    default: return FALSE;
  }

  return TRUE;
}

void WidgetText::GetValue()
{
  TSTR buf;
  edit->GetText(buf);
  param.value = buf.data();
  parent->SaveParams();
}

void WidgetText::SetValue(const M_STD_STRING &v)
{
  edit->SetText(v.c_str());
  param.value = v;
}

//////////////////////////////////////////////////////////////////////

WidgetColor::WidgetColor(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p), col(NULL)
{
  hPanel = p->AppendDialog(param.name.c_str(), IDD_DAGORPAR_COLOR, widget_dlg_proc<WidgetColor>, (LPARAM)this);
}

WidgetColor::~WidgetColor() { ReleaseIColorSwatch(col); }

INT_PTR WidgetColor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      AColor c;
      col = GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter"));
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
      GetValue();
    }
    break;

    case CC_COLOR_CHANGE:
    {
      int buttonUp = HIWORD(wParam);
      if (buttonUp)
        theHold.Begin();
      GetValue();
      if (buttonUp)
        theHold.Accept(GetString(IDS_COLOR_CHANGE));
    }
    break;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_DELETE, BN_CLICKED))
      {
        parent->RemParam(param.name);
        return FALSE;
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

void WidgetColor::GetValue()
{
  AColor c = col->GetAColor();
  TCHAR buf[128];
  _stprintf(buf, _T("%.3f,%.3f,%.3f,%.3f"), c.r, c.g, c.b, c.a);

  param.value = buf;
  parent->SaveParams();
}

void WidgetColor::SetValue(const M_STD_STRING &v)
{
  float r = 0, g = 0, b = 0, a = 0;
  int res = _stscanf(v.c_str(), _T(" %f , %f , %f , %f"), &r, &g, &b, &a);
  if (res != 4)
    col->SetAColor(AColor(0, 0, 0, 0), TRUE);
  else
    col->SetAColor(AColor(r, g, b, a), TRUE);

  param.value = v;
}

//////////////////////////////////////////////////////////////////////

WidgetBool::WidgetBool(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  hPanel = p->AppendDialog(param.name.c_str(), IDD_DAGORPAR_BOOL, widget_dlg_proc<WidgetBool>, (LPARAM)this);
}

WidgetBool::~WidgetBool() {}

INT_PTR WidgetBool::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
      break;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_DELETE, BN_CLICKED))
      {
        parent->RemParam(param.name);
        return FALSE;
      }
      if (LOWORD(wParam) == IDC_PAR_BOOL_VALUE)
        GetValue();
      break;

    default: return FALSE;
  }

  return TRUE;
}

void WidgetBool::GetValue()
{
  param.value = IsDlgButtonChecked(hPanel, IDC_PAR_BOOL_VALUE) ? _T("yes") : _T("no");
  parent->SaveParams();
}

void WidgetBool::SetValue(const M_STD_STRING &v)
{
  CheckDlgButton(hPanel, IDC_PAR_BOOL_VALUE, iequal(v, L"yes") || iequal(v, L"true"));
  param.value = v;
}

//////////////////////////////////////////////////////////////////////

static long widget_p4_idc[4][2] = {
  {IDC_PAR_POINT_VALUE_SPINNER_0, IDC_PAR_POINT_VALUE_0},
  {IDC_PAR_POINT_VALUE_SPINNER_1, IDC_PAR_POINT_VALUE_1},
  {IDC_PAR_POINT_VALUE_SPINNER_2, IDC_PAR_POINT_VALUE_2},
  {IDC_PAR_POINT_VALUE_SPINNER_3, IDC_PAR_POINT_VALUE_3},
};

WidgetNumeric::WidgetNumeric(const ParamInfo &param, Dagormat2Dialog *p) :
  AbstractWidget(param, p), is_float(true), is_color_ui(false), is_spinner_used(false), size(4), col(0)
{
  memset(spinner, 0, 4 * sizeof(*spinner));

  long idd = 0;

  if (param.type == DataBlock::ParamType::TYPE_POINT4)
  {
    is_color_ui = iequal(param.custom_ui, L"color");
    size = 4;
    idd = is_color_ui ? IDD_DAGORPAR_POINT4_COLOR : IDD_DAGORPAR_POINT4;
  }
  else if (param.type == DataBlock::ParamType::TYPE_POINT3)
  {
    size = 3;
    idd = IDD_DAGORPAR_POINT3;
  }
  else if (param.type == DataBlock::ParamType::TYPE_POINT2)
  {
    size = 2;
    idd = IDD_DAGORPAR_POINT2;
  }
  else if (param.type == DataBlock::ParamType::TYPE_REAL)
  {
    size = 1;
    idd = IDD_DAGORPAR_REAL;
  }
  else if (param.type == DataBlock::ParamType::TYPE_IPOINT3)
  {
    is_float = false;
    size = 3;
    idd = IDD_DAGORPAR_POINT3;
  }
  else if (param.type == DataBlock::ParamType::TYPE_IPOINT2)
  {
    is_float = false;
    size = 2;
    idd = IDD_DAGORPAR_POINT2;
  }
  else if (param.type == DataBlock::ParamType::TYPE_INT)
  {
    is_float = false;
    size = 1;
    idd = IDD_DAGORPAR_REAL;
  }
  assert(idd);
  hPanel = p->AppendDialog(param.name.c_str(), idd, widget_dlg_proc<WidgetNumeric>, (LPARAM)this);

  hFramePen = (HPEN)CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
}

WidgetNumeric::~WidgetNumeric()
{
  if (is_color_ui && col)
    ReleaseIColorSwatch(col);
  DeleteObject(hFramePen);
}

bool WidgetNumeric::isOutOfRange() const
{
  for (int i = 0; i < size; ++i)
  {
    float f = is_float ? spinner[i]->GetFVal() : spinner[i]->GetIVal();
    if ((param.soft_min_enabled && f < param.soft_min) || (param.soft_max_enabled && f > param.soft_max))
      return true;
  }
  return false;
}

INT_PTR WidgetNumeric::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
      if (is_color_ui)
      {
        AColor c;
        col = GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter"));
      }
      for (int i = 0; i < size; ++i)
      {
        long idc_spin = widget_p4_idc[i][0];
        long idc_edit = widget_p4_idc[i][1];
        if (is_float)
          spinner[i] = ::SetupFloatSpinner(hWnd, idc_spin, idc_edit, -FLT_MAX, +FLT_MAX, 0);
        else
          spinner[i] = ::SetupIntSpinner(hWnd, idc_spin, idc_edit, -INT_MAX, +INT_MAX, 0);
      }
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
      InvalidateRect(hWnd, NULL, TRUE);
      UpdateWindow(hWnd);
      parent->MarkUnknownParams(parent->blk.get());
      break;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_DELETE, BN_CLICKED))
      {
        parent->RemParam(param.name);
        return FALSE;
      }
      break;

    case CC_SPINNER_CHANGE:
      if (!theHold.Holding())
        theHold.Begin();
      if (LOWORD(wParam) == IDC_PAR_POINT_VALUE_SPINNER_0 || LOWORD(wParam) == IDC_PAR_POINT_VALUE_SPINNER_1 ||
          LOWORD(wParam) == IDC_PAR_POINT_VALUE_SPINNER_2 || LOWORD(wParam) == IDC_PAR_POINT_VALUE_SPINNER_3)
      {
        size_t i = 0;
        for (; i < 4; ++i)
          if (LOWORD(wParam) == widget_p4_idc[i][0])
            break;

        if (is_spinner_used)
        {
          float f = is_float ? spinner[i]->GetFVal() : spinner[i]->GetIVal();

          if (param.soft_min_enabled && f < param.soft_min)
            f = param.soft_min;

          if (param.soft_max_enabled && f > param.soft_max)
            f = param.soft_max;

          if (is_float)
            spinner[i]->SetValue(f, true);
          else
            spinner[i]->SetValue(int(f), true);
        }
        GetValue();
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        parent->MarkUnknownParams(parent->blk.get());
      }
      break;

    case CC_SPINNER_BUTTONDOWN:
      is_spinner_used = true;
      theHold.Begin();
      break;

    case WM_CUSTEDIT_ENTER:
    case CC_SPINNER_BUTTONUP:
      is_spinner_used = false;
      if (HIWORD(wParam) || msg == WM_CUSTEDIT_ENTER)
        theHold.Accept(GetString(IDS_PARAM_CHANGE));
      else
        theHold.Cancel();
      break;

    case CC_COLOR_CHANGE:
    {
      int buttonUp = HIWORD(wParam);
      if (buttonUp)
        theHold.Begin();
      AColor c = col->GetAColor();
      spinner[0]->SetValue(c.r, FALSE);
      spinner[1]->SetValue(c.g, FALSE);
      spinner[2]->SetValue(c.b, FALSE);
      spinner[3]->SetValue(c.a, TRUE);
      GetValue();
      if (buttonUp)
        theHold.Accept(GetString(IDS_COLOR_CHANGE));
    }
    break;

    case WM_PAINT:
    {
      if (isOutOfRange())
      {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        float kx = float(GetDeviceCaps(hdc, LOGPIXELSX)) / 96.0f;
        float ky = float(GetDeviceCaps(hdc, LOGPIXELSY)) / 96.0f;
        RECT rc;
        GetWindowRect(hWnd, &rc);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hFramePen);
        HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
        Rectangle(hdc, kx, ky, rc.right - rc.left - kx, rc.bottom - rc.top - 2 * ky);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        EndPaint(hWnd, &ps);
      }
    }
      return 0;

    default: return FALSE;
  }

  return TRUE;
}

void WidgetNumeric::GetValue()
{
  float f[4] = {};
  M_STD_OSTRINGSTREAM str;
  str.imbue(std::locale::classic());
  for (int i = 0; i < size; ++i)
  {
    if (is_float)
      str << (f[i] = spinner[i]->GetFVal());
    else
      str << spinner[i]->GetIVal();

    if (i < size - 1)
      str << ", ";
  }
  param.value = str.str();
  parent->SaveParams();
  if (is_color_ui)
    UpdateColorSwatch(f);
}

void WidgetNumeric::SetValue(const M_STD_STRING &v)
{
  float f[4] = {};
  M_STD_ISTRINGSTREAM str(v);
  str.imbue(std::locale::classic());
  for (int i = 0; i < size; ++i)
  {
    if (is_float)
    {
      str >> f[i];
      spinner[i]->SetValue(f[i], TRUE);
    }
    else
    {
      int d;
      str >> d;
      spinner[i]->SetValue(d, TRUE);
    }
    while (str.peek() == L' ' || str.peek() == L',')
      str.ignore();
  }
  if (is_color_ui)
    UpdateColorSwatch(f);
  param.value = v;
}

void WidgetNumeric::UpdateColorSwatch(float f[4])
{
  float k = 0.f;
  k = std::max(k, f[0]);
  k = std::max(k, f[1]);
  k = std::max(k, f[2]);
  k = std::max(k, f[3]);
  if (k == 0.f)
    k = 1.f;

  AColor c;
  c.r = clamp(f[0] / k, 0.f, 1.f);
  c.g = clamp(f[1] / k, 0.f, 1.f);
  c.b = clamp(f[2] / k, 0.f, 1.f);
  c.a = clamp(f[3] / k, 0.f, 1.f);
  col->SetAColor(c, FALSE);
}

//////////////////////////////////////////////////////////////////////

WidgetGroup::WidgetGroup(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  hPanel = p->AppendDialog(param.name.c_str(), IDD_DAGORPAR_GROUP, widget_dlg_proc<WidgetGroup>, (LPARAM)this);
}

WidgetGroup::~WidgetGroup() {}

INT_PTR WidgetGroup::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG: ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE); break;

    case WM_COMMAND:
      if (wParam == MAKEWPARAM(IDC_PARAM_DELETE, BN_CLICKED))
      {
        parent->RemParamGroup(param.name);
        return FALSE;
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

//////////////////////////////////////////////////////////////////////

void Dagormat2Dialog::DialogsReposition()
{
  IRollupWindow *irw = ip->GetMtlEditorRollup();
  int ind = irw->GetPanelIndex(hParam);

  if (parameters.empty())
  {
    if (ind >= 0)
      irw->SetPageDlgHeight(ind, paramOrg.y);

    return;
  }

  RECT param_rc;
  GetWindowRect(hParam, &param_rc);

  POINT pt = paramOrg;
  for (size_t i = 0; i < parameters.size(); ++i)
  {
    auto &par = *parameters[i];
    ::SetWindowPos(par.hPanel, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    ::GetWindowRect(par.hPanel, &par.screen_rc);
    pt.x = par.screen_rc.left;
    pt.y = par.screen_rc.bottom + PARAM_DLG_GAP;
    ::ScreenToClient(hParam, &pt);

    par.screen_rc.left -= param_rc.left;
    par.screen_rc.top -= param_rc.top;
    par.screen_rc.right -= param_rc.left;
    par.screen_rc.bottom -= param_rc.top;
  }

  if (ind >= 0)
    irw->SetPageDlgHeight(ind, pt.y + PARAM_DLG_GAP);
}

//////////////////////////////////////////////////////////////////////

std::wstring fix_param_values(const std::wstring &script, const std::wstring &classname,
  std::wstring (*fix)(DataBlock::ParamType, const std::wstring &))
{
  std::wstring buffer;
  auto params = get_blk_params(get_blk().get(), script, classname);
  for (auto &param : params)
  {
    buffer += param.name;
    buffer += _T('=');
    buffer += fix(param.type, param.value);
    buffer += _T("\r\n");
  }
  return buffer;
}

template <typename T>
std::wstring normalize_value(const std::wstring &v)
{
  std::wistringstream iss(v);
  iss.imbue(std::locale::classic());

  std::wostringstream oss;
  oss.imbue(std::locale::classic());

  for (T t; iss >> t;)
  {
    oss << t;

    while (iss.peek() == L' ')
      iss.ignore();

    if (iss.peek() == L',')
    {
      iss.ignore();
      oss << L',';
    }
  }
  return oss.str();
}

static std::wstring normalize_param(DataBlock::ParamType type, const std::wstring &value)
{
  std::wstring v = fix_empty_param(type, value);
  trim(v);

  // guess type for type:t="text
  if (type == DataBlock::ParamType::TYPE_STRING)
  {
    if (std::find(value.begin(), value.end(), L'.') != value.end())
      type = DataBlock::ParamType::TYPE_POINT4;
    else
      type = DataBlock::ParamType::TYPE_IPOINT3;
  }

  switch (type)
  {
    case DataBlock::ParamType::TYPE_BOOL:
    {
      if (iequal(v, L"no") || iequal(v, L"false") || v == L"0")
        return L"no";
      if (iequal(v, L"yes") || iequal(v, L"true") || v == L"1")
        return L"yes";
      return value; // cannot recognize the value, return as is
    }

    case DataBlock::ParamType::TYPE_INT:
    case DataBlock::ParamType::TYPE_IPOINT2:
    case DataBlock::ParamType::TYPE_IPOINT3:
    case DataBlock::ParamType::TYPE_E3DCOLOR: return normalize_value<int>(value);

    case DataBlock::ParamType::TYPE_REAL:
    case DataBlock::ParamType::TYPE_POINT2:
    case DataBlock::ParamType::TYPE_POINT3:
    case DataBlock::ParamType::TYPE_POINT4: return normalize_value<float>(value);

    default: break;
  }

  return value; // unknown type, return as is
}

std::wstring fix_empty_param_values(const std::wstring &script, const std::wstring &classname)
{
  return fix_param_values(script, classname, fix_empty_param);
}

std::wstring normalize_param_values(const std::wstring &script, const std::wstring &classname)
{
  return fix_param_values(script, classname, normalize_param);
}
