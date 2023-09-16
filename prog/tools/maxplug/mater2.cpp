#include <max.h>
#include <stdmat.h>
#include <math.h>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <iparamb2.h>

#include "dagor.h"
#include "mater.h"
#include "texmaps.h"
#include "resource.h"
#include "cfg.h"
#include "debug.h"

#include <d3dx9.h>
#include <IHardwareMaterial.h>
#include "dag_auto_ptr.h"

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

//////////////////////////////////////////////////////////////////////////////////

#define TX_MODULATE   0
#define TX_ALPHABLEND 1
#define BMIDATA(x)    ((UBYTE *)((BYTE *)(x) + sizeof(BITMAPINFOHEADER)))

class MaterParNew2
{
  // : public ParamDlg {
public:
  class MaterDlg2 *parent;
  HWND hWnd;
  BOOL isActive;

  M_STD_STRING shader;

  int param_name_ind;
  int param_status_ind;

  M_STD_STRING param_status;
  M_STD_STRING param_name;

  M_STD_STRING GetParamStatus();
  M_STD_STRING GetParamName();

  MaterParNew2(MaterDlg2 *p);
  virtual ~MaterParNew2();

  int DoModal();
  /*
  void ReloadDialog() {};
  Class_ID ClassID() {return DagorMat2_CID;}
  void SetThing(ReferenceTarget *m) {}
  ReferenceTarget* GetThing() { return NULL; }
  void DeleteThis() { delete this; }
  void SetTime(TimeValue t) {}
  void ActivateDlg(BOOL onOff) {}
  */
  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  HWND GetHWND();

private:
  int param_shader_starts_from;
};


class MaterPar2 : public ParamDlg
{
public:
  void SetOwnerDlg(MaterDlg2 *dlg2);

  class MaterDlg2 *parent;

  //////////////////////////

  HWND hPanel;
  HWND hWnd;
  BOOL isActive;
  BOOL valid;
  BOOL creating;

  M_STD_STRING name;
  M_STD_STRING owner;
  M_STD_STRING value;

  //////////////////////////

  MaterPar2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterPar2();

  //////////////////////////

  void Invalidate();

  void ReloadDialog(){};
  Class_ID ClassID() { return DagorMat2_CID; }
  void SetThing(ReferenceTarget *m) {}
  ReferenceTarget *GetThing() { return NULL; }
  void DeleteThis() { delete this; }
  void SetTime(TimeValue t) {}
  void ActivateDlg(BOOL onOff) {}

  //////////////////////////

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  HWND GetHWND();

  int GetType();
  int GetStatus();

  virtual M_STD_STRING GetName();
  virtual M_STD_STRING GetValue();

  virtual void SetName(const M_STD_STRING n);
  virtual void SetValue(const M_STD_STRING v);

  virtual void Move(const Rect &place);

  virtual void Reheight(const unsigned cy);
  virtual void Rewidth(const unsigned cx);

protected:
  bool rollUpLess;
  MaterDlg2 *ownerDlg2;
};

class MaterParText2 : public MaterPar2
{
public:
  MaterParText2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterParText2();

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  virtual M_STD_STRING GetValue();
  virtual void SetValue(const M_STD_STRING v);

  ICustEdit *edit;
};

class MaterParReal2 : public MaterPar2
{
public:
  MaterParReal2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterParReal2();

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  virtual M_STD_STRING GetValue();
  virtual void SetValue(const M_STD_STRING v);

  ISpinnerControl *spinner;
};

class MaterParRange2 : public MaterPar2
{
public:
  MaterParRange2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterParRange2();

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  virtual M_STD_STRING GetValue();
  virtual void SetValue(const M_STD_STRING v);

  ISpinnerControl *spinner;
};

class MaterParEnum2 : public MaterPar2
{
public:
  MaterParEnum2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterParEnum2();

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  virtual M_STD_STRING GetValue();
  virtual void SetValue(const M_STD_STRING v);

  int GetCount();

  TCHAR *GetRadioName(int at);
  int GetRadioState(int at);

  int GetRadioSelected();

  int count;
};

class MaterParColor2 : public MaterPar2
{
public:
  MaterParColor2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p);
  virtual ~MaterParColor2();

  virtual BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  virtual M_STD_STRING GetValue();
  virtual void SetValue(const M_STD_STRING v);

  IColorSwatch *col;
};

typedef dag_auto_ptr<MaterPar2> MaterParPtr2;

typedef std::vector<MaterParPtr2> MaterParVector2;
typedef std::map<M_STD_STRING, MaterParPtr2> MaterParMap2;

static int iPDC;

///////////////////////////////////////////////////////////////////////////

class MaterDlg2 : public ParamDlg
{
public:
  void DialogsReposition();
  HWND hwmedit;
  IMtlParams *ip;
  class DagorMat2 *theMtl;
  HWND hPanel;
  HWND hWnd;
  BOOL valid;
  BOOL isActive;
  IColorSwatch *csa, *csd, *css, *cse;
  BOOL creating;
  TexDADMgr dadmgr;
  ICustButton *texbut[NUMTEXMAPS];
  ICustStatus *texname[NUMTEXMAPS];

  MaterParMap2 parameters;

  MaterDlg2(HWND hwMtlEdit, IMtlParams *imp, DagorMat2 *m);
  ~MaterDlg2();

  void RestoreParams();
  void SaveParams();

  void FillSlotNames();

  M_STD_STRING GetShaderName();
  MaterPar2 *AddParam(M_STD_STRING owner, M_STD_STRING name, M_STD_STRING value);
  MaterPar2 *GetParam(M_STD_STRING owner, M_STD_STRING name);
  bool FindParam(M_STD_STRING owner, M_STD_STRING name);
  void DelParam(M_STD_STRING owner, M_STD_STRING name);
  void RemParam(M_STD_STRING owner, M_STD_STRING name);
  void DelParamAll();

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  void Invalidate();
  void UpdateMtlDisplay() { ip->MtlChanged(); }
  void UpdateTexDisplay(int i);

  // methods inherited from ParamDlg:
  int FindSubTexFromHWND(HWND hw);
  void ReloadDialog();
  Class_ID ClassID() { return DagorMat2_CID; }
  BOOL KeyAtCurTime(int id);
  void SetThing(ReferenceTarget *m);
  ReferenceTarget *GetThing() { return (ReferenceTarget *)theMtl; }
  void DeleteThis() { delete this; }
  void SetTime(TimeValue t) { Invalidate(); }
  void ActivateDlg(BOOL onOff)
  {
    csa->Activate(onOff);
    csd->Activate(onOff);
    css->Activate(onOff);
    cse->Activate(onOff);
  }

