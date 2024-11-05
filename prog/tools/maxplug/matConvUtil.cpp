// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <stdmat.h>
#include <utilapi.h>
#include <locale.h>
#include "dagor.h"
#include "mater.h"
#include "resource.h"

#include <string>

#define ERRMSG_DELAY 3000

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);


enum
{
  MAT_SIMPLE,
  MAT_ABLEND,
  MAT_AUTO,
};


enum
{
  LIGHT_AUTO,
  LIGHT_NONE,
  LIGHT_LIGHTMAP,
  LIGHT_VLTMAP
};


struct MatPair
{
  Mtl *oldMat;
  Mtl *newMat;
};


class MatConvUtil : public UtilityObj
{
public:
  IUtil *iu;
  Interface *ip;
  HWND hPanel;
  Tab<MatPair> convMats;

  ICustEdit *atestEdit;

  bool twoSided, realTwoSided;
  int atest;


  MatConvUtil();
  void BeginEditParams(Interface *ip, IUtil *iu);
  void EndEditParams(Interface *ip, IUtil *iu);
  void DeleteThis() {}

  void Init(HWND hWnd);
  void Destroy(HWND hWnd);

  bool convertMat(Mtl *&mat, int type, int lighting, const TCHAR *slotName);
  void convertNodes(int type, int lighting);
};

static MatConvUtil util;


class MatConvUtilDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading = FALSE) { return &util; }
  const TCHAR *ClassName() { return GetString(IDS_MatConvUtil_NAME); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return UTILITY_CLASS_ID; }
  Class_ID ClassID() { return MatConvUtil_CID; }
  const TCHAR *Category() { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() { return TRUE; }
  IOResult Save(ISave *);
  IOResult Load(ILoad *);
};

IOResult MatConvUtilDesc::Save(ISave *io) { return IO_OK; }

IOResult MatConvUtilDesc::Load(ILoad *io) { return IO_OK; }

static MatConvUtilDesc utilDesc;
ClassDesc *GetMatConvUtilCD() { return &utilDesc; }

static INT_PTR CALLBACK MatConvUtilDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG: util.Init(hWnd); break;

    case WM_DESTROY: util.Destroy(hWnd); break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_AUTO: util.convertNodes(MAT_AUTO, LIGHT_AUTO); break;
        case IDC_SIMPLE_VLTMAP: util.convertNodes(MAT_SIMPLE, LIGHT_VLTMAP); break;
        case IDC_SIMPLE_LIGHTMAP: util.convertNodes(MAT_SIMPLE, LIGHT_LIGHTMAP); break;
        case IDC_ABLEND_VLTMAP: util.convertNodes(MAT_ABLEND, LIGHT_VLTMAP); break;
        case IDC_ABLEND_LIGHTMAP: util.convertNodes(MAT_ABLEND, LIGHT_LIGHTMAP); break;
        case IDC_2SIDED: util.twoSided = IsDlgButtonChecked(hWnd, LOWORD(wParam)) ? true : false; break;
        case IDC_REAL2SIDED: util.realTwoSided = IsDlgButtonChecked(hWnd, LOWORD(wParam)) ? true : false; break;
      }
      break;
      /*
                      case WM_LBUTTONDOWN:
                      case WM_LBUTTONUP:
                      case WM_MOUSEMOVE:
                              util.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
                              break;
      */
    default: return FALSE;
  }
  return TRUE;
}

MatConvUtil::MatConvUtil()
{
  iu = NULL;
  ip = NULL;
  hPanel = NULL;
  atestEdit = NULL;

  twoSided = false;
  realTwoSided = false;
  atest = 0;
}

void MatConvUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_MatConvUtil), MatConvUtilDlgProc, GetString(IDS_MatConvUtil_ROLL), 0);
}

void MatConvUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  if (hPanel)
    ip->DeleteRollupPage(hPanel);
  hPanel = NULL;
}

void MatConvUtil::Init(HWND hw)
{
  CheckDlgButton(hw, IDC_2SIDED, twoSided);
  CheckDlgButton(hw, IDC_REAL2SIDED, realTwoSided);
  atestEdit = GetICustEdit(GetDlgItem(hw, IDC_ATEST));
  atestEdit->SetText(atest);
}

void MatConvUtil::Destroy(HWND hWnd)
{
  if (atestEdit)
    atest = atestEdit->GetInt();
  if (atestEdit)
  {
    ReleaseICustEdit(atestEdit);
    atestEdit = NULL;
  }
}


