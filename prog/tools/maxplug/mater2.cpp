// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vector>
#include <array>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <locale>
#include <format>
#include <string_view>
#include <optional>
#include <filesystem>
#include <iterator>

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
#include "dagorLogWindow.h"
#include "common.h"
#include "ci.h"
#include "layout.h"
#include "enumnode.h"

#include <d3d9types.h>
#include <ihardwarematerial.h>

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////////////


static const std::wstring DEFAULT_SHADER_NAME(_T("gi_black"));
static const TSTR DAGOR_SHADERS_CONFIG(_T("dagorShaders.blk"));
static const TCHAR *REAL_TWO_SIDED(_T("real_two_sided"));
static const char *SHADER_CATEGORY = "shader_category";

/////////////////////////////////////////////////////////////////////////////////

static const char *blk_tex_name(int i)
{
  static_assert(DAGTEXNUM == 16);
  static const char *s[DAGTEXNUM] = {"tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7", "tex8", "tex9", "tex10", "tex11",
    "tex12", "tex13", "tex14", "tex15"};
  return s[(i >= 0 && i < DAGTEXNUM) ? i : 0];
}

/////////////////////////////////////////////////////////////////////////////////

static std::wstring mangled_category_name(unsigned depth, std::wstring_view name)
{
  std::wstring prefix;
  while (depth-- > 0)
    prefix += _T("--- ");
  prefix += name;
  return prefix;
}

static std::unique_ptr<DataBlock> get_blk()
{
  std::unique_ptr<DataBlock> dataBlk = std::make_unique<DataBlock>(std::make_shared<NameMap>());

  const fs::path fname = get_cfg_filename(DAGOR_SHADERS_CONFIG);
  if (!dataBlk->load(fname))
  {
    // the config is read once per session, so this is the only chance to say that it is not there
    std::error_code ec;
    const wchar_t *what = fs::exists(fname, ec) ? L"could not be read" : L"was not found";
    DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error,
      L"the shader config %s: %s, so every material parameter will be listed as unknown\r\n", what, fname.c_str());

    if (!are_prompts_suppressed())
    {
      const std::wstring msg = std::format(L"The shader config {}:\n\n{}\n\nIt is looked for one directory above the "
                                           L"plugin first, then next to it. Until it loads, the shader list stays empty, every "
                                           L"material parameter is listed as unknown and its type is guessed from its value.",
        what, fname.native());
      MessageBox(GetCOREInterface()->GetMAXHWnd(), msg.data(), L"Dagor shader config", MB_ICONERROR | MB_OK);
    }
  }

  return dataBlk;
}

static const DataBlock *get_shared_blk()
{
  static std::unique_ptr<DataBlock> blk = get_blk();
  return blk.get();
}


// the shape of the config, in one place: a shader_category holds shaders and other categories, and
// every other named block is a shader. fn() gets each named entry, a category before its contents,
// and stops the walk by returning false.
template <typename Fn>
static bool for_each_shader_entry(const DataBlock *blk, unsigned depth, Fn &&fn)
{
  if (!blk)
    return true;

  for (int i = 0; i < blk->blockCount(); ++i)
  {
    const DataBlock *sub = blk->getBlock(i);
    if (!sub)
      continue;

    const bool is_category = iequal(sub->getBlockName(), SHADER_CATEGORY);
    const int name_i = sub->findParam("name");
    const unsigned sub_depth = is_category ? depth + 1 : depth;

    if (name_i != -1 && !fn(sub, strToWide(sub->getStr(name_i)), sub_depth, is_category))
      return false;

    if (is_category && !for_each_shader_entry(sub, sub_depth, fn))
      return false;
  }

  return true;
}


std::vector<std::wstring> get_blk_shader_list(const DataBlock *dataBlk)
{
  std::vector<std::wstring> shader_list;
  for_each_shader_entry(dataBlk, 0, [&shader_list](const DataBlock *, const std::wstring &name, unsigned depth, bool is_category) {
    shader_list.push_back(is_category ? mangled_category_name(depth, name) : name);
    return true;
  });
  return shader_list;
}


static const DataBlock *get_blk_shader(const DataBlock *dataBlk, std::wstring_view shader_name)
{
  const DataBlock *found = nullptr;
  for_each_shader_entry(dataBlk, 0, [&found, shader_name](const DataBlock *blk, const std::wstring &name, unsigned, bool is_category) {
    if (is_category || !iequal(name, shader_name))
      return true;

    found = blk;
    return false;
  });
  return found;
}

/////////////////////////////////////////////////////////////////////////////////

struct ParamInfo
{
  DataBlock::ParamType type = DataBlock::ParamType::TYPE_NONE;
  std::wstring name, description, custom_ui, value, def, parent;
  std::wstring type_suffix; // the ':type' the script spelled after the name, empty when it spelled none
  float soft_min = 0;
  float soft_max = 0;
  bool is_group = false;
  bool def_enabled = false;
  bool soft_min_enabled = false;
  bool soft_max_enabled = false;