  HWND AppendDialog(const TCHAR *paramName, long Resource, DLGPROC lpDialogProc, LPARAM param);
};

#define MAX_PARAM_DLGS 20
HWND paramDlgs[MAX_PARAM_DLGS];


//////////////////////////////////////////////////////////////////////////////////

class DagorMat2PostLoadCallback : public PostLoadCallback
{
public:
  DagorMat2 *parent;

  DagorMat2PostLoadCallback::DagorMat2PostLoadCallback() { parent = NULL; }

  virtual void proc(ILoad *iload);
};


class DagorMat2 : public Mtl, public IDagorMat
{
public:
  class MaterDlg2 *dlg;
  IParamBlock *pblock;
  Texmaps *texmaps;
  TexHandle *texHandle[NUMTEXMAPS];
  Interval ivalid;
  float shin;
  float power;
  Color cola, cold, cols, cole;
  BOOL twosided;
  TSTR classname, script;
  DagorMat2PostLoadCallback postLoadCallback;

  DagorMat2(BOOL loading);
  ~DagorMat2();
  void NotifyChanged();

  void *GetInterface(ULONG);
  void ReleaseInterface(ULONG, void *);

  // From IDagorMat
  Color get_amb();
  Color get_diff();
  Color get_spec();
  Color get_emis();
  float get_power();
  BOOL get_2sided();
  const TCHAR *get_classname();
  const TCHAR *get_script();
  const TCHAR *get_texname(int);
  float get_param(int);

  void set_amb(Color);
  void set_diff(Color);
  void set_spec(Color);
  void set_emis(Color);
  void set_power(float);
  void set_2sided(BOOL);
  void set_classname(const TCHAR *);
  void set_script(const TCHAR *);
  void set_texname(int, const TCHAR *);
  void set_param(int, float);

  // From MtlBase and Mtl
  int NumSubTexmaps();
  Texmap *GetSubTexmap(int);
  void SetSubTexmap(int i, Texmap *m);

  void SetAmbient(Color c, TimeValue t);
  void SetDiffuse(Color c, TimeValue t);
  void SetSpecular(Color c, TimeValue t);
  void SetShininess(float v, TimeValue t);

  Color GetAmbient(int mtlNum = 0, BOOL backFace = FALSE);
  Color GetDiffuse(int mtlNum = 0, BOOL backFace = FALSE);
  Color GetSpecular(int mtlNum = 0, BOOL backFace = FALSE);
  float GetXParency(int mtlNum = 0, BOOL backFace = FALSE);
  float GetShininess(int mtlNum = 0, BOOL backFace = FALSE);
  float GetShinStr(int mtlNum = 0, BOOL backFace = FALSE);

  ParamDlg *CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);

  void Shade(ShadeContext &sc);
  void Update(TimeValue t, Interval &valid);
  void Reset();
  Interval Validity(TimeValue t);

  Class_ID ClassID();
  SClass_ID SuperClassID();
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  void GetClassName(TSTR &s, bool localized) { s = GetString(IDS_DAGORMAT2); }
#else
  void GetClassName(TSTR &s) { s = GetString(IDS_DAGORMAT2); }
#endif

  void DeleteThis();

  ULONG Requirements(int subMtlNum);
  ULONG LocalRequirements(int subMtlNum);
  int NumSubs();
  Animatable *SubAnim(int i);
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  MSTR SubAnimName(int i, bool localized);
#else
  TSTR SubAnimName(int i);
#endif
  int SubNumToRefNum(int subNum);

  // From ref
  int NumRefs();
  RefTargetHandle GetReference(int i);
  void SetReference(int i, RefTargetHandle rtarg);

  IOResult Save(ISave *isave);
  IOResult Load(ILoad *iload);

  RefTargetHandle Clone(RemapDir &remap = NoRemap());

#if defined(MAX_RELEASE_R17) && MAX_RELEASE >= MAX_RELEASE_R17
  RefResult NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message, BOOL propagate);
#else
  RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message);
#endif

  virtual BOOL SupportTexDisplay();
  virtual BOOL SupportsMultiMapsInViewport();
  virtual void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb);
  virtual void ActivateTexDisplay(BOOL onoff);
  void DiscardTexHandles();
  void updateViewportTexturesState();
  bool hasAlpha();
};


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
  int IsPublic() { return 1; }
  void *Create(BOOL loading) { return new DagorMat2(loading); }
  const TCHAR *ClassName() { return GetString(IDS_DAGORMAT2_LONG); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
  Class_ID ClassID() { return DagorMat2_CID; }
  const TCHAR *Category() { return _T(""); }
};
static MaterClassDesc2 materCD;
ClassDesc *GetMaterCD2() { return &materCD; }

class TexmapsClassDesc2 : public ClassDesc
{
public:
  int IsPublic() { return 0; }
  void *Create(BOOL loading) { return new Texmaps; }
  const TCHAR *ClassName() { return _T("DagorTexmaps"); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return TEXMAP_CONTAINER_CLASS_ID; }
  Class_ID ClassID() { return Texmaps_CID; }
  const TCHAR *Category() { return _T(""); }
};
static TexmapsClassDesc2 texmapsCD;
ClassDesc *GetTexmapsCD2() { return &texmapsCD; }

//--- MaterDlg2 ------------------------------------------------------

static int texidc[NUMTEXMAPS] =
  {
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
},
           texnameidc[NUMTEXMAPS] = {
             IDC_TEXNAME0,
             IDC_TEXNAME1,
             IDC_TEXNAME2,
             IDC_TEXNAME3,
             IDC_TEXNAME4,
             IDC_TEXNAME5,
             IDC_TEXNAME6,
             IDC_TEXNAME7,
             IDC_TEXNAME8,
             IDC_TEXNAME9,
             IDC_TEXNAME10,
             IDC_TEXNAME11,
             IDC_TEXNAME12,
             IDC_TEXNAME13,
             IDC_TEXNAME14,
             IDC_TEXNAME15,
};

static INT_PTR CALLBACK PanelDlgProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterDlg2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterDlg2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
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
  hWnd = NULL;
  dadmgr.Init(this);
  hwmedit = hwMtlEdit;
  ip = imp;
  theMtl = m;
  valid = FALSE;
  creating = TRUE;
  //    iPDC = 0;
  //    memset(paramDlgs, 0, sizeof(HWND)*MAX_PARAM_DLGS);
  for (int i = 0; i < NUMTEXMAPS; ++i)
  {
    texbut[i] = NULL;
    texname[i] = NULL;
  }
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT2), PanelDlgProc2, GetString(IDS_DAGORMAT_LONG), (LPARAM)this);
  creating = FALSE;
}

