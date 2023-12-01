// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _DE2_PLUGIN_ROADS_SPLINEOBJECTS_H_
#define _DE2_PLUGIN_ROADS_SPLINEOBJECTS_H_
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <de3_splineClassData.h>
#include <de3_splineGenSrv.h>
#include <3d/dag_texMgr.h>
#include <util/dag_simpleString.h>

#include <math/dag_bounds2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <EditorCore/ec_interface.h>

#include <EditorCore/ec_rect.h>

class ObjLibMaterialListMgr;
class DynRenderBuffer;
class TMatrix4;
class Point2;
class Point4;
class SplineObject;
class LoftObject;
class HmapLandObjectEditor;

class PropertyContainerControlBase;

class GeomObject;

static inline Point4 toPoint4(const Point3 &p, real w) { return Point4(p.x, p.y, p.z, w); }

static bool pt_intersects(const Point3 &p, float rad, int x, int y, IGenViewportWnd *vp)
{
  BBox2 ptBox;
  Point2 pt;

  if (!vp->worldToClient(p, pt))
    return false;
  ptBox[0] = pt - Point2(rad, rad);
  ptBox[1] = pt + Point2(rad, rad);
  return ptBox & Point2(x, y);
}

static bool pt_intersects(const Point3 &p, float rad, const EcRect &rect, IGenViewportWnd *vp)
{
  BBox2 ptBox;
  BBox2 rectBox;
  Point2 pt;

  if (!vp->worldToClient(p, pt))
    return false;
  ptBox[0] = pt - Point2(rad, rad);
  ptBox[1] = pt + Point2(rad, rad);

  rectBox[0].x = rect.l;
  rectBox[1].x = rect.r;
  rectBox[0].y = rect.t;
  rectBox[1].y = rect.b;

  return ptBox & rectBox;
}

class SplinePointObject;

static constexpr unsigned CID_SplinePointObject = 0x48839E05u; // SplinePointObject

//
// Spline point
//
class SplinePointObject : public RenderableEditableObject
{
protected:
  ~SplinePointObject();

public:
  EO_IMPLEMENT_RTTI(CID_SplinePointObject, RenderableEditableObject)
  
  SplinePointObject();

  virtual void update(float) {}
  virtual void beforeRender() {}
  virtual void render() {}
  virtual void renderTrans() {}
  void renderPts(DynRenderBuffer &dynBuf, const TMatrix4 &gtm, const Point2 &s, bool start = false);

  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void fillProps(PropertyContainerControlBase &op, DClassID for_class_id, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void onPPChange(int pid, bool edit_finished, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void onPPBtnPressed(int pid, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void onPPClose(PropertyContainerControlBase &panel, dag::ConstSpan<RenderableEditableObject *> objects);

  // restrict rotate/scale transformations to BASIS_Local/selCenter

  virtual void moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis);
  virtual void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) {}
  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);
  virtual void putMoveUndo();
  virtual void putRotateUndo();
  virtual void putScaleUndo();

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);

  virtual bool mayRename() { return false; }
  virtual bool mayDelete() { return true; }
  virtual void setWtm(const TMatrix &wtm);

  virtual void onRemove(ObjectEditor *);
  virtual void onAdd(ObjectEditor *objEditor);

  virtual void selectObject(bool select = true);

  virtual bool setPos(const Point3 &p);


  Point3 getPt() const { return props.pt; }
  Point3 getBezierIn() const { return props.pt + getPtEffRelBezierIn(); }
  Point3 getBezierOut() const { return props.pt + getPtEffRelBezierOut(); }
  Point3 getUpDir() const;
  float getRoadHalfWidth() const { return 0; }

  void setRelBezierIn(const Point3 &p) { props.relIn = p; }
  void setRelBezierOut(const Point3 &p) { props.relOut = p; }

  Point3 getPtEffRelBezierIn() const;
  Point3 getPtEffRelBezierOut() const;

  void FlattenY();
  void FlattenAverage(float mul);

  void pointChanged();
  void swapHelpers();

  int linkCount();
  SplinePointObject *getLinkedPt(int id);

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }

public:
  struct Props
  {
    SimpleString blkGenName;
    Point3 pt, relIn, relOut;
    bool useDefSet;
    short cornerType; // -2=def as spline, -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    splineclass::Attr attr;

    void defaults();
  };

  static void renderPoint(DynRenderBuffer &db, const Point4 &p, const Point2 &s, E3DCOLOR c);
  static void saveProps(const Props &p, DataBlock &blk, bool bezier_helpers);
  static void loadProps(Props &p, const DataBlock &blk);

  static void initStatics();

  BBox3 getGeomBox(bool render_loft, bool render_road);
  void renderRoadGeom(bool opaque);

  void setBlkGenName(const char *name)
  {
    props.blkGenName = name;
    props.useDefSet = (name == NULL);
  }

  splineclass::AssetData *prepareSplineClass(const char *def_name, splineclass::AssetData *prev);
  void resetSplineClass();
  void clearSegment();

  bool hasSplineClass() const { return !props.useDefSet; }
  const splineclass::AssetData *getSplineClass() const { return effSplineClass; }
  ISplineGenObj *getSplineGen() { return splineGenObj ? splineGenObj->queryInterface<ISplineGenObj>() : nullptr; }
  void setEditLayerIdx(int idx) { splineGenObj ? splineGenObj->setEditLayerIdx(idx) : (void)0; }

  const char *getEffectiveAsset(int ofs = 0) const;
  void setEffectiveAsset(const char *asset_name, bool put_undo, int ofs = 0);

  void deleteUnusedPoolsEntities();
  void resetUsedPoolsEntities();

  GeomObject *getRoadGeom() { return roadGeom; }

  GeomObject &getClearedRoadGeom();
  void removeRoadGeom();
  void updateRoadBox();

  void removeLoftGeom();

  void markChanged();
  const Props &getProps() const { return props; }
  void setProps(const Props &p) { props = p; }

  bool getAltLocalDir(Point3 &out_dir) const
  {
    switch (selObj)
    {
      case SELOBJ_IN: out_dir = normalize(props.relIn); return true;
      case SELOBJ_OUT: out_dir = normalize(props.relOut); return true;
    }
    return false;
  }


  short int arrId;
  unsigned short visible : 1, segChanged : 1;
  // real cross (mean, not road joint and not mere spline/road cross) must be checked together with isCross
  unsigned short isCross : 1, isRealCross : 1;
  SplineObject *spline;
  Point3 tmpUpDir; //< computed when generating straight segments, used when generating cross roads

  static E3DCOLOR norm_col, sel_col, sel2_col, hlp_col, norm_col_start_point;
  static TEXTUREID texPt;
  static float ptScreenRad;
  static int ptRenderPassId;

  static Props *defaultProps;

protected:
  Props props;

  splineclass::AssetData *effSplineClass = nullptr;
  IObjEntity *splineGenObj = nullptr;
  GeomObject *roadGeom;
  BBox3 roadGeomBox;

  enum
  {
    SELOBJ_IN = -1,
    SELOBJ_POINT = 0,
    SELOBJ_OUT = 1
  };
  int selObj, targetSelObj;

  PropertyContainerControlBase *ppanel_ptr;

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<SplinePointObject> obj;
    SplinePointObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(SplinePointObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    virtual void restore(bool save_redo);
    virtual void redo();

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoSpolinePointPropsChange"; }
  };

  void importGenerationParams(const char *blkname);

  HmapLandObjectEditor &getObjEd() const { return *(HmapLandObjectEditor *)getObjEditor(); }
  static HmapLandObjectEditor &getObjEd(ObjectEditor *oe) { return *(HmapLandObjectEditor *)oe; }
};

#endif
