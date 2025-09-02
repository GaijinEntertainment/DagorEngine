// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vector>
#include <array>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>

#include <max.h>
#include <stdmat.h>
#include <math.h>
#include <iparamb2.h>

#include "dagor.h"
#include "mater.h"
#include "texmaps.h"
#include "resource.h"
#include "cfg.h"
#include "debug.h"

#include <d3dx9.h>
#include <d3d9types.h>
#include <ihardwarematerial.h>
#include "dag_auto_ptr.h"

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

//////////////////////////////////////////////////////////////////////////////////

#define TX_MODULATE   0
#define TX_ALPHABLEND 1
#define BMIDATA(x)    ((UBYTE *)((BYTE *)(x) + sizeof(BITMAPINFOHEADER)))

static const TSTR default_shader_name(_T("gi_black"));
static const TSTR dagor_shaders_config(_T("DagorShaders.cfg"));
static const TCHAR *real_two_sided(_T("real_two_sided"));

class MaterDlg2;

class MaterParNew2
{
public:
  MaterParNew2(MaterDlg2 *p, M_STD_STRING &shdr);
  virtual ~MaterParNew2();

  M_STD_STRING GetNewParamName() const { return param_name; }

  int DoModal();
  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  M_STD_STRING GetParamName();

  HWND hWnd;
  MaterDlg2 *parent;
  M_STD_STRING shader;
  M_STD_STRING param_name;
};


class MaterPar2 : public ParamDlg
{
public:
  //////////////////////////

  HWND hPanel;
  HWND hSType;
  BOOL creating;

  M_STD_STRING name;
  M_STD_STRING value;

  //////////////////////////

  MaterPar2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterPar2() override;

  //////////////////////////

  void ReloadDialog() override {}
  Class_ID ClassID() override { return DagorMat2_CID; }
  void SetThing(ReferenceTarget *m) override {}
  ReferenceTarget *GetThing() override { return NULL; }
  void DeleteThis() override { delete this; }
  void SetTime(TimeValue t) override {}
  void ActivateDlg(BOOL onOff) override {}

  //////////////////////////

  virtual const TCHAR *GetType() const = 0;

protected:
  MaterDlg2 *parent;
};

class MaterParText2 : public MaterPar2
{
public:
  MaterParText2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParText2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("t"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);

  ICustEdit *edit;
};

class MaterParReal2 : public MaterPar2
{
public:
  MaterParReal2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParReal2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("r"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);

  ISpinnerControl *spinner;
};

class MaterParEnum2 : public MaterPar2
{
public:
  MaterParEnum2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParEnum2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("e"); }

  int GetCount();

  TCHAR *GetRadioName(int at);
  int GetRadioState(int at);

  int GetRadioSelected();

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);

  int count;
};

class MaterParColor2 : public MaterPar2
{
public:
  MaterParColor2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParColor2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("c"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);

  IColorSwatch *col;
};

class MaterParBool2 : public MaterPar2
{
public:
  MaterParBool2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParBool2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("b"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);
};

class MaterParInt2 : public MaterPar2
{
public:
  MaterParInt2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p);
  ~MaterParInt2() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  const TCHAR *GetType() const override { return _T("i"); }

private:
  void GetValue();
  void SetValue(const M_STD_STRING v);

  ISpinnerControl *spinner;
};

///////////////////////////////////////////////////////////////////////////

class MaterDlg2 : public ParamDlg
{
public:
  void DialogsReposition();
  HWND hwmedit;
  IMtlParams *ip;
  class DagorMat2 *theMtl;
  HWND hPanel;
  BOOL valid;
  BOOL isActive;
  IColorSwatch *csa, *csd, *css, *cse;
  BOOL creating;
  TexDADMgr dadmgr;

  std::array<ICustButton *, NUMTEXMAPS> texbut;
  POINT paramOrg;

  std::vector<std::unique_ptr<MaterPar2>> parameters;

  MaterDlg2(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m);
  ~MaterDlg2() override;

  void RestoreParams();
  void SaveParams();

  void FillSlotNames();

  M_STD_STRING GetShaderName();
  void UpdateShaderName();
  void ChangeShaderName();
  void UpdateEnum2Sided();
  void AddParam(int type, M_STD_STRING name, M_STD_STRING value = M_STD_STRING());
  MaterPar2 *GetParam(M_STD_STRING name);
  void RemParam(M_STD_STRING name);
  void MarkUnknownParams(CfgShader &cfg);

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
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
};

static const size_t MAX_PARAM_DLGS = 20;
static const int PARAM_DLG_GAP = 5;