  // atest:i where the script declared a type, plain atest otherwise
  std::wstring name_with_type() const { return type_suffix.empty() ? name : name + L':' + type_suffix; }

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

static std::wstring get_param_value_as_string(const DataBlock *blk, int param_number)
{
  if (!blk || param_number == -1)
    return std::wstring();

  auto param_ref = blk->getParam(param_number);
  if (!param_ref)
    return std::wstring();

  const DataBlock::Param &p = param_ref->get();

  std::wstringstream os;
  os.imbue(std::locale::classic());

  switch (type(p))
  {
    case DataBlock::ParamType::TYPE_STRING: return strToWide(std::get<std::string>(p));
    case DataBlock::ParamType::TYPE_BOOL: return std::get<bool>(p) ? L"yes" : L"no";

    case DataBlock::ParamType::TYPE_INT: os << std::get<int>(p); break;
    case DataBlock::ParamType::TYPE_REAL: os << std::get<float>(p); break;

    case DataBlock::ParamType::TYPE_POINT2:
    {
      auto &p2 = std::get<Point2>(p);
      os << p2.x << ", " << p2.y;
    }
    break;

    case DataBlock::ParamType::TYPE_POINT3:
    {
      auto &p3 = std::get<Point3>(p);
      os << p3.x << ", " << p3.y << ", " << p3.z;
    }
    break;

    case DataBlock::ParamType::TYPE_POINT4:
    {
      auto &p4 = std::get<Point4>(p);
      os << p4.x << ", " << p4.y << ", " << p4.z << ", " << p4.w;
    }
    break;

    case DataBlock::ParamType::TYPE_IPOINT2:
    {
      auto &ip2 = std::get<IPoint2>(p);
      os << ip2.x << ", " << ip2.y;
    }
    break;

    case DataBlock::ParamType::TYPE_IPOINT3:
    {
      auto &ip3 = std::get<IPoint3>(p);
      os << ip3.x << ", " << ip3.y << ", " << ip3.z;
    }
    break;

    case DataBlock::ParamType::TYPE_E3DCOLOR:
    {
      auto &c = std::get<E3DCOLOR>(p);
      os << c.r << ", " << c.g << ", " << c.b << ", " << c.a;
    }
    break;

    case DataBlock::ParamType::TYPE_MATRIX:
    {
      auto &m = std::get<TMatrix>(p);
      os << "[";
      os << "[" << m.getcol(0).x << ", " << m.getcol(0).y << ", " << m.getcol(0).z << "]";
      os << "[" << m.getcol(1).x << ", " << m.getcol(1).y << ", " << m.getcol(1).z << "]";
      os << "[" << m.getcol(2).x << ", " << m.getcol(2).y << ", " << m.getcol(2).z << "]";
      os << "[" << m.getcol(3).x << ", " << m.getcol(3).y << ", " << m.getcol(3).z << "]";
      os << "]";
    }
    break;

    default: debug("unknown type"); break;
  }

  return os.str();
}

static float get_param_value_as_float(const DataBlock *blk, int param_number, float def)
{
  if (!blk || param_number == -1)
    return def;

  auto param_ref = blk->getParam(param_number);
  if (!param_ref)
    return def;

  const DataBlock::Param &p = param_ref->get();

  if (type(p) == DataBlock::ParamType::TYPE_INT)
    return std::get<int>(p);

  if (type(p) == DataBlock::ParamType::TYPE_REAL)
    return std::get<float>(p);

  return def;
}

static std::vector<ParamInfo> get_blk_shader_params_of_group(const DataBlock *groupBlk, std::wstring_view parent)
{
  std::vector<ParamInfo> shader_params;

  if (!groupBlk)
    return shader_params;

  for (int i = 0; i < groupBlk->blockCount(); ++i)
  {
    DataBlock *blk = groupBlk->getBlock(i);
    if (!blk)
      continue;

    if (iequal(blk->getBlockName(), "parameters_group"))
    {
      ParamInfo group;
      group.parent = parent;
      group.is_group = true;
      int group_name_i = blk->findParam("name");
      if (group_name_i != -1)
        group.name = strToWide(blk->getStr(group_name_i));
      std::vector<ParamInfo> params = get_blk_shader_params_of_group(blk, group.name);
      shader_params.push_back(std::move(group));
      shader_params.insert(shader_params.end(), std::make_move_iterator(params.begin()), std::make_move_iterator(params.end()));
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
      param.type = DataBlock::deserialize_param_type(blk->getStr(param_type_i));
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

    shader_params.push_back(std::move(param));
  }

  return shader_params;
}


static std::vector<ParamInfo> get_blk_shader_params(const DataBlock *shaderBlk)
{
  // shader is the root group
  return get_blk_shader_params_of_group(shaderBlk, std::wstring());
}


static ParamInfo get_param_info(const std::vector<ParamInfo> &shader_params, std::wstring_view classname, std::wstring_view param_name)
{
  if (classname.empty() || param_name.empty())
    return ParamInfo();

  auto it =
    std::ranges::find_if(shader_params, [&param_name](const ParamInfo &p_i) { return !p_i.is_group && iequal(p_i.name, param_name); });
  if (it != shader_params.end())
    return *it;

  ParamInfo param;
  param.type = DataBlock::ParamType::TYPE_STRING;
  param.name = param_name;
  return param;
}

static DataBlock::ParamType guess_blk_type_by_value(const std::wstring &value)
{
  if (iequal(value, L"yes") || iequal(value, L"true") || iequal(value, L"no") || iequal(value, L"false"))
    return DataBlock::ParamType::TYPE_BOOL;

  const std::string narrow = wideToStr(value);

  // an integer type is only guessed when the whole value is consumed, a real one may leave a tail
  auto whole_value = [&narrow](auto &...v) {
    std::string_view left(narrow);
    return parse_nums_sep(left, ",", v...) == int(sizeof...(v)) && left.empty();
  };

  int ix, iy, iz;
  if (whole_value(ix, iy, iz))
    return DataBlock::ParamType::TYPE_IPOINT3;
  if (whole_value(ix, iy))
    return DataBlock::ParamType::TYPE_IPOINT2;
  if (whole_value(ix))
    return DataBlock::ParamType::TYPE_INT;

  float x, y, z, w;
  if (4 == parse_nums(narrow, x, y, z, w))
    return DataBlock::ParamType::TYPE_POINT4;
  if (3 == parse_nums(narrow, x, y, z))
    return DataBlock::ParamType::TYPE_POINT3;
  if (2 == parse_nums(narrow, x, y))
    return DataBlock::ParamType::TYPE_POINT2;
  if (1 == parse_nums(narrow, x))
    return DataBlock::ParamType::TYPE_REAL;

  return DataBlock::ParamType::TYPE_STRING;
}

// the name a script line declares: everything before its first '=', so the rest of the line, any
// further '=' included, belongs to the value
static std::optional<std::wstring_view> param_line_name(std::wstring_view line)
{
  const size_t eq = line.find(L'=');
  if (eq == std::wstring_view::npos)
    return std::nullopt;

  return line.substr(0, eq);
}

// how a script line spells the name part: a single colon with a type after it declares one, so
// atest:i is atest of type i, while a:b:c and atest: are names of their own
struct ParamName
{
  std::wstring_view name;
  std::wstring_view type; // empty when the part declares none
};

static ParamName split_param_name(std::wstring_view part)
{
  const size_t colon = part.find(L':');
  const std::wstring_view type = colon == std::wstring_view::npos ? std::wstring_view() : part.substr(colon + 1);
  if (type.empty() || type.find(L':') != std::wstring_view::npos)
    return {part, {}};

  return {part.substr(0, colon), type};
}

static void append_param_line(std::wstring &script, std::wstring_view name, std::wstring_view value)
{
  script += name;
  script += L'=';
  script += value;
  script += L"\r\n";
}

// the line with the ':type' dropped: nothing outside the dialog reads that annotation
static std::wstring bare_param_line(std::wstring_view line)
{
  const std::optional<std::wstring_view> part = param_line_name(line);
  if (!part)
    return std::wstring(line);

  const ParamName parsed = split_param_name(*part);
  if (parsed.type.empty())
    return std::wstring(line);

  return std::wstring(parsed.name) + std::wstring(line.substr(part->size()));
}

// rewrites a script without the lines drop() picks by their bare name, passing every survivor
// through as it was read. A line with no '=' names nothing, so it goes without asking drop().
template <typename Drop>
static std::wstring drop_param_lines(std::wstring_view script, Drop drop)
{
  std::wstring res;
  for (const std::wstring &line : split(simplifyRN(script), L'\n'))
  {
    const std::optional<std::wstring_view> part = param_line_name(line);
    if (!part || drop(split_param_name(*part).name))
      continue;

    res += line;
    res += L"\r\n";
  }
  return res;
}

static ParamInfo get_blk_param_info_value(const std::vector<ParamInfo> &shader_params, std::wstring_view line,
  std::wstring_view classname)
{
  ParamInfo res;

  const std::optional<std::wstring_view> part = param_line_name(line);
  if (!part || part->empty())
    return res; // invalid input

  // name itself might contain type: atest:f --> { atest, f }
  const ParamName parsed = split_param_name(*part);

  res = get_param_info(shader_params, classname, parsed.name);
  if (!parsed.type.empty())
  {
    // the type the line declares wins over the config
    res.type = DataBlock::deserialize_param_type(wideToStr(parsed.type));
    res.type_suffix = parsed.type;
  }

  // override value
  res.value = line.substr(part->size() + 1);

  // special case
  if (res.name == REAL_TWO_SIDED)
    res.type = DataBlock::ParamType::TYPE_BOOL;

  if (res.type == DataBlock::ParamType::TYPE_NONE)
    res.type = guess_blk_type_by_value(res.value);

  return res;
}

static std::vector<ParamInfo> get_blk_params(const DataBlock *dataBlk, std::wstring_view _script, std::wstring_view classname)
{
  const std::vector<ParamInfo> shader_params = get_blk_shader_params(get_blk_shader(dataBlk, classname));

  std::vector<std::wstring> lines = split(simplifyRN(_script), L'\n');

  std::vector<ParamInfo> params;
  params.reserve(lines.size());

  bool has_real_two_sided = false;

  for (std::wstring &line : lines)
  {
    if (line.empty())
      continue;

    auto param = get_blk_param_info_value(shader_params, line, classname);
    if (param.name.empty())
      continue;

    if (param.name == REAL_TWO_SIDED)
      has_real_two_sided = true;

    params.push_back(std::move(param));
  }

  if (!has_real_two_sided)
  {
    ParamInfo param;
    param.name = REAL_TWO_SIDED;
    param.type = DataBlock::ParamType::TYPE_BOOL;
    param.value = L"no";
    params.push_back(std::move(param));
  }

  return params;
}

/////////////////////////////////////////////////////////////////////////////////

class Dagormat2Dialog;

class NewParameterDialog
{
public:
  NewParameterDialog(Dagormat2Dialog *p, std::wstring_view shdr);
  virtual ~NewParameterDialog() = default;

  const std::vector<std::wstring> &GetNewParamNames() const { return selected_names; }

  int DoModal();
  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  std::vector<std::wstring> GetSelectedNames() const;
  bool GetAndVerifyParamName();

  HWND hWnd;
  Dagormat2Dialog *parent;
  std::wstring shader;
  std::vector<std::wstring> selected_names;
};

class ShaderClassDialog
{
public:
  ShaderClassDialog(Dagormat2Dialog *p);
  virtual ~ShaderClassDialog() = default;

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

  // false when CreateDialogParam() refused: the widget then holds its script line and nothing else
  bool hasWindow() const { return hPanel != NULL; }

  virtual bool IsGroup() const { return false; }

protected:
  Dagormat2Dialog *parent;
};

class WidgetText : public AbstractWidget
{
public:
  WidgetText(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetText() override = default;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  void GetValue();
  void SetValue(const std::wstring &v);

  ControlPtr<ICustEdit> edit;
};

class WidgetColor : public AbstractWidget
{
public:
  WidgetColor(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetColor() override = default;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  void GetValue();
  void SetValue(const std::wstring &v);

  ControlPtr<IColorSwatch> col;
};

class WidgetBool : public AbstractWidget
{
public:
  WidgetBool(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetBool() override = default;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  void GetValue();
  void SetValue(const std::wstring &v);
};

class WidgetNumeric : public AbstractWidget
{
public:
  WidgetNumeric(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetNumeric() override;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  bool isOutOfRange() const;

private:
  void GetValue();
  void SetValue(const std::wstring &v);
  void UpdateColorSwatch(float f[4]);

  bool is_float;
  bool is_color_ui;
  bool is_spinner_used;
  int size;
  ControlPtr<ISpinnerControl> spinner[4];
  ControlPtr<IColorSwatch> col;
  HPEN hFramePen;
};

class WidgetGroup : public AbstractWidget
{
public:
  WidgetGroup(const ParamInfo &pinfo, Dagormat2Dialog *p);
  ~WidgetGroup() override = default;

  INT_PTR WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
  HWND hShader, hParam;
  std::vector<HWND> rollupPages;
  bool valid;
  bool isActive;
  bool paramsMirrorScript; // false until RestoreParams() has built the widgets out of the script
  ControlPtr<IColorSwatch> csa, csd, css, cse;
  TexDADMgr dadmgr;

  std::array<ControlPtr<ICustButton>, NUMTEXMAPS> texbut;
  POINT paramOrg;
  HPEN hFramePen;
  ToolTipExtender tooltip;

  std::vector<std::unique_ptr<AbstractWidget>> parameters;

  fs::path proxyPath;
  FilterList filterList;
  static const TCHAR *defaultExtension;

  Dagormat2Dialog(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m);
  ~Dagormat2Dialog() override;

  HWND AddPage(int res_id, DLGPROC proc, const wchar_t *title);

  void RestoreParams();
  void SaveParams();
  void DeleteAllParams();

  std::wstring GetShaderName();
  void UpdateShaderNameWidget();
  void ChangeShaderName();
  void Update2SidedWidget();
  void AddParam(ParamInfo param);
  AbstractWidget *GetParam(std::wstring_view name);
  void RemParam(std::wstring_view name);
  void RemParamGroup(std::wstring_view group_name);
  void SetScript(const std::wstring &script);
  void MarkUnknownParams(const DataBlock *dataBlk);
  void setToolTip(HWND hWnd, const std::wstring &hint, HWND hExclude);
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

  template <typename T>
  void AppendDialog(long Resource, T *widget);

  void ImportProxymat();
  void ExportProxymat();
};

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
  std::wstring classname, script;
  DagorMat2PostLoadCallback postLoadCallback;

  DagorMat2(BOOL loading);
  ~DagorMat2() override;
  void NotifyChanged();
  void ClearDlg(Dagormat2Dialog *d)
  {
    if (dlg == d)
      dlg = NULL;
  }
  void RestoreDialogParams();
  void fill_texmap_slot(int i, Texmap *m);

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
  void GetClassName(TSTR &s, bool localized) const override { s = GetString(IDS_DAGORMAT2); }

  void DeleteThis() override;

  ULONG Requirements(int subMtlNum) override;
  ULONG LocalRequirements(int subMtlNum) override;
  int NumSubs() override;
  Animatable *SubAnim(int i) override;
  MSTR SubAnimName(int i, bool localized) override;
  int SubNumToRefNum(int subNum) override;

  // From ref
  int NumRefs() override;
  RefTargetHandle GetReference(int i) override;
  void SetReference(int i, RefTargetHandle rtarg) override;

  IOResult Save(ISave *isave) override;
  IOResult Load(ILoad *iload) override;

  RefTargetHandle Clone(RemapDir &remap) override;

  RefResult NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
    BOOL propagate) override;

  BOOL SupportTexDisplay() override;
  BOOL SupportsMultiMapsInViewport() override;
  void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb) override;
  void ActivateTexDisplay(BOOL onoff) override;
  void DiscardTexHandles();
  void updateViewportTexturesState();
  bool hasAlpha();
};

void Dagormat2Dialog::setToolTip(HWND hWnd, const std::wstring &hint, HWND hExclude)
{
  if (hWnd == hExclude)
    return;

  tooltip.SetToolTip(hWnd, hint.data());
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
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
  SClass_ID SuperClassID() override { return MATERIAL_CLASS_ID; }
  Class_ID ClassID() override { return DagorMat2_CID; }
  const TCHAR *Category() override { return _T(""); }
};
static MaterClassDesc2 materCD;
ClassDesc *GetMaterCD2() { return &materCD; }

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
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
      for (size_t i = 0; i < dlg->texbut.size(); ++i)
      {
        dlg->texbut[i].reset(GetICustButton(GetDlgItem(hWnd, texidc[i])));
        dlg->texbut[i]->SetDADMgr(&dlg->dadmgr);
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
        std::wstring shader = dlg->GetShaderName();
        NewParameterDialog new_par(dlg, shader);
        if (new_par.DoModal() == IDOK)
        {
          const auto &names = new_par.GetNewParamNames();
          const std::vector<ParamInfo> shader_params = get_blk_shader_params(get_blk_shader(get_shared_blk(), shader));
          for (const auto &name : names)
          {
            const ParamInfo param = get_param_info(shader_params, shader, name);
            append_param_line(dlg->theMtl->script, param.name, param.value);
          }
          dlg->RestoreParams();
        }
      }
      else if (wParam == MAKEWPARAM(IDC_PARAM_DELETE_ALL, BN_CLICKED) &&
               MessageBox(hWnd, L"Delete all parameters? Are you sure?", L"Delete all", MB_ICONQUESTION | MB_YESNO) == IDYES)
      {
        dlg->DeleteAllParams();
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
  }
  else if ((dlg = (Dagormat2Dialog *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
    return FALSE;

  Autotoggle guard(dlg->isActive);

  switch (msg)
  {
    case WM_INITDIALOG:
      dlg->csa.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_AMB), dlg->theMtl->cola, GetString(IDS_AMB_COLOR)));
      dlg->csd.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_DIFF), dlg->theMtl->cold, GetString(IDS_DIFF_COLOR)));
      dlg->css.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_SPEC), dlg->theMtl->cols, GetString(IDS_SPEC_COLOR)));
      dlg->cse.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_EMIS), dlg->theMtl->cole, GetString(IDS_EMIS_COLOR)));
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
          cs = dlg->csa.get();
          pbid = PB_AMB;
          break;

        case IDC_DIFF:
          cs = dlg->csd.get();
          pbid = PB_DIFF;
          break;

        case IDC_SPEC:
          cs = dlg->css.get();
          pbid = PB_SPEC;
          break;

        case IDC_EMIS:
          cs = dlg->cse.get();
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
        if (get_open_filename(hWnd, L"Import from proxymat...", dlg->filterList, dlg->defaultExtension, dlg->proxyPath))
          dlg->ImportProxymat();
      }
      else if (wParam == MAKEWPARAM(IDC_PROXY_EXPORT, BN_CLICKED))
      {
        if (get_save_filename(hWnd, L"Export to proxymat...", dlg->filterList, dlg->defaultExtension, dlg->proxyPath, false))
          dlg->ExportProxymat();
      }
      break;

    default: return FALSE;
  }

  return TRUE;
}