MaterDlg2::~MaterDlg2()
{
  SaveParams();
  DelParamAll();
  theMtl->dlg = NULL;
  ReleaseIColorSwatch(csa);
  ReleaseIColorSwatch(csd);
  ReleaseIColorSwatch(css);
  ReleaseIColorSwatch(cse);
  for (int i = 0; i < NUMTEXMAPS; ++i)
  {
    ReleaseICustButton(texbut[i]);
    ReleaseICustStatus(texname[i]);
  }
  SetWindowLongPtr(hPanel, GWLP_USERDATA, NULL);
}

static M_STD_STRING simplifyRN(const M_STD_STRING &from)
{
  M_STD_STRING to;
  size_t start = 0;
  while (start != M_STD_STRING::npos && !from.empty() && start != from.length())
  {
    size_t end = from.find(_T("\r\n"), start);
    if (end == M_STD_STRING::npos)
    {
      to += from.substr(start, from.length() - start);
      return to;
    }
    else
    {
      to += from.substr(start, end - start);
      to += _T("\n");
    }
    start = end + 2;
  }
  return to;
}

void MaterDlg2::RestoreParams()
{
  M_STD_STRING script = simplifyRN(M_STD_STRING(theMtl->script));

  TCHAR filename[MAX_PATH];
  CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
  CfgShader cfg(filename);

  int start = 0;
  int end = 0;
  iPDC = 0;

  StringVector params;

  while (start != M_STD_STRING::npos && !script.empty())
  {
    start = end > 0 ? end + 1 : end;
    if (start >= script.length())
      break;
    end = (int)script.find(_T("\n"), start);
    if (end == M_STD_STRING::npos)
      end = (int)script.length();
    M_STD_STRING data = script.substr(start, end - start);
    if (!data.empty())
    {
      int par = (int)data.find(_T("="));
      if (par == M_STD_STRING::npos)
        continue;
      M_STD_STRING param_name = data.substr(0, par);
      M_STD_STRING param_value = data.substr(par + 1, data.length() - par + 1);
      if (param_name.empty() || param_value.empty())
        continue;
      params.push_back(data);

      M_STD_STRING param_owner = cfg.GetParamOwner(param_name);

      if (param_name != _T("real_two_sided"))
      {
        MaterPar2 *dlg = AddParam(param_owner, param_name, param_value);
      }
      else
      {
        if (param_value == _T("yes"))
          CheckDlgButton(hWnd, IDC_2SIDEDREAL, 1);
        else
          CheckDlgButton(hWnd, IDC_2SIDEDREAL, 0);
      }
    }
  }

  cfg.GetGlobalParams();

  int index = 0;

  for (index = 0; index < cfg.global_params.size(); ++index)
  {
    int mode = cfg.GetParamMode(CFG_GLOBAL_NAME, cfg.global_params.at(index));
    if (mode == CFG_COMMON && !FindParam(CFG_GLOBAL_NAME, cfg.global_params.at(index)))
    {
      if (cfg.global_params.at(index) != _T("real_two_sided"))
      {
        AddParam(CFG_GLOBAL_NAME, cfg.global_params.at(index), _T(""));
      }
    }
  }

  M_STD_STRING sh = GetShaderName();

  if (!sh.empty())
  {
    cfg.GetShaderParams(sh);

    for (index = 0; index < cfg.shader_params.size(); ++index)
    {
      int mode = cfg.GetParamMode(sh, cfg.shader_params.at(index));
      if (mode == CFG_COMMON && !FindParam(sh, cfg.shader_params.at(index)))
      {
        AddParam(sh, cfg.shader_params.at(index), _T(""));
      }
    }
  }
}

void MaterDlg2::SaveParams()
{
  M_STD_STRING buffer;
  MaterParMap2::iterator itr = parameters.begin();
  while (itr != parameters.end())
  {
    if (!itr->second.get())
      continue;
    M_STD_STRING name = itr->second.get()->name;
    M_STD_STRING value = itr->second.get()->value;

    if (!name.empty() && !value.empty())
    {
      buffer += name;
      buffer += _T("=");
      buffer += value;
      buffer += _T("\r\n");
    }
    itr++;
  }
  // MessageBox(NULL, (LPCSTR)buffer.c_str(), "Buffer", MB_OK);

  if (IsDlgButtonChecked(hWnd, IDC_2SIDEDREAL))
  {
    buffer += _T("real_two_sided");
    buffer += _T("=");
    buffer += _T("yes");
    buffer += _T("\r\n");
  }
  else
  {
    buffer += _T("real_two_sided");
    buffer += _T("=");
    buffer += _T("no");
    buffer += _T("\r\n");
  }

  theMtl->script = buffer.c_str();
  theMtl->NotifyChanged();

  long a = 0;
}

int MaterDlg2::FindSubTexFromHWND(HWND hw)
{
  for (int i = 0; i < NUMTEXMAPS; ++i)
    if (texbut[i])
      if (texbut[i]->GetHwnd() == hw)
        return i;
  return -1;
}