//////////////////////////////////////////////////////////////////////////////////

class DagorMat2PostLoadCallback : public PostLoadCallback
{
public:
  DagorMat2 *parent;

  DagorMat2PostLoadCallback() { parent = NULL; }

  void proc(ILoad *iload) override;
};


class DagorMat2 : public Mtl, public IDagorMat
{
public:
  class MaterDlg2 *dlg;
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

static INT_PTR CALLBACK PanelDlgProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterDlg2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterDlg2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
    dlg->hPanel = hWnd;
  }
  else
  {
    if ((dlg = (MaterDlg2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}


MaterDlg2::MaterDlg2(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m)
{
  dadmgr.Init(this);
  hwmedit = hwMtlEdit;
  ip = imp;
  theMtl = m;
  valid = FALSE;
  creating = TRUE;
  std::fill(texbut.begin(), texbut.end(), nullptr);
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2), PanelDlgProc2, GetString(IDS_DAGORMAT_LONG), (LPARAM)this);
  creating = FALSE;
}

MaterDlg2::~MaterDlg2()
{
  SaveParams();
  ReleaseIColorSwatch(csa);
  ReleaseIColorSwatch(csd);
  ReleaseIColorSwatch(css);
  ReleaseIColorSwatch(cse);
  for (ICustButton *cb : texbut)
    ReleaseICustButton(cb);
  SetWindowLongPtr(hPanel, GWLP_USERDATA, NULL);
}

void MaterDlg2::MarkUnknownParams(CfgShader &cfg)
{
  M_STD_STRING sh = GetShaderName();
  if (sh.empty())
    return;

  cfg.GetShaderParams(sh);

  for (size_t i = 0; i < parameters.size(); ++i)
  {
    MaterPar2 *p = parameters[i].get();
    if (p->name.empty())
      continue;

    // mark unknown parameter
    if (std::find(cfg.shader_params.begin(), cfg.shader_params.end(), p->name) == cfg.shader_params.end())
    {
      if (p->hSType)
        ::SetWindowText(p->hSType, (_T("*") + p->name).data());

      std::wstringstream ss;
      ss << p->name << _T("\n*not specified in the shader\n") << sh;
      DagorMat2::tooltip.SetToolTip(p->hPanel, ss.str().data());
    }
    else
      DagorMat2::tooltip.SetToolTip(p->hPanel, p->name.data());
  }
}

static M_STD_STRING simplifyRN(const M_STD_STRING &from)
{
  M_STD_STRING s = from;
  size_t pos = 0;
  while ((pos = s.find(_T("\r\n"), pos)) != std::string::npos)
    s.replace(pos, 2, _T("\n"));
  return s;
}

static int deserialize_param_type(const std::wstring &s)
{
  if (s == _T("t"))
    return CFG_TEXT;

  if (s == _T("b"))
    return CFG_BOOL;

  if (s == _T("i"))
    return CFG_INT;

  if (s == _T("r"))
    return CFG_REAL;

  if (s == _T("c"))
    return CFG_COLOR;

  if (s == _T("e"))
    return CFG_ENUM;

  return CFG_UNKNOWN;
}

static std::vector<std::wstring> split(const std::wstring &text, const wchar_t delim)
{
  std::vector<std::wstring> tokens;
  std::wistringstream iss(text + delim);
  std::wstring tok;
  while (std::getline(iss, tok, delim))
    tokens.push_back(tok);
  return tokens;
}

static bool get_param_type(CfgShader cfg, const std::wstring classname, const std::wstring name, const std::wstring value, int &type)
{
  type = CFG_UNKNOWN;

  if (!classname.empty())
  {
    type = cfg.GetParamType(classname, name);
    if (type != CFG_UNKNOWN)
      return true;
  }

  type = cfg.GetParamType(CFG_GLOBAL_NAME, name);
  if (type != CFG_UNKNOWN)
    return true;

  type = CFG_TEXT;
  return true;
}

static bool get_param_name_value_type(CfgShader cfg, const std::wstring &line, const std::wstring &classname, std::wstring &name,
  std::wstring &value, int &type)
{
  // split the line into name and value
  std::vector<std::wstring> tokens = split(line, _T('='));
  if (tokens.size() != 2)
    return false;

  if (tokens[0].empty())
    return false;

  name = tokens[0];
  value = tokens[1];
  type = CFG_UNKNOWN;

  if (!get_param_type(cfg, classname, name, value, type))
    return false;

  return type != CFG_UNKNOWN;
}

void MaterDlg2::RestoreParams()
{
  parameters.clear();
  DagorMat2::tooltip.RemoveToolTips();

  TCHAR filename[MAX_PATH];
  CfgShader::GetCfgFilename(dagor_shaders_config, filename);
  CfgShader cfg(filename);

  M_STD_STRING script = simplifyRN(M_STD_STRING(theMtl->script));

  // cut the script into lines
  std::vector<std::wstring> lines = split(script, _T('\n'));

  for (std::wstring &line : lines)
    if (!line.empty())
    {
      // split each line into name, value and type (if any)
      int type = CFG_UNKNOWN;
      std::wstring param_name, param_value;
      if (!get_param_name_value_type(cfg, line, theMtl->classname.data(), param_name, param_value, type))
        continue; // invalid name or type

      // "real_two_sided" is treated as a dropdown enum
      if (param_name == real_two_sided)
      {
        ::SendMessage(GetDlgItem(hPanel, IDC_ENUM_2SIDED), CB_SETCURSEL,
          WPARAM(param_value == _T("yes") ? IDagorMat::Sides::RealDoubleSided : IDagorMat::Sides::OneSided), NULL);
        continue;
      }

      AddParam(type, param_name, param_value);
      SaveParams();
    }

  DialogsReposition();
  MarkUnknownParams(cfg);
}

void MaterDlg2::SaveParams()
{
  M_STD_STRING buffer;

  for (size_t i = 0; i < parameters.size(); ++i)
  {
    MaterPar2 *p = parameters[i].get();

    if (!p->name.empty())
    {
      buffer += p->name;
      buffer += _T('=');
      buffer += p->value;
      buffer += _T("\r\n");
    }
  }

  if (::SendMessage(GetDlgItem(hPanel, IDC_ENUM_2SIDED), CB_GETCURSEL, 0, 0) == int(IDagorMat::Sides::RealDoubleSided))
    buffer += _T("real_two_sided=yes\r\n");
  else
    buffer += _T("real_two_sided=no\r\n");

  theMtl->script = buffer.c_str();
  theMtl->NotifyChanged();
}

int MaterDlg2::FindSubTexFromHWND(HWND hw)
{
  for (size_t i = 0; i < texbut.size(); ++i)
    if (texbut[i])
      if (texbut[i]->GetHwnd() == hw)
        return int(i);
  return -1;
}

M_STD_STRING MaterDlg2::GetShaderName()
{
  HWND hName = ::GetDlgItem(hPanel, IDC_CLASSNAME);
  long index = ::SendMessage(hName, CB_GETCURSEL, 0, 0);
  long length = ::SendMessage(hName, CB_GETLBTEXTLEN, (WPARAM)index, NULL);
  M_STD_STRING name;
  if (length > 0)
  {
    name.resize(length);
    ::SendMessage(hName, CB_GETLBTEXT, index, (LPARAM)name.c_str());
  }
  return name;
}

void MaterDlg2::UpdateShaderName()
{
  HWND hCmb = ::GetDlgItem(hPanel, IDC_CLASSNAME);
  int iclass = ::SendMessage(hCmb, CB_FINDSTRINGEXACT, 0, (LPARAM)theMtl->classname.data());
  ::SendMessage(hCmb, CB_SETCURSEL, iclass, NULL);
}

void MaterDlg2::ChangeShaderName()
{
  M_STD_STRING str = GetShaderName();
  if (str.substr(0, 1) == _T("-"))
  {
    TSTR msg;
    msg.printf(_T("\"%s\" category cannot be set as shader class. Revert to \"%s\"."), TSTR(str.data()), theMtl->classname);
    MessageBox(NULL, msg, _T("Class selection error"), MB_ICONERROR | MB_OK);
    UpdateShaderName();
    return;
  }
  theMtl->classname = str.c_str();
  UpdateShaderName();
  theMtl->NotifyChanged();
  SaveParams();
  RestoreParams();
  FillSlotNames();
  DialogsReposition();
}

void MaterDlg2::UpdateEnum2Sided()
{
  HWND hw = ::GetDlgItem(hPanel, IDC_ENUM_2SIDED);
  ::SendMessage(hw, CB_SETCURSEL, WPARAM(theMtl->twosided), NULL);
}

void MaterDlg2::AddParam(int type, M_STD_STRING name, M_STD_STRING value)
{
  if (name.empty())
    return;

  MaterPar2 *par = nullptr;
  switch (type)
  {
    case CFG_TEXT: par = new MaterParText2(name, value, this); break;
    case CFG_BOOL: par = new MaterParBool2(name, value, this); break;
    case CFG_INT: par = new MaterParInt2(name, value, this); break;
    case CFG_REAL: par = new MaterParReal2(name, value, this); break;
    case CFG_ENUM: par = new MaterParEnum2(name, value, this); break;
    case CFG_COLOR: par = new MaterParColor2(name, value, this); break;
    default: return;
  }

  parameters.emplace_back(std::unique_ptr<MaterPar2>(par));
}

MaterPar2 *MaterDlg2::GetParam(M_STD_STRING name)
{
  for (auto &p : parameters)
    if (p->name == name)
      return p.get();
  return nullptr;
}

void MaterDlg2::RemParam(M_STD_STRING name)
{
  auto it = std::find_if(parameters.begin(), parameters.end(), [&name](std::unique_ptr<MaterPar2> &p) { return p->name == name; });
  if (it == parameters.end())
    return;

  parameters.erase(it);
  SaveParams();
}

HWND MaterDlg2::AppendDialog(const TCHAR *paramName, long Resource, DLGPROC lpDialogProc, LPARAM param)
{
  HWND result = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(Resource), hPanel, lpDialogProc, param);

  MaterPar2 *dlg = (MaterParText2 *)GetWindowLongPtr(result, GWLP_USERDATA);

  ::SetWindowText(result, paramName);
  dlg->hSType = ::FindWindowEx(result, NULL, _T("STATIC"), _T("StaticType"));
  if (dlg->hSType)
    ::SetWindowText(dlg->hSType, paramName);
  ::ShowWindow(result, SW_SHOW);

  dlg->creating = false;
  return result;
}

