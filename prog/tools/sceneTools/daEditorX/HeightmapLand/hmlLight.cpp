// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlLight.h"
#include <de3_interface.h>
#include <EditorCore/ec_IEditorCore.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/matFlags.h>
#include <EditorCore/ec_rect.h>
#include <math/dag_math3d.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_math2d.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include "hmlObjectsEditor.h"

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

enum
{
  PID_DIRECTIONAL = 1,
  PID_GEOM_POS,
  PID_GEOM_SIZE,
  PID_GEOM_DIR,
  PID_GEOM_SEGMENTS,
  PID_ANGLE_SIZE,
  PID_LT_COLOR,
  PID_LT_BRIGHTNESS,
  PID_AMB_COLOR,
  PID_AMB_BRIGHTNESS,
};

static void generateSphereMesh(MeshData &mesh, int segments, const Color4 &col);
static int get_cos_power_from_ang(real alfa, real part, real &real_part);
static real square_rec(int iter_no, real val, real *hashed_iter);

int SphereLightObject::lightGeomSubtype = -1;

void SphereLightObject::Props::defaults()
{
  color = E3DCOLOR(220, 220, 220);
  brightness = 300;
  ambColor = E3DCOLOR(220, 220, 220);
  ambBrightness = 0;
  ang = 30 * DEG_TO_RAD;
  directional = false;
  segments = 8;
}

SphereLightObject::SphereLightObject() : ltGeom(NULL)
{
  props.defaults();
  if (lightGeomSubtype == -1)
    lightGeomSubtype = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lt_geom");
}
SphereLightObject::~SphereLightObject() { dagGeom->deleteGeomObject(ltGeom); }

bool SphereLightObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
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
bool SphereLightObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
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
bool SphereLightObject::getWorldBox(BBox3 &box) const
{
  box = matrix * BSphere3(Point3(0, 0, 0), 0.5);
  return true;
}

void SphereLightObject::render()
{
  if (ltGeom)
  {
    ltGeom->setTm(getWtm());
    dagGeom->geomObjectRender(*ltGeom);
  }
}
void SphereLightObject::renderTrans()
{
  if (ltGeom)
  {
    ltGeom->setTm(getWtm());
    dagGeom->geomObjectRenderTrans(*ltGeom);
  }
}

void SphereLightObject::renderObject()
{
  TMatrix tm = getWtm();
  Point3 sz(matrix.getcol(0).length(), matrix.getcol(1).length(), matrix.getcol(2).length());
  bool sel = isSelected();
  E3DCOLOR c = props.directional ? props.ambColor : props.color;
  float br = props.directional ? props.ambBrightness : props.brightness;
  float br_scale = 0.5; // toneMapper.hdrScaleMul*toneMapper.postScaleMul
  E3DCOLOR dc = sel ? E3DCOLOR(200, 200, 200, 255) : E3DCOLOR(128, 160, 128, 255);

  dagRender->setLinesTm(tm);
  dagRender->renderSphere(Point3(0, 0, 0), 0.5, dc);

  if (sel && !props.directional)
  {
    real fallof = sqrt(PI * sz.lengthSq() * max(c.r, max(c.g, c.b)) * br * br_scale);

    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, getPos());

    dagRender->setLinesTm(tm);
    dagRender->renderSphere(Point3(0, 0, 0), fallof, c);
    // dagRender->renderSphere(Point3(0, 0, 0), visRad, dc);
  }

  if (props.directional)
  {
    real dirFallof =
      sqrt(props.ang * (sz / 2).lengthSq() * max(props.color.r, max(props.color.g, props.color.b)) * props.brightness * br_scale);

    Point3 top = Point3(0, 1, 0) * dirFallof;

    Tab<Point3> points(tmpmem);
    Tab<Point3> points1(tmpmem);
    TMatrix scaleTm = TMatrix::IDENT;
    scaleTm[0][0] = getSize().x / 2;
    scaleTm[2][2] = getSize().z / 2;
    TMatrix tM = getRotTm();
    tM.setcol(3, getPos());
    int pointsNum;
    points1.push_back(-Point3(0, 0, 1));
    pointsNum = 8;
    const float STEP = PI * 2 / ((float)pointsNum);
    for (int i = 1; i < pointsNum; i++)
      points1.push_back(rotyTM(STEP) * points1[i - 1]);

    for (int i = 0; i < pointsNum; i++)
    {
      Point3 side = points1[i];
      side.normalize();
      points1[i] = scaleTm * points1[i];
      points.push_back(points1[i] + sin(props.ang) * side * dirFallof + cos(props.ang) * top);
      points[i] = tM * points[i];
      points1[i] = tM * points1[i];
    }

    dagRender->setLinesTm(TMatrix::IDENT);
    E3DCOLOR dirc(props.color.r, props.color.g, props.color.b);
    for (int i = 0; i < points.size(); i++)
    {
      dagRender->renderLine(points[i], points1[i], dirc);
      if (i > 0)
        dagRender->renderLine(points[i - 1], points[i], dirc);
      else
        dagRender->renderLine(points.back(), points[i], dirc);
    }
  }
}

