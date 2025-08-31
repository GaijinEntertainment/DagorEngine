// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <utilapi.h>
#include <locale.h>
#include "dagor.h"
#include "enumnode.h"
#include "resource.h"
#include <string>

#define ERRMSG_DELAY 3000

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

int get_save_filename(HWND owner, const TCHAR *title, FilterList &filter, const TCHAR *def_ext, TSTR &exp_fname);

class FontUtil : public UtilityObj
{
public:
  IUtil *iu;
  Interface *ip;
  HWND hPanel;
  TSTR fd_fname;

  FontUtil();
  void BeginEditParams(Interface *ip, IUtil *iu) override;
  void EndEditParams(Interface *ip, IUtil *iu) override;
  void DeleteThis() override {}

  void Init(HWND hWnd);
  void Destroy(HWND hWnd);
  int input_fd_fname();
  void exportData();
};
static FontUtil util;

class FontUtilDesc : public ClassDesc
{
public:
  int IsPublic() override { return 1; }
  void *Create(BOOL loading = FALSE) override { return &util; }
  const TCHAR *ClassName() override { return GetString(IDS_FONTUTIL_NAME); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
  Class_ID ClassID() override { return FontUtil_CID; }
  const TCHAR *Category() override { return GetString(IDS_UTIL_CAT); }
  BOOL NeedsToSave() override { return TRUE; }
  IOResult Save(ISave *) override;
  IOResult Load(ILoad *) override;
};

enum
{
  CH_FD_FNAME = 1,
};

IOResult FontUtilDesc::Save(ISave *io)
{
  //        ULONG nw;
  if (util.fd_fname.Length())
  {
    io->BeginChunk(CH_FD_FNAME);
    if (io->WriteCString(util.fd_fname) != IO_OK)
      return IO_ERROR;
    io->EndChunk();
  }
  return IO_OK;
}

IOResult FontUtilDesc::Load(ILoad *io)
{
  //        ULONG nr;
  TCHAR *str;
  while (io->OpenChunk() == IO_OK)
  {
    switch (io->CurChunkID())
    {
      case CH_FD_FNAME:
        if (io->ReadCStringChunk(&str) != IO_OK)
          return IO_ERROR;
        util.fd_fname = str;
        break;
    }
    io->CloseChunk();
  }
  return IO_OK;
}

static FontUtilDesc utilDesc;
ClassDesc *GetFontUtilCD() { return &utilDesc; }

static INT_PTR CALLBACK FontUtilDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG: util.Init(hWnd); break;

    case WM_DESTROY: util.Destroy(hWnd); break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_EXPORT:
        {
          if (!util.input_fd_fname())
            break;
          util.exportData();
        }
        break;
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

FontUtil::FontUtil()
{
  iu = NULL;
  ip = NULL;
  hPanel = NULL;
}

void FontUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
  this->iu = iu;
  this->ip = ip;
  hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_FONTUTIL), FontUtilDlgProc, GetString(IDS_FONTUTIL_ROLL), 0);
}

void FontUtil::EndEditParams(Interface *ip, IUtil *iu)
{
  this->iu = NULL;
  this->ip = NULL;
  if (hPanel)
    ip->DeleteRollupPage(hPanel);
  hPanel = NULL;
}

void FontUtil::Init(HWND hWnd) {}

void FontUtil::Destroy(HWND hWnd) {}

int FontUtil::input_fd_fname()
{
  FilterList fl;
  fl.Append(GetString(IDS_FONTDATA_FILES));
  fl.Append(_T("*.fd"));
  return get_save_filename(hPanel, GetString(IDS_SAVE_FONTDATA_TITLE), fl, _T("fd"), fd_fname);
}

struct VDiv
{
  float u, v;
  int d;
};
struct Chars
{
  float v;
  TCHAR *s;
};

static int float_cmp(const void *aa, const void *bb)
{
  float a = *(float *)aa, b = *(float *)bb;
  if (a < b)
    return -1;
  if (a > b)
    return +1;
  return 0;
}

static int vdiv_cmp(const void *aa, const void *bb)
{
  VDiv *a = (VDiv *)aa, *b = (VDiv *)bb;
  if (a->d != b->d)
    return a->d - b->d;
  if (a->u < b->u)
    return -1;
  if (a->u > b->u)
    return +1;
  return 0;
}

static int chars_cmp(const void *aa, const void *bb)
{
  Chars *a = (Chars *)aa, *b = (Chars *)bb;
  if (a->v < b->v)
    return -1;
  if (a->v > b->v)
    return +1;
  return 0;
}

class FontENCB : public ENodeCB
{
public:
  Tab<float> hdiv;
  Tab<float> base;
  Tab<VDiv> vdiv;
  Tab<Chars> chars;
  Matrix3 org;
  TimeValue time;