BOOL MaterDlg2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      HWND h2sided = ::GetDlgItem(hWnd, IDC_ENUM_2SIDED);
      ::SendMessage(h2sided, CB_INSERTSTRING, -1, (LPARAM) _T("1 sided"));
      ::SendMessage(h2sided, CB_INSERTSTRING, -1, (LPARAM) _T("2 sided"));
      ::SendMessage(h2sided, CB_INSERTSTRING, -1, (LPARAM) _T("Real 2 sided"));
      UpdateEnum2Sided();

      csa = GetIColorSwatch(GetDlgItem(hWnd, IDC_AMB), theMtl->cola, GetString(IDS_AMB_COLOR));
      assert(csa);
      csd = GetIColorSwatch(GetDlgItem(hWnd, IDC_DIFF), theMtl->cold, GetString(IDS_DIFF_COLOR));
      assert(csd);
      css = GetIColorSwatch(GetDlgItem(hWnd, IDC_SPEC), theMtl->cols, GetString(IDS_SPEC_COLOR));
      assert(css);
      cse = GetIColorSwatch(GetDlgItem(hWnd, IDC_EMIS), theMtl->cole, GetString(IDS_EMIS_COLOR));
      assert(cse);

      HWND hCmb = ::GetDlgItem(hWnd, IDC_CLASSNAME);

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(dagor_shaders_config, filename);
      CfgShader cfg(filename);

      cfg.GetShaderNames();

      for (int pos = 0; pos < cfg.shader_names.size(); ++pos)
        ::SendMessage(hCmb, CB_INSERTSTRING, -1, (LPARAM)cfg.shader_names.at(pos).c_str());

      UpdateShaderName();
      theMtl->NotifyChanged();

      for (size_t i = 0; i < texbut.size(); ++i)
      {
        texbut[i] = GetICustButton(GetDlgItem(hWnd, texidc[i]));
        texbut[i]->SetDADMgr(&dadmgr);
      }

      FillSlotNames();

      RECT rc;
      GetWindowRect(GetDlgItem(hWnd, IDC_PARAM_START), &rc);
      paramOrg.x = rc.left;
      paramOrg.y = rc.top;
      ScreenToClient(hWnd, &paramOrg);
      DialogsReposition();
      break;
    }
    case WM_SHOWWINDOW:
      if (wParam)
        RestoreParams();
      else
        SaveParams();
      break;

    case WM_PAINT:
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;

    case WM_COMMAND:
    {
      for (size_t i = 0; i < NUMTEXMAPS; ++i)
        if (LOWORD(wParam) == texidc[i])
          PostMessage(hwmedit, WM_TEXMAP_BUTTON, i, (LPARAM)theMtl);

      switch (LOWORD(wParam))
      {
        case IDC_ENUM_2SIDED:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            if (creating)
              break;

            theMtl->twosided = IDagorMat::Sides(::SendMessage(GetDlgItem(hWnd, IDC_ENUM_2SIDED), CB_GETCURSEL, 0, 0));
            theMtl->NotifyChanged();
            SaveParams();
            UpdateMtlDisplay();
          }
          break;
        case IDC_CLASSNAME:
        {
          if (creating)
            break;
          if (HIWORD(wParam) == CBN_SELCHANGE)
            ChangeShaderName();
        }
        break;
        case IDC_SCRIPT:
        {
          switch (HIWORD(wParam))
          {
            case EN_SETFOCUS: DisableAccelerators(); break;
            case EN_KILLFOCUS: EnableAccelerators(); break;
            case EN_CHANGE:
            {
              if (creating)
                break;
              HWND hw = (HWND)lParam;
              int l = GetWindowTextLength(hw);
              TSTR s;
              s.Resize(l + 1);
              GetWindowText(hw, STR_DEST(s), l + 1);
              theMtl->script = s;
              theMtl->NotifyChanged();
            }
            break;
          }
        }
        break;
        case IDC_PARAM_NEW:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              M_STD_STRING shader = GetShaderName();
              MaterParNew2 new_par(this, shader);
              if (new_par.DoModal() == IDOK)
              {
                M_STD_STRING name = new_par.GetNewParamName();

                TCHAR filename[MAX_PATH];
                CfgShader::GetCfgFilename(dagor_shaders_config, filename);
                CfgShader cfg(filename);

                int type = cfg.GetParamType(shader, name); // recognize as local
                if (type == CFG_UNKNOWN)
                {
                  type = cfg.GetParamType(CFG_GLOBAL_NAME, name); // try as global
                  if (type == CFG_UNKNOWN)
                    type = CFG_TEXT; // treat unknown as text
                }

                AddParam(type, name);
                SaveParams();
                DialogsReposition();
                MarkUnknownParams(cfg);
              }
            }
            break;
          }
        }
        break;
      }
    }
    break;

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
          cs = csa;
          pbid = PB_AMB;
          break;
        case IDC_DIFF:
          cs = csd;
          pbid = PB_DIFF;
          break;
        case IDC_SPEC:
          cs = css;
          pbid = PB_SPEC;
          break;
        case IDC_EMIS:
          cs = cse;
          pbid = PB_EMIS;
          break;
      }
      if (!cs)
        break;
      int buttonUp = HIWORD(wParam);
      if (buttonUp)
        theHold.Begin();
      theMtl->pblock->SetValue(pbid, ip->GetTime(), (Color &)Color(cs->GetColor()));
      cs->SetKeyBrackets(KeyAtCurTime(pbid));
      if (buttonUp)
      {
        theHold.Accept(GetString(IDS_COLOR_CHANGE));
        UpdateMtlDisplay();
        theMtl->NotifyChanged();
      }
    }
    break;

    case CC_SPINNER_CHANGE:
      if (!theHold.Holding())
        theHold.Begin();
      switch (LOWORD(wParam))
      {
        case IDC_SHIN_S:
        {
          ISpinnerControl *spin = (ISpinnerControl *)lParam;
          theMtl->pblock->SetValue(PB_SHIN, ip->GetTime(), spin->GetFVal());
          spin->SetKeyBrackets(KeyAtCurTime(PB_SHIN));
        }
        break;
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

void MaterDlg2::FillSlotNames()
{
  // reserved for future update
}

void MaterDlg2::Invalidate()
{
  valid = FALSE;
  isActive = FALSE;
  Rect rect;
  rect.left = rect.top = 0;
  rect.right = rect.bottom = 10;
  InvalidateRect(hPanel, &rect, FALSE);
}

void MaterDlg2::SetThing(ReferenceTarget *m)
{
  theMtl = (DagorMat2 *)m;
  if (theMtl)
    theMtl->dlg = this;
  ReloadDialog();
}

BOOL MaterDlg2::KeyAtCurTime(int id) { return theMtl->pblock->KeyFrameAtTime(id, ip->GetTime()); }

void MaterDlg2::UpdateTexDisplay(int i)
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

void MaterDlg2::ReloadDialog()
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
  UpdateEnum2Sided();
  SetDlgItemText(hPanel, IDC_SCRIPT, theMtl->script);
  for (int i = 0; i < NUMTEXMAPS; ++i)
    UpdateTexDisplay(i);

  UpdateShaderName();
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
  classname = default_shader_name;
  script = _T("");
  updateViewportTexturesState();
}

