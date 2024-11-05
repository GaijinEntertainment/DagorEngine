// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <simpobj.h>
#include <iparamm.h>
#include "dagor.h"
#include "resource.h"

// SimpleObject *SimpleObject::editOb=NULL;

class LinkDummy : public SimpleObject
{
public:
  static LinkDummy *editOb;
  static IParamMap *pmapParam;
  static IObjParam *ip;
  static float crtSize;

  LinkDummy();

  void InvalidateUI();

  //	virtual void DrawIt(GraphicsWindow *gw,TimeValue t,INode *inode,int sel,int flags, ViewExp *vpt)=0;
  virtual void set_axis_color(GraphicsWindow *gw, INode *, int axis);
  int Display(TimeValue t, INode *inode, ViewExp *vpt, int flags);
  int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);

  void DeleteThis() { delete this; }
  SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
  int IsRenderable() { return 0; }
  int UsesWireColor() { return 1; }
  int DoOwnSelectHilite() { return 1; }

  CreateMouseCallBack *GetCreateMouseCallBack();
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  const MCHAR *GetObjectName(bool localized = true) { return GetString(IDS_Dummy); }
#else
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  const
#endif
    MCHAR *
    GetObjectName()
  {
    return GetString(IDS_Dummy);
  }
#endif
  RefTargetHandle Clone(RemapDir &remap = NoRemap());

  //	IOResult Load(ILoad *iload);
  //	IOResult Save(ISave *iload);

  BOOL OKtoDisplay(TimeValue t);
  ParamDimension *GetParameterDim(int pbIndex);
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  MSTR GetParameterName(int pbIndex, bool localized) { return MSTR(_T("???")); }
#else
  TSTR GetParameterName(int pbIndex) { return TSTR(_T("???")); }
#endif

  void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
  void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

  void BuildMesh(TimeValue);
  void DrawIt(GraphicsWindow *gw, TimeValue t, INode *inode, int sel, int flags, ViewExp *vpt);
  void GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box);
  void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box);
  void GetDeformBBox(TimeValue t, Box3 &box, Matrix3 *tm = NULL, BOOL useSel = FALSE);

  Class_ID ClassID() { return Dummy_CID; }
};

LinkDummy *LinkDummy::editOb = NULL;
IObjParam *LinkDummy::ip = NULL;
IParamMap *LinkDummy::pmapParam = NULL;

void LinkDummy::InvalidateUI()
{
  if (pmapParam)
    pmapParam->Invalidate();
}

void LinkDummy::set_axis_color(GraphicsWindow *gw, INode *n, int a)
{
  if (n->IsFrozen())
  {
    gw->setColor(LINE_COLOR, 0.5, 0.5, 0.5);
    return;
  }
  if (n->Selected())
    gw->setColor(LINE_COLOR, (a == 0) ? 1 : 0.5f, (a == 1) ? 1 : 0.5f, (a == 2) ? 1 : 0.5f);
  else
    gw->setColor(LINE_COLOR, (a == 0) ? 0.7f : 0, (a == 1) ? 0.7f : 0, (a == 2) ? 0.7f : 0);
}

static Matrix3 ident(1);

int LinkDummy::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
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

int LinkDummy::Display(TimeValue t, INode *inode, ViewExp *vpt, int flags)
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

float LinkDummy::crtSize = 20.0f;

class LinkDummyClassDesc : public ClassDesc
{
public:
  int IsPublic() { return TRUE; }
  void *Create(BOOL loading = FALSE) { return new LinkDummy(); }
  const TCHAR *ClassName() { return GetString(IDS_Dummy); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
  Class_ID ClassID() { return Dummy_CID; }
  const TCHAR *Category() { return GetString(IDS_DAGOR_CAT); }
};
static LinkDummyClassDesc LinkDummyCD;

ClassDesc *GetDummyCD() { return &LinkDummyCD; }

static ParamUIDesc descParam[] = {ParamUIDesc(PB_SIZE, EDITTYPE_UNIVERSE, IDC_SIZ, IDC_SIZE_S, 0.0f, 9999999999.0f, SPIN_AUTOSCALE)};
#define PARAM_LEN 1

static ParamBlockDescID descVer0[] = {
  {TYPE_FLOAT, NULL, FALSE, PB_SIZE},
};
#define PBLOCK_LEN 1

#define CURRENT_VER 0
static ParamVersionDesc curVersion(descVer0, PBLOCK_LEN, CURRENT_VER);

LinkDummy::LinkDummy()
{
  MakeRefByID(FOREVER, 0, CreateParameterBlock(descVer0, PBLOCK_LEN, CURRENT_VER));
  assert(pblock);
  pblock->SetValue(PB_SIZE, 0, crtSize);
}

/*
IOResult LinkDummy::Load(ILoad *io) {
  io->RegisterPostLoadCallback(
    new ParamBlockPLCB(NULL,0,&curVersion,this,0));
  char *str;
  while(io->OpenChunk()==IO_OK) {
    switch(io->CurChunkID()) {
      case CH_NAME:
        if(io->ReadCStringChunk(&str)!=IO_OK) return IO_ERROR;
        memset(name,0,sizeof(name));
        strncpy(name,str,sizeof(name)-1);
        break;
    }
    io->CloseChunk();
  }
  if(editOb==this) if(edname) edname->SetText(name);
  return IO_OK;
}

IOResult LinkDummy::Save(ISave *io) {
  if(editOb==this) if(edname) {
    edname->GetText(name,sizeof(name));
    ivalid=NEVER;
  }
  io->BeginChunk(CH_NAME);
  if(io->WriteCString(name)!=IO_OK) return IO_ERROR;
  io->EndChunk();
  return IO_OK;
}
*/

void LinkDummy::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
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
    pmapParam =
      CreateCPParamMap(descParam, PARAM_LEN, pblock, ip, hInstance, MAKEINTRESOURCE(IDD_DUMMY_PARAM), GetString(IDS_DUMMY_ROLL), 0);
  }
}

