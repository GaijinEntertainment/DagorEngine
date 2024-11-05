// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSnow.h"
#include "hmlPlugin.h"
#include <de3_interface.h>
#include <EditorCore/ec_IEditorCore.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <EditorCore/ec_rect.h>
#include <math/dag_math3d.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_math2d.h>
#include <debug/dag_debug.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

enum
{
  PID_DIRECTIONAL = 1,
  PID_GEOM_POS,
  PID_GEOM_RADIUS,
  PID_SNOW_VALUE,
  PID_SNOW_BORDER,
};

void SnowSourceObject::Props::defaults()
{
  value = 0.25;
  border = 1.0;
}


int SnowSourceObject::showSubtypeId = -1;


SnowSourceObject::SnowSourceObject()
{
  props.defaults();
  if (showSubtypeId == -1)
    showSubtypeId = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("snow_obj");
}


SnowSourceObject::~SnowSourceObject() {}


bool SnowSourceObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  Point2 cp[8];
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  BBox2 box2;
  real z;
  bool in_frustum = false;

#define TEST_POINT(i, P)                                                                         \
  in_frustum |= vp->worldToClient(matrix * P, cp[i], &z) && z > 0;                               \
  if (z > 0 && rect.l <= cp[i].x && rect.t <= cp[i].y && cp[i].x <= rect.r && cp[i].y <= rect.b) \
    return true;                                                                                 \
  else                                                                                           \
    box2 += cp[i];

#define TEST_SEGMENT(i, j)                          \
  if (::isect_line_segment_box(cp[i], cp[j], rbox)) \
  return true

  for (int i = 0; i < 8; i++)
  {
    TEST_POINT(i, box.point(i));
  }

  if (!in_frustum)
    return false;
  BBox2 rbox(Point2(rect.l, rect.t), Point2(rect.r, rect.b));
  if (!(box2 & rbox))
    return false;

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

#undef TEST_POINT
#undef TEST_SEGMENT

  return isSelectedByPointClick(vp, rect.l, rect.t);
  return false;
}


bool SnowSourceObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  Point3 dir, p0;
  float out_t;

  vp->clientToWorld(Point2(x, y), p0, dir);
  float wtm_det = getWtm().det();
  if (fabsf(wtm_det) < 1e-12)
    return false;
  TMatrix iwtm = inverse(getWtm(), wtm_det);
  return rayIntersectSphere(iwtm * p0, normalize(iwtm % dir), Point3(0, 0, 0), 0.5 * 0.5);
}


bool SnowSourceObject::getWorldBox(BBox3 &box) const
{
  box = matrix * BSphere3(Point3(0, 0, 0), 0.5);
  return true;
}


void SnowSourceObject::render() {}


void SnowSourceObject::renderTrans() {}


void SnowSourceObject::renderObject()
{
  TMatrix tm = getWtm();
  bool sel = isSelected();
  E3DCOLOR dc = sel ? E3DCOLOR(8, 242, 242, 255) : E3DCOLOR(0, 0, 255, 255);

  dagRender->setLinesTm(tm);
  dagRender->renderSphere(Point3(0, 0, 0), 0.5, dc);
}

void SnowSourceObject::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  bool one_type = true;

  for (int i = 0; i < objects.size(); ++i)
  {
    SnowSourceObject *o = RTTI_cast<SnowSourceObject>(objects[i]);
    if (!o)
    {
      one_type = false;
      continue;
    }
  }

  if (one_type)
  {
    op.createIndent(-1);

    op.createPoint3(PID_GEOM_POS, "Position", getPos());
    op.createTrackFloat(PID_GEOM_RADIUS, "Radius", getRadius(), 0.1, 1000, 0.1);

    op.createTrackFloat(PID_SNOW_VALUE, "Snow value", props.value, -4, 4, 0.001);
    op.createTrackFloat(PID_SNOW_BORDER, "Border", props.border, 0, 1, 0.01);
  }
}

void SnowSourceObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
#define CHANGE_VAL(type, pname, getfunc)                             \
  {                                                                  \
    type val = panel.getfunc(pid);                                   \
    for (int i = 0; i < objects.size(); ++i)                         \
    {                                                                \
      SnowSourceObject *o = RTTI_cast<SnowSourceObject>(objects[i]); \
      if (!o || o->pname == val)                                     \
        continue;                                                    \
      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));  \
      o->pname = val;                                                \
      o->propsChanged();                                             \
    }                                                                \
  }

  switch (pid)
  {
    case PID_GEOM_POS: setPos(panel.getPoint3(pid)); break;

    case PID_GEOM_RADIUS:
    {
      float d = panel.getFloat(pid) * 2.0;
      setSize(Point3(d, d, d));
      propsChanged();
    }
    break;

    case PID_SNOW_VALUE: CHANGE_VAL(float, props.value, getFloat) break;

    case PID_SNOW_BORDER: CHANGE_VAL(float, props.border, getFloat) break;
  }

  DAGORED2->repaint();

#undef CHANGE_VAL
}


void SnowSourceObject::scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  RenderableEditableObject::scaleObject(delta, origin, basis);

  Point3 size = getSize();
  if (size.x != size.y || size.x != size.z)
    size.y = size.z = size.x;
  setSize(size);

  propsChanged();
}


void SnowSourceObject::save(DataBlock &blk)
{
  blk.setStr("name", getName());
  blk.setTm("tm", matrix);
  blk.setReal("value", props.value);
  blk.setReal("border", props.border);
}


void SnowSourceObject::load(const DataBlock &blk)
{
  getObjEditor()->setUniqName(this, blk.getStr("name", ""));
  setWtm(blk.getTm("tm", TMatrix::IDENT));
  props.value = blk.getReal("value", 0.25);
  props.border = blk.getReal("border", 1.0);

  propsChanged();
}


void SnowSourceObject::setWtm(const TMatrix &wtm) { RenderableEditableObject::setWtm(wtm); }

void SnowSourceObject::onRemove(ObjectEditor *) { propsChanged(); }

void SnowSourceObject::onAdd(ObjectEditor *objEditor) {}

bool SnowSourceObject::setPos(const Point3 &p)
{
  if (!RenderableEditableObject::setPos(p))
    return false;

  propsChanged();
  return true;
}


void SnowSourceObject::propsChanged() { HmapLandPlugin::self->updateSnowSources(); }


SnowSourceObject *SnowSourceObject::clone()
{
  SnowSourceObject *obj = new SnowSourceObject;

  getObjEditor()->setUniqName(obj, getName());

  obj->setProps(getProps());

  TMatrix tm = getWtm();
  obj->setWtm(tm);

  return obj;
}


void SnowSourceObject::putMoveUndo()
{
  HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();
  if (!ed->isCloneMode())
    RenderableEditableObject::putMoveUndo();
}

//-----------------------------------------

static PtrTab<StaticGeometryMaterial> originalSharedMats(tmpmem);
static PtrTab<StaticGeometryMaterial> snowSharedMats(tmpmem);
static Tab<int> differentMatIdx(tmpmem);

void SnowSourceObject::clearMats()
{
  clear_and_shrink(originalSharedMats);
  clear_and_shrink(snowSharedMats);
  clear_and_shrink(differentMatIdx);
}

StaticGeometryMaterial *SnowSourceObject::makeSnowMat(StaticGeometryMaterial *source_mat)
{
  if (!source_mat)
    return NULL;

  for (int i = 0; i < originalSharedMats.size(); ++i)
    if (originalSharedMats[i].get() == source_mat)
      return snowSharedMats[i];

  for (int i = 0; i < differentMatIdx.size(); ++i)
    if (originalSharedMats[differentMatIdx[i]]->equal(*source_mat))
    {
      StaticGeometryMaterial *gm = snowSharedMats[differentMatIdx[i]];
      originalSharedMats.push_back(source_mat);
      snowSharedMats.push_back(gm);
      return gm;
    }

  StaticGeometryMaterial *gm = new StaticGeometryMaterial(*source_mat);
  gm->name = String(256, "%s_snow", gm->name.str());
  gm->scriptText = String(128, "%s\r\nhas_snow=1\r\n", gm->scriptText.str());

  differentMatIdx.push_back(originalSharedMats.size());
  originalSharedMats.push_back(source_mat);
  snowSharedMats.push_back(gm);
  return gm;
}


