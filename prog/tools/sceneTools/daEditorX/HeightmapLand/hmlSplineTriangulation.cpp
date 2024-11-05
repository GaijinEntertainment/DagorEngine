// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlObjectsEditor.h"

#include <EditorCore/ec_IEditorCore.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <de3_genObjData.h>
#include <de3_lightService.h>
#include <de3_csgEntity.h>
#include <math/dag_math2d.h>
#include <math/misc/polyLineReduction.h>
#include <debug/dag_log.h>

// #include <debug/dag_debug.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

#define ERROR_ANGLE 5

static Point2 to_point2(const Point3 &p) { return Point2(p.x, p.z); }
static Point3 planeProj(const Point4 &plane, const Point2 &p)
{
  Point3 ret = Point3(p.x, 0, p.y);
  ret.y = (-plane.w - plane.x * p.x - plane.z * p.y) / plane.y;

  return ret;
}
static Point3 planeProj(const Point4 &plane, const Point3 &p) { return planeProj(plane, to_point2(p)); }

static bool triangulateArbitraryPoly(Tab<Point2> &pts, Tab<SplineObject::PolyGeom::Triangle> &tris, const char *poly_name);

int get_open_poly_intersect(dag::ConstSpan<Point2> poly, const Point2 &p0, const Point2 &p1, Point2 &ip)
{
  int idx = -1;
  for (int i = 0, ie = poly.size() - 1; i < ie; i++)
    if (get_lines_intersection(poly[i], poly[i + 1], p0, p1, &ip))
      idx = i;
  return idx;
}

int is_open_poly_intersect_excluding(dag::ConstSpan<Point2> poly, const Point2 &p0, const Point2 &p1, int excl_idx)
{
  Point2 ip;
  for (int i = 0, ie = poly.size() - 1; i < ie; i++)
  {
    if (i == excl_idx || i + 1 == excl_idx)
      continue;

    if (get_lines_intersection(poly[i], poly[i + 1], p0, p1, &ip))
      return i;
  }

  return -1;
}

int get_fence_intersect(dag::ConstSpan<Point2> poly, dag::ConstSpan<Point2> inner, dag::ConstSpan<int> in_idx, const Point2 &p0,
  const Point2 &p1)
{
  int idx = -1;
  for (int i = poly.size() - 1; i >= 0; i--)
    if (is_lines_intersect(poly[i], inner[in_idx[i]], p0, p1))
      idx = in_idx[i];
  return idx;
}

static void process_poly(dag::ConstSpan<Point2> poly, float bw, Tab<Point2> &inner_poly)
{
  Tab<Point2> inner(tmpmem);
  Tab<int> innerIdx(tmpmem);
  Point2 dir = normalize(poly[0] - poly.back());
  Point2 lastN(-dir.y, dir.x), nextN, avgN;
  Point2 inp, ipt;

  innerIdx.resize(poly.size());
  inner.reserve(poly.size());
  for (int i = 0; i < poly.size(); i++)
  {
    int i1 = (i + 1) % poly.size();
    dir = normalize(poly[i1] - poly[i]);
    nextN = Point2(-dir.y, dir.x);
    avgN = normalize(lastN + nextN);
    lastN = nextN;

    inp = poly[i] + avgN * (bw / 0.9);
    if (inner.size() < 2)
    {
      innerIdx[i] = inner.size();
      inner.push_back(inp);
    }
    else
    {
      int idx = get_open_poly_intersect(make_span_const(inner).first(inner.size() - 1), inner.back(), inp, ipt);
      int idx2 = get_fence_intersect(make_span_const(poly).first(i), inner, innerIdx, inp, poly[i]);

      // if (idx == -1)
      //   idx = get_open_poly_intersect(inner, poly[i], inp, ipt);

      if (idx2 >= 0)
      {
        idx = idx2;
        ipt = inner[idx];
      }

      if (idx == -1)
      {
        innerIdx[i] = inner.size();
        inner.push_back(inp);
      }
      else
      {
        inner[idx + 1] = ipt;
        innerIdx[i] = idx + 1;

        if (i == poly.size() - 1 && idx == 0)
        {
          inner[0] = ipt;
          inner[poly.size() - 1] = ipt;
        }
        else
        {
          erase_items(inner, idx + 2, inner.size() - idx - 2);
          for (int k = i - 1; k >= 0; k--)
            if (innerIdx[k] > idx + 1)
              innerIdx[k] = idx + 1;
            else
              break;
        }
      }
    }
  }

  inner_poly.resize(poly.size());
  for (int i = 0; i < poly.size(); i++)
    inner_poly[i] = poly[i] * 0.1 + inner[innerIdx[i]] * 0.9;
}


