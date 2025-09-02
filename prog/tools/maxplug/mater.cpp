// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <stdmat.h>
#include "dagor.h"
#include "mater.h"
#include "texmaps.h"
#include "resource.h"
#include <IHardwareMaterial.h>
#include <iparamb2.h>
#include <d3d9types.h>
// #include "debug.h"

#include <string>

#define TX_MODULATE   0
#define TX_ALPHABLEND 1
#define BMIDATA(x)    ((UBYTE *)((BYTE *)(x) + sizeof(BITMAPINFOHEADER)))

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

class MaterDlg : public ParamDlg
{
public:
  HWND hwmedit;
  IMtlParams *ip;
  class DagorMat *theMtl;
  HWND hPanel;
  BOOL valid;
  BOOL isActive;
  ISpinnerControl *iShin;
  IColorSwatch *csa, *csd, *css, *cse;
  ICustEdit *eclassname;
  BOOL creating;
  TexDADMgr dadmgr;
  ICustButton *tbut[NUMTEXMAPS];

  MaterDlg(HWND hwMtlEdit, IMtlParams *imp, DagorMat *m);
  ~MaterDlg() override;

  BOOL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  void Invalidate();
  void UpdateMtlDisplay() { ip->MtlChanged(); }
  void UpdateTexDisplay(int i);

  // methods inherited from ParamDlg:
  int FindSubTexFromHWND(HWND hw) override;
  void ReloadDialog() override;
  Class_ID ClassID() override { return DagorMat_CID; }
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
};


class DagorMat : public Mtl, public IDagorMat
{
  void DiscardTexHandles();
  void updateViewportTexturesState();
  bool hasAlpha();
  TexHandle *texHandle[NUMTEXMAPS];

public:
  class MaterDlg *dlg;
  IParamBlock *pblock;
  Texmaps *texmaps;
  Interval ivalid;
  float shin;
  float power;
  Color cola, cold, cols, cole;
  Sides twosided;
  TSTR classname, script;

  DagorMat(BOOL loading);
  ~DagorMat() override;
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
  void GetClassName(TSTR &s, bool localized) { s = GetString(IDS_DAGORMAT); }
#else
  void GetClassName(TSTR &s) { s = GetString(IDS_DAGORMAT); }
#endif

  void DeleteThis() override;

  ULONG Requirements(int subMtlNum) override;
  int NumSubs() override;
  Animatable *SubAnim(int i) override;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  MSTR SubAnimName(int i, bool localized) override;
#else
  TSTR SubAnimName(int i);
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
  void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb) override;
  void ActivateTexDisplay(BOOL onoff) override;
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

class MaterClassDesc : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading) override { return new DagorMat(loading); }
  const TCHAR *ClassName() override { return GetString(IDS_DAGORMAT_LONG); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return MATERIAL_CLASS_ID; }
  Class_ID ClassID() override { return DagorMat_CID; }
  const TCHAR *Category() override { return _T(""); }
};
static MaterClassDesc materCD;
ClassDesc *GetMaterCD() { return &materCD; }

class TexmapsClassDesc : public ClassDesc
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
static TexmapsClassDesc texmapsCD;
ClassDesc *GetTexmapsCD() { return &texmapsCD; }

//--- MaterDlg ------------------------------------------------------

static int texidc[NUMTEXMAPS] = {
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

static INT_PTR CALLBACK PanelDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  MaterDlg *dlg;
  if (msg == WM_INITDIALOG)
  {
    dlg = (MaterDlg *)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
  }
  else
  {
    if ((dlg = (MaterDlg *)GetWindowLongPtr(hWnd, GWLP_USERDATA)) == NULL)
      return FALSE;
  }
  dlg->isActive = TRUE;
  int res = dlg->WndProc(hWnd, msg, wParam, lParam);
  dlg->isActive = FALSE;
  return res;
}