ParamDlg *DagorMat2::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
  dlg = new MaterDlg2(hwMtlEdit, imp, this);
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
  mtl->classname = (classname ? classname : default_shader_name);
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
          classname = (s ? s : default_shader_name);
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

void *DagorMat2::GetInterface(ULONG id)
{
  if (id == I_DAGORMAT)
  {
    return (IDagorMat *)this;
  }
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
    classname = default_shader_name;
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

static INT_PTR CALLBACK MaterParNewProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParNew2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParNew2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParNew2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }

  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParNew2::MaterParNew2(MaterDlg2 *p, M_STD_STRING &shdr) : parent(p), hWnd(NULL), param_name(), shader(shdr) {}

MaterParNew2::~MaterParNew2() {}

int MaterParNew2::DoModal()
{
  return ::DialogBoxParam(hInstance, (const TCHAR *)IDD_DAGORPAR_NEW, NULL, MaterParNewProc2, (LPARAM)this);
}

static bool is_parameter_name_valid(const M_STD_STRING &name)
{
  return std::all_of(name.begin(), name.end(), [](TCHAR c) { return isalnum(c) || c == '_'; });
}

BOOL MaterParNew2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;

      HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(dagor_shaders_config, filename);
      CfgShader cfg(filename);

      ::SendMessage(hCmbName, CB_RESETCONTENT, 0, 0);

      cfg.GetGlobalParams();
      for (M_STD_STRING &param : cfg.global_params)
        if (!parent->GetParam(param) && param != real_two_sided)
          ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM)param.data());

      if (!shader.empty())
      {
        cfg.GetShaderParams(shader);
        for (M_STD_STRING &param : cfg.shader_params)
          if (!parent->GetParam(param))
            ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM)param.data());
      }
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDOK:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              GetParamName();

              const TCHAR *err = nullptr;

              if (param_name.empty())
                err = _T("Name is empty");

              if (!is_parameter_name_valid(param_name))
                err = _T("Name must contain characters: 'A'..'Z', 'a'..'z', '_'");

              if (parent->GetParam(param_name))
                err = _T("Name already exists");

              if (err)
              {
                MessageBox(hWnd, err, _T("Error"), MB_OK | MB_ICONSTOP);
                return TRUE;
              }

              ::EndDialog(hWnd, IDOK);
            }
            break;
          };
        }
        break;

        case IDCANCEL:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED: ::EndDialog(hWnd, IDCANCEL); break;
          };
        }
        break;

        case IDC_PARAM_NAME:
        {
          switch (HIWORD(wParam))
          {
            case CBN_DBLCLK:
            {
              GetParamName();
              ::EndDialog(hWnd, IDOK);
              break;
            }
          }
          break;
        }
        break;

        default: break;
      }
    }
    break;

    default: return FALSE;
  }

  return TRUE;
}