static real get_contour_length(dag::ConstSpan<Point3> cont)
{
  real ret = 0;
  for (int i = 0; i < cont.size(); i++)
  {
    int idx = (i + 1) % cont.size();
    ret += (cont[idx] - cont[i]).length();
  }

  return ret;
}


static void get_contour_positions(dag::ConstSpan<Point3> cont, Tab<real> &pos)
{
  pos.resize(cont.size());
  real ln = 0;
  pos[0] = 0;
  for (int i = 1; i < cont.size(); i++)
  {
    ln += (cont[i] - cont[i - 1]).length();
    pos[i] = ln;
  }
}


SplineObject::PolyGeom::PolyGeom() : centerTri(midmem), borderTri(midmem), verts(midmem)
{
  mainMesh = dagGeom->newGeomObject();
  borderMesh = dagGeom->newGeomObject();
  altGeom = false;
  bboxAlignStep = 1.0;
}

SplineObject::PolyGeom::~PolyGeom()
{
  clear();
  if (mainMesh)
    dagGeom->deleteGeomObject(mainMesh);

  if (borderMesh)
    dagGeom->deleteGeomObject(borderMesh);
}

void SplineObject::PolyGeom::clear()
{
  centerTri.clear();
  borderTri.clear();

  clear_and_shrink(verts);

  if (mainMesh)
    dagGeom->geomObjectClear(*mainMesh);

  if (borderMesh)
    dagGeom->geomObjectClear(*borderMesh);
}

void SplineObject::PolyGeom::renderLines()
{
  if (!verts.size())
    return;

  for (int i = 0; i < centerTri.size(); i++)
  {
    dagRender->renderLine(verts[centerTri[i][0]], verts[centerTri[i][1]], E3DCOLOR(255, 255, 0, 100));
    dagRender->renderLine(verts[centerTri[i][1]], verts[centerTri[i][2]], E3DCOLOR(255, 255, 0, 100));
    dagRender->renderLine(verts[centerTri[i][2]], verts[centerTri[i][0]], E3DCOLOR(255, 255, 0, 100));
  }

  for (int i = 0; i < borderTri.size(); i++)
  {
    dagRender->renderLine(verts[borderTri[i][0]], verts[borderTri[i][1]], E3DCOLOR(255, 0, 100, 100));
    dagRender->renderLine(verts[borderTri[i][1]], verts[borderTri[i][2]], E3DCOLOR(255, 0, 100, 100));
    dagRender->renderLine(verts[borderTri[i][2]], verts[borderTri[i][0]], E3DCOLOR(255, 0, 100, 100));
  }
}