M_STD_STRING MaterDlg2::GetShaderName()
{
  HWND hName = ::GetDlgItem(hWnd, IDC_CLASSNAME);
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

MaterPar2 *MaterDlg2::AddParam(M_STD_STRING owner, M_STD_STRING name, M_STD_STRING value)
{
  if (owner.empty() || name.empty())
    return NULL;

  TCHAR filename[MAX_PATH];
  CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
  CfgShader cfg(filename);

  MaterPar2 *par = NULL;
  MaterParPtr2 ptr;

  M_STD_STRING key = owner + _T("/") + name;

  switch (cfg.GetParamType(owner, name))
  {
    case CFG_TEXT:
    {
      par = new MaterParText2(name, owner, value, this);
    }
    break;
    case CFG_REAL:
    {
      par = new MaterParReal2(name, owner, value, this);
    }
    break;
    case CFG_RANGE:
    {
      par = new MaterParRange2(name, owner, value, this);
    }
    break;
    case CFG_ENUM:
    {
      par = new MaterParEnum2(name, owner, value, this);
    }
    break;
    case CFG_COLOR:
    {
      par = new MaterParColor2(name, owner, value, this);
    }
    break;
  }
  if (par)
  {
    // ptr = MaterParPtr2(par);
    // ptr.release();
    parameters[key] = MaterParPtr2(par);
  }
  SaveParams();
  return par;
}

MaterPar2 *MaterDlg2::GetParam(M_STD_STRING owner, M_STD_STRING name)
{
  M_STD_STRING key = owner + _T("/") + name;
  MaterParMap2::iterator itr = parameters.find(key);
  if (itr == parameters.end())
    return NULL;
  return itr->second.get();
}

bool MaterDlg2::FindParam(M_STD_STRING owner, M_STD_STRING name)
{
  M_STD_STRING key = owner + _T("/") + name;
  MaterParMap2::iterator itr = parameters.find(key);
  if (itr == parameters.end())
    return false;
  // debug ( "findParam: key=%s itr->second.get()=%p", key.c_str(), itr->second.get());
  return itr->second.get() != NULL ? true : false;
}
void MaterDlg2::DelParam(M_STD_STRING owner, M_STD_STRING name)
{
  MaterPar2 *param = GetParam(owner, name);
  if (!param)
    return;
  M_STD_STRING key = owner + _T("/") + name;
  parameters.erase(key);
  delete param;
  SaveParams();
}

void MaterDlg2::RemParam(M_STD_STRING owner, M_STD_STRING name)
{
  MaterPar2 *param = GetParam(owner, name);
  if (!param)
    return;
  M_STD_STRING key = owner + _T("/") + name;
  parameters.erase(key);
  SaveParams();
}

HWND MaterDlg2::AppendDialog(const TCHAR *paramName, long Resource, DLGPROC lpDialogProc, LPARAM param)
{
  assert(iPDC < 100);
  HWND result = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(Resource), hWnd, lpDialogProc, param);
  if (!result)
  {
    DWORD error = GetLastError();
  }


  RECT rct;
  POINT pt;
  HWND parWnd = (::IsWindow(hPanel) ? (hPanel) : (hWnd));
  if (iPDC > 0)
  {
    ::GetWindowRect(paramDlgs[iPDC], &rct);
    pt.x = rct.left;
    pt.y = rct.bottom + 5;
    ::ScreenToClient(parWnd, &pt);

    ::SetWindowPos(result, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
  }
  else
  {
    HWND stHwnd = ::FindWindowEx(parWnd, NULL, _T("Button"), _T("ParamStart"));
    ::GetWindowRect(stHwnd, &rct);
    pt.x = rct.left;
    pt.y = rct.top;
    ::ScreenToClient(parWnd, &pt);

    ::SetWindowPos(result, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
  }

  ::GetWindowRect(result, &rct);
  pt.y = rct.bottom;
  pt.x = 0;
  ::ScreenToClient(parWnd, &pt);

  IRollupWindow *irw = ip->GetMtlEditorRollup();
  int ind = irw->GetPanelIndex(::GetParent(result));
  if (ind >= 0)
  {
    irw->SetPageDlgHeight(ind, pt.y + 5);
  }
  else
  {
    // irw->SetPageDlgHeight(0,pt.y+5);
  }


  ::SetWindowText(result, paramName);
  HWND stype = ::FindWindowEx(result, NULL, _T("STATIC"), _T("StaticType"));
  if (stype)
    ::SetWindowText(stype, paramName);
  ::ShowWindow(result, SW_SHOW);
  iPDC++;
  paramDlgs[iPDC] = result;
  return result;
}


void MaterDlg2::DelParamAll()
{
  MaterParMap2::iterator itr = parameters.begin();
  while (itr != parameters.end())
  {
    //                delete itr->second.get();
    //              itr->second.reset(NULL);
    itr++;
  }

  parameters.clear();
  // SaveParams();
}

BOOL MaterDlg2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;

      csa = GetIColorSwatch(GetDlgItem(hWnd, IDC_AMB), theMtl->cola, GetString(IDS_AMB_COLOR));
      assert(csa);
      csd = GetIColorSwatch(GetDlgItem(hWnd, IDC_DIFF), theMtl->cold, GetString(IDS_DIFF_COLOR));
      assert(csd);
      css = GetIColorSwatch(GetDlgItem(hWnd, IDC_SPEC), theMtl->cols, GetString(IDS_SPEC_COLOR));
      assert(css);
      cse = GetIColorSwatch(GetDlgItem(hWnd, IDC_EMIS), theMtl->cole, GetString(IDS_EMIS_COLOR));
      assert(cse);
      CheckDlgButton(hWnd, IDC_2SIDED, theMtl->twosided);

      HWND hCmb = ::GetDlgItem(hWnd, IDC_CLASSNAME);

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);

      cfg.GetShaderNames();

      HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);

      for (int pos = 0; pos < cfg.shader_names.size(); ++pos)
      {
        ::SendMessage(hCmb, CB_INSERTSTRING, -1, (LPARAM)cfg.shader_names.at(pos).c_str());
        // (LPARAM) (LPCTSTR) cfg.shader_names.at(pos).c_str());
      }

      if (!theMtl->classname.isNull())
      {
        HWND hCmb = ::GetDlgItem(hWnd, IDC_CLASSNAME);
        int iclass = ::SendMessage(hCmb, CB_FINDSTRINGEXACT, 0, (LPARAM)theMtl->classname.data());
        ::SendMessage(hCmb, CB_SETCURSEL, iclass, NULL);
      }
      else
      {
        ::SendMessage(hCmb, CB_SETCURSEL, 0, NULL);
        M_STD_STRING str = GetShaderName();
        theMtl->classname = (TCHAR *)str.c_str();
        theMtl->NotifyChanged();
      }

      for (int i = 0; i < NUMTEXMAPS; ++i)
      {
        texbut[i] = GetICustButton(GetDlgItem(hWnd, texidc[i]));
        texbut[i]->SetDADMgr(&dadmgr);
        texname[i] = GetICustStatus(GetDlgItem(hWnd, texnameidc[i]));
      }

      FillSlotNames();

      RestoreParams();
    }
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
      for (int i = 0; i < NUMTEXMAPS; ++i)
        if (LOWORD(wParam) == texidc[i])
        {
          PostMessage(hwmedit, WM_TEXMAP_BUTTON, i, (LPARAM)theMtl);
        }
      // else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == texnameidc[i]) {}

      switch (LOWORD(wParam))
      {
        case IDC_2SIDED:
        {
          if (creating)
            break;
          theMtl->twosided = IsDlgButtonChecked(hWnd, IDC_2SIDED);
          theMtl->NotifyChanged();
          UpdateMtlDisplay();
        }
        break;
        case IDC_2SIDEDREAL:
        {
          if (creating)
            break;
          theMtl->NotifyChanged();
          UpdateMtlDisplay();
        }
        break;
        case IDC_CLASSNAME:
        {
          if (creating)
            break;
          int a = CBN_SELCHANGE;
          int b = HIWORD(wParam);
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            M_STD_STRING str = GetShaderName();
            theMtl->classname = (TCHAR *)str.c_str();
            theMtl->NotifyChanged();
            SaveParams();
            DelParamAll();
            RestoreParams();
            FillSlotNames();
          }
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
              MaterParNew2 new_par(this);
              new_par.shader = GetShaderName();
              if (new_par.DoModal() == IDOK)
              {
                M_STD_STRING owner;
                M_STD_STRING name = new_par.param_name;

                switch (new_par.param_status_ind)
                {
                  case CFG_GLOBAL:
                  {
                    owner = CFG_GLOBAL_NAME;
                  }
                  break;
                  case CFG_SHADER:
                  {
                    owner = new_par.shader;
                  }
                  break;
                }

                AddParam(owner, name, _T(""));
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
      // MessageBox(NULL, "WM_CUSTEDIT_ENTER", "!", MB_OK);
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
  TCHAR filename[MAX_PATH], textureShadersFName[MAX_PATH];
  CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
  CfgShader::GetCfgFilename(_T("dagorTextures.cfg"), textureShadersFName);
  CfgShader cfg(filename), texturesCfg(textureShadersFName);

  cfg.GetSettingsParams();

  for (int p = 0; p < cfg.settings_params.size(); ++p)
  {
    M_STD_STRING value = cfg.GetParamValue(CFG_SETTINGS_NAME, cfg.settings_params.at(p));
    texname[p]->SetText((TCHAR *)value.c_str());

    M_STD_STRING shader_name = theMtl->classname;
    M_STD_STRING tex_slot = cfg.settings_params.at(p);
    texturesCfg.GetShaderParams(shader_name);

    for (int j = 0; j < texturesCfg.shader_params.size(); ++j)
      if (texturesCfg.shader_params.at(j) == tex_slot)
      {
        value = texturesCfg.GetParamValue(shader_name, tex_slot.c_str());
        texname[p]->SetText((TCHAR *)value.c_str());
        break;
      }

    texbut[p]->Enable(value != _T(""));
  }
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
    nm = t->GetFullName();
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
  CheckDlgButton(hPanel, IDC_2SIDED, theMtl->twosided);
  SetDlgItemText(hPanel, IDC_SCRIPT, theMtl->script);
  for (int i = 0; i < NUMTEXMAPS; ++i)
    UpdateTexDisplay(i);

  if (!theMtl->classname.isNull())
  {
    HWND hCmb = ::GetDlgItem(hWnd, IDC_CLASSNAME);
    int iclass = ::SendMessage(hCmb, CB_FINDSTRINGEXACT, 0, (LPARAM)theMtl->classname.data());
    ::SendMessage(hCmb, CB_SETCURSEL, iclass, NULL);
  }
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
  ZeroMemory(texHandle, sizeof(texHandle));

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
  twosided = 0;
  classname = _T("");
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
  for (int i = 0; i < NUMTEXMAPS; i++)
  {
    if (texHandle[i])
    {
      // debug("mat=0x%08x discards tex", (unsigned int)this);

      texHandle[i]->DeleteThis();
      texHandle[i] = NULL;
    }
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
  if (twosided)
    r |= MTLREQ_2SIDE;
  for (int i = 0; i < NUMTEXMAPS; ++i)
    if (texmaps->texmap[i])
      r |= texmaps->texmap[i]->Requirements(subm);
  //      if(texmap[0]) r|=texmap[0]->Requirements(subm);

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
  mtl->classname = classname;
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
  if (twosided)
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

  twosided = 0;
  while (IO_OK == (res = iload->OpenChunk()))
  {
    switch (id = iload->CurChunkID())
    {
      case MTL_HDR_CHUNK:
        res = MtlBase::Load(iload);
        ivalid.SetEmpty();
        break;
      case CH_2SIDED:
        twosided = 1;
        ivalid.SetEmpty();
        break;
      case CH_CLASSNAME:
      {
        TCHAR *s;
        res = iload->ReadCStringChunk(&s);
        if (res == IO_OK)
          classname = s;
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
  pblock->GetValue(PB_AMB, 0, c, FOREVER);
  return c;
}

Color DagorMat2::get_diff()
{
  Color c;
  pblock->GetValue(PB_DIFF, 0, c, FOREVER);
  return c;
}

Color DagorMat2::get_spec()
{
  Color c;
  pblock->GetValue(PB_SPEC, 0, c, FOREVER);
  return c;
}

Color DagorMat2::get_emis()
{
  Color c;
  pblock->GetValue(PB_EMIS, 0, c, FOREVER);
  return c;
}

float DagorMat2::get_power()
{
  float f = 0;
  pblock->GetValue(PB_SHIN, 0, f, FOREVER);
  return powf(2.0f, f / 10.0f) * 4.0f;
}

BOOL DagorMat2::get_2sided() { return twosided; }

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

void DagorMat2::set_2sided(BOOL b)
{
  twosided = b;
  NotifyChanged();
}

void DagorMat2::set_classname(const TCHAR *s)
{
  if (s)
    classname = s;
  else
    classname = _T("");
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

  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;

  return res;
}

MaterParNew2::MaterParNew2(MaterDlg2 *p) :
  param_status_ind(-1), param_name_ind(-1), param_status(_T("")), param_name(_T("")), parent(p)
{}

MaterParNew2::~MaterParNew2() {}

int MaterParNew2::DoModal()
{
  return ::DialogBoxParam(hInstance, (const TCHAR *)IDD_DAGORPAR_NEW, NULL, MaterParNewProc2, (LPARAM)this);
}

BOOL MaterParNew2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;

      //                                      HWND hCmbStatus = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);

      //                                      ::SendMessage(hCmbStatus, CB_INSERTSTRING, -1, (LPARAM) (LPCTSTR)"global parameter");
      //                                      ::SendMessage(hCmbStatus, CB_INSERTSTRING, -1, (LPARAM) (LPCTSTR)"shader parameter");

      HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);

      // Parameters list
      // HWND hCmbStatus       = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);
      //                                       param_status_ind = ::SendMessage(hCmbStatus, CB_GETCURSEL, 0, 0);

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      // HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);

      ::SendMessage(hCmbName, CB_RESETCONTENT, 0, 0);

      //                                      ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM) (LPCTSTR)"Global");

      cfg.GetGlobalParams();
      int pcount = 0;
      for (int pos = 0; pos < cfg.global_params.size(); ++pos)
      {
        int mode = cfg.GetParamMode(CFG_GLOBAL_NAME, cfg.global_params.at(pos));
        TCHAR pname[255];
        _tcscpy(pname, cfg.global_params.at(pos).c_str());

        if (mode != CFG_COMMON && !parent->FindParam(CFG_GLOBAL_NAME, cfg.global_params.at(pos)) &&
            _tcscmp(pname, _T( "real_two_sided" )) != 0)
        {
          ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM)pname);
          pcount++;
        }
      }

      param_shader_starts_from = pcount;

      //                                      ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM) (LPCTSTR)"Shader");

      if (!shader.empty())
      {
        cfg.GetShaderParams(shader);
        for (int pos = 0; pos < cfg.shader_params.size(); ++pos)
        {
          int mode = cfg.GetParamMode(shader, cfg.shader_params.at(pos));
          if (mode != CFG_COMMON && !parent->FindParam(shader, cfg.shader_params.at(pos)))
          {
            TCHAR pname[255];
            _tcscpy(pname, cfg.shader_params.at(pos).c_str());
            ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM)pname);
          }
        }
      }
      //-Paramlist
    }
    break;

    case WM_PAINT:
    {
      return FALSE;
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
              HWND hCmbStatus = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);
              HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);
              param_status_ind = ((::SendMessage(hCmbName, CB_GETCURSEL, 0, 0)) < param_shader_starts_from) ? (0) : (1);

              // char dbg_str[255];
              // sprintf(dbg_str, "%d>%d", param_shader_starts_from,(::SendMessage(hCmbName, CB_GETCURSEL, 0, 0)));
              // MessageBox(NULL, dbg_str, "Debug", MB_OK);

              param_name_ind = ::SendMessage(hCmbName, CB_GETCURSEL, 0, 0);

              GetParamStatus();
              GetParamName();


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
            case BN_CLICKED:
            {
              ::EndDialog(hWnd, IDCANCEL);
            }
            break;
          };
        }
        break;

        case IDC_PARAM_NAME:
        {
          switch (HIWORD(wParam))
          {
            case CBN_DBLCLK:
            {
              HWND hCmbOK = ::GetDlgItem(hWnd, IDOK);
              //::SendMessage(hWnd, WM_COMMAND, MAKELONG(IDOK,BN_CLICKED), (LONG_PTR)hCmbOK);

              HWND hCmbStatus = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);
              HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);
              param_status_ind = ((::SendMessage(hCmbName, CB_GETCURSEL, 0, 0)) < param_shader_starts_from) ? (0) : (1);


              param_name_ind = ::SendMessage(hCmbName, CB_GETCURSEL, 0, 0);

              GetParamStatus();
              GetParamName();


              ::EndDialog(hWnd, IDOK);

              break;
            }
          }
        }

        case IDC_PARAM_STATUS:
        {
          switch (HIWORD(wParam))
          {
            case CBN_SELCHANGE:
            {
              /*                                                                              HWND hCmbStatus = ::GetDlgItem(hWnd,
              IDC_PARAM_STATUS); param_status_ind = ::SendMessage(hCmbStatus, CB_GETCURSEL, 0, 0); char filename[MAX_PATH];
              CfgShader::GetCfgFilename("DagorShaders.cfg", filename);
              CfgShader cfg(filename);
              HWND hCmbName = ::GetDlgItem(hWnd, IDC_PARAM_NAME);
              ::SendMessage(hCmbName, CB_RESETCONTENT , 0, 0);
              if(param_status_ind == 0) {
              cfg.GetGlobalParams();
              for(int pos=0; pos<cfg.global_params.size();++pos) {
              int mode = cfg.GetParamMode(CFG_GLOBAL_NAME, cfg.global_params.at(pos));
              if(mode != CFG_COMMON && !parent->FindParam(CFG_GLOBAL_NAME, cfg.global_params.at(pos)) && cfg.global_params.at(pos) !=
              "real_two_sided")
              ::SendMessage(hCmbName, CB_INSERTSTRING, -1, (LPARAM) (LPCTSTR) cfg.global_params.at(pos).c_str());
              }
              }
              else if(!shader.empty()){
              cfg.GetShaderParams(shader);
              for(int pos=0; pos<cfg.shader_params.size();++pos) {
              int mode = cfg.GetParamMode(shader, cfg.shader_params.at(pos));
              if(mode != CFG_COMMON && !parent->FindParam(shader, cfg.shader_params.at(pos))) ::SendMessage(hCmbName, CB_INSERTSTRING,
              -1, (LPARAM) (LPCTSTR) cfg.shader_params.at(pos).c_str());
              }
              }
              */
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

M_STD_STRING MaterParNew2::GetParamStatus()
{
  HWND hCmbStatus = ::GetDlgItem(hWnd, IDC_PARAM_STATUS);
  int len = ::GetWindowTextLength(hCmbStatus);

  if (len > 0)
  {
    param_status.resize(len);
    ::GetWindowText(hCmbStatus, (TCHAR *)param_status.c_str(), len + 1);
  }

  return param_status;
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

MaterPar2::MaterPar2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) :
  name(n), owner(o), value(v), parent(p), hWnd(NULL), hPanel(NULL), valid(FALSE), creating(TRUE)
{
  rollUpLess = false;
}

MaterPar2::~MaterPar2()
{
  if (!rollUpLess)
  {
    IRollupWindow *irw = parent->ip->GetMtlEditorRollup();
    int ind = irw->GetPanelIndex(hPanel);
    irw->DeleteRollup(ind, 1);
  }
  else
  {
    ::DestroyWindow(hPanel);
  }
}

void MaterPar2::Invalidate()
{
  valid = FALSE;
  isActive = FALSE;
  Rect rect;
  rect.left = rect.top = 0;
  rect.right = rect.bottom = 10;
  InvalidateRect(hPanel, &rect, FALSE);
}

BOOL MaterPar2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
    }
    break;

    case WM_COMMAND:
    {
    }
    break;
    case WM_CUSTEDIT_ENTER:
    case CC_SPINNER_BUTTONUP:
    {
      //                                                                                      MessageBox(NULL, "123", "321", MB_OK);
      //                                                                                      if (ownerDlg2) ownerDlg2->SaveParams();
      break;
    }

    default:
    {
      return FALSE;
    }
    break;
  }

  return TRUE;
}

