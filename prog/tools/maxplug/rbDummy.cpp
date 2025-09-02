// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <simpobj.h>
#include <iparamm.h>
#include "dagor.h"
#include "resource.h"

// SimpleObject *SimpleObject::editOb=NULL;

class RBHelper : public SimpleObject
{
public:
  static RBHelper *editOb;
  static IParamMap *pmapParam;
  static IObjParam *ip;
  static float crtSize;

  RBHelper();

  void InvalidateUI() override;

  void set_axis_color(GraphicsWindow *gw, INode *, int axis);
  int Display(TimeValue t, INode *inode, ViewExp *vpt, int flags) override;
  int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;

  void DeleteThis() override { delete this; }
  SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
  int IsRenderable() override { return 0; }
  int UsesWireColor() override { return 1; }
  int DoOwnSelectHilite() override { return 1; }

  CreateMouseCallBack *GetCreateMouseCallBack() override;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *GetObjectName(bool localized = true) { return GetString(IDS_Dummy); }
#else
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  const
#endif
    MCHAR *
    GetObjectName()
  {
    return GetString(IDS_RBHelper);
  }
#endif
  RefTargetHandle Clone(RemapDir &remap) override;

  BOOL OKtoDisplay(TimeValue t) override;
  ParamDimension *GetParameterDim(int pbIndex) override;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  MSTR GetParameterName(int pbIndex, bool localized) override { return MSTR(_T("???")); }
#else
  TSTR GetParameterName(int pbIndex) override { return TSTR(_T("???")); }
#endif

  void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
  void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;

  void BuildMesh(TimeValue) override;
  void DrawIt(GraphicsWindow *gw, TimeValue t, INode *inode, int sel, int flags, ViewExp *vpt);
  void GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box) override;
  void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box) override;
  void GetDeformBBox(TimeValue t, Box3 &box, Matrix3 *tm = NULL, BOOL useSel = FALSE) override;

  Class_ID ClassID() override { return RBDummy_CID; }
};

RBHelper *RBHelper::editOb = NULL;
IObjParam *RBHelper::ip = NULL;
IParamMap *RBHelper::pmapParam = NULL;

void RBHelper::InvalidateUI()
{
  if (pmapParam)
    pmapParam->Invalidate();
}

void RBHelper::set_axis_color(GraphicsWindow *gw, INode *n, int a)
{
  if (n->IsFrozen())
  {
    gw->setColor(LINE_COLOR, 0.5, 0.5, 0.5);
    return;
  }
  if (n->Selected())
    gw->setColor(LINE_COLOR, 1, 1, 1);
  else
    gw->setColor(LINE_COLOR, 0.5, 1.0, 0.5);
}

static Matrix3 ident(1);

int RBHelper::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
  HitRegion hitRegion;
  GraphicsWindow *gw = vpt->getGW();
  MakeHitRegion(hitRegion, type, crossing, 4, p);
  UpdateMesh(t);

  DWORD rlim = gw->getRndLimits();

  gw->setTransform(ident);
  gw->setHitRegion(&hitRegion);
  gw->clearHitCode();
  gw->setRndLimits(rlim | GW_PICK);

  DrawIt(gw, t, inode, inode->Selected(), flags, vpt);
  int res = gw->checkHitCode();

  gw->setRndLimits(rlim);
  return res;
}

int RBHelper::Display(TimeValue t, INode *inode, ViewExp *vpt, int flags)
{
  UpdateMesh(t);
  GraphicsWindow *gw = vpt->getGW();
  DWORD rlim = gw->getRndLimits();

  //	gw->setRndLimits(GW_WIREFRAME|GW_BACKCULL| (rlim&GW_Z_BUFFER?GW_Z_BUFFER:0) );

  gw->setTransform(ident);
  DrawIt(gw, t, inode, inode->Selected(), flags, vpt);

  gw->setRndLimits(rlim);
  return 0;
}


enum
{
  PB_SIZE,
};

float RBHelper::crtSize = 20.0f;