void SplineObject::PolyGeom::createMeshes(const char *parent_name, SplineObject &spline)
{
  landclass::PolyGeomGenData &genGeom = *spline.landClass->data->genGeom;

  if (centerTri.size())
  {
    Ptr<MaterialData> sm = NULL;
    IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
    if (assetSrv)
      sm = assetSrv->getMaterialData(genGeom.mainMat);

    if (!sm)
    {
      DAEDITOR3.conError("can't find material %s", genGeom.mainMat.str());
      return;
    }

    // debug(">> shader: %s, class: %s",(char*)name, (char*)sm->className);

    int nn = centerTri.size();

    Mesh *mesh = dagGeom->newMesh();

    if (borderTri.size())
      mesh->vert = make_span_const(verts).first(verts.size() / 2);
    else
      mesh->vert = verts;

    mesh->face.resize(nn);

    for (int i = 0; i < nn; i++)
    {
      mesh->face[i].set(centerTri[i][0], centerTri[i][1], centerTri[i][2]);
      mesh->face[i].mat = 1;
    }

    for (int i = 0; i < genGeom.mainUV.size(); i++)
    {
      int ch = genGeom.mainUV[i].chanIdx - 1;
      mesh->tvert[ch].resize(mesh->vert.size());
      mesh->tface[ch].resize(nn);

      for (int j = 0; j < nn; j++)
      {
        mesh->tface[ch][j].t[0] = mesh->face[j].v[0];
        mesh->tface[ch][j].t[1] = mesh->face[j].v[1];
        mesh->tface[ch][j].t[2] = mesh->face[j].v[2];
      }

      int tp = genGeom.mainUV[i].type;
      if (tp == 0) // planar
      {
        Point2 sz = genGeom.mainUV[i].size;
        real rotW = genGeom.mainUV[i].rotationW;

        for (int j = 0; j < mesh->tvert[ch].size(); j++)
        {
          Point3 p = mesh->vert[j] + Point3(genGeom.mainUV[i].offset.x, 0, genGeom.mainUV[i].offset.y);

          mesh->tvert[ch][j] = genGeom.mainUV[i].swapUV ? Point2(p.z / sz.y, p.x / sz.x) : Point2(p.x / sz.x, p.z / sz.y);

          Point3 rr = Point3(mesh->tvert[ch][j].x, 0, mesh->tvert[ch][j].y);
          rr = rotyTM(DEG_TO_RAD * rotW) * rr;

          mesh->tvert[ch][j] = Point2(rr.x, rr.z);
        }
      }
    }


    mesh->calc_ngr();
    mesh->calc_vertnorms();
    for (int i = 0; i < mesh->vertnorm.size(); ++i)
      mesh->vertnorm[i] = -mesh->vertnorm[i];
    for (int i = 0; i < mesh->facenorm.size(); ++i)
      mesh->facenorm[i] = -mesh->facenorm[i];

    MaterialDataList *mdl = new MaterialDataList;
    mdl->addSubMat(sm);

    StaticGeometryContainer *g = dagGeom->newStaticGeometryContainer();

    String node_name(128, "poly mesh %s", parent_name);
    dagGeom->objCreator3dAddNode(node_name, mesh, mdl, *g);

    StaticGeometryContainer *geom = mainMesh->getGeometryContainer();
    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      node->flags = genGeom.flags | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;
      node->normalsDir = genGeom.normalsDir;
      if (genGeom.stage)
        node->script.setInt("stage", genGeom.stage);
      if (!genGeom.layerTag.empty())
        node->script.setStr("layerTag", genGeom.layerTag);

      dagGeom->staticGeometryNodeCalcBoundBox(*node);
      dagGeom->staticGeometryNodeCalcBoundSphere(*node);

      geom->addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
    }

    dagGeom->deleteStaticGeometryContainer(g);

    mainMesh->setTm(TMatrix::IDENT);

    recalcLighting(mainMesh);

    mainMesh->notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(*mainMesh);
    mainMesh->notChangeVertexColors(false);
  }

  if (borderTri.size())
  {
    Ptr<MaterialData> sm = NULL;
    IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
    if (assetSrv)
      sm = assetSrv->getMaterialData(genGeom.borderMat);

    if (!sm)
    {
      DAEDITOR3.conError("can't find material %s", genGeom.borderMat.str());
      return;
    }

    // debug(">> shader: %s, class: %s",(char*)name, (char*)sm->className);

    int nn = borderTri.size();

    Mesh *mesh = dagGeom->newMesh();
    mesh->vert = verts;
    mesh->setnumfaces(nn);

    for (int i = 0; i < nn; i++)
    {
      mesh->face[i].set(borderTri[i][0], borderTri[i][1], borderTri[i][2]);
      mesh->face[i].mat = 1;
    }

    for (int i = 0; i < genGeom.borderUV.size(); i++)
    {
      int ch = genGeom.borderUV[i].chanIdx - 1;
      mesh->tvert[ch].resize(mesh->vert.size());
      mesh->tface[ch].resize(nn);

      for (int j = 0; j < nn; j++)
      {
        mesh->tface[ch][j].t[0] = mesh->face[j].v[0];
        mesh->tface[ch][j].t[1] = mesh->face[j].v[1];
        mesh->tface[ch][j].t[2] = mesh->face[j].v[2];
      }

      int tp = genGeom.borderUV[i].type;
      if (tp == 0) // planar
      {
        Point2 sz = genGeom.borderUV[i].size;
        real rotW = genGeom.borderUV[i].rotationW;

        for (int j = 0; j < mesh->tvert[ch].size(); j++)
        {
          Point3 p = mesh->vert[j] + Point3(genGeom.borderUV[i].offset.x, 0, genGeom.borderUV[i].offset.y);

          mesh->tvert[ch][j] = genGeom.borderUV[i].swapUV ? Point2(p.z / sz.y, p.x / sz.x) : Point2(p.x / sz.x, p.z / sz.y);

          Point3 rr = Point3(mesh->tvert[ch][j].x, 0, mesh->tvert[ch][j].y);
          rr = rotyTM(DEG_TO_RAD * rotW) * rr;

          mesh->tvert[ch][j] = Point2(rr.x, rr.z);
        }
      }
      else if (tp == 1) // spline
      {
        real vsize = genGeom.borderUV[i].sizeV;

        real len = get_contour_length(make_span_const(verts).subspan(verts.size() / 2, verts.size() / 2));

        if (genGeom.borderUV[i].autotilelength)
        {
          int dl = (int)len % (int)vsize;
          int tln = len - dl;
          vsize = len * genGeom.borderUV[i].sizeV / tln;
        }

        Tab<real> contPos(tmpmem);
        get_contour_positions(make_span_const(verts).subspan(verts.size() / 2, verts.size() / 2), contPos);

        for (int j = 0; j < mesh->vert.size(); j++)
        {
          real tu, tv;
          if (j < mesh->vert.size() / 2) // inner contour
          {
            tv = contPos[j] / vsize;
            tu = 0;
          }
          else
          {
            tv = contPos[j - mesh->vert.size() / 2] / vsize;
            tu = genGeom.borderUV[i].tileU;
          }

          if (genGeom.borderUV[i].swapUV)
          {
            mesh->tvert[ch][j].x = tu;
            mesh->tvert[ch][j].y = tv;
          }
          else
          {
            mesh->tvert[ch][j].x = tv;
            mesh->tvert[ch][j].y = tu;
          }
        }
      }
    }

    mesh->calc_ngr();
    mesh->calc_vertnorms();
    for (int i = 0; i < mesh->vertnorm.size(); ++i)
      mesh->vertnorm[i] = -mesh->vertnorm[i];
    for (int i = 0; i < mesh->facenorm.size(); ++i)
      mesh->facenorm[i] = -mesh->facenorm[i];

    MaterialDataList *mdl = new MaterialDataList;

    mdl->addSubMat(sm);

    StaticGeometryContainer *g = dagGeom->newStaticGeometryContainer();

    String node_name(128, "poly mesh %s", parent_name);
    dagGeom->objCreator3dAddNode(node_name, mesh, mdl, *g);
    if (!g->nodes.size())
      return;

    StaticGeometryContainer *geom = borderMesh->getGeometryContainer();
    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      node->flags = genGeom.flags | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;
      node->normalsDir = genGeom.normalsDir;
      if (genGeom.stage)
        node->script.setInt("stage", genGeom.stage);
      if (!genGeom.layerTag.empty())
        node->script.setStr("layerTag", genGeom.layerTag);

      dagGeom->staticGeometryNodeCalcBoundBox(*node);
      dagGeom->staticGeometryNodeCalcBoundSphere(*node);

      geom->addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
    }

    dagGeom->deleteStaticGeometryContainer(g);

    borderMesh->setTm(TMatrix::IDENT);

    recalcLighting(borderMesh);

    borderMesh->notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(*borderMesh);
    borderMesh->notChangeVertexColors(false);
  }
}


