// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <max.h>
#include <simpobj.h>
#include <iparamm.h>
#include "dagor.h"
#include "resource.h"

inline const Matrix3 dummyIdentTm(1);

// Traits supplies superClassId(), classId(), nameId, rollupId, paramDialogId and axisColor(selected, axis)
template <class Traits>
class DagorDummyObject : public SimpleObject
{
public:
  static constexpr int PB_SIZE = 0;

  inline static IParamMap *pmapParam = NULL;
  inline static float crtSize = 20.0f;

  DagorDummyObject()
  {
    static ParamBlockDescID descVer0[] = {{TYPE_FLOAT, NULL, FALSE, PB_SIZE}};
    MakeRefByID(FOREVER, 0, CreateParameterBlock(descVer0, sizeof(descVer0) / sizeof(descVer0[0]), 0));
    assert(pblock);
    pblock->SetValue(PB_SIZE, 0, crtSize);
  }

  void InvalidateUI() override
  {
    if (pmapParam)
      pmapParam->Invalidate();
  }

  void set_axis_color(GraphicsWindow *gw, INode *n, int a)
  {
    const Point3 c = n->IsFrozen() ? Point3(0.5f, 0.5f, 0.5f) : Traits::axisColor(n->Selected(), a);
    gw->setColor(LINE_COLOR, c.x, c.y, c.z);
  }

  int Display(TimeValue t, INode *inode, ViewExp *vpt, int flags) override
  {
    UpdateMesh(t);
    GraphicsWindow *gw = vpt->getGW();

    gw->setTransform(dummyIdentTm);
    DrawIt(gw, t, inode, inode->Selected(), flags, vpt);
    return 0;
  }

  int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override
  {
    HitRegion hitRegion;
    GraphicsWindow *gw = vpt->getGW();
    MakeHitRegion(hitRegion, type, crossing, 4, p);
    UpdateMesh(t);

    DWORD rlim = gw->getRndLimits();

    gw->setTransform(dummyIdentTm);
    gw->setHitRegion(&hitRegion);
    gw->clearHitCode();
    gw->setRndLimits(rlim | GW_PICK);

    DrawIt(gw, t, inode, inode->Selected(), flags, vpt);
    int res = gw->checkHitCode();

    gw->setRndLimits(rlim);
    return res;
  }

  void DeleteThis() override { delete this; }
  SClass_ID SuperClassID() override { return Traits::superClassId(); }
  int IsRenderable() override { return 0; }
  int UsesWireColor() override { return 1; }
  int DoOwnSelectHilite() override { return 1; }

  CreateMouseCallBack *GetCreateMouseCallBack() override
  {
    static CreateCallBack createCb;
    createCb.SetObj(this);
    return &createCb;
  }

  const wchar_t *GetObjectName(bool localized) const override { return GetString(Traits::nameId); }
  RefTargetHandle Clone(RemapDir &remap) override
  {
    DagorDummyObject *newob = new DagorDummyObject();
    newob->ReplaceReference(0, pblock->Clone(remap));
    newob->ivalid.SetEmpty();
    BaseClone(this, newob, remap);
    return newob;
  }

  BOOL OKtoDisplay(TimeValue t) override
  {
    float r;
    pb_get_value(*pblock, PB_SIZE, t, r);
    return r != 0.0f;
  }

  ParamDimension *GetParameterDim(int pbIndex) override
  {
    switch (pbIndex)
    {
      case PB_SIZE: return stdWorldDim;
      default: return defaultDim;
    }
  }

  MSTR GetParameterName(int pbIndex, bool localized) override { return MSTR(_T("???")); }

  void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override
  {
    static ParamUIDesc descParam[] = {
      ParamUIDesc(PB_SIZE, EDITTYPE_UNIVERSE, IDC_SIZ, IDC_SIZE_S, 0.0f, 9999999999.0f, SPIN_AUTOSCALE)};

    SimpleObject::BeginEditParams(ip, flags, prev);

    if (pmapParam)
    {
      pmapParam->SetParamBlock(pblock);
    }
    else
    {
      pmapParam = CreateCPParamMap(descParam, sizeof(descParam) / sizeof(descParam[0]), pblock, ip, hInstance,
        MAKEINTRESOURCE(Traits::paramDialogId), GetString(Traits::rollupId), 0);
    }
  }