void SphereLightObject::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  bool one_type = true;

  for (int i = 0; i < objects.size(); ++i)
  {
    SphereLightObject *o = RTTI_cast<SphereLightObject>(objects[i]);
    if (!o)
    {
      one_type = false;
      continue;
    }
  }

  if (one_type)
  {
    op.createIndent(-1);

    op.createCheckBox(PID_DIRECTIONAL, "Directional", props.directional);
    // op.createCheckBox(PID_SPHERICAL, "Spherical", props.spherical);

    op.createPoint3(PID_GEOM_POS, "Position", getPos(), false);
    op.createPoint3(PID_GEOM_SIZE, "Size", getSize(), false);
    op.createPoint3(PID_GEOM_DIR, "Direction", getWtm().getcol(1), false);
    op.createEditInt(PID_GEOM_SEGMENTS, "Segments", props.segments);

    op.createEditFloat(PID_ANGLE_SIZE, "Angle size", props.ang, 2, props.directional);

    op.createColorBox(PID_LT_COLOR, "Color", props.color);
    op.createEditFloat(PID_LT_BRIGHTNESS, "Brightness", props.brightness);

    op.createColorBox(PID_AMB_COLOR, "Amb color", props.ambColor, props.directional);
    op.createEditFloat(PID_AMB_BRIGHTNESS, "Amb brightness", props.ambBrightness, 2, props.directional);

    for (int i = 0; i < objects.size(); ++i)
    {
      SphereLightObject *o = RTTI_cast<SphereLightObject>(objects[i]);
      if (o->props.directional != props.directional)
        op.resetById(PID_DIRECTIONAL);
      if (lengthSq(o->getPos() - getPos()) > 1e-6)
        op.resetById(PID_GEOM_POS);
      if (lengthSq(o->getSize() - getSize()) > 1e-6)
        op.resetById(PID_GEOM_SIZE);
      if (lengthSq(o->getWtm().getcol(1) - getWtm().getcol(1)) > 1e-6)
        op.resetById(PID_GEOM_DIR);
      if (o->props.segments != props.segments)
        op.resetById(PID_GEOM_SEGMENTS);
      if (o->props.ang != props.ang)
        op.resetById(PID_ANGLE_SIZE);
      if (o->props.color != props.color)
        op.resetById(PID_LT_COLOR);
      if (o->props.brightness != props.brightness)
        op.resetById(PID_LT_BRIGHTNESS);
      if (o->props.ambColor != props.ambColor)
        op.resetById(PID_AMB_COLOR);
      if (o->props.ambBrightness != props.ambBrightness)
        op.resetById(PID_AMB_BRIGHTNESS);
    }
  }
}

void SphereLightObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
#define CHANGE_VAL(type, pname, getfunc)                               \
  {                                                                    \
    type val = panel.getfunc(pid);                                     \
    for (int i = 0; i < objects.size(); ++i)                           \
    {                                                                  \
      SphereLightObject *o = RTTI_cast<SphereLightObject>(objects[i]); \
      if (!o || o->pname == val)                                       \
        continue;                                                      \
      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));    \
      o->pname = val;                                                  \
      o->propsChanged();                                               \
    }                                                                  \
  }
  if (!edit_finished)
    return;

  if (pid == PID_DIRECTIONAL)
  {
    CHANGE_VAL(bool, props.directional, getBool)

    panel.setEnabledById(PID_ANGLE_SIZE, props.directional);
    panel.setEnabledById(PID_AMB_COLOR, props.directional);
    panel.setEnabledById(PID_AMB_BRIGHTNESS, props.directional);
  }
  else if (pid == PID_GEOM_SEGMENTS)
  {
    CHANGE_VAL(int, props.segments, getInt)
  }
  else if (pid == PID_ANGLE_SIZE)
  {
    CHANGE_VAL(float, props.ang, getFloat)
  }
  else if (pid == PID_LT_BRIGHTNESS)
  {
    CHANGE_VAL(float, props.brightness, getFloat)
  }
  else if (pid == PID_AMB_BRIGHTNESS)
  {
    CHANGE_VAL(float, props.ambBrightness, getFloat)
  }
  else if (pid == PID_LT_COLOR)
  {
    CHANGE_VAL(E3DCOLOR, props.color, getColor)
  }
  else if (pid == PID_AMB_COLOR)
  {
    CHANGE_VAL(E3DCOLOR, props.ambColor, getColor)
  }
  DAGORED2->repaint();

#undef CHANGE_VAL
}

void SphereLightObject::save(DataBlock &blk)
{
  blk.setStr("name", getName());
  blk.setE3dcolor("color", props.color);
  blk.setReal("output", props.brightness);
  blk.setBool("directional", props.directional);
  blk.setReal("ambBrightness", props.ambBrightness);
  blk.setE3dcolor("ambColor", props.ambColor);
  blk.setReal("ang", props.ang);
  blk.setTm("tm", matrix);
  blk.setInt("segments", props.segments);
}
void SphereLightObject::load(const DataBlock &blk)
{
  String uniqName(blk.getStr("name", ""));
  if (uniqName == "")
    uniqName = "Sph_Light_Source_001";
  getObjEditor()->setUniqName(this, uniqName);
  setWtm(blk.getTm("tm", TMatrix::IDENT));

  props.color = blk.getE3dcolor("color", E3DCOLOR(220, 220, 220, 255));
  props.brightness = blk.getReal("output", 220);
  props.directional = blk.getBool("directional", false);
  props.ambBrightness = blk.getReal("ambBrightness", 0);
  props.ambColor = blk.getE3dcolor("ambColor", E3DCOLOR(220, 220, 220, 255));

  props.ang = blk.getReal("ang", 30 * DEG_TO_RAD);
  props.segments = blk.getInt("segments", 8);

  propsChanged();
}

void SphereLightObject::setWtm(const TMatrix &wtm) { RenderableEditableObject::setWtm(wtm); }

void SphereLightObject::onRemove(ObjectEditor *) {}
void SphereLightObject::onAdd(ObjectEditor *objEditor) { propsChanged(); }

bool SphereLightObject::setPos(const Point3 &p)
{
  if (!RenderableEditableObject::setPos(p))
    return false;
  return true;
}

void SphereLightObject::propsChanged() { buildGeom(); }

SphereLightObject *SphereLightObject::clone()
{
  SphereLightObject *obj = new SphereLightObject;

  getObjEditor()->setUniqName(obj, getName());

  obj->setProps(getProps());

  TMatrix tm = getWtm();
  obj->setWtm(tm);

  return obj;
}