void SplineObject::PolyGeom::recalcLighting(GeomObject *go)
{
  if (!go)
    return;

  const StaticGeometryContainer &geom = *go->getGeometryContainer();

  ISceneLightService *ltService = DAGORED2->queryEditorInterface<ISceneLightService>();

  if (!ltService)
    return;

  Color3 ltCol1, ltCol2, ambCol;
  Point3 ltDir1, ltDir2;

  ltService->getDirectLight(ltDir1, ltCol1, ltDir2, ltCol2, ambCol);

  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = geom.nodes[ni];

    if (node && node->mesh)
    {
      Mesh &mesh = node->mesh->mesh;

      mesh.cvert.resize(mesh.face.size() * 3);
      mesh.cface.resize(mesh.face.size());

      Point3 normal;

      for (int f = 0; f < mesh.face.size(); ++f)
      {
        for (unsigned v = 0; v < 3; ++v)
        {
          normal = mesh.vertnorm[mesh.facengr[f][v]];

          const int vi = f * 3 + v;

          Color3 resColor = ambCol;
          real k = normal * ltDir1;
          if (k > 0)
            resColor += ltCol1 * k;
          k = normal * ltDir2;
          if (k > 0)
            resColor += ltCol2 * k;

          mesh.cvert[vi] = ::color4(resColor, 1);
          mesh.cface[f].t[v] = vi;
        }
      }
    }
  }
}