int MaterPar2::GetType() { return 0; }

int MaterPar2::GetStatus() { return 0; }

HWND MaterPar2::GetHWND() { return hPanel; }

M_STD_STRING MaterPar2::GetName() { return name; }

M_STD_STRING MaterPar2::GetValue() { return value; }

void MaterPar2::SetName(const M_STD_STRING n) { name = n; }

void MaterPar2::SetValue(const M_STD_STRING v) { value = v; }

void MaterPar2::Move(const Rect &place)
{
  ::MoveWindow(hPanel, place.left, place.top, place.right - place.left, place.bottom - place.top, TRUE);
  IRollupWindow *irw = parent->ip->GetMtlEditorRollup();
  int ind = irw->GetPanelIndex(::GetParent(hPanel));
  irw->GetPanelHeight(ind);
  irw->SetPageDlgHeight(ind, place.bottom - place.top);
}

void MaterPar2::Reheight(const unsigned cy)
{
  if (!rollUpLess)
  {
    RECT rect;
    ::GetWindowRect(hPanel, &rect);
    POINT point;
    point.x = rect.left;
    point.y = rect.top;
    ::ScreenToClient(::GetParent(hPanel), &point);
    rect.bottom += cy;
    ::MoveWindow(hPanel, point.x, point.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);

    IRollupWindow *irw = parent->ip->GetMtlEditorRollup();
    int ind = irw->GetPanelIndex(::GetParent(hPanel));
    irw->GetPanelHeight(ind);
    irw->SetPageDlgHeight(ind, rect.bottom - rect.top);
  }
}