void SphereLightObject::putMoveUndo()
{
  HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();
  if (!ed->isCloneMode())
    RenderableEditableObject::putMoveUndo();
}

void SphereLightObject::buildGeom()
{
  if (!ltGeom)
  {
    ltGeom = dagGeom->newGeomObject(midmem);
    ltGeom->notChangeVertexColors(true);
  }
  else
    dagGeom->geomObjectClear(*ltGeom);

  StaticGeometryContainer *cont = ltGeom->getGeometryContainer();
  int seg = props.segments < 5 ? 5 : props.segments;

  Color4 col = color4(props.color);
  Color4 ambCol = color4(props.ambColor);

  Ptr<MaterialData> matMain =
    SphereLightObject::makeMat("editor_helper_no_tex_blend", props.directional ? ambCol : col, Color4(0, 0, 0, 0), 0);

  Mesh *mesh = new Mesh;
  MaterialDataList mat;
  mat.addSubMat(matMain);

  generateSphereMesh(mesh->getMeshData(), seg, props.directional ? ambCol : col);
  dagGeom->objCreator3dAddNode(name, mesh, &mat, *cont);

  if (!cont->nodes.size())
    return;

  StaticGeometryNode *node = cont->nodes[0];

  if (node)
  {
    node->visRange = -1;
    node->name = name;

    node->wtm = TMatrix::IDENT;

    node->flags = StaticGeometryNode::FLG_RENDERABLE | StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF |
                  StaticGeometryNode::FLG_CASTSHADOWS | StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;

    if (props.directional)
    {
      node->normalsDir = matrix.getcol(1);
      node->flags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
    }

    node->lighting = StaticGeometryNode::LIGHT_NONE;
  }

  ltGeom->setTm(getWtm());
  ltGeom->setUseNodeVisRange(true);
  dagGeom->geomObjectSetDefNodeVis(*ltGeom);

  dagGeom->geomObjectRecompile(*ltGeom);
}
void SphereLightObject::gatherGeom(StaticGeometryContainer &container) const
{
  const StaticGeometryContainer *cont = ltGeom->getGeometryContainer();

  if (cont)
  {
    Color4 col = color4(props.color) * props.brightness;
    Color4 ambCol = color4(props.ambColor) * props.ambBrightness;

    StaticGeometryMaterial *matMain = NULL;

    if (props.directional)
    {
      real eps;
      real power = ::get_cos_power_from_ang(props.ang, 0.9, eps);
      matMain = makeMat2("hdr_dir_light", col, ambCol, power);
    }
    else
      matMain = makeMat2("hdr_envi", col, Color4(0, 0, 0, 0), 0);

    for (int i = 0; i < cont->nodes.size(); ++i)
    {
      const StaticGeometryNode *srcNode = cont->nodes[i];

      if (srcNode && srcNode->mesh)
      {
        StaticGeometryNode *node = new StaticGeometryNode(*srcNode);

        node->wtm = getWtm();
        node->mesh->mesh.cvert[0] = col;

        for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
          node->mesh->mats[mi] = matMain;

        container.addNode(node);
      }
    }
  }
}


static PtrTab<MaterialData> sharedMats(tmpmem);
static PtrTab<StaticGeometryMaterial> sharedMats2(tmpmem);

void SphereLightObject::clearMats()
{
  clear_and_shrink(sharedMats);
  clear_and_shrink(sharedMats2);
}