M_STD_STRING MaterParNew2::GetParamName()
{
  HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);
  int len = ::GetWindowTextLength(hCmbName);

  if (len > 0)
  {
    param_name.resize(len);
    ::GetWindowText(hCmbName, (TCHAR *)param_name.c_str(), len + 1);
  }

  return param_name;
}

////////////////////////////////////////////////////////////////

MaterPar2::MaterPar2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) :
  parent(p), hPanel(NULL), hSType(NULL), creating(TRUE), name(n), value(v)
{}

MaterPar2::~MaterPar2() { ::DestroyWindow(hPanel); }

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParTextProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParText2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParText2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParText2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParText2::MaterParText2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p), edit(NULL)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_TEXT, MaterParTextProc2, (LPARAM)this);
}

MaterParText2::~MaterParText2() {}

BOOL MaterParText2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      edit = ::GetICustEdit(GetDlgItem(hWnd, IDC_PAR_TEXT_VALUE));
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDC_PAR_TEXT_VALUE:
        {
          if (creating)
            break;
          if (HIWORD(wParam) == EN_CHANGE)
          {
            GetValue();
          }
        }
        break;

        case IDC_PARAM_DELETE:
        {
          if (creating)
            break;
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;
        default:
        {
        }
        break;
      }
    }
    break;

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParText2::GetValue()
{
  TSTR buf;
  edit->GetText(buf);
  value = buf.data();
  parent->SaveParams();
}

