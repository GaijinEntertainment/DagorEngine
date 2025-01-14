// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "box_csg.h"
#include "plugIn.h"

#include <libTools/renderUtil/dynRenderBuf.h>
#include <EditorCore/ec_IEditorCore.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_math2d.h>

#include <EditorCore/ec_rect.h>
#include <vector>

// #include <debug/dag_debug.h>

using editorcore_extapi::dagRender;

static DynRenderBuffer::Vertex boxVert[8];
static int boxInd[36];
static int boxqi[] = {0, 1, 3, 2, 4, 5, 7, 6, 0, 1, 5, 4, 3, 2, 6, 7, 0, 4, 6, 2, 1, 5, 7, 3};

const BBox3 BoxCSG::normBox(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
static BBox3 normQuad(Point3(-0.5, -0.5, 0), Point3(0.5, 0.5, 0));

E3DCOLOR BoxCSG::norm_col(200, 10, 10), BoxCSG::sel_col(255, 255, 255);


void BoxCSG::initStatics()
{
  memset(boxVert, 0, sizeof(boxVert));
  for (int i = 0; i < 6; i++)
  {
    int *ind = boxInd + i * 6;
    ind[0] = boxqi[i * 4 + 0];
    ind[1] = boxqi[i * 4 + 1];
    ind[2] = boxqi[i * 4 + 2];
    ind[3] = boxqi[i * 4 + 0];
    ind[4] = boxqi[i * 4 + 2];
    ind[5] = boxqi[i * 4 + 3];
  }
}


BoxCSG::BoxCSG() { box = true; }

BoxCSG::BoxCSG(const BoxCSG &src) : RenderableEditableObject(src), box(src.box) {}

void BoxCSG::onAdd(ObjectEditor *objEditor)
{
  if (name.empty())
    objEditor->setUniqName(this, "BoxCSG");
}

bool BoxCSG::getQuad(Point3 v[4])
{
  if (box)
    return false;
  v[0] = getWtm() * normQuad.point(0);
  v[1] = getWtm() * normQuad.point(1);
  v[2] = getWtm() * normQuad.point(3);
  v[3] = getWtm() * normQuad.point(2);
  return true;
}

void BoxCSG::render()
{
  dagRender->setLinesTm(getWtm());
  if (box)
    dagRender->renderBox(normBox, isSelected() ? sel_col : norm_col);
  else
  {
    E3DCOLOR c = isSelected() ? sel_col : norm_col;
    dagRender->renderLine(normQuad.point(0), normQuad.point(1), c);
    dagRender->renderLine(normQuad.point(1), normQuad.point(3), c);
    dagRender->renderLine(normQuad.point(3), normQuad.point(2), c);
    dagRender->renderLine(normQuad.point(2), normQuad.point(0), c);
  }
}

void BoxCSG::renderCommonGeom(DynRenderBuffer &dynBuf, const TMatrix4 &gtm)
{
  bool curPlug = DAGORED2->curPlugin() == CSGPlugin::self;
  E3DCOLOR rendCol = (isSelected() && curPlug) ? sel_col : norm_col;
  rendCol.a = 63;
  if (box)
    renderBox(dynBuf, getWtm(), rendCol);
  else
  {
    rendCol.r = 255 - rendCol.r;
    renderQuad(dynBuf, getWtm(), rendCol);
  }
}


void BoxCSG::renderBox(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c)
{
  for (int i = 0; i < 8; i++)
  {
    boxVert[i].pos = tm * normBox.point(i);
    boxVert[i].color = c;
  }

  db.addInd(boxInd, 36, db.addVert(boxVert, 8));
}


void BoxCSG::renderQuad(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c)
{
  for (int i = 0; i < 4; i++)
  {
    boxVert[i].pos = tm * normQuad.point(i);
    boxVert[i].color = c;
  }
  db.addInd(boxInd, 6, db.addVert(boxVert, 4));
}

void BoxCSG::renderQuad(DynRenderBuffer &db, const Point3 v[4], E3DCOLOR c, bool ring)
{
  if (!ring)
  {
    boxVert[0].pos = v[0];
    boxVert[0].color = c;
    boxVert[1].pos = v[1];
    boxVert[1].color = c;
    boxVert[2].pos = v[3];
    boxVert[2].color = c;
    boxVert[3].pos = v[2];
    boxVert[3].color = c;
  }
  else
    for (int i = 0; i < 4; i++)
    {
      boxVert[i].pos = v[i];
      boxVert[i].color = c;
    }

  db.addInd(boxInd, 6, db.addVert(boxVert, 4));
}

bool BoxCSG::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  Point2 cp[8];
  BBox2 box2;
  real z;
  bool in_frustum = false;

  TMatrix tm = getWtm();

#define TEST_POINT(i, P)                                                                         \
  in_frustum |= vp->worldToClient(tm * P, cp[i], &z) && z > 0;                                   \
  if (z > 0 && rect.l <= cp[i].x && rect.t <= cp[i].y && cp[i].x <= rect.r && cp[i].y <= rect.b) \
    return true;                                                                                 \
  else                                                                                           \
    box2 += cp[i];

#define TEST_SEGMENT(i, j)                          \
  if (::isect_line_segment_box(cp[i], cp[j], rbox)) \
  return true

  if (box)
    for (int i = 0; i < 8; i++)
    {
      TEST_POINT(i, normBox.point(i));
    }
  else
    for (int i = 0; i < 4; i++)
    {
      TEST_POINT(i, normQuad.point(i));
    }

  if (!in_frustum)
    return false;
  BBox2 rbox(Point2(rect.l, rect.t), Point2(rect.r, rect.b));
  if (!(box2 & rbox))
    return false;

  if (box)
  {
    TEST_SEGMENT(0, 4);
    TEST_SEGMENT(4, 5);
    TEST_SEGMENT(5, 1);
    TEST_SEGMENT(1, 0);
    TEST_SEGMENT(2, 6);
    TEST_SEGMENT(6, 7);
    TEST_SEGMENT(7, 3);
    TEST_SEGMENT(3, 2);
    TEST_SEGMENT(0, 2);
    TEST_SEGMENT(1, 3);
    TEST_SEGMENT(4, 6);
    TEST_SEGMENT(5, 7);
  }
  else
  {
    TEST_SEGMENT(1, 0);
    TEST_SEGMENT(3, 2);
    TEST_SEGMENT(0, 2);
    TEST_SEGMENT(1, 3);
  }

#undef TEST_POINT
#undef TEST_SEGMENT

  return isSelectedByPointClick(vp, rect.l, rect.t);
}
bool BoxCSG::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  Point3 dir, p0;
  float out_t;

  vp->clientToWorld(Point2(x, y), p0, dir);
  return ray_intersect_box(p0, dir, box ? normBox : normQuad, getWtm(), out_t);
}