MaterDlg::MaterDlg(HWND hwMtlEdit, IMtlParams *imp, DagorMat *m)
{
  dadmgr.Init(this);
  hwmedit = hwMtlEdit;
  ip = imp;
  theMtl = m;
  valid = FALSE;
  iShin = NULL;
  creating = TRUE;
  eclassname = NULL;
  for (int i = 0; i < NUMTEXMAPS; ++i)
  {
    tbut[i] = NULL;
  }
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_DAGORMAT), PanelDlgProc, GetString(IDS_DAGORMAT_LONG), (LPARAM)this);
  creating = FALSE;
}

MaterDlg::~MaterDlg()
{
  theMtl->dlg = NULL;
  ReleaseISpinner(iShin);
  ReleaseICustEdit(eclassname);
  ReleaseIColorSwatch(csa);
  ReleaseIColorSwatch(csd);
  ReleaseIColorSwatch(css);
  ReleaseIColorSwatch(cse);
  for (int i = 0; i < NUMTEXMAPS; ++i)
  {
    ReleaseICustButton(tbut[i]);
  }
  SetWindowLongPtr(hPanel, GWLP_USERDATA, NULL);
}

int MaterDlg::FindSubTexFromHWND(HWND hw)
{
  for (int i = 0; i < NUMTEXMAPS; ++i)
    if (tbut[i])
      if (tbut[i]->GetHwnd() == hw)
        return i;
  return -1;
}

BOOL MaterDlg::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      iShin = SetupFloatSpinner(hWnd, IDC_SHIN_S, IDC_SHIN, 0.0f, 100.0f, 30.0f, 1.0f);
      assert(iShin);
      csa = GetIColorSwatch(GetDlgItem(hWnd, IDC_AMB), theMtl->cola, GetString(IDS_AMB_COLOR));
      assert(csa);
      csd = GetIColorSwatch(GetDlgItem(hWnd, IDC_DIFF), theMtl->cold, GetString(IDS_DIFF_COLOR));
      assert(csd);
      css = GetIColorSwatch(GetDlgItem(hWnd, IDC_SPEC), theMtl->cols, GetString(IDS_SPEC_COLOR));
      assert(css);
      cse = GetIColorSwatch(GetDlgItem(hWnd, IDC_EMIS), theMtl->cole, GetString(IDS_EMIS_COLOR));
      assert(cse);

      SendMessage(GetDlgItem(hWnd, IDC_ENUM_2SIDED), CB_SETCURSEL, WPARAM(theMtl->twosided), NULL);

      eclassname = GetICustEdit(GetDlgItem(hWnd, IDC_CLASSNAME));
      assert(eclassname);
      eclassname->SetText(theMtl->classname);
      SetDlgItemText(hWnd, IDC_SCRIPT, STR_DEST(theMtl->script));
      for (int i = 0; i < NUMTEXMAPS; ++i)
      {
        tbut[i] = GetICustButton(GetDlgItem(hWnd, texidc[i]));
        tbut[i]->SetDADMgr(&dadmgr);
      }
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
      switch (LOWORD(wParam))
      {
        case IDC_ENUM_2SIDED:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            if (creating)
              break;

            theMtl->twosided = IDagorMat::Sides(SendMessage(GetDlgItem(hWnd, IDC_ENUM_2SIDED), CB_GETCURSEL, 0, 0));
            theMtl->NotifyChanged();
            UpdateMtlDisplay();
          }
          break;
        case IDC_CLASSNAME:
          if (creating)
            break;
          if (HIWORD(wParam) == EN_CHANGE)
          {
            HWND hw = (HWND)lParam;
            int l = GetWindowTextLength(hw);
            TSTR s;
            s.Resize(l + 1);
            GetWindowText(hw, STR_DEST(s), l + 1);
            theMtl->classname = s;
            theMtl->NotifyChanged();
          }
          break;
        case IDC_SCRIPT:
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

void MaterDlg::Invalidate()
{
  valid = FALSE;
  isActive = FALSE;
  Rect rect;
  rect.left = rect.top = 0;
  rect.right = rect.bottom = 10;
  InvalidateRect(hPanel, &rect, FALSE);
}

void MaterDlg::SetThing(ReferenceTarget *m)
{
  theMtl = (DagorMat *)m;
  if (theMtl)
    theMtl->dlg = this;
  ReloadDialog();
}