void SplineObject::movePointByNormal(Point3 &p, int id, SplineObject *flatt_spl)
{
  landclass::PolyGeomGenData &genGeom = *landClass->data->genGeom;

  Tab<Point3> pts(tmpmem);
  pts.resize(points.size());
  for (int i = 0; i < points.size(); i++)
    pts[i] = points[i]->getPt();

  make_clockwise_coords(make_span(pts));

  Point3 pt0 = (id == 0 ? pts.back() : pts[id - 1]);
  Point3 pt1 = (id == pts.size() - 1 ? pts[0] : pts[id + 1]);

  Point3 first = p - pt0;
  Point3 second = pt1 - p;

  real projT = 0;
  Point3 ds1 = p - flatt_spl->getProjPoint(p, &projT);
  Point3 dst1 = flatt_spl->getBezierSpline().get_tang(projT);
  Point3 up = flatt_spl->isDirReversed(p, pt1) ? ds1 % dst1 : dst1 % ds1;

  Point3 norm1 = first % up;
  norm1.normalize();
  Point3 norm2 = second % up;
  norm2.normalize();

  Point3 resNorm = norm1 + norm2;
  resNorm.normalize();

  p += resNorm * genGeom.borderWidth;
}

bool SplineObject::calcPolyPlane(Point4 &plane)
{
  Point3 planeNorm = Point3(0, 0, 0);

  for (int i = 0; i < points.size(); i++)
  {
    Point3 pt0 = points[i == 0 ? points.size() - 1 : i - 1]->getPt();
    Point3 pt1 = points[(i + 1) % points.size()]->getPt();

    Point3 vec1 = points[i]->getPt() - pt0;
    vec1.normalize();
    Point3 vec2 = pt1 - points[i]->getPt();
    vec2.normalize();

    if (points.size() < 4)
    {
      real dot = vec1 * vec2;
      if (fabs(acos(dot)) < ERROR_ANGLE * DEG_TO_RAD)
      {
        DAEDITOR3.conError("Angle between segments %d and %d < %d degreeds", i - 1, i, ERROR_ANGLE);
        return false;
      }
    }

    Point3 cross = vec1 % vec2;
    if (cross.y < 0)
      cross = -cross;

    cross.normalize();

    planeNorm += cross;
  }

  planeNorm.normalize();

  Point3 sc = splineCenter();

  real planeD = -sc.x * planeNorm.x - sc.y * planeNorm.y - sc.z * planeNorm.z;
  plane = Point4(planeNorm.x, planeNorm.y, planeNorm.z, planeD);

  return true;
}