void LinkDummy::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
  SimpleObject::EndEditParams(ip, flags, next);
  editOb = NULL;
  this->ip = NULL;

  if (flags & END_EDIT_REMOVEUI)
  {
    DestroyCPParamMap(pmapParam);
    pmapParam = NULL;
  }

  pblock->GetValue(PB_SIZE, ip->GetTime(), crtSize, FOREVER);
}

class LinkDummyCreateCallBack : public CreateMouseCallBack
{
  IPoint2 sp1;
  LinkDummy *ob;
  Point3 p0, p1;

public:
  int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat);
  void SetObj(LinkDummy *obj) { ob = obj; }
};

int LinkDummyCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat)
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

static LinkDummyCreateCallBack LinkDummyCreateCB;

CreateMouseCallBack *LinkDummy::GetCreateMouseCallBack()
{
  LinkDummyCreateCB.SetObj(this);
  return (&LinkDummyCreateCB);
}

BOOL LinkDummy::OKtoDisplay(TimeValue t)
{
  float r;
  pblock->GetValue(PB_SIZE, t, r, FOREVER);
  if (r == 0.0f)
    return FALSE;
  else
    return TRUE;
}

ParamDimension *LinkDummy::GetParameterDim(int pbIndex)
{
  switch (pbIndex)
  {
    case PB_SIZE: return stdWorldDim;
    default: return defaultDim;
  }
}

RefTargetHandle LinkDummy::Clone(RemapDir &remap)
{
  LinkDummy *newob = new LinkDummy();
  newob->ReplaceReference(0, pblock->Clone(remap));
  newob->ivalid.SetEmpty();
#if MAX_RELEASE >= 4000
  BaseClone(this, newob, remap);
#endif
  return (newob);
}

void LinkDummy::BuildMesh(TimeValue tm)
{
  ivalid.SetInfinite();
  mesh.setNumTVerts(0);
  mesh.setNumTVFaces(0);
  mesh.setNumFaces(0);
  mesh.setNumVerts(0);
}

void LinkDummy::DrawIt(GraphicsWindow *gw, TimeValue t, INode *inode, int sel, int flags, ViewExp *vpt)
{
  DWORD rlim = gw->getRndLimits();
  gw->setRndLimits((rlim | GW_WIREFRAME) & ~(GW_ILLUM | GW_BACKCULL | GW_FLAT | GW_SPECULAR));
  Matrix3 tm = inode->GetNodeTM(t);
  Point3 vx, vy, vz, p0, p[9];
  float sz;
  pblock->GetValue(PB_SIZE, t, sz, FOREVER);
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

void LinkDummy::GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
  UpdateMesh(t);
  Matrix3 mat = Inverse(inode->GetNodeTM(t));
  float sz;
  pblock->GetValue(PB_SIZE, t, sz, FOREVER);
  box.MakeCube(Point3(0, 0, 0), sz * 2);
}

void LinkDummy::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box)
{
  UpdateMesh(t);
  Matrix3 mat = inode->GetNodeTM(t);
  GetLocalBoundBox(t, inode, vpt, box);
  box = box * mat;
}

void LinkDummy::GetDeformBBox(TimeValue t, Box3 &box, Matrix3 *tm, BOOL useSel)
{
  UpdateMesh(t);
  float sz;
  pblock->GetValue(PB_SIZE, t, sz, FOREVER);
  box.MakeCube(Point3(0, 0, 0), sz * 2);
  if (tm)
    box = box * (*tm);
}