void MaterParText2::SetValue(const M_STD_STRING v)
{
  edit->SetText((TCHAR *)v.c_str());
  value = v;
}

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParRealProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParReal2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParReal2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParReal2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParReal2::MaterParReal2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p), spinner(NULL)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_REAL, MaterParRealProc2, (LPARAM)this);
}

MaterParReal2::~MaterParReal2() {}

BOOL MaterParReal2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      spinner = ::SetupFloatSpinner(hWnd, IDC_PAR_REAL_VALUE_SPINNER, IDC_PAR_REAL_VALUE, -2147483648.0f, 2147483648.0f, 0.0f, 1.0f);
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDC_PARAM_DELETE:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;
        default:
        {
        }
        break;
      }
    }
    break;

    case CC_SPINNER_CHANGE:
      if (!theHold.Holding())
        theHold.Begin();
      switch (LOWORD(wParam))
      {
        case IDC_PAR_REAL_VALUE_SPINNER:
        {
          GetValue();
        }
        break;
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

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParReal2::GetValue()
{
  float f = spinner->GetFVal();

  M_STD_OSTRINGSTREAM str;
  str << f;

  value = str.str();
  parent->SaveParams();
}

void MaterParReal2::SetValue(const M_STD_STRING v)
{
  M_STD_ISTRINGSTREAM str(v);
  float f;
  str >> f;
  spinner->SetValue(f, TRUE);

  value = v;
}

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParColorProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParColor2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParColor2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParColor2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParColor2::MaterParColor2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p), col(NULL)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_COLOR, MaterParColorProc2, (LPARAM)this);
}