MaterialData *SphereLightObject::makeMat(const char *class_name, const Color4 &emis, const Color4 &amb_emis, real power)
{
  for (int i = 0; i < sharedMats.size(); i++)
    if (sharedMats[i] && stricmp(sharedMats[i]->className, class_name) == NULL && sharedMats[i]->mat.emis == emis &&
        sharedMats[i]->mat.amb == amb_emis && sharedMats[i]->mat.power == power)
      return sharedMats[i];

  MaterialData *gm = new MaterialData;
  gm->matName = "light_material";
  gm->className = class_name;
  gm->matScript = "emission=1000";
  gm->mat.diff = emis;
  gm->mat.amb = amb_emis;
  gm->mat.emis = emis;
  gm->mat.spec = emis;
  gm->mat.power = power;
  // gm->mat.cosPower = power;
  gm->flags = MatFlags::FLG_USE_IN_GI;

  sharedMats.push_back(gm);
  return gm;
}

StaticGeometryMaterial *SphereLightObject::makeMat2(const char *class_name, const Color4 &emis, const Color4 &amb_emis, real power)
{
  for (int i = 0; i < sharedMats2.size(); i++)
    if (sharedMats2[i] && stricmp(sharedMats2[i]->className, class_name) == NULL && sharedMats2[i]->emis == emis &&
        sharedMats2[i]->amb == amb_emis && sharedMats2[i]->power == power)
      return sharedMats2[i];

  StaticGeometryMaterial *gm = new StaticGeometryMaterial;
  gm->name = "light_material";
  gm->className = class_name;
  gm->scriptText = "emission=1000";
  gm->diff = emis;
  gm->amb = amb_emis;
  gm->emis = emis;
  gm->spec = emis;
  gm->power = power;
  gm->cosPower = power;
  gm->flags = MatFlags::FLG_USE_IN_GI;

  sharedMats2.push_back(gm);
  return gm;
}

static void generateRing(Tab<Point3> &vert, Tab<Point2> &tvert, int segments, real y)
{
  const real radius = 0.5 * sqrtf(1 - y * y * 4);

  vert.resize(segments);
  tvert.resize(segments + 1);

  const real step = TWOPI / segments;
  const real ty = acos(y * 2) / PI;
  real angle = 0;

  for (int i = 0; i < segments; ++i, angle += step)
  {
    vert[i] = Point3(radius * cos(angle), y, radius * sin(angle));
    tvert[i] = Point2(angle / TWOPI, ty);
  }

  tvert[segments] = Point2(angle / TWOPI, ty);
}