class RBHelperClassDesc : public ClassDesc
{
public:
  int IsPublic() override { return TRUE; }
  void *Create(BOOL loading = FALSE) override { return new RBHelper(); }
  const TCHAR *ClassName() override { return GetString(IDS_RBHelper); }
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
#else
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
#endif
  SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
  Class_ID ClassID() override { return RBDummy_CID; }
  const TCHAR *Category() override { return GetString(IDS_DAGOR_CAT); }
};
static RBHelperClassDesc RBHelperCD;

ClassDesc *GetRBDummyCD() { return &RBHelperCD; }

static ParamUIDesc descParam[] = {ParamUIDesc(PB_SIZE, EDITTYPE_UNIVERSE, IDC_SIZ, IDC_SIZE_S, 0.0f, 9999999999.0f, SPIN_AUTOSCALE)};
#define PARAM_LEN 1

static ParamBlockDescID descVer0[] = {
  {TYPE_FLOAT, NULL, FALSE, PB_SIZE},
};
#define PBLOCK_LEN 1

#define CURRENT_VER 0
static ParamVersionDesc curVersion(descVer0, PBLOCK_LEN, CURRENT_VER);

RBHelper::RBHelper()
{
  MakeRefByID(FOREVER, 0, CreateParameterBlock(descVer0, PBLOCK_LEN, CURRENT_VER));
  assert(pblock);
  pblock->SetValue(PB_SIZE, 0, crtSize);
}

void RBHelper::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
  SimpleObject::BeginEditParams(ip, flags, prev);
  editOb = this;
  this->ip = ip;

  if (pmapParam)
  {
    pmapParam->SetParamBlock(pblock);
  }
  else
  {
    pmapParam = CreateCPParamMap(descParam, PARAM_LEN, pblock, ip, hInstance, MAKEINTRESOURCE(IDD_RBDUMMY_PARAM),
      GetString(IDS_RBDUMMY_ROLL), 0);
  }
}

void RBHelper::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
  SimpleObject::EndEditParams(ip, flags, next);
  editOb = NULL;
  this->ip = NULL;

  if (flags & END_EDIT_REMOVEUI)
  {
    DestroyCPParamMap(pmapParam);
    pmapParam = NULL;
  }

  pb_get_value(*pblock, PB_SIZE, ip->GetTime(), crtSize);
}

class RBHelperCreateCallBack : public CreateMouseCallBack
{
  IPoint2 sp1;
  RBHelper *ob;
  Point3 p0, p1;

public:
  int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat) override;
  void SetObj(RBHelper *obj) { ob = obj; }
};

int RBHelperCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat)
{
  if (msg == MOUSE_POINT || msg == MOUSE_MOVE)
  {
    switch (point)
    {
      case 0: // only happens with MOUSE_POINT msg
        ob->suspendSnap = TRUE;
        sp1 = m;
        p0 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
        mat.SetTrans(p0);
        ob->GetParamBlock()->SetValue(PB_SIZE, 0, 0.01f);
        ob->pmapParam->Invalidate();
        break;
      case 1:
        p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
        mat.SetRow(1, Point3(0, 0, 1));
        mat.SetRow(2, Normalize(p1 - p0));
        mat.SetRow(0, Point3(0, 0, 1) ^ mat.GetRow(2));
        mat.SetRow(3, p0);
        ob->GetParamBlock()->SetValue(PB_SIZE, 0, Length(p1 - p0));
        ob->pmapParam->Invalidate();
        if (msg == MOUSE_POINT)
        {
          ob->suspendSnap = FALSE;
          return (Length(m - sp1) < 3) ? CREATE_ABORT : CREATE_STOP;
        }
        break;
    }
  }
  else
  {
    if (msg == MOUSE_ABORT)
      return CREATE_ABORT;
  }
  return TRUE;
}

static RBHelperCreateCallBack RBHelperCreateCB;

CreateMouseCallBack *RBHelper::GetCreateMouseCallBack()
{
  RBHelperCreateCB.SetObj(this);
  return (&RBHelperCreateCB);
}

BOOL RBHelper::OKtoDisplay(TimeValue t)
{
  float r;
  pb_get_value(*pblock, PB_SIZE, t, r);
  if (r == 0.0f)
    return FALSE;
  else
    return TRUE;
}