void MaterPar2::Rewidth(const unsigned cx)
{
  if (!rollUpLess)
  {
    RECT rect;
    ::GetWindowRect(hPanel, &rect);
    POINT point;
    point.x = rect.left;
    point.y = rect.top;
    ::ScreenToClient(::GetParent(hPanel), &point);
    rect.right += cx;
    ::MoveWindow(hPanel, point.x, point.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);

    IRollupWindow *irw = parent->ip->GetMtlEditorRollup();
    int ind = irw->GetPanelIndex(::GetParent(hPanel));
    irw->GetPanelHeight(ind);
    irw->SetPageDlgHeight(ind, rect.bottom - rect.top);
  }
}

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
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}

MaterParText2::MaterParText2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) : edit(NULL), MaterPar2(n, o, v, p)
{
  M_STD_STRING title(GetString(IDS_DAGORPAR_TEXT_LONG));
  title += _T(": ");
  title += name;
  rollUpLess = true;

  //        hPanel = p->ip->AddRollupPage(
  //                hInstance,
  //                MAKEINTRESOURCE(IDD_DAGORPAR_TEXT),
  //                MaterParTextProc2,
  //                (LPSTR)title.c_str(),
  //                (LPARAM)this);
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_TEXT, MaterParTextProc2, (LPARAM)this);
  Reheight(0);
  creating = FALSE;
}