MaterParColor2::~MaterParColor2() { ReleaseIColorSwatch(col); }

BOOL MaterParColor2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      AColor c;
      col = GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter"));
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
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
    {
      switch (LOWORD(wParam))
      {
        case IDC_PARAM_DELETE:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;
        default:
        {
        }
        break;
      }
    }
    break;

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParColor2::GetValue()
{
  AColor c = col->GetAColor();
  TCHAR buf[128];
  _stprintf(buf, _T("%.3f,%.3f,%.3f,%.3f"), c.r, c.g, c.b, c.a);

  value = buf;
  parent->SaveParams();
}

void MaterParColor2::SetValue(const M_STD_STRING v)
{
  float r = 0, g = 0, b = 0, a = 0;
  int res = _stscanf(v.c_str(), _T(" %f , %f , %f , %f"), &r, &g, &b, &a);
  if (res != 4)
    col->SetAColor(AColor(0, 0, 0, 0), TRUE);
  else
    col->SetAColor(AColor(r, g, b, a), TRUE);

  value = v;
}

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParEnumProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParEnum2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParEnum2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParEnum2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParEnum2::MaterParEnum2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p), count(0)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_ENUM, MaterParEnumProc2, (LPARAM)this);
}

MaterParEnum2::~MaterParEnum2() {}

BOOL MaterParEnum2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
#if 0
      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(dagor_shaders_config, filename);
      CfgShader cfg(filename);


      StringVector buttons = cfg.GetParamData(owner, name);
      count = (int)buttons.size();

      for (int pos = 0; pos < count; ++pos)
      {
        int col = pos % 4;
        int row = pos / 4;

        HMENU hID = (HMENU)(intptr_t)(IDC_PAR_ENUM_RADIO + (row * 4) + col);

        HWND hRadioNew = ::CreateWindowEx(0, _T("BUTTON"), _T(""), BS_AUTORADIOBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
          67 + 67 * col, 15 * row, 67, 15, hWnd, hID, ::hInstance, NULL);
        ::SetWindowText(hRadioNew, buttons.at((row * 4) + col).c_str());
      }
#endif

      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_COMMAND:
    {
      if (LOWORD(wParam) >= IDC_PAR_ENUM_RADIO && LOWORD(wParam) < IDC_PAR_ENUM_RADIO + count)
      {
        if (creating)
          break;
        GetValue();
      }
      switch (LOWORD(wParam))
      {
        case IDC_PARAM_DELETE:
        {
          if (creating)
            break;
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;

        default:
        {
        }
        break;
      }
    }
    break;

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParEnum2::GetValue()
{
  for (int pos = 0; pos < count; ++pos)
  {
    int id = (IDC_PAR_ENUM_RADIO + pos);
    int state = ::SendMessage(::GetDlgItem(hPanel, id), BM_GETCHECK, 0, NULL);
    if (state)
    {
      int length = ::GetWindowTextLength(::GetDlgItem(hPanel, id));
      value.resize(length);
      ::GetWindowText(::GetDlgItem(hPanel, id), (TCHAR *)value.c_str(), length + 1);
      break;
    }
  }
  parent->SaveParams();
}

void MaterParEnum2::SetValue(const M_STD_STRING v)
{
  for (int pos = 0; pos < count; ++pos)
  {
    int id = (IDC_PAR_ENUM_RADIO + pos);
    M_STD_STRING str;
    int length = ::GetWindowTextLength(::GetDlgItem(hPanel, id));
    str.resize(length);
    ::GetWindowText(::GetDlgItem(hPanel, id), (TCHAR *)str.c_str(), length + 1);
    ::SendMessage(::GetDlgItem(hPanel, id), BM_SETCHECK, str == v, NULL);
  }
}

int MaterParEnum2::GetCount() { return (int)(count / 2); }

TCHAR *MaterParEnum2::GetRadioName(int at) { return NULL; }

int MaterParEnum2::GetRadioState(int at) { return 0; }

int MaterParEnum2::GetRadioSelected() { return 0; }

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParBoolProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParBool2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParBool2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParBool2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParBool2::MaterParBool2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_BOOL, MaterParBoolProc2, (LPARAM)this);
}