void SplineObject::triangulatePoly()
{
  if (polyGeom.mainMesh || polyGeom.borderMesh)
    HmapLandObjectEditor::geomBuildCntPoly++;

  polyGeom.clear();

  int n = points.size();
  if (n < 3)
    return;

  if (landClass && landClass->data && landClass->data->csgGen)
  {
    destroy_it(csgGen);
    csgGen = DAEDITOR3.cloneEntity(landClass->data->csgGen);
    csgGen->setSubtype(polygonSubtypeMask);
    csgGen->setTm(points[0]->getWtm());
    if (ICsgEntity *c = csgGen->queryInterface<ICsgEntity>())
    {
      Tab<Point3> p;
      p.resize(points.size() - (isClosed() ? 1 : 0));
      for (int i = 0; i < p.size(); i++)
        p[i] = points[i]->getPt();
      c->setFoundationPath(make_span(p), isClosed());
    }
  }

  if (!landClass || !landClass->data || !landClass->data->genGeom)
    return;
  if (landClass->data->genGeom->waterSurface)
    return;

  landclass::PolyGeomGenData &genGeom = *landClass->data->genGeom;
  bool border = (fabs(genGeom.borderWidth) > 1e-3);

  if (flattenBySpline)
  {
    SplineObject *s = flattenBySpline;

    for (int i = 0; i < points.size(); i++)
    {
      Point3 ddest = s->getProjPoint(points[i]->getPt());

      Point3 npos = points[i]->getPt();
      npos.y = ddest.y;
      points[i]->setPos(npos);
    }

    Point3 first = points[0]->getPt();
    Point3 second = points[1]->getPt();

    Point3 last1 = first, last2 = s->getProjPoint(first);
    Point3 lastb1 = first;

    for (int i = 1; i < points.size(); i++)
    {
      if (!s->lineIntersIgnoreY(first, second))
      {
        Point3 p1 = first;
        Point3 dpP1 = p1;

        real startT = 0;
        Point3 sp1 = s->getProjPoint(p1, &startT);

        if (border)
          movePointByNormal(p1, i, flattenBySpline);

        Point3 p2 = second;
        Point3 dpP2 = p2;

        real endT = 0;
        Point3 sp2 = s->getProjPoint(p2, &endT);

        if (border)
        {
          int tid = (i == points.size() - 1 ? 0 : i + 1);
          bool extr = false;

          if (!extr)
            movePointByNormal(p2, tid, flattenBySpline);
        }

        int isteps = 10;
        real step = (endT - startT) / (real)isteps;

        bool reverse = false;
        if (startT > endT)
          reverse = true;

        if (fabs(step) > 1e-3)
        {
          int j = 0;
          for (real t = startT; reverse ? t >= endT : t <= endT; t += step, j++)
          {
            real dt = (real)j / (real)isteps;
            Point3 pp1 = p1 * (1 - dt) + p2 * dt;
            Point3 pp2 = s->getBezierSpline().get_pt(t);

            Point3 newb1 = dpP1 * (1 - dt) + dpP2 * dt;

            int idx = polyGeom.verts.size();
            polyGeom.verts.push_back(last1);
            polyGeom.verts.push_back(last2);
            polyGeom.verts.push_back(pp2);
            polyGeom.verts.push_back(pp1);

            PolyGeom::Triangle *ptri = &polyGeom.centerTri[append_items(polyGeom.centerTri, 2)];
            ptri[0][0] = idx + 0;
            ptri[0][1] = idx + 1;
            ptri[0][2] = idx + 2;
            ptri[1][0] = idx + 2;
            ptri[1][1] = idx + 3;
            ptri[1][2] = idx + 0;

            last1 = pp1;
            last2 = pp2;

            lastb1 = newb1;
          }
        }
      }

      first = second;
      second = (i == points.size() - 1 ? points[0]->getPt() : points[i + 1]->getPt());
    }
  }
  else
  {
    Tab<Point3> pts_3(tmpmem);
    Tab<Point2> pts(tmpmem);
    getSmoothPoly(pts_3);

    n = pts_3.size();
    pts.resize(n);
    for (int i = 0; i < n; i++)
      pts[i].set_xz(pts_3[i]);

    bool reverced = (triang_area(pts) < 0.0f);
    if (reverced)
    {
      Tab<Point2> npts(tmpmem);
      npts.resize(pts.size());
      for (int i = 0; i < n; i++)
        npts[i] = pts[n - i - 1];
      pts = npts;
    }

    Point4 plane;
    if (!calcPolyPlane(plane))
      return;

    if (!border)
    {
      if (!triangulateArbitraryPoly(pts, polyGeom.centerTri, getName()))
      {
        if (pts.size() > 3)
          DAEDITOR3.conError("can't triangulate poly: %d pts", pts.size());
        return;
      }
      for (int i = 0; i < pts_3.size(); i++)
        polyGeom.verts.push_back(pts_3[reverced ? pts_3.size() - i - 1 : i]);
    }
    else
    {
      Point2 intersection;
      for (int i = 0; i < n; i++)
      {
        Point2 pt0 = pts[i];
        Point2 pt1 = pts[i == n - 1 ? 0 : i + 1];

        for (int k = 0; k < n; k++)
        {
          if (k == i || k == i + 1 || k + 1 == i)
            continue;

          Point2 pt2 = pts[k];
          Point2 pt3 = pts[k == n - 1 ? 0 : k + 1];

          if (get_lines_intersection(pt0, pt1, pt2, pt3, &intersection))
          {
            DAEDITOR3.conError("There are self-intersections on segments %d and %d", i, k);
            return;
          }
        }
      }

      // make border

      Tab<Point2> npts(tmpmem);
      process_poly(pts, genGeom.borderWidth, npts);

      if (!npts.size())
        return;

      for (int i = 0; i < npts.size(); i++)
        polyGeom.verts.push_back(planeProj(plane, npts[i]));
      polyGeom.verts.resize(polyGeom.verts.size() + 1);
      polyGeom.verts.back() = polyGeom.verts[0];

      if (!triangulateArbitraryPoly(npts, polyGeom.centerTri, getName()))
      {
        if (pts.size() > 3)
          DAEDITOR3.conError("can't triangulate poly(border): %d pts", pts.size());
        return;
      }

      int offs = npts.size() + 1;

      for (int i = 0; i < pts.size(); i++)
        polyGeom.verts.push_back(planeProj(plane, pts[i]));
      polyGeom.verts.resize(polyGeom.verts.size() + 1);
      polyGeom.verts.back() = polyGeom.verts[offs];

      int invalidTriangles = 0;

      for (int i = 0; i < pts.size(); i++)
      {
        int idx1 = i, idx2 = offs + i + 1, idx3 = offs + i;
        int idx4 = i, idx5 = i + 1, idx6 = offs + i + 1;
        int idx = is_open_poly_intersect_excluding(pts, to_point2(polyGeom.verts[idx1]), to_point2(polyGeom.verts[idx2]), i + 1);
        if (idx >= 0)
        {
          invalidTriangles++;
          logerr("selfcross %d: seg %d, " FMT_P2 "-" FMT_P2 "", invalidTriangles, idx, P2D(pts[idx]),
            P2D(pts[(idx + 1) % pts.size()]));
          // continue;
        }

        PolyGeom::Triangle *ptri = &polyGeom.borderTri[append_items(polyGeom.borderTri, 2)];
        ptri[0][0] = i;
        ptri[0][1] = offs + i + 1;
        ptri[0][2] = offs + i;

        ptri[1][0] = i;
        ptri[1][1] = i + 1;
        ptri[1][2] = offs + i + 1;
      }

      if (invalidTriangles)
        DAEDITOR3.conError("There are invalid triangles in polygon %s,"
                           " %d segments, see details in debug",
          getName(), invalidTriangles);
    }
  }

  polyGeom.createMeshes(getName(), *this);
  if (polyGeom.mainMesh || polyGeom.borderMesh)
    HmapLandObjectEditor::geomBuildCntPoly++;
}