const TCHAR *Dagormat2Dialog::defaultExtension = L"proxymat.blk";

// constraint: the destructor detaches only the pages recorded here, so a page added elsewhere keeps a dangling dialog pointer
HWND Dagormat2Dialog::AddPage(int res_id, DLGPROC proc, const wchar_t *title)
{
  HWND page = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(res_id), proc, title, (LPARAM)this);
  rollupPages.push_back(page);
  return page;
}

Dagormat2Dialog::Dagormat2Dialog(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m) : isActive(false), paramsMirrorScript(false)
{
  dadmgr.Init(this);
  hwmedit = hwMtlEdit;
  ip = imp;
  theMtl = m;
  valid = FALSE;
  paramOrg.x = paramOrg.y = 0;

  filterList.Append(_T("Proxymat (proxymat.blk)"));
  filterList.Append(_T("*.proxymat.blk"));

  hShader = AddPage(IDD_DAGORMAT2_SHADER, ShaderDlgProc, L"Shader class");
  AddPage(IDD_DAGORMAT2_PHONG, PhongDlgProc, L"Phong parameters");
  AddPage(IDD_DAGORMAT2_TEX, TexDlgProc, L"Texture slots");
  hParam = AddPage(IDD_DAGORMAT2_PARAM, ParamDlgProc, L"Shader parameters");
  AddPage(IDD_DAGORMAT2_PROXY, ProxyDlgProc, L"Proxymat import/export");

  hFramePen = (HPEN)CreatePen(PS_SOLID, 2, RGB(56, 56, 56));
}