bool BoxCSG::getWorldBox(BBox3 &box) const
{
  box = getWtm() * normBox;
  return true;
}

void BoxCSG::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  bool one_type = true;

  for (int i = 0; i < objects.size(); ++i)
  {
    BoxCSG *o = RTTI_cast<BoxCSG>(objects[i]);
    if (!o)
    {
      one_type = false;
      continue;
    }
  }

  G_UNUSED(one_type);

  for (int i = 0; i < objects.size(); ++i)
  {
    BoxCSG *o = RTTI_cast<BoxCSG>(objects[i]);
    if (!o)
      continue;
  }
}

void BoxCSG::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (!edit_finished)
    return;
}


void BoxCSG::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects) {}


// bool BoxCSG::onPPValidateParam(int pid, PropPanel::ContainerPropertyControl &panel,
//     dag::ConstSpan<RenderableEditableObject*> objects)
//{
//   return true;
// }


void BoxCSG::load(const DataBlock &blk)
{
  if (blk.getStr("name", NULL))
    setName(blk.getStr("name", NULL));
  box = blk.getBool("box", true);
  setWtm(blk.getTm("wtm", TMatrix::IDENT));
}


void BoxCSG::save(DataBlock &blk)
{
  blk.setStr("name", getName());
  blk.setBool("box", box);
  blk.setTm("wtm", getWtm());
}

void BoxCSG::update(float dt) {}