void SplineObject::buildInnerSpline(Tab<Point3> &out_pts, float offset, float pts_y, float eps)
{
  int n = points.size();
  if (n < 3)
    return;

  Tab<Point3> pts_3(tmpmem);
  Tab<Point2> pts(tmpmem);
  getSmoothPoly(pts_3);

  n = pts_3.size();
  pts.resize(n);
  for (int i = 0; i < n; i++)
    pts[i].set_xz(pts_3[i]);

  bool rev = (triang_area(pts) < 0.0f);
  if (rev)
    for (int i = 0; i < n; i++)
      pts[i].set_xz(pts_3[n - i - 1]);

  Point2 intersection;
  for (int i = 0; i < n; i++)
  {
    Point2 pt0 = pts[i];
    Point2 pt1 = pts[i == n - 1 ? 0 : i + 1];

    for (int k = 0; k < n; k++)
    {
      if (k == i || k == i + 1 || k + 1 == i)
        continue;

      Point2 pt2 = pts[k];
      Point2 pt3 = pts[k == n - 1 ? 0 : k + 1];

      if (get_lines_intersection(pt0, pt1, pt2, pt3, &intersection))
      {
        DAEDITOR3.conError("There are self-intersections on segments %d and %d", i, k);
        return;
      }
    }
  }

  // make border
  Tab<Point2> npts(tmpmem);
  process_poly(pts, offset, npts);

  if (!npts.size())
    return;

  if (eps < 1e-6)
  {
    out_pts.resize(npts.size());
    for (int i = 0; i < npts.size(); i++)
      out_pts[i].set_xVy(npts[i], pts_y);
    return;
  }

  {
    Tab<double> pt(tmpmem);
    Tab<int> ptmark(tmpmem);
    int n = npts.size(), original_n = n;
    pt.resize(n * 2);
    ptmark.resize(n);
    mem_set_0(ptmark);

    for (int i = 0; i < n; i++)
    {
      pt[i] = npts[i].x;
      pt[i + n] = npts[i].y;
    }
    ReducePoints(pt.data(), pt.data() + n, n, ptmark.data(), eps);

    out_pts.clear();
    out_pts.reserve(n);
    for (int i = 0; i < n; i++)
      if (ptmark[i])
        out_pts.push_back().set(pt[i], pts_y, pt[i + n]);
  }
}