MaterParText2::~MaterParText2() {}

BOOL MaterParText2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      edit = ::GetICustEdit(GetDlgItem(hWnd, IDC_PAR_TEXT_VALUE));
      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      if (cfg.GetParamMode(owner, name) == CFG_OPTIONAL)
        ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
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
              parent->RemParam(owner, name);
              parent->DialogsReposition();
              //                                                                                        delete this;
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

M_STD_STRING MaterParText2::GetValue()
{
  TCHAR buff[MAX_PATH];
  edit->GetText(buff, MAX_PATH);
  value = buff;
  parent->SaveParams();

  return value;
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
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}

MaterParReal2::MaterParReal2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) : spinner(NULL), MaterPar2(n, o, v, p)
{
  M_STD_STRING title(GetString(IDS_DAGORPAR_REAL_LONG));
  title += _T(": ");
  title += name;

  rollUpLess = true;
  // hPanel = p->ip->AddRollupPage(
  //         hInstance,
  //         MAKEINTRESOURCE(IDD_DAGORPAR_REAL),
  //         MaterParRealProc2,
  //         (LPSTR)title.c_str(),
  //         (LPARAM)this);

  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_REAL, MaterParRealProc2, (LPARAM)this);

  Reheight(0);
  creating = FALSE;
}

MaterParReal2::~MaterParReal2()
{
  //      ::DestroyWindow(hPanel);
  //      EndDialog(hPanel, 0);
  //      hPanel = parent->ip->AddRollupPage(
  //              hInstance,
  //              MAKEINTRESOURCE(IDD_DAGORPAR_REAL),
  //              MaterParRealProc2,
  //             (LPSTR)"11",
  //              (LPARAM)this);
  //        rollUpLess=false;
}

BOOL MaterParReal2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      spinner = ::SetupFloatSpinner(hWnd, IDC_PAR_REAL_VALUE_SPINNER, IDC_PAR_REAL_VALUE, -2147483648.0f, 2147483648.0f, 0.0f, 1.0f);
      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      if (cfg.GetParamMode(owner, name) == CFG_OPTIONAL)
        ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
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
              parent->RemParam(owner, name);
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

M_STD_STRING MaterParReal2::GetValue()
{
  float f = spinner->GetFVal();

  M_STD_OSTRINGSTREAM str;
  str << f;

  value = str.str();

  parent->theMtl->script;
  parent->SaveParams();

  return value;
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
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}

MaterParColor2::MaterParColor2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) : col(NULL), MaterPar2(n, o, v, p)
{
  M_STD_STRING title(GetString(IDS_DAGORPAR_COLOR_LONG));
  title += _T( ": ");
  title += name;

  rollUpLess = true;

  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_COLOR, MaterParColorProc2, (LPARAM)this);

  Reheight(0);
  creating = FALSE;
}

MaterParColor2::~MaterParColor2() { ReleaseIColorSwatch(col); }

BOOL MaterParColor2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      AColor c;
      col = GetIColorSwatch(GetDlgItem(hWnd, IDC_PAR_COLOR_VALUE), c, _T("Color4 parameter"));

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      if (cfg.GetParamMode(owner, name) == CFG_OPTIONAL)
        ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
      GetValue();
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
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
              parent->RemParam(owner, name);
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

M_STD_STRING MaterParColor2::GetValue()
{
  AColor c = col->GetAColor();
  TCHAR buf[128];
  _stprintf(buf, _T("%.3f,%.3f,%.3f,%.3f"), c.r, c.g, c.b, c.a);

  value = buf;

  parent->theMtl->script;
  parent->SaveParams();

  return value;
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

static INT_PTR CALLBACK MaterParRangeProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterParRange2 *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterParRange2 *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterParRange2 *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}

MaterParRange2::MaterParRange2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) : spinner(NULL), MaterPar2(n, o, v, p)
{
  M_STD_STRING title(GetString(IDS_DAGORPAR_RANGE_LONG));
  title += _T(": ");
  title += name;
  rollUpLess = true;
  // hPanel = p->ip->AddRollupPage(
  //         hInstance,
  //         MAKEINTRESOURCE(IDD_DAGORPAR_RANGE),
  //         MaterParRangeProc2,
  //         (LPSTR)title.c_str(),
  //         (LPARAM)this);

  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_RANGE, MaterParRangeProc2, (LPARAM)this);

  Reheight(0);
  creating = FALSE;
}