Dagormat2Dialog::~Dagormat2Dialog()
{
  for (HWND page : rollupPages)
    SetWindowLongPtr(page, GWLP_USERDATA, NULL);
  theMtl->ClearDlg(this);
  SaveParams();
  DeleteObject(hFramePen);
}

void Dagormat2Dialog::MarkUnknownParams(const DataBlock *dataBlk)
{
  if (!dataBlk)
    return;

  std::wstring shader_name = GetShaderName();
  if (shader_name.empty())
    return;

  const DataBlock *shader_blk = get_blk_shader(dataBlk, shader_name);
  if (!shader_blk)
    return;

  std::vector<ParamInfo> params = get_blk_shader_params(shader_blk);

  for (auto &p : parameters)
  {
    if (!p->hasWindow())
      continue;

    HWND hDel = GetDlgItem(p->hPanel, IDC_PARAM_DELETE);

    if (p->param.is_group)
    {
      tooltip.SetToolTip(hDel, (L"Delete group " + p->param.name).data());
      continue;
    }
    else
      tooltip.SetToolTip(hDel, (L"Delete parameter " + p->param.name).data());

    auto it = std::ranges::find_if(params, [&p](ParamInfo &p_i) { return iequal(p_i.name, p->param.name); });

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
      if (!w->hasWindow())
      {
        ++i;
        continue;
      }

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
      if (w->hasWindow() && w->IsGroup())
      {
        rc = w->screen_rc;
        collect_vertical_size = true;
      }

      ++i;
    }
  }

  return rect;
}


