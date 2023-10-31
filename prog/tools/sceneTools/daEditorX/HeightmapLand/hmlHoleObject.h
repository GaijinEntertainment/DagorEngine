#pragma once


#include "hmlPlugin.h"

#include <EditorCore/ec_ObjectEditor.h>


static constexpr unsigned CID_HmapLandHoleObject = 0xB00BA8AFu; // HmapLandHoleObject
static const int HMAP_TYPE_HOLE = 0;


class DataBlock;


class HmapLandHoleObject : public RenderableEditableObject
{
public:
  Point3 boxSize;

  HmapLandHoleObject();

  virtual void update(real dt);
  virtual void beforeRender();
  virtual void render();
  virtual void renderTrans();

  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void fillProps(PropPanel2 &panel, DClassID for_class_id, dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void onPPChange(int pid, bool edit_finished, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) {}

  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);

  virtual void putRotateUndo() {}

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);


  EO_IMPLEMENT_RTTI(CID_HmapLandHoleObject)


  void buildBoxEdges(IGenViewportWnd *vp, Point2 edges[12][2]) const;
  void setBoxSize(Point3 &s);
  Point3 getBoxSize() { return boxSize; }


  E3DCOLOR normal_color, selected_color;
};