BOOL MaterDlg::KeyAtCurTime(int id) { return theMtl->pblock->KeyFrameAtTime(id, ip->GetTime()); }

void MaterDlg::UpdateTexDisplay(int i)
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
  tbut[i]->SetText(nm);
}

void MaterDlg::ReloadDialog()
{
  Interval valid;
  TimeValue time = ip->GetTime();
  theMtl->Update(time, valid);
  iShin->SetValue(theMtl->shin, FALSE);
  iShin->SetKeyBrackets(KeyAtCurTime(PB_SHIN));
  csa->SetColor(theMtl->cola);
  csa->SetKeyBrackets(KeyAtCurTime(PB_AMB));
  csd->SetColor(theMtl->cold);
  csd->SetKeyBrackets(KeyAtCurTime(PB_DIFF));
  css->SetColor(theMtl->cols);
  css->SetKeyBrackets(KeyAtCurTime(PB_SPEC));
  cse->SetColor(theMtl->cole);
  cse->SetKeyBrackets(KeyAtCurTime(PB_EMIS));

  SendMessage(GetDlgItem(hPanel, IDC_ENUM_2SIDED), CB_SETCURSEL, WPARAM(theMtl->twosided), NULL);

  eclassname->SetText(theMtl->classname);
  SetDlgItemText(hPanel, IDC_SCRIPT, theMtl->script);
  for (int i = 0; i < NUMTEXMAPS; ++i)
    UpdateTexDisplay(i);
}

//--- DagorMat -------------------------------------------------

#define VERSION 0

static ParamBlockDescID pbdesc[] = {
  {TYPE_RGBA, NULL, FALSE, PB_AMB},
  {TYPE_RGBA, NULL, FALSE, PB_DIFF},
  {TYPE_RGBA, NULL, FALSE, PB_SPEC},
  {TYPE_RGBA, NULL, FALSE, PB_EMIS},
  {TYPE_FLOAT, NULL, FALSE, PB_SHIN},
};

DagorMat::DagorMat(BOOL loading)
{
  dlg = NULL;
  pblock = NULL;
  texmaps = NULL;
  ivalid.SetEmpty();
  ::ZeroMemory(texHandle, sizeof(texHandle));
  Reset();
}

DagorMat::~DagorMat() { DiscardTexHandles(); }

void DagorMat::Reset()
{
  ReplaceReference(TEX_REF, (Texmaps *)CreateInstance(TEXMAP_CONTAINER_CLASS_ID, Texmaps_CID));
  ReplaceReference(PB_REF, CreateParameterBlock(pbdesc, sizeof(pbdesc) / sizeof(pbdesc[0]), VERSION));
  pblock->SetValue(PB_AMB, 0, cola = Color(1, 1, 1));
  pblock->SetValue(PB_DIFF, 0, cold = Color(1, 1, 1));
  pblock->SetValue(PB_SPEC, 0, cols = Color(1, 1, 1));
  pblock->SetValue(PB_EMIS, 0, cole = Color(0, 0, 0));
  pblock->SetValue(PB_SHIN, 0, shin = 30.0f);
  twosided = Sides::OneSided;
  classname = _T("");
  script = _T("");
  updateViewportTexturesState();
}

ParamDlg *DagorMat::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
  dlg = new MaterDlg(hwMtlEdit, imp, this);
  return dlg;
}


static Color blackCol(0, 0, 0);

void DagorMat::Shade(ShadeContext &sc)
{
  if (gbufID)
    sc.SetGBufferID(gbufID);
  if (sc.mode == SCMODE_SHADOW)
  {
    sc.out.t = blackCol;
    return;
  }
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
    Color c(mc.r, mc.g, mc.b);
    sc.out.c = (diffIllum * cold + cole + cola * sc.ambientLight) * c + specIllum * cols;
  }
  else
    sc.out.c = diffIllum * cold + cole + cola * sc.ambientLight + specIllum * cols;
  sc.out.t = blackCol;
}