static void generateSphereMesh(MeshData &mesh, int segments, const Color4 &col)
{
  const int resizeStep = segments * 10;
  const int endRing = segments / 2 + 1;
  const real dy = 2.0 / segments;

  const real segAngle = PI / (endRing - 1);

  real y = 0.5;
  Tab<Point3> vert(tmpmem);
  Tab<Point2> tvert(tmpmem);

  Face face;
  face.smgr = 1;

  TFace tface;

  for (int ring = 0; ring < endRing; y = 0.5 * cosf(segAngle * ++ring))
  {
    // north pole
    if (!ring)
    {
      mesh.vert.push_back(Point3(0, 0.5, 0));
      mesh.tvert[0].push_back(Point2(0, 1));

      continue;
    }

    // south pole
    if (ring == endRing - 1)
    {
      mesh.vert.push_back(Point3(0, -0.5, 0));
      mesh.tvert[0].push_back(Point2(0, 0));

      continue;
    }

    // add ring vertexes
    generateRing(vert, tvert, segments, y);
    append_items(mesh.vert, segments, vert.data());
    append_items(mesh.tvert[0], segments + 1, tvert.data());

    if (ring > 1 && ring < endRing - 1)
    {
      for (int vi = mesh.vert.size() - segments, ti = mesh.tvert[0].size() - segments - 1; vi < mesh.vert.size(); ++vi, ++ti)
      {
        face.v[0] = vi;
        face.v[1] = vi - segments;
        face.v[2] = vi + 1;

        tface.t[0] = ti;
        tface.t[1] = ti - segments - 1;
        tface.t[2] = ti + 1;

        if (vi == mesh.vert.size() - 1)
          face.v[2] = mesh.vert.size() - segments;

        mesh.face.push_back(face);
        mesh.tface[0].push_back(tface);

        face.v[0] = vi - segments;
        face.v[1] = vi - segments + 1;
        face.v[2] = vi + 1;

        tface.t[0] = ti - segments - 1;
        tface.t[1] = ti - segments;
        tface.t[2] = ti + 1;

        if (vi == mesh.vert.size() - 1)
        {
          face.v[1] = mesh.vert.size() - segments * 2;
          face.v[2] = mesh.vert.size() - segments;
        }

        mesh.face.push_back(face);
        mesh.tface[0].push_back(tface);
      }
    }

    // next ring after north pole
    if (ring == 1)
    {
      for (int vi = 1; vi <= segments; ++vi)
      {
        face.v[0] = tface.t[0] = vi;
        face.v[1] = tface.t[1] = 0;
        face.v[2] = tface.t[2] = vi + 1;

        if (vi == segments)
          face.v[2] = 1;

        mesh.face.push_back(face);
        mesh.tface[0].push_back(tface);
      }
    }

    // ring before south pole
    if (ring == endRing - 2)
    {
      for (int vi = mesh.vert.size() - segments, ti = mesh.tvert[0].size() - segments - 1; vi < mesh.vert.size(); ++vi, ++ti)
      {
        face.v[0] = mesh.vert.size();
        face.v[1] = vi;
        face.v[2] = vi + 1;

        if (vi == mesh.vert.size() - 1)
          face.v[2] = mesh.vert.size() - segments;

        tface.t[0] = mesh.tvert[0].size();
        tface.t[1] = ti;
        tface.t[2] = ti + 1;

        mesh.face.push_back(face);
        mesh.tface[0].push_back(tface);
      }
    }
  }

  // set colors
  mesh.cvert.resize(1);
  mesh.cvert[0] = col;

  mesh.cface.resize(mesh.face.size());
  for (int fi = 0; fi < mesh.face.size(); ++fi)
  {
    mesh.cface[fi].t[0] = 0;
    mesh.cface[fi].t[1] = 0;
    mesh.cface[fi].t[2] = 0;
  }

  mesh.vert.shrink_to_fit();
  mesh.face.shrink_to_fit();
  mesh.tface[0].shrink_to_fit();
  mesh.facenorm.shrink_to_fit();

  mesh.calc_ngr();
  mesh.calc_vertnorms();
}

static int get_cos_power_from_ang(real alfa, real part, real &real_part)
{
  const int maxDegree = 100000;
  real alfaIters[(maxDegree - 1) / 2], oneIters[(maxDegree - 1) / 2], zeroIters[(maxDegree - 1) / 2];
  int d = 0;
  int k = 0;
  for (d = 1; d < maxDegree; d += 2)
  {
    real zeroIter = 0;
    real oneIter = square_rec(d, 1, d > 1 ? &oneIters[(d - 3) / 2] : NULL);
    real alfaIter = square_rec(d, sin(alfa), d > 1 ? &alfaIters[(d - 3) / 2] : NULL);
    alfaIters[k] = alfaIter;
    oneIters[k] = oneIter;
    zeroIters[k] = zeroIter;
    k++;
    real_part = (alfaIter - zeroIter) / (oneIter - zeroIter);
    if (real_part > part)
      break;
  }
  return d;
}
static real square_rec(int iter_no, real val, real *hashed_iter)
{
  if (iter_no == 1)
    return val;
  real mulRez = 1;
  real mul = 1 - val * val;
  for (int i = 0; i < (iter_no - 1) / 2; i++)
    mulRez *= mul;
  if (hashed_iter)
    return (val * mulRez - (1 - iter_no) * (*hashed_iter)) / iter_no;
  else
    return (val * mulRez - (1 - iter_no) * square_rec(iter_no - 2, val, NULL)) / iter_no;
}