static std::optional<ParamInfo> find_param_info(const std::vector<ParamInfo> &params, std::wstring_view name)
{
  auto it = std::ranges::find_if(params, [name](const ParamInfo &p) { return !p.is_group && p.name == name; });
  if (it != params.end())
    return *it;
  return std::nullopt;
}

static std::vector<ParamInfo> get_script_params(const std::vector<ParamInfo> &shader_params, std::wstring_view classname,
  std::wstring_view script)
{
  std::vector<ParamInfo> params;
  for (const std::wstring &line : split(simplifyRN(script), L'\n'))
    if (!line.empty())
      params.emplace_back(get_blk_param_info_value(shader_params, line, classname));

  return params;
}

// the parameters of a material in the order the dialog lays them out: those the shader declares
// outside any group, then each group it declares, then what the script holds and the shader does not
// know. group() comes before the parameters it holds, and param() stops the walk by returning false.
template <typename Group, typename Param>
static void for_each_material_param(const std::vector<ParamInfo> &all_params, const std::vector<ParamInfo> &script_params,
  Group &&group, Param &&param)
{
  std::vector<ParamInfo> params;
  auto emit = [&](const ParamInfo &grp) {
    if (params.empty())
      return true;

    group(grp);
    for (const ParamInfo &p : params)
      if (!param(p, grp))
        return false;

    return true;
  };

  for (const ParamInfo &p : all_params)
    if (!p.is_group && p.parent.empty())
      if (auto res = find_param_info(script_params, p.name))
        params.emplace_back(*res);

  ParamInfo uncategorized;
  uncategorized.is_group = true;
  uncategorized.name = L"Uncategorized";
  if (!emit(uncategorized))
    return;

  for (const ParamInfo &grp : all_params)
  {
    if (!grp.is_group)
      continue;

    params.clear();
    for (const ParamInfo &p : all_params)
      if (!p.is_group && p.parent == grp.name)
        if (auto res = find_param_info(script_params, p.name))
          params.emplace_back(*res);

    if (!emit(grp))
      return;
  }

  // real_two_sided is a radio button of its own, not a parameter of the material
  params.clear();
  for (const ParamInfo &p : script_params)
    if (!iequal(p.name, REAL_TWO_SIDED) && !find_param_info(all_params, p.name))
      params.emplace_back(p);

  ParamInfo unknown;
  unknown.is_group = true;
  unknown.name = L"Unknown";
  emit(unknown);
}

void Dagormat2Dialog::RestoreParams()
{
  parameters.clear();
  paramsMirrorScript = false;
  tooltip.RemoveToolTips();

  std::wstring shader_name = theMtl->classname.data();
  if (shader_name.empty())
    return;

  UpdateShaderNameWidget();

  // get list of shader parameters (including group names)
  const DataBlock *shader_blk = get_blk_shader(get_shared_blk(), shader_name);
  std::vector<ParamInfo> all_params = get_blk_shader_params(shader_blk);

  const std::vector<ParamInfo> script_params = get_script_params(all_params, shader_name, theMtl->script);

  // "real_two_sided" is treated as a radio button
  for (const ParamInfo &param : script_params)
    if (iequal(param.name, REAL_TWO_SIDED))
    {
      if (iequal(param.value, L"yes"))
        theMtl->twosided = IDagorMat::Sides::RealDoubleSided;
      else if (theMtl->twosided == IDagorMat::Sides::RealDoubleSided)
        theMtl->twosided = IDagorMat::Sides::OneSided;
      Update2SidedWidget();
      break;
    }

  for_each_material_param(
    all_params, script_params,
    [this](const ParamInfo &group) { parameters.emplace_back(std::unique_ptr<WidgetGroup>(new WidgetGroup(group, this))); },
    [this](ParamInfo param, const ParamInfo &group) {
      param.parent = group.name;
      AddParam(param);
      return true;
    });

  paramsMirrorScript = true;
  SaveParams();
  DialogsReposition();
  MarkUnknownParams(get_shared_blk());
}

// an emptied list stands for the whole script on purpose, unlike one RestoreParams() never filled,
// so this is the one caller that lifts the guard in SaveParams()
void Dagormat2Dialog::DeleteAllParams()
{
  parameters.clear();
  paramsMirrorScript = true;
  SaveParams();
  RestoreParams();
}