ULONG DagorMat::Requirements(int subm)
{
  ULONG r = MTLREQ_PHONG;
  if (twosided == IDagorMat::Sides::DoubleSided)
    r |= MTLREQ_2SIDE;
  for (int i = 0; i < NUMTEXMAPS; ++i)
    if (texmaps->texmap[i])
      r |= texmaps->texmap[i]->Requirements(subm);
  //      if(texmap[0]) r|=texmap[0]->Requirements(subm);
  return r;
}


void DagorMat::Update(TimeValue t, Interval &valid)
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

Interval DagorMat::Validity(TimeValue t)
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

int DagorMat::NumSubTexmaps() { return NUMTEXMAPS; }

Texmap *DagorMat::GetSubTexmap(int i) { return texmaps->gettex(i); }

void DagorMat::SetSubTexmap(int i, Texmap *m)
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

int DagorMat::NumSubs() { return 10; }

int DagorMat::SubNumToRefNum(int subNum) { return subNum; }

int DagorMat::NumRefs() { return 10; }

Animatable *DagorMat::SubAnim(int i)
{
  switch (i)
  {
    case PB_REF: return pblock;
    case TEX_REF: return texmaps;
    default: return NULL;
  }
}

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
MSTR DagorMat::SubAnimName(int i, bool localized)
#else
TSTR DagorMat::SubAnimName(int i)
#endif
{
  switch (i)
  {
    case PB_REF: return _T("Parameters");
    case TEX_REF: return _T("Maps");
    default: return _T("");
  }
}

RefTargetHandle DagorMat::GetReference(int i)
{
  switch (i)
  {
    case PB_REF: return pblock;
    case TEX_REF: return texmaps;
    default: return NULL;
  }
}

void DagorMat::SetReference(int i, RefTargetHandle rtarg)
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

RefTargetHandle DagorMat::Clone(RemapDir &remap)
{
  DagorMat *mtl = new DagorMat(FALSE);
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
RefResult DagorMat::NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
  BOOL propagate)
#else
RefResult DagorMat::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message)
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

IOResult DagorMat::Save(ISave *isave)
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

IOResult DagorMat::Load(ILoad *iload)
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
  return IO_OK;
}

void DagorMat::ActivateTexDisplay(BOOL onoff)
{
  if (!onoff)
    DiscardTexHandles();
}


void DagorMat::SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb)
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

    mtl->texture[0].textHandle = texHandle[0]->GetHandle();
    mtl->texture[0].colorOp = GW_TEX_REPLACE;
    mtl->texture[0].colorAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].colorScale = GW_TEX_SCALE_1X;
    mtl->texture[0].alphaOp = GW_TEX_REPLACE;
    mtl->texture[0].alphaAlphaSource = GW_TEX_TEXTURE;
    mtl->texture[0].alphaScale = GW_TEX_SCALE_1X;
  }
}

bool DagorMat::hasAlpha() { return _tcsstr(script, _T("atest")) || _tcsstr(classname, _T("alpha")); }

void DagorMat::DiscardTexHandles()
{
  for (int i = 0; i < NUMTEXMAPS; i++)
    if (texHandle[i])
    {
      texHandle[i]->DeleteThis();
      texHandle[i] = NULL;
    }
}

void DagorMat::updateViewportTexturesState()
{
  if (hasAlpha())
  {
    SetMtlFlag(MTL_TEX_DISPLAY_ENABLED);
    SetMtlFlag(MTL_HW_TEX_ENABLED);
  }
  else if (texmaps->gettex(0))
  {
    SetActiveTexmap(texmaps->gettex(0));
    SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS);
    ClearMtlFlag(MTL_TEX_DISPLAY_ENABLED);
  }
}

void DagorMat::NotifyChanged()
{
  updateViewportTexturesState();
  NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
  DiscardTexHandles();
}

void DagorMat::SetAmbient(Color c, TimeValue t)
{
  cola = c;
  pblock->SetValue(PB_AMB, t, c);
}

void DagorMat::SetDiffuse(Color c, TimeValue t)
{
  cold = c;
  pblock->SetValue(PB_DIFF, t, c);
}