static bool triangulateArbitraryPoly(Tab<Point2> &pts, Tab<SplineObject::PolyGeom::Triangle> &tris, const char *poly_name)
{
  int nv = pts.size();

  int *V = new int[nv];
  for (int i = 0; i < nv; i++)
    V[i] = i;

  /*  remove nv-2 Vertices, creating 1 triangle every time */
  int count = 2 * nv;

  /* error detection */
  for (int m = 0, v = nv - 1; nv > 2;)
  {
    /* if we loop, it is probably a non-simple polygon */
    if (0 >= (count--))
    {
      DAEDITOR3.conError(">> ERROR: bad polygon, can't triangulate %s", poly_name);
      return false;
    }

    /* three consecutive vertices in current polygon, <u,v,w> */
    int u = v;
    if (nv <= u)
      u = 0; /* previous */

    v = u + 1;
    if (nv <= v)
      v = 0; /* new v    */

    int w = v + 1;
    if (nv <= w)
      w = 0; /* next     */

    if (triang_snip(pts, u, v, w, nv, V))
    {
      int a, b, c, s, t;

      /* true names of the vertices */
      a = V[u];
      b = V[v];
      c = V[w];

      /* output Triangle */

      SplineObject::PolyGeom::Triangle &ptri = tris.push_back();
      ptri[0] = c;
      ptri[1] = b;
      ptri[2] = a;

      m++;

      /* remove v from remaining polygon */
      for (s = v, t = v + 1; t < nv; s++, t++)
        V[s] = V[t];

      nv--;

      /* resest error detection counter */
      count = 2 * nv;
    }
  }
  delete[] V;
  return true;
}