ParamDimension *RBHelper::GetParameterDim(int pbIndex)
{
  switch (pbIndex)
  {
    case PB_SIZE: return stdWorldDim;
    default: return defaultDim;
  }
}

RefTargetHandle RBHelper::Clone(RemapDir &remap)
{
  RBHelper *newob = new RBHelper();
  newob->ReplaceReference(0, pblock->Clone(remap));
  newob->ivalid.SetEmpty();
#if MAX_RELEASE >= 4000
  BaseClone(this, newob, remap);
#endif
  return (newob);
}

void RBHelper::BuildMesh(TimeValue tm)
{
  ivalid.SetInfinite();
  mesh.setNumTVerts(0);
  mesh.setNumTVFaces(0);
  mesh.setNumFaces(0);
  mesh.setNumVerts(0);
}

void RBHelper::DrawIt(GraphicsWindow *gw, TimeValue t, INode *inode, int sel, int flags, ViewExp *vpt)
{
  DWORD rlim = gw->getRndLimits();
  gw->setRndLimits((rlim | GW_WIREFRAME) & ~(GW_ILLUM | GW_BACKCULL | GW_FLAT | GW_SPECULAR));
  Matrix3 tm = inode->GetNodeTM(t);
  Point3 vx, vy, vz, p0, p[9];
  float sz;
  pb_get_value(*pblock, PB_SIZE, t, sz);
  vx = tm.GetRow(0) * sz;
  vy = tm.GetRow(1) * sz;
  vz = tm.GetRow(2) * sz;
  p0 = tm.GetRow(3);

  set_axis_color(gw, inode, 0);
  p[0] = p0 - vx;
  p[1] = p0 + vx;
  p[2] = p0 + vx * 0.5f - vy * 0.25f;
  p[3] = p0 + vx * 0.5f + vy * 0.25f;
  p[4] = p[1];
  p[5] = p0 + vx * 0.5f - vz * 0.25f;
  p[6] = p0 + vx * 0.5f + vz * 0.25f;
  p[7] = p[1];
  gw->polyline(8, p, NULL, NULL, FALSE, NULL);

  set_axis_color(gw, inode, 1);
  p[0] = p0 - vy;
  p[1] = p0 + vy;
  p[2] = p0 + vy * 0.5f - vx * 0.25f;
  p[3] = p0 + vy * 0.5f + vx * 0.25f;
  p[4] = p[1];
  p[5] = p0 + vy * 0.5f - vz * 0.25f;
  p[6] = p0 + vy * 0.5f + vz * 0.25f;
  p[7] = p[1];
  gw->polyline(8, p, NULL, NULL, FALSE, NULL);

  set_axis_color(gw, inode, 2);
  p[0] = p0 - vz;
  p[1] = p0 + vz;
  p[2] = p0 + vz * 0.5f - vx * 0.25f;
  p[3] = p0 + vz * 0.5f + vx * 0.25f;
  p[4] = p[1];
  p[5] = p0 + vz * 0.5f - vy * 0.25f;
  p[6] = p0 + vz * 0.5f + vy * 0.25f;
  p[7] = p[1];
  gw->polyline(8, p, NULL, NULL, FALSE, NULL);

  gw->setRndLimits(rlim);
}

void RBHelper::GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
  UpdateMesh(t);
  Matrix3 mat = Inverse(inode->GetNodeTM(t));
  float sz;
  pb_get_value(*pblock, PB_SIZE, t, sz);
  box.MakeCube(Point3(0, 0, 0), sz * 2);
}

void RBHelper::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
  UpdateMesh(t);
  Matrix3 mat = inode->GetNodeTM(t);
  GetLocalBoundBox(t, inode, vpt, box);
  box = box * mat;
}

void RBHelper::GetDeformBBox(TimeValue t, Box3 &box, Matrix3 *tm, BOOL useSel)
{
  UpdateMesh(t);
  float sz;
  pb_get_value(*pblock, PB_SIZE, t, sz);
  box.MakeCube(Point3(0, 0, 0), sz * 2);
  if (tm)
    box = box * (*tm);
}