void DagorMat::SetSpecular(Color c, TimeValue t)
{
  cols = c;
  pblock->SetValue(PB_SPEC, t, c);
}

void DagorMat::SetShininess(float v, TimeValue t)
{
  shin = v * 100.0f;
  power = powf(2.0f, shin / 10.0f) * 4.0f;
  pblock->SetValue(PB_SHIN, t, shin);
}

Color DagorMat::GetAmbient(int mtlNum, BOOL backFace) { return cola; }

Color DagorMat::GetDiffuse(int mtlNum, BOOL backFace) { return cold; }

Color DagorMat::GetSpecular(int mtlNum, BOOL backFace) { return cols; }

float DagorMat::GetXParency(int mtlNum, BOOL backFace) { return 0; }

float DagorMat::GetShininess(int mtlNum, BOOL backFace) { return shin / 100.0f; }

float DagorMat::GetShinStr(int mtlNum, BOOL backFace)
{
  if (cols == Color(0, 0, 0))
    return 0;
  return 1;
}

Class_ID DagorMat::ClassID() { return DagorMat_CID; }

SClass_ID DagorMat::SuperClassID() { return MATERIAL_CLASS_ID; }

void DagorMat::DeleteThis() { delete this; }

Color DagorMat::get_amb()
{
  Color c;
  pb_get_value(*pblock, PB_AMB, 0, c);
  return c;
}

Color DagorMat::get_diff()
{
  Color c;
  pb_get_value(*pblock, PB_DIFF, 0, c);
  return c;
}

Color DagorMat::get_spec()
{
  Color c;
  pb_get_value(*pblock, PB_SPEC, 0, c);
  return c;
}

Color DagorMat::get_emis()
{
  Color c;
  pb_get_value(*pblock, PB_EMIS, 0, c);
  return c;
}

float DagorMat::get_power()
{
  float f = 0;
  pb_get_value(*pblock, PB_SHIN, 0, f);
  return powf(2.0f, f / 10.0f) * 4.0f;
}

IDagorMat::Sides DagorMat::get_2sided() { return twosided; }

const TCHAR *DagorMat::get_classname() { return classname; }

const TCHAR *DagorMat::get_script() { return script; }

const TCHAR *DagorMat::get_texname(int i)
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

float DagorMat::get_param(int i) { return 0; }

void *DagorMat::GetInterface(ULONG id)
{
  if (id == I_DAGORMAT)
  {
    return (IDagorMat *)this;
  }
  else
    return Mtl::GetInterface(id);
}

void DagorMat::ReleaseInterface(ULONG id, void *i)
{
  if (id == I_DAGORMAT)
    ; // no-op
  else
    Mtl::ReleaseInterface(id, i);
}

void DagorMat::set_amb(Color c)
{
  cola = c;
  pblock->SetValue(PB_AMB, 0, c);
}

void DagorMat::set_diff(Color c)
{
  cold = c;
  pblock->SetValue(PB_DIFF, 0, c);
}

void DagorMat::set_spec(Color c)
{
  cols = c;
  pblock->SetValue(PB_SPEC, 0, c);
}

void DagorMat::set_emis(Color c)
{
  cole = c;
  pblock->SetValue(PB_EMIS, 0, c);
}

void DagorMat::set_power(float p)
{
  if (p <= 0.0f)
    p = 32.0f;
  power = p;
  shin = float(log(double(power) / 4.0) / log(2.0) * 10.0);
  pblock->SetValue(PB_SHIN, 0, shin);
}

void DagorMat::set_2sided(Sides b)
{
  twosided = b;
  NotifyChanged();
}

void DagorMat::set_classname(const TCHAR *s)
{
  if (s)
    classname = s;
  else
    classname = _T("");
  NotifyChanged();
}

void DagorMat::set_script(const TCHAR *s)
{
  if (s)
    script = s;
  else
    script = _T("");
  NotifyChanged();
}

void DagorMat::set_texname(int i, const TCHAR *s)
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

void DagorMat::set_param(int i, float p) {}
