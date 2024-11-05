// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <bmmlib.h>
#include <fltlib.h>
#include <pixelbuf.h>
#include "dagor.h"

class VPnorm : public ImageFilter
{
public:
  const TCHAR *Description() { return _T("Normals to RGB"); }
  const TCHAR *AuthorName() { return _T("Aleksey Volynskov"); }
  const TCHAR *CopyrightMessage() { return _T("(c) Aleksey Volynskov, 2001"); }
  UINT Version() { return 1; }
  DWORD Capability() { return IMGFLT_FILTER; }
  void ShowAbout(HWND hw)
  {
    MessageBox(hw,
      _T( "Normals to RGB convertor\n" )
      _T( "(c) Aleksey Volynskov, 2001" ),
      _T( "About..."), MB_OK | MB_ICONINFORMATION);
  }
  BOOL Render(HWND hw);
  DWORD ChannelsRequired() { return BMM_CHAN_NORMAL; }
};

class VPzbufa : public ImageFilter
{
public:
  const TCHAR *Description() { return _T("Z to Alpha"); }
  const TCHAR *AuthorName() { return _T("Aleksey Volynskov"); }
  const TCHAR *CopyrightMessage() { return _T("(c) Aleksey Volynskov, 2001"); }
  UINT Version() { return 1; }
  DWORD Capability() { return IMGFLT_FILTER; }
  void ShowAbout(HWND hw)
  {
    MessageBox(hw,
      _T( "Z to Alpha convertor\n")
      _T( "(c) Aleksey Volynskov, 2001"),
      _T( "About..."), MB_OK | MB_ICONINFORMATION);
  }
  BOOL Render(HWND hw);
  DWORD ChannelsRequired() { return BMM_CHAN_Z; }
};

static inline int lim16(int a)
{
  if (a < 0)
    return 0;
  if (a > 0xFFFF)
    return 0xFFFF;
  return a;
}

BOOL VPnorm::Render(HWND hw)
{
  DWORD type;
  DWORD *cnorm = (DWORD *)srcmap->GetChannel(BMM_CHAN_NORMAL, type);
  if (!cnorm)
    return FALSE;
  int wd = srcmap->Width(), ht = srcmap->Height();
  PixelBuf64 pbuf(wd);
  for (int y = 0; y < ht; ++y)
  {
    if (!srcmap->GetPixels(0, y, wd, pbuf.Ptr()))
      return FALSE;
    for (int x = 0; x < wd; ++x, ++cnorm)
    {
      Point3 n = Normalize(DeCompressNormal(*cnorm));
      pbuf[x].r = lim16((n.x + 1) * (0x8000 - .5f));
      pbuf[x].g = lim16((n.y + 1) * (0x8000 - .5f));
      pbuf[x].b = lim16((n.z + 1) * (0x8000 - .5f));
    }
    if (!srcmap->PutPixels(0, y, wd, pbuf.Ptr()))
      return FALSE;
  }
  return TRUE;
}

static int get_zminmax(INode *n, float &z0, float &z1)
{
  if (!n)
    return 0;
  if (n->GetUserPropFloat(_T("zlow"), z0))
  {
    if (n->GetUserPropFloat(_T("zhigh"), z1))
      return 1;
  }
  int cn = n->NumChildren();
  for (int i = 0; i < cn; ++i)
    if (get_zminmax(n->GetChildNode(i), z0, z1))
      return 1;
  return 0;
}

BOOL VPzbufa::Render(HWND hw)
{
  float z0, z1;
  if (!get_zminmax(Max()->GetRootNode(), z0, z1))
  {
    z1 = 0;
    z0 = -500;
  }
  float zk = 1 / (z1 - z0);
  DWORD type;
  float *zbuf = (float *)srcmap->GetChannel(BMM_CHAN_Z, type);
  if (!zbuf)
    return FALSE;
  int wd = srcmap->Width(), ht = srcmap->Height();
  PixelBuf64 pbuf(wd);
  for (int y = 0; y < ht; ++y)
  {
    if (!srcmap->GetPixels(0, y, wd, pbuf.Ptr()))
      return FALSE;
    for (int x = 0; x < wd; ++x, ++zbuf)
    {
      float h = (*zbuf - z0) * zk;
      if (h < 0)
        h = 0;
      else if (h > 1)
        h = 1;
      pbuf[x].a = lim16(h * 0xFFFF);
    }
    if (!srcmap->PutPixels(0, y, wd, pbuf.Ptr()))
      return FALSE;
  }
  return TRUE;
}

class VPnormClassDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading) { return new VPnorm; }
  const TCHAR *ClassName() { return _T("VPnorm2rgb"); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return FLT_CLASS_ID; }
  Class_ID ClassID() { return VPNORM_CID; }
  const TCHAR *Category() { return _T(""); }
};

static VPnormClassDesc VPnormCD;
ClassDesc *GetVPnormCD() { return &VPnormCD; }

class VPzbufaClassDesc : public ClassDesc
{
public:
  int IsPublic() { return 1; }
  void *Create(BOOL loading) { return new VPzbufa; }
  const TCHAR *ClassName() { return _T("VPzbuf2a"); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return FLT_CLASS_ID; }
  Class_ID ClassID() { return VPZBUF_CID; }
  const TCHAR *Category() { return _T(""); }
};

static VPzbufaClassDesc VPzbufaCD;
ClassDesc *GetVPzbufCD() { return &VPzbufaCD; }