void Dagormat2Dialog::SaveParams()
{
  // the widgets only stand for the whole script once RestoreParams() has run through. Writing them
  // back before that would cut the script down to whatever few of them exist.
  if (!paramsMirrorScript)
    return;

  std::wstring buffer;

  for (size_t i = 0; i < parameters.size(); ++i)
  {
    AbstractWidget *p = parameters[i].get();

    if (!p || p->param.is_group || p->param.name.empty())
      continue;

    // the script Max keeps is the only text where the ':type' survives; every serializer out of the plugin drops it
    append_param_line(buffer, p->param.name_with_type(), p->param.value);
  }

  buffer += (theMtl->twosided == IDagorMat::Sides::RealDoubleSided) ? _T("real_two_sided=yes\r\n") : _T("real_two_sided=no\r\n");

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

std::wstring Dagormat2Dialog::GetShaderName() { return get_window_text(GetDlgItem(hShader, IDC_CLASSNAME)); }

void Dagormat2Dialog::UpdateShaderNameWidget()
{
  HWND hwnd = GetDlgItem(hShader, IDC_CLASSNAME);
  SetWindowText(hwnd, theMtl->classname.data());
  tooltip.SetToolTip(hwnd, theMtl->classname.data());
}

void Dagormat2Dialog::ChangeShaderName()
{
  std::wstring str = GetShaderName();
  theMtl->classname = str.c_str();
  UpdateShaderNameWidget();
  theMtl->NotifyChanged();
  SaveParams();
  RestoreParams();
  DialogsReposition();
}

void Dagormat2Dialog::Update2SidedWidget()
{
  CheckDlgButton(hShader, IDC_BACKFACE_1, theMtl->twosided == IDagorMat::Sides::OneSided);
  CheckDlgButton(hShader, IDC_BACKFACE_2, theMtl->twosided == IDagorMat::Sides::DoubleSided);
  CheckDlgButton(hShader, IDC_BACKFACE_REAL2, theMtl->twosided == IDagorMat::Sides::RealDoubleSided);
}

static std::wstring fix_empty_param(DataBlock::ParamType type, std::wstring_view value)
{
  return value.empty() ? get_default_param_value(type) : std::wstring(value);
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

AbstractWidget *Dagormat2Dialog::GetParam(std::wstring_view name)
{
  for (auto &p : parameters)
    if (iequal(p->param.name, name))
      return p.get();
  return nullptr;
}

void Dagormat2Dialog::RemParam(std::wstring_view name)
{
  SetScript(drop_param_lines(theMtl->script, [name](std::wstring_view param_name) { return iequal(param_name, name); }));
}

void Dagormat2Dialog::RemParamGroup(std::wstring_view group_name)
{
  SetScript(drop_param_lines(theMtl->script, [this, group_name](std::wstring_view param_name) {
    auto it = std::ranges::find_if(parameters,
      [param_name](const std::unique_ptr<AbstractWidget> &w) { return !w->param.is_group && iequal(w->param.name, param_name); });
    return it != parameters.end() && (*it)->param.parent == group_name;
  }));
}

void Dagormat2Dialog::SetScript(const std::wstring &script)
{
  theMtl->script = script;
  theMtl->NotifyChanged();
  RestoreParams();
}

template <typename T>
static INT_PTR CALLBACK widget_dlg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

template <typename T>
void Dagormat2Dialog::AppendDialog(long Resource, T *widget)
{
  const std::wstring &name = widget->param.name;

  HWND result = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(Resource), hParam, widget_dlg_proc<T>, (LPARAM)widget);
  if (!result)
  {
    DagorLogWindow::addToLog(DagorLogWindow::LogLevel::Error, L"cannot create the dialog of parameter '%s'\r\n", name.c_str());
    DagorLogWindow::show();
    return;
  }

  // widget_dlg_proc<T> has stored the window in widget->hPanel, from the WM_INITDIALOG above
  ::SetWindowText(result, name.c_str());
  widget->hSType = ::FindWindowEx(result, NULL, L"STATIC", L"StaticType");
  if (widget->hSType)
    ::SetWindowText(widget->hSType, name.c_str());
  ::ShowWindow(result, SW_SHOW);

  widget->creating = false;
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
  if (theMtl)
    theMtl->ClearDlg(this);
  theMtl = (DagorMat2 *)m;
  if (theMtl)
    theMtl->dlg = this;
  ReloadDialog();
  RestoreParams();
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
  if (!proxyBlk.load(proxyPath))
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

  proxyBlk.addStr("class", wideToStr(theMtl->classname).data());
  proxyBlk.addBool("tex16support", true);
  proxyBlk.addBool("twosided", theMtl->twosided == IDagorMat::Sides::DoubleSided);

  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    const TCHAR *tex = theMtl->get_texname(i);
    if (tex)
      proxyBlk.addStr(blk_tex_name(i), wideToStr(tex).data());
  }

  for (const std::wstring &line : split(simplifyRN(theMtl->script), L'\n'))
    if (!line.empty())
      proxyBlk.addStr("script", wideToStr(bare_param_line(line)).data());

  proxyBlk.saveToTextFile(proxyPath);
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

  Reset();
}


DagorMat2::~DagorMat2() { DiscardTexHandles(); }


void DagorMat2::Reset()
{
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
  RestoreDialogParams();
}

ParamDlg *DagorMat2::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
  dlg = new Dagormat2Dialog(hwMtlEdit, imp, this);
  return dlg;
}


static Color blackCol(0, 0, 0);

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


bool DagorMat2::hasAlpha()
{
  return (script.find(L"atest") != std::wstring::npos) || (classname.find(L"alpha") != std::wstring::npos);
}


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
    texHandle[0] = make_vp_tex_handle(texmaps, t, cb, valid);
    if (!texHandle[0])
      return;
  }

  IHardwareMaterial *pIHWMat = (IHardwareMaterial *)GetProperty(PROPID_HARDWARE_MATERIAL);
  if (pIHWMat)
  {
    pIHWMat->SetNumTexStages(1);
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

    mtl->texture[0].textHandle = texHandle[0]->GetHandle();
    mtl->texture[0].colorOp = GW_TEX_REPLACE;
    mtl->texture[0].colorAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].colorScale = GW_TEX_SCALE_1X;
    mtl->texture[0].alphaOp = GW_TEX_REPLACE;
    mtl->texture[0].alphaAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].alphaScale = GW_TEX_SCALE_1X;
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

static void setup_texmap(Texmap *m)
{
  if (!m)
    return;

  m->SetMtlFlag(MTL_HW_TEX_ENABLED);

  if (m->ClassID() != Class_ID(BMTEX_CLASS_ID, 0x00))
    return;

  BitmapTex *bitmap = (BitmapTex *)m;
  IParamBlock2 *pb = bitmap->GetParamBlock(0);
  if (pb)
    pb->SetValue(12, 0, 0);
  bitmap->SetAlphaSource(ALPHA_FILE);
  bitmap->SetAlphaAsMono(TRUE);
}