MaterParBool2::~MaterParBool2() {}

BOOL MaterParBool2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      hPanel = hWnd;
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDC_PARAM_DELETE:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;

        case IDC_PAR_BOOL_VALUE: GetValue(); break;

        default: break;
      }
      break;
    }
    break;

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParBool2::GetValue()
{
  value = IsDlgButtonChecked(hPanel, IDC_PAR_BOOL_VALUE) ? _T("yes") : _T("no");
  parent->SaveParams();
}

void MaterParBool2::SetValue(const M_STD_STRING v)
{
  CheckDlgButton(hPanel, IDC_PAR_BOOL_VALUE, v == _T("yes") || v == _T("true"));
  value = v;
}

//////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK MaterParIntProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParInt2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParInt2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParInt2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  return dlg->WndProc(hWnd, msg, wParam, lParam);
}

MaterParInt2::MaterParInt2(M_STD_STRING n, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, v, p), spinner(NULL)
{
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_REAL, MaterParIntProc2, (LPARAM)this);
}

MaterParInt2::~MaterParInt2() {}

BOOL MaterParInt2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      spinner = ::SetupIntSpinner(hWnd, IDC_PAR_REAL_VALUE_SPINNER, IDC_PAR_REAL_VALUE, -INT_MAX, INT_MAX, 0);
      ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
        case IDC_PARAM_DELETE:
        {
          switch (HIWORD(wParam))
          {
            case BN_CLICKED:
            {
              parent->RemParam(name);
              parent->DialogsReposition();
              return FALSE;
            }
            break;
          };
        }
        break;
        default:
        {
        }
        break;
      }
    }
    break;

    case CC_SPINNER_CHANGE:
      if (!theHold.Holding())
        theHold.Begin();
      switch (LOWORD(wParam))
      {
        case IDC_PAR_REAL_VALUE_SPINNER:
        {
          GetValue();
        }
        break;
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

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

void MaterParInt2::GetValue()
{
  int i = spinner->GetIVal();

  M_STD_OSTRINGSTREAM str;
  str << i;

  value = str.str();
  parent->SaveParams();
}

void MaterParInt2::SetValue(const M_STD_STRING v)
{
  M_STD_ISTRINGSTREAM str(v);
  int i;
  str >> i;
  spinner->SetValue(i, TRUE);

  value = v;
}

//////////////////////////////////////////////////////////////////////

void MaterDlg2::DialogsReposition()
{
  IRollupWindow *irw = ip->GetMtlEditorRollup();
  int ind = irw->GetPanelIndex(hPanel);

  if (parameters.empty())
  {
    if (ind >= 0)
      irw->SetPageDlgHeight(ind, paramOrg.y);

    return;
  }

  RECT rct;
  POINT pt = paramOrg;

  // 0th parameter
  HWND par = parameters.front()->hPanel;
  ::SetWindowPos(par, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
  ::GetWindowRect(par, &rct);
  pt.x = rct.left;
  pt.y = rct.bottom + PARAM_DLG_GAP;
  ::ScreenToClient(hPanel, &pt);

  // other parameters
  for (size_t i = 1; i < parameters.size(); ++i)
  {
    par = parameters[i]->hPanel;
    ::SetWindowPos(par, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ::GetWindowRect(par, &rct);
    pt.x = rct.left;
    pt.y = rct.bottom + PARAM_DLG_GAP;
    ::ScreenToClient(hPanel, &pt);
  }

  if (ind >= 0)
    irw->SetPageDlgHeight(ind, pt.y + PARAM_DLG_GAP);
}