  FontENCB(TimeValue t) { time = t; }
  ~FontENCB() override
  {
    for (int i = 0; i < chars.Count(); ++i)
      if (chars[i].s)
        free(chars[i].s);
  }
  void getuv(INode *n, float *u, float *v)
  {
    Matrix3 m = n->GetNodeTM(time);
    Point3 p = org * m.GetTrans();
    if (u)
      *u = p.x;
    if (v)
      *v = p.z;
  }
  int proc(INode *n) override
  {
    if (!n)
      return ECB_CONT;
    if (n->IsNodeHidden())
      return ECB_CONT;
    if (n->GetVisibility(time) < 0)
      return ECB_CONT;
    const TCHAR *nm = n->GetName();
    if (_tcsnicmp(nm, _T("hdiv"), 4) == 0)
    {
      float v;
      getuv(n, NULL, &v);
      hdiv.Append(1, &v);
    }
    else if (_tcsnicmp(nm, _T("base"), 4) == 0)
    {
      float v;
      getuv(n, NULL, &v);
      base.Append(1, &v);
    }
    else if (_tcsnicmp(nm, _T("vdiv"), 4) == 0)
    {
      VDiv d;
      getuv(n, &d.u, &d.v);
      vdiv.Append(1, &d);
    }
    else if (_tcsnicmp(nm, _T("chars"), 4) == 0)
    {
      Chars c;
      getuv(n, NULL, &c.v);
      TSTR s;
      n->GetUserPropBuffer(s);
      c.s = _tcsdup(s);
      assert(c.s);
      chars.Append(1, &c);
    }
    return ECB_CONT;
  }
  int check(Interface *ip)
  {
    if (hdiv.Count() != base.Count() || hdiv.Count() != chars.Count())
    {
      ip->DisplayTempPrompt(GetString(IDS_FONTERR_HDNEBASE), ERRMSG_DELAY);
      return 0;
    }
    if (hdiv.Count() <= 0)
    {
      ip->DisplayTempPrompt(GetString(IDS_FONTERR_NOHDIV), ERRMSG_DELAY);
      return 0;
    }
    hdiv.Sort(float_cmp);
    base.Sort(float_cmp);
    chars.Sort(chars_cmp);
    int i;
    for (i = 0; i < vdiv.Count(); ++i)
    {
      if (vdiv[i].u < 0 || vdiv[i].u > 256)
      {
        ip->DisplayTempPrompt(GetString(IDS_FONTERR_VDOUTU), ERRMSG_DELAY);
        return 0;
      }
      float v = vdiv[i].v;
      if (v < 0)
      {
        ip->DisplayTempPrompt(GetString(IDS_FONTERR_VDOUTV), ERRMSG_DELAY);
        return 0;
      }
      int j;
      for (j = 0; j < hdiv.Count(); ++j)
        if (v < hdiv[j])
          break;
      if (j >= hdiv.Count())
      {
        ip->DisplayTempPrompt(GetString(IDS_FONTERR_VDOUTV), ERRMSG_DELAY);
        return 0;
      }
      vdiv[i].d = j;
    }
    vdiv.Sort(vdiv_cmp);
    for (i = 0; i < hdiv.Count(); ++i)
    {
      int n = 0;
      for (int j = 0; j < vdiv.Count(); ++j)
        if (vdiv[j].d == i)
          ++n;
      if (!(n & 1))
      {
        ip->DisplayTempPrompt(GetString(IDS_FONTERR_VDEVEN), ERRMSG_DELAY);
        return 0;
      }
    }
    return 1;
  }
  int save(const TCHAR *fn, Interface *ip)
  {
    FILE *h = _tfopen(fn, _T("wt"));
    if (!h)
    {
      ip->DisplayTempPrompt(GetString(IDS_FILE_CREATE_ERR), ERRMSG_DELAY);
      return 0;
    }
    setlocale(LC_NUMERIC, "C");
    for (int i = 0; i < hdiv.Count(); ++i)
    {
      fprintf(h, "\nhdiv %g %g \"", hdiv[i], base[i]);
      for (TCHAR *s = chars[i].s; *s; ++s)
      {
        if (*s == '\"')
          fprintf(h, "~\"");
        else if (*s == '~')
          fprintf(h, "~~");
        else
        {
          std::string s1 = wideToStr(s);
          fputc(*s1.c_str(), h);
        }
      }
      fprintf(h, "\" {\n");
      char f = 1;
      for (int j = 0; j < vdiv.Count(); ++j)
        if (vdiv[j].d == i)
        {
          if (f)
            fprintf(h, "%g\n", vdiv[j].u);
          else
            fprintf(h, "%g ", vdiv[j].u);
          f = !f;
        }
      fprintf(h, "}\n");
    }
    setlocale(LC_NUMERIC, "");
    fclose(h);
    return 1;
  }
};

void FontUtil::exportData()
{
  FontENCB cb(ip->GetTime());
  cb.org.IdentityMatrix();
  INode *on = ip->GetINodeByName(_T("origin"));
  if (on)
  {
    cb.org = on->GetNodeTM(ip->GetTime());
    cb.org.NoScale();
  }
  enum_nodes(ip->GetRootNode(), &cb);
  if (!cb.check(ip))
    return;
  if (!cb.save(fd_fname.data(), ip))
    return;
}