void DagorMat2::fill_texmap_slot(int i, Texmap *m)
{
  setup_texmap(m);
  texmaps->settex(i, m);
  NotifyChanged();
}

void DagorMat2::SetSubTexmap(int i, Texmap *m)
{
  fill_texmap_slot(i, m);
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

MSTR DagorMat2::SubAnimName(int i, bool localized)
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
  mtl->classname = (!classname.empty() ? classname : DEFAULT_SHADER_NAME);
  mtl->script = script;
  BaseClone(this, mtl, remap);
  return mtl;
}

RefResult DagorMat2::NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
  BOOL propagate)
{
  switch (message)
  {
    case REFMSG_CHANGE:
    case REFMSG_TARGET_DELETED:
      if (hTarget == pblock)
        ivalid.SetEmpty();
      if (dlg && dlg->theMtl == this && !dlg->isActive)
        dlg->Invalidate();
      DiscardTexHandles();
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
      return REF_STOP;
    }

    case REFMSG_GET_PARAM_NAME_NONLOCALIZED:
    {
      GetParamName *gpn = (GetParamName *)partID;
      gpn->name.printf(_T("param#%d"), gpn->index);
      return REF_STOP;
    }
  }

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
  res = isave->WriteCString(classname.data());
  if (res != IO_OK)
    return res;
  isave->EndChunk();
  isave->BeginChunk(CH_SCRIPT);
  res = isave->WriteCString(script.data());
  if (res != IO_OK)
    return res;
  isave->EndChunk();
  return IO_OK;
}

IOResult DagorMat2::Load(ILoad *iload)
{
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

  DiscardTexHandles();
  iload->RegisterPostLoadCallback(&postLoadCallback);

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

const TCHAR *DagorMat2::get_classname() { return classname.data(); }

const TCHAR *DagorMat2::get_script() { return script.data(); }

const TCHAR *DagorMat2::get_texname(int i) { return texmaps->gettexname(i); }

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
  std::wstring shader_name = classname.data();
  if (shader_name.empty())
    return;

  // get list of shader parameters (including group names)
  const DataBlock *shader_blk = get_blk_shader(get_shared_blk(), shader_name);
  std::vector<ParamInfo> all_params = get_blk_shader_params(shader_blk);

  const std::vector<ParamInfo> script_params = get_script_params(all_params, shader_name, script);

  for_each_material_param(
    all_params, script_params, [](const ParamInfo &) {},
    [&cb](const ParamInfo &param, const ParamInfo &group) {
      return cb.proc(group.name.data(), param.name.data(), int(param.type), param.value.data()) != ECB_STOP;
    });
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
  if (s && *s)
    classname = s;
  else
    classname = DEFAULT_SHADER_NAME;
  NotifyChanged();
  RestoreDialogParams();
}

void DagorMat2::set_script(const TCHAR *s)
{
  if (s)
    script = s;
  else
    script = _T("");
  NotifyChanged();
  RestoreDialogParams();
}

void DagorMat2::RestoreDialogParams()
{
  if (dlg && dlg->theMtl == this)
    dlg->RestoreParams();
}