void SnowSourceObject::calcSnow(dag::ConstSpan<SnowSourceObject *> src, float avg_snow, StaticGeometryContainer &cont)
{
  static const int SNOW_CH = 7;
  Tab<int> matRemap(tmpmem);
  int node_cnt = 0, vert_cnt = 0;

  clearMats();
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    if (!node.mesh->mats.size())
      continue;

    Mesh *mesh = &node.mesh->mesh;
    bool has_snow = false;

    for (int j = 0; j < mesh->vert.size(); ++j)
    {
      Point3 v = node.wtm * mesh->vert[j];
      float snow_value = avg_snow;

      for (int k = 0; k < src.size(); ++k)
      {
        float distance = (src[k]->getPos() - v).length();
        if (distance < src[k]->getRadius())
        {
          float add_value = src[k]->getProps().value;

          float to_center_pos = distance / src[k]->getRadius();
          float brush_bs = src[k]->getProps().border;
          if (to_center_pos > brush_bs && brush_bs < 1.0)
            add_value = lerp(add_value, 0.0f, (to_center_pos - brush_bs) / (1.0 - brush_bs));

          snow_value += add_value;
        }
      }
      if (snow_value <= 0)
        continue;

      if (!has_snow)
      {
        has_snow = true;
        if (mesh->tface[SNOW_CH].size())
          DAEDITOR3.conWarning("tface[%d].count=%d (is not empty) in node \"%s\"", SNOW_CH, mesh->tface[SNOW_CH].size(),
            node.name.str());

        if (node.mesh->getRefCount() > 1)
        {
          // clone mesh when it is used by more than one node
          StaticGeometryMesh *m = new StaticGeometryMesh;
          m->mats = node.mesh->mats;
          m->mesh = *mesh;
          node.mesh = m;
          mesh = &m->mesh;
        }

        mesh->tvert[SNOW_CH].resize(mesh->vert.size());
        mem_set_0(mesh->tvert[SNOW_CH]);
        mesh->tface[SNOW_CH].resize(mesh->face.size());
        for (int k = 0; k < mesh->face.size(); k++)
        {
          mesh->tface[SNOW_CH][k].t[0] = mesh->face[k].v[0];
          mesh->tface[SNOW_CH][k].t[1] = mesh->face[k].v[1];
          mesh->tface[SNOW_CH][k].t[2] = mesh->face[k].v[2];
        }
      }

      mesh->tvert[SNOW_CH][j].x = min(snow_value, 1.0f);
      vert_cnt++;
    }

    if (has_snow)
    {
      Point2 *tv = mesh->tvert[SNOW_CH].data();
      matRemap.resize(node.mesh->mats.size());
      mem_set_ff(matRemap);
      node_cnt++;

      int last_mat_index = node.mesh->mats.size();
      for (Face *f = mesh->face.data(), *fe = f + mesh->face.size(); f < fe; f++)
      {
        if (tv[f->v[0]].x + tv[f->v[1]].x + tv[f->v[2]].x <= 0)
          continue;
        int mat = f->mat % matRemap.size();
        if (matRemap[mat] == -1)
        {
          matRemap[mat] = node.mesh->mats.size();
          node.mesh->mats.push_back(makeSnowMat(node.mesh->mats[mat]));
        }
        f->mat = matRemap[mat];
      }
    }
  }
  if (vert_cnt)
    DAEDITOR3.conNote("separated snow meshed: %d nodes, %d vertices, %d mats", node_cnt, vert_cnt, differentMatIdx.size());
  clearMats();
}

void SnowSourceObject::updateSnowSources(dag::ConstSpan<NearSnowSource> nearSources)
{
  for (int i = 0; i < NEAR_SRC_COUNT; ++i)
  {
    Color4 snow_pos(0, 0, 0, 0);
    Color4 snow_params(0, 0, 0, 0);

    if (i < nearSources.size())
    {
      float r = nearSources[i].source->getRadius();
      float b = nearSources[i].source->getProps().border;

      Point3 _pos = nearSources[i].source->getPos();
      snow_pos.r = _pos.x;
      snow_pos.g = _pos.y;
      snow_pos.b = _pos.z;
      snow_params.r = b * r;
      snow_params.g = nearSources[i].source->getProps().value;
      snow_params.b = r * (1.0 - b);
      snow_params.b = (snow_params.b < 1e-3) ? 1e+3 : 1.0 / snow_params.b;
    }

    int snowPos_id = dagGeom->getShaderVariableId(String(64, "snow_pos%d", i));
    dagGeom->shaderGlobalSetColor4(snowPos_id, snow_pos);

    int snowParam_id = dagGeom->getShaderVariableId(String(64, "snow_param%d", i));
    dagGeom->shaderGlobalSetColor4(snowParam_id, snow_params);
  }
}