  void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override
  {
    SimpleObject::EndEditParams(ip, flags, next);

    if (flags & END_EDIT_REMOVEUI)
    {
      DestroyCPParamMap(pmapParam);
      pmapParam = NULL;
    }

    pb_get_value(*pblock, PB_SIZE, ip->GetTime(), crtSize);
  }

  void BuildMesh(TimeValue) override
  {
    ivalid.SetInfinite();
    mesh.setNumTVerts(0);
    mesh.setNumTVFaces(0);
    mesh.setNumFaces(0);
    mesh.setNumVerts(0);
  }

  void DrawIt(GraphicsWindow *gw, TimeValue t, INode *inode, int sel, int flags, ViewExp *vpt)
  {
    DWORD rlim = gw->getRndLimits();
    gw->setRndLimits((rlim | GW_WIREFRAME) & ~(GW_ILLUM | GW_BACKCULL | GW_FLAT | GW_SPECULAR));
    Matrix3 tm = inode->GetNodeTM(t);
    float sz;
    pb_get_value(*pblock, PB_SIZE, t, sz);
    const Point3 p0 = tm.GetRow(3);
    const Point3 v[3] = {tm.GetRow(0) * sz, tm.GetRow(1) * sz, tm.GetRow(2) * sz};

    for (int a = 0; a < 3; ++a)
    {
      const Point3 &axis = v[a];
      const Point3 &s1 = v[a == 0 ? 1 : 0];
      const Point3 &s2 = v[a == 2 ? 1 : 2];

      Point3 p[8];
      p[0] = p0 - axis;
      p[1] = p0 + axis;
      p[2] = p0 + axis * 0.5f - s1 * 0.25f;
      p[3] = p0 + axis * 0.5f + s1 * 0.25f;
      p[4] = p[1];
      p[5] = p0 + axis * 0.5f - s2 * 0.25f;
      p[6] = p0 + axis * 0.5f + s2 * 0.25f;
      p[7] = p[1];
      set_axis_color(gw, inode, a);
      gw->polyline(8, p, NULL, NULL, FALSE, NULL);
    }

    gw->setRndLimits(rlim);
  }

  void GetLocalBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box) override
  {
    UpdateMesh(t);
    float sz;
    pb_get_value(*pblock, PB_SIZE, t, sz);
    box.MakeCube(Point3(0, 0, 0), sz * 2);
  }

  void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp *vpt, Box3 &box) override
  {
    UpdateMesh(t);
    Matrix3 mat = inode->GetNodeTM(t);
    GetLocalBoundBox(t, inode, vpt, box);
    box = box * mat;
  }

  void GetDeformBBox(TimeValue t, Box3 &box, Matrix3 *tm = NULL, BOOL useSel = FALSE) override
  {
    GetLocalBoundBox(t, NULL, NULL, box);
    if (tm)
      box = box * (*tm);
  }

  Class_ID ClassID() override { return Traits::classId(); }

private:
  class CreateCallBack : public CreateMouseCallBack
  {
    IPoint2 sp1;
    DagorDummyObject *ob;
    Point3 p0, p1;

  public:
    void SetObj(DagorDummyObject *obj) { ob = obj; }

    int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3 &mat) override
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
            if (p1 == p0)
            {
              if (msg != MOUSE_POINT)
                break;
              ob->suspendSnap = FALSE;
              return CREATE_ABORT;
            }
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
  };
};

template <class Traits>
class DummyClassDesc : public ClassDesc
{
public:
  int IsPublic() override { return TRUE; }
  void *Create(BOOL loading = FALSE) override { return new DagorDummyObject<Traits>(); }
  const TCHAR *ClassName() override { return GetString(Traits::nameId); }
  const MCHAR *NonLocalizedClassName() override { return ClassName(); }
  SClass_ID SuperClassID() override { return Traits::superClassId(); }
  Class_ID ClassID() override { return Traits::classId(); }
  const TCHAR *Category() override { return GetString(IDS_DAGOR_CAT); }
};