MaterParRange2::~MaterParRange2() {}

BOOL MaterParRange2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;
      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
      CfgShader cfg(filename);
      StringVector buttons = cfg.GetParamData(owner, name);

      float fmin = ::_tstof(buttons.at(0).c_str());
      float fmax = ::_tstof(buttons.at(1).c_str());

      spinner = ::SetupFloatSpinner(hWnd, IDC_PAR_RANGE_SPINNER, IDC_PAR_RANGE_VALUE, fmin, fmax, 0.0f, 1.0f);

      if (cfg.GetParamMode(owner, name) == CFG_OPTIONAL)
        ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);
      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
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
              parent->RemParam(owner, name);
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
        case IDC_PAR_RANGE_SPINNER:
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

M_STD_STRING MaterParRange2::GetValue()
{
  float f = spinner->GetFVal();

  M_STD_OSTRINGSTREAM str;
  str << f;

  value = str.str();

  parent->theMtl->script;
  parent->SaveParams();

  return value;
}

void MaterParRange2::SetValue(const M_STD_STRING v)
{
  M_STD_ISTRINGSTREAM str(v);
  float f;
  str >> f;
  spinner->SetValue(f, TRUE);

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
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}

MaterParEnum2::MaterParEnum2(M_STD_STRING n, M_STD_STRING o, M_STD_STRING v, MaterDlg2 *p) : MaterPar2(n, o, v, p), count(0)
{
  M_STD_STRING title(GetString(IDS_DAGORPAR_ENUM_LONG));
  title += _T(": ");
  title += name;
  rollUpLess = true;
  //        hPanel = p->ip->AddRollupPage(
  //                hInstance,
  //                MAKEINTRESOURCE(IDD_DAGORPAR_ENUM),
  //                MaterParEnumProc2,
  //                (LPSTR)title.c_str(),
  //                (LPARAM)this);
  hPanel = p->AppendDialog(name.c_str(), IDD_DAGORPAR_ENUM, MaterParEnumProc2, (LPARAM)this);

  Reheight((count / 4) * 15);
  creating = FALSE;
}

MaterParEnum2::~MaterParEnum2() {}

BOOL MaterParEnum2::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      this->hWnd = hWnd;

      TCHAR filename[MAX_PATH];
      CfgShader::GetCfgFilename(_T("DagorShaders.cfg"), filename);
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

      if (cfg.GetParamMode(owner, name) == CFG_OPTIONAL)
        ::EnableWindow(::GetDlgItem(hWnd, IDC_PARAM_DELETE), TRUE);

      if (!value.empty())
        SetValue(value);
    }
    break;

    case WM_PAINT:
    {
      if (!valid)
      {
        valid = TRUE;
        ReloadDialog();
      }
      return FALSE;
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
              parent->RemParam(owner, name);
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

M_STD_STRING MaterParEnum2::GetValue()
{
  for (int pos = 0; pos < count; ++pos)
  {
    int id = (IDC_PAR_ENUM_RADIO + pos);
    int state = ::SendMessage(::GetDlgItem(hWnd, id), BM_GETCHECK, 0, NULL);
    if (state)
    {
      int length = ::GetWindowTextLength(::GetDlgItem(hWnd, id));
      value.resize(length);
      ::GetWindowText(::GetDlgItem(hWnd, id), (TCHAR *)value.c_str(), length + 1);
      break;
    }
  }
  parent->SaveParams();
  return value;
}

void MaterParEnum2::SetValue(const M_STD_STRING v)
{
  for (int pos = 0; pos < count; ++pos)
  {
    int id = (IDC_PAR_ENUM_RADIO + pos);
    M_STD_STRING str;
    int length = ::GetWindowTextLength(::GetDlgItem(hWnd, id));
    str.resize(length);
    ::GetWindowText(::GetDlgItem(hWnd, id), (TCHAR *)str.c_str(), length + 1);
    if (str == v)
    {
      ::SendMessage(::GetDlgItem(hWnd, id), BM_SETCHECK, 1, NULL);
    }
    else
    {
      ::SendMessage(::GetDlgItem(hWnd, id), BM_SETCHECK, 0, NULL);
    }
  }
}

int MaterParEnum2::GetCount() { return (int)(count / 2); }

TCHAR *MaterParEnum2::GetRadioName(int at) { return NULL; }

int MaterParEnum2::GetRadioState(int at) { return 0; }

int MaterParEnum2::GetRadioSelected() { return 0; }

void MaterPar2::SetOwnerDlg(MaterDlg2 *dlg2) { ownerDlg2 = dlg2; }


void MaterDlg2::DialogsReposition()
{
  assert(iPDC < 100);
  // Remove deleted window from paramDlgs array
  UINT i;
  for (i = 1; i <= iPDC; i++)
  {
    if (!::IsWindow(paramDlgs[i]))
    {
      for (UINT n = i; n <= iPDC; n++)
        paramDlgs[n] = paramDlgs[(n + 1 <= iPDC) ? (n + 1) : (n)];
      if (iPDC > 0)
        iPDC--;
      break;
    }
  }

  // Dialogs reposition
  RECT rct;
  POINT pt;
  HWND parWnd = (::IsWindow(hWnd) ? (hWnd) : (hPanel));
  for (i = 1; i <= iPDC; i++)
    if (i > 1)
    {
      ::GetWindowRect(paramDlgs[i - 1], &rct);
      pt.x = rct.left;
      pt.y = rct.bottom + 5;
      ::ScreenToClient(parWnd, &pt);

      ::SetWindowPos(paramDlgs[i], HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    else
    {
      HWND stHwnd = ::FindWindowEx(parWnd, NULL, _T("Button"), _T("ParamStart"));
      ::GetWindowRect(stHwnd, &rct);
      pt.x = rct.left;
      pt.y = rct.top;
      ::ScreenToClient(parWnd, &pt);

      ::SetWindowPos(paramDlgs[i], HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }

  //      if(::IsWindow(paramDlgs[i]))
  //    {
  //      ::GetWindowRect(paramDlgs[i], &rct);
  //      pt.y=rct.bottom;
  //      pt.x=0;
  //      ::ScreenToClient(parWnd, &pt);
  //
  //      IRollupWindow *irw = ip->GetMtlEditorRollup();
  //      int ind = irw->GetPanelIndex(::GetParent(parWnd));
  //      irw->GetPanelHeight(ind);
  //      irw->SetPageDlgHeight(ind,pt.y+5);
  //    }
}