void DagorMat2::set_texname(int i, const TCHAR *s)
{
  if (i < 0 || i >= NUMTEXMAPS)
    return;

  const std::wstring path = (s && *s) ? resolve_tex_path(s).native() : std::wstring();

  // a change notification throws away the viewport texture handles, so ask for one only when the
  // slot does not already hold what was asked for. A kept slot keeps its own settings too: what
  // the file or a clone put there wins over the defaults setup_texmap() would impose.
  if (texmaps->holds_texname(i, path))
    return;

  BitmapTex *bm = NULL;
  if (!path.empty())
  {
    bm = NewDefaultBitmapTex();
    assert(bm);
    bm->SetMapName(path.c_str());
  }
  fill_texmap_slot(i, bm);
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

NewParameterDialog::NewParameterDialog(Dagormat2Dialog *p, std::wstring_view shdr) : parent(p), hWnd(NULL), shader(shdr) {}

int NewParameterDialog::DoModal()
{
  return ::DialogBoxParam(hInstance, (const TCHAR *)IDD_DAGORPAR_NEW, parent->hParam, NewParameterDialogProc, (LPARAM)this);
}

static bool is_parameter_name_valid(std::wstring_view name)
{
  return std::ranges::all_of(name,
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

      const DataBlock *shader_blk = get_blk_shader(get_shared_blk(), shader);
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

    case WM_DESTROY: detach_layout_from_dialog(hWnd); return 0;

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
    // LB_GETTEXT takes no buffer size, so the length has to be asked for up front
    const int len = ListBox_GetTextLen(lb, sel_items[i]);
    if (len < 0) // LB_ERR
      continue;

    std::wstring text(size_t(len) + 1, L'\0');
    const int copied = ListBox_GetText(lb, sel_items[i], text.data());
    if (copied < 0) // LB_ERR
      continue;

    text.resize(copied);
    selected.push_back(std::move(text));
  }

  return selected;
}

bool NewParameterDialog::GetAndVerifyParamName()
{
  std::vector<std::wstring> selected = GetSelectedNames();

  if (selected.empty())
  {
    // no selection, check the edit field

    const wchar_t *err = nullptr;

    std::wstring name = get_window_text(GetDlgItem(hWnd, IDC_PARAM_NAME_EDIT));

    if (name.empty())
      err = _T("Name is empty");

    if (!is_parameter_name_valid(name))
      err = L"Can't assign parameter,\nit contains non-supported characters.\n\nAllowed characters: "
            L"'A'..'Z', 'a'..'z', '0'..'9', '_'";

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

  auto is_selected = [&selected](std::wstring_view name) -> bool { return std::ranges::find(selected, name) != selected.end(); };

  const DataBlock *shader_blk = get_blk_shader(get_shared_blk(), shader);
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
  {
    MessageBox(hWnd, L"The material already has every parameter that is selected", L"Error", MB_OK | MB_ICONSTOP);
    return false;
  }

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

      std::vector<std::wstring> shader_list = get_blk_shader_list(get_shared_blk());
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

    case WM_DESTROY: detach_layout_from_dialog(this->hWnd); return 0;

    default: return FALSE;
  }

  return TRUE;
}

bool ShaderClassDialog::GetName()
{
  std::wstring s = get_window_text(::GetDlgItem(hWnd, IDC_SHADER_CLASS_LIST));

  if (s.empty())
  {
    MessageBox(hWnd, _T("Empty shader class is not allowed"), _T("Class selection error"), MB_ICONERROR | MB_OK);
    return false;
  }

  if (s.starts_with(L'-'))
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
  hPanel(NULL), hSType(NULL), creating(TRUE), param(pinfo_), screen_rc{}, parent(p)
{}

AbstractWidget::~AbstractWidget()
{
  // the derived part is gone by now, so nothing DestroyWindow() sends may reach this object
  SetWindowLongPtr(hPanel, GWLP_USERDATA, NULL);
  ::DestroyWindow(hPanel);
}

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

WidgetText::WidgetText(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  p->AppendDialog(IDD_DAGORPAR_TEXT, this);
}

INT_PTR WidgetText::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
      edit.reset(::GetICustEdit(GetDlgItem(hWnd, IDC_PAR_TEXT_VALUE)));
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

void WidgetText::SetValue(const std::wstring &v)
{
  edit->SetText(v.c_str());
  param.value = v;
}

//////////////////////////////////////////////////////////////////////

WidgetColor::WidgetColor(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  p->AppendDialog(IDD_DAGORPAR_COLOR, this);
}

INT_PTR WidgetColor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      AColor c;
      col.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter")));
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
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

  param.value = std::format(L"{:.3f},{:.3f},{:.3f},{:.3f}", c.r, c.g, c.b, c.a);
  parent->SaveParams();
}

void WidgetColor::SetValue(const std::wstring &v)
{
  float r = 0, g = 0, b = 0, a = 0;
  if (parse_nums(wideToStr(v), r, g, b, a) != 4)
    r = g = b = a = 0;
  col->SetAColor(AColor(r, g, b, a), FALSE);

  param.value = v;
}

//////////////////////////////////////////////////////////////////////

WidgetBool::WidgetBool(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  p->AppendDialog(IDD_DAGORPAR_BOOL, this);
}

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

void WidgetBool::SetValue(const std::wstring &v)
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
  AbstractWidget(param, p), is_float(true), is_color_ui(false), is_spinner_used(false), size(4)
{
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
  p->AppendDialog(idd, this);

  hFramePen = (HPEN)CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
}

WidgetNumeric::~WidgetNumeric() { DeleteObject(hFramePen); }

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
        col.reset(GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter")));
      }
      for (int i = 0; i < size; ++i)
      {
        long idc_spin = widget_p4_idc[i][0];
        long idc_edit = widget_p4_idc[i][1];
        if (is_float)
          spinner[i].reset(::SetupFloatSpinner(hWnd, idc_spin, idc_edit, -FLT_MAX, +FLT_MAX, 0));
        else
          spinner[i].reset(::SetupIntSpinner(hWnd, idc_spin, idc_edit, -INT_MAX, +INT_MAX, 0));
      }
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      SetValue(param.value);
      InvalidateRect(hWnd, NULL, TRUE);
      UpdateWindow(hWnd);
      parent->MarkUnknownParams(get_shared_blk());
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
            spinner[i]->SetValue(f, FALSE);
          else
            spinner[i]->SetValue(int(f), FALSE);
        }
        GetValue();
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        parent->MarkUnknownParams(get_shared_blk());
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
      spinner[3]->SetValue(c.a, FALSE);
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
  std::wostringstream str;
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

void WidgetNumeric::SetValue(const std::wstring &v)
{
  float f[4] = {};
  std::wistringstream str(v);
  str.imbue(std::locale::classic());
  for (int i = 0; i < size; ++i)
  {
    if (is_float)
    {
      str >> f[i];
      spinner[i]->SetValue(f[i], FALSE);
    }
    else
    {
      int d;
      str >> d;
      spinner[i]->SetValue(d, FALSE);
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
  c.r = std::clamp(f[0] / k, 0.f, 1.f);
  c.g = std::clamp(f[1] / k, 0.f, 1.f);
  c.b = std::clamp(f[2] / k, 0.f, 1.f);
  c.a = std::clamp(f[3] / k, 0.f, 1.f);
  col->SetAColor(c, FALSE);
}

//////////////////////////////////////////////////////////////////////

WidgetGroup::WidgetGroup(const ParamInfo &param, Dagormat2Dialog *p) : AbstractWidget(param, p)
{
  p->AppendDialog(IDD_DAGORPAR_GROUP, this);
}

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
    if (!par.hasWindow())
      continue; // its rectangle would carry into every widget below

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

std::wstring fix_param_values(std::wstring_view script, std::wstring_view classname,
  std::wstring (*fix)(DataBlock::ParamType, std::wstring_view))
{
  std::wstring buffer;
  auto params = get_blk_params(get_shared_blk(), script, classname);
  // the bare name, never name_with_type(): this text is what the export writes into the material, and
  // the engine reads that as cfg text, where the key 'atest:i' would not resolve to the variable atest
  for (const ParamInfo &param : params)
    append_param_line(buffer, param.name, fix(param.type, param.value));
  return buffer;
}

template <typename T>
std::wstring normalize_value(std::wstring_view v)
{
  std::wistringstream iss{std::wstring(v)};
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

static std::wstring normalize_param(DataBlock::ParamType type, std::wstring_view raw)
{
  std::wstring value(raw);
  trim(value);
  value = fix_empty_param(type, value);

  // a parameter declared as text may still hold a number, and "1, 2" has to normalize the same way
  // whatever it was declared as. Text that is not a number stays text.
  if (type == DataBlock::ParamType::TYPE_STRING)
    type = guess_blk_type_by_value(value);

  switch (type)
  {
    case DataBlock::ParamType::TYPE_BOOL:
    {
      if (iequal(value, L"no") || iequal(value, L"false") || value == L"0")
        return L"no";
      if (iequal(value, L"yes") || iequal(value, L"true") || value == L"1")
        return L"yes";
      return std::wstring(raw); // cannot recognize the value, return as is
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

  return std::wstring(raw); // unknown type, return as is
}

std::wstring fix_empty_param_values(std::wstring_view script, std::wstring_view classname)
{
  return fix_param_values(script, classname, fix_empty_param);
}

std::wstring normalize_param_values(std::wstring_view script, std::wstring_view classname)
{
  return fix_param_values(script, classname, normalize_param);
}