bool MatConvUtil::convertMat(Mtl *&mat, int type, int lighting, const TCHAR *slotName)
{
  for (int i = 0; i < convMats.Count(); ++i)
    if (convMats[i].oldMat == mat)
    {
      mat = convMats[i].newMat;
      return true;
    }

  IDagorMat *srcDm = (IDagorMat *)mat->GetInterface(I_DAGORMAT);

  Class_ID cid = mat->ClassID();
  if (cid == Class_ID(DMTL_CLASS_ID, 0) || srcDm)
  {
    TimeValue time = ip->GetTime();

    Color amb, diff, spec, emis;
    float power;

    Texmap *tex = mat->GetSubTexmap(srcDm ? 0 : ID_DI);

    if (cid == Class_ID(DMTL_CLASS_ID, 0))
    {
      StdMat *sm = (StdMat *)mat;

      float si = sm->GetSelfIllum(time);
      amb = sm->GetAmbient(time) * (1 - si);

      Color smdiff = sm->GetDiffuse(time);
      diff = smdiff * (1 - si);
      spec = sm->GetSpecular(time) * sm->GetShinStr(time);
      emis = smdiff * si;
      power = powf(2.0f, sm->GetShininess(time) * 10.0f) * 4.0f;

      if (tex)
      {
        Color c = Color(1, 1, 1) * (1 - si);
        amb = c;
        diff = c;
        emis = Color(1, 1, 1) * si;
      }
    }
    else
    {
      assert(srcDm);

      amb = srcDm->get_amb();
      diff = srcDm->get_diff();
      spec = srcDm->get_spec();
      emis = srcDm->get_emis();
      power = srcDm->get_power();

      mat->ReleaseInterface(I_DAGORMAT, srcDm);
    }

    Mtl *dm = (Mtl *)ip->CreateInstance(MATERIAL_CLASS_ID, DagorMat2_CID);
    assert(dm);
    IDagorMat *d = (IDagorMat *)dm->GetInterface(I_DAGORMAT);
    assert(d);

    dm->SetName(mat->GetName());
    d->set_2sided(twoSided || realTwoSided); // sm->GetTwoSided()

    d->set_amb(amb);
    d->set_diff(diff);
    d->set_spec(spec);
    d->set_emis(emis);
    d->set_power(power);

    if (tex)
    {
      dm->SetSubTexmap(0, tex);
    }

    char script[1024];
    script[0] = 0;

    std::string s = wideToStr(mat->GetName());

    switch (lighting)
    {
      case LIGHT_AUTO:
        if (strstr(s.c_str(), "lightmap") || _tcsstr(slotName, _T("lightmap" )))
          strcat(script, "lighting=lightmap\r\n");
        else if (strstr(s.c_str(), "vertex") || strstr(s.c_str(), "vltmap") || _tcsstr(slotName, _T("vertex")) ||
                 _tcsstr(slotName, _T("vltmap")))
          strcat(script, "lighting=vltmap\r\n");
        else
          strcat(script, "lighting=vltmap\r\n");
        break;
      case LIGHT_NONE: strcat(script, "lighting=none\r\n"); break;
      case LIGHT_LIGHTMAP: strcat(script, "lighting=lightmap\r\n"); break;
      case LIGHT_VLTMAP: strcat(script, "lighting=vltmap\r\n"); break;
      default: assert(0);
    }

    int atestVal = atest;

    switch (type)
    {
      case MAT_AUTO:
        if (strstr(s.c_str(), "alpha") || _tcsstr(slotName, _T("alpha")))
          d->set_classname(_T("alpha_blend"));
        else
          d->set_classname(_T("simple"));
        break;
      case MAT_SIMPLE: d->set_classname(_T("simple")); break;
      case MAT_ABLEND:
        d->set_classname(_T("alpha_blend"));
        if (atestVal < 1)
          atestVal = 1;
        break;
      default: assert(0);
    }

    if (atestVal == 0)
    {
      if (strstr(s.c_str(), "atest") || _tcsstr(slotName, _T("atest")))
        atestVal = 128;
      else if (strstr(s.c_str(), "alpha") || _tcsstr(slotName, _T("alpha")))
        atestVal = 1;
    }

    if (atestVal > 0)
      sprintf(script + strlen(script), "atest=%d\r\n", atestVal);

    if (realTwoSided)
      strcat(script, "real_two_sided=yes\r\n");

    d->set_script(strToWide(script).c_str());

    dm->ReleaseInterface(I_DAGORMAT, d);

    MatPair mp;
    mp.oldMat = mat;
    mp.newMat = dm;
    convMats.Append(1, &mp);

    mat = dm;
    return true;
  }
  return false;
}


void MatConvUtil::convertNodes(int type, int lighting)
{
  if (atestEdit)
    atest = atestEdit->GetInt();

  convMats.ZeroCount();

  int nodeNum = ip->GetSelNodeCount();
  int convertedNum = 0;
  Tab<Mtl *> converted;

  for (int nodeId = 0; nodeId < nodeNum; ++nodeId)
  {
    INode *node = ip->GetSelNode(nodeId);
    if (!node)
      continue;
    Mtl *mat = node->GetMtl();
    if (!mat)
      continue;
    int mi;
    for (mi = 0; mi < converted.Count(); ++mi)
      if (converted[mi] == mat)
        break;
    if (mi < converted.Count())
      continue;
    bool changed = false;

    if (mat->IsMultiMtl())
    {
      int num = mat->NumSubMtls();
      for (int i = 0; i < num; ++i)
      {
        Mtl *sm = mat->GetSubMtl(i);
#if defined(MAX_RELEASE_R27) && MAX_RELEASE >= MAX_RELEASE_R27
        if (sm && convertMat(sm, type, lighting, mat->GetSubMtlSlotName(i, false).data()))
#else
        if (sm && convertMat(sm, type, lighting, mat->GetSubMtlSlotName(i)))
#endif
        {
          mat->SetSubMtl(i, sm);
          changed = true;
        }
      }
      converted.Append(1, &mat);
    }
    else if (convertMat(mat, type, lighting, _T("")))
    {
      node->SetMtl(mat);
      changed = true;
    }


    if (changed)
    {
      node->InvalidateWS();
      ++convertedNum;
    }
  }

  ip->RedrawViews(ip->GetTime());

  TCHAR str[128];
  _stprintf(str, _T("Converted materials on %d/%d nodes"), convertedNum, nodeNum);
  ip->DisplayTempPrompt(str, 3000);

  convMats.ZeroCount();
}
