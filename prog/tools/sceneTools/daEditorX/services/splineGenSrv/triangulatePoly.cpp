// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "poly.h"
#include "commonMath.h"
#include "spline.h"

#include <de3_genObjData.h>
#include <math/dag_math2d.h>
#include <math/misc/polyLineReduction.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/ObjCreator3d/objCreator3d.h>

#define ERROR_ANGLE 5

static Point3 project_to_plane(const Point4 &plane, const Point2 &p)
{
  return Point3::xVy(p, (-plane.w - plane.x * p.x - plane.z * p.y) / plane.y);
}
static Point3 project_to_plane(const Point4 &plane, const Point3 &p) { return project_to_plane(plane, Point2::xz(p)); }

static bool triangulate_arbitrary_poly(Tab<Point2> &pts, Tab<IPolygonGenObj::Geom::Triangle> &tris, const char *poly_name);

static int get_open_poly_intersect(dag::ConstSpan<Point2> poly, const Point2 &p0, const Point2 &p1, Point2 &ip)
{
  int idx = -1;
  for (int i = 0, ie = poly.size() - 1; i < ie; i++)
    if (get_lines_intersection(poly[i], poly[i + 1], p0, p1, &ip))
      idx = i;
  return idx;
}

static int is_open_poly_intersect_excluding(dag::ConstSpan<Point2> poly, const Point2 &p0, const Point2 &p1, int excl_idx)
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

static int get_fence_intersect(dag::ConstSpan<Point2> poly, dag::ConstSpan<Point2> inner, dag::ConstSpan<int> in_idx, const Point2 &p0,
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


void PolygonGenEntity::clear_poly_geom(Geom &g)
{
  g.centerTri.clear();
  g.borderTri.clear();

  clear_and_shrink(g.verts);

  if (g.mainMesh)
    g.mainMesh->clear();

  if (g.borderMesh)
    g.borderMesh->clear();
}

static void create_meshes(IPolygonGenObj::Geom &g, const char *parent_name, landclass::AssetData *land)
{
  landclass::PolyGeomGenData &genGeom = *land->genGeom;
  auto &mainMesh = g.mainMesh, &borderMesh = g.borderMesh;
  auto &centerTri = g.centerTri, &borderTri = g.borderTri;
  auto &verts = g.verts;

  if (centerTri.size())
  {
    Ptr<MaterialData> sm = NULL;
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (assetSrv)
      sm = assetSrv->getMaterialData(genGeom.mainMat);

    if (!sm)
    {
      DAEDITOR3.conError("can't find material %s", genGeom.mainMat.str());
      return;
    }

    // debug(">> shader: %s, class: %s",(char*)name, (char*)sm->className);

    int nn = centerTri.size();

    Mesh *mesh = new Mesh;

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

    StaticGeometryContainer *g = new StaticGeometryContainer;

    String node_name(128, "poly mesh %s", parent_name);
    ObjCreator3d::addNode(node_name, mesh, mdl, *g);

    StaticGeometryContainer *geom = mainMesh->getGeometryContainer();
    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      node->flags = genGeom.flags | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;
      node->normalsDir = genGeom.normalsDir;
      if (genGeom.stage)
        node->script.setInt("stage", node->stage = genGeom.stage);
      if (!genGeom.layerTag.empty())
        node->script.setStr("layerTag", genGeom.layerTag);

      node->calcBoundBox();
      node->calcBoundSphere();

      geom->addNode(new (tmpmem) StaticGeometryNode(*node));
    }

    delete g;

    mainMesh->setTm(TMatrix::IDENT);

    SplineGenEntity::recalcGeomLighting(*mainMesh->getGeometryContainer());

    mainMesh->notChangeVertexColors(true);
    mainMesh->recompile();
    mainMesh->notChangeVertexColors(false);
  }

  if (borderTri.size())
  {
    Ptr<MaterialData> sm = NULL;
    IAssetService *assetSrv = EDITORCORE->queryEditorInterface<IAssetService>();
    if (assetSrv)
      sm = assetSrv->getMaterialData(genGeom.borderMat);

    if (!sm)
    {
      DAEDITOR3.conError("can't find material %s", genGeom.borderMat.str());
      return;
    }

    // debug(">> shader: %s, class: %s",(char*)name, (char*)sm->className);

    int nn = borderTri.size();

    Mesh *mesh = new Mesh;
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

    StaticGeometryContainer *g = new StaticGeometryContainer;

    String node_name(128, "poly mesh %s", parent_name);
    ObjCreator3d::addNode(node_name, mesh, mdl, *g);
    if (!g->nodes.size())
      return;

    StaticGeometryContainer *geom = borderMesh->getGeometryContainer();
    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      node->flags = genGeom.flags | StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS;
      node->normalsDir = genGeom.normalsDir;
      if (genGeom.stage)
        node->script.setInt("stage", node->stage = genGeom.stage);
      if (!genGeom.layerTag.empty())
        node->script.setStr("layerTag", genGeom.layerTag);

      node->calcBoundBox();
      node->calcBoundSphere();

      geom->addNode(new (tmpmem) StaticGeometryNode(*node));
    }

    delete g;

    borderMesh->setTm(TMatrix::IDENT);

    SplineGenEntity::recalcGeomLighting(*borderMesh->getGeometryContainer());

    borderMesh->notChangeVertexColors(true);
    borderMesh->recompile();
    borderMesh->notChangeVertexColors(false);
  }
}

static Point3 calc_spline_center(dag::ConstSpan<Point3> points)
{
  if (!points.size())
    return Point3(0, 0, 0);

  BBox3 box;
  for (int i = 0; i < points.size(); i++)
    box += points[i];

  BSphere3 sph;
  sph = box;

  return sph.c;
}

static Point3 find_closest_point_on_spline(const BezierSpline3d &bezierSpline, const Point3 &p, real *proj_t)
{
  real splineLen = bezierSpline.getLength(), step_dt = splineLen / 1024.f;
  if (splineLen <= 0.f)
    return Point3(0.f, 0.f, 0.f);

  real ret_t = 0.0f;
  Point3 ret_pt = bezierSpline.get_pt(ret_t);
  real min_dist_sq = lengthSq(ret_pt - p);

  for (real t = step_dt; t <= splineLen; t += step_dt)
  {
    Point3 pt1 = bezierSpline.get_pt(t);
    real dist_sq = lengthSq(pt1 - p);
    if (dist_sq < min_dist_sq)
    {
      min_dist_sq = dist_sq;
      ret_pt = pt1;
      ret_t = t;
    }
  }

  if (proj_t)
    *proj_t = ret_t;
  return ret_pt;
}

static bool is_dir_reversed(const BezierSpline3d &bezierSpline, Point3 &p1, Point3 &p2)
{
  real t1 = 0, t2 = 1;
  find_closest_point_on_spline(bezierSpline, p1, &t1);
  find_closest_point_on_spline(bezierSpline, p2, &t2);
  return t1 > t2;
}

static bool check_line_crosses_spline_xz(const BezierSpline3d &bezierSpline, Point3 &p1, Point3 &p2)
{
  for (int i = 0; i < (int)bezierSpline.segs.size() - 1; i++)
  {
    Point3 pp1 = bezierSpline.segs[i].point(0.f);
    Point3 pp2 = bezierSpline.segs[i + 1].point(0.f);

    if (lines_inters_ignore_Y(p1, p2, pp1, pp2))
      return true;
  }

  return false;
}

static void move_point_towards_normal(dag::ConstSpan<Point3> points, landclass::AssetData *land, Point3 &p, int id,
  const BezierSpline3d &flatt_spl)
{
  landclass::PolyGeomGenData &genGeom = *land->genGeom;

  Tab<Point3> pts(tmpmem);
  pts = points;
  make_clockwise_coords(make_span(pts));

  Point3 pt0 = (id == 0 ? pts.back() : pts[id - 1]);
  Point3 pt1 = (id == pts.size() - 1 ? pts[0] : pts[id + 1]);

  Point3 first = p - pt0;
  Point3 second = pt1 - p;

  real projT = 0;
  Point3 ds1 = p - find_closest_point_on_spline(flatt_spl, p, &projT);
  Point3 dst1 = flatt_spl.get_tang(projT);
  Point3 up = is_dir_reversed(flatt_spl, p, pt1) ? ds1 % dst1 : dst1 % ds1;

  Point3 norm1 = first % up;
  norm1.normalize();
  Point3 norm2 = second % up;
  norm2.normalize();

  Point3 resNorm = norm1 + norm2;
  resNorm.normalize();

  p += resNorm * genGeom.borderWidth;
}

static bool calc_poly_plane(dag::ConstSpan<Point3> points, Point4 &plane, const char *poly_name)
{
  Point3 planeNorm = Point3(0, 0, 0);

  for (int i = 0; i < points.size(); i++)
  {
    Point3 pt0 = points[i == 0 ? points.size() - 1 : i - 1];
    Point3 pt1 = points[(i + 1) % points.size()];

    Point3 vec1 = points[i] - pt0;
    vec1.normalize();
    Point3 vec2 = pt1 - points[i];
    vec2.normalize();

    if (points.size() < 4)
    {
      real dot = vec1 * vec2;
      if (fabs(safe_acos(dot)) < ERROR_ANGLE * DEG_TO_RAD)
      {
        DAEDITOR3.conError("%s: Angle between segments %d and %d < %d degreeds (point[%d]=%@, angle=%g, points.size()=%d)", //
          poly_name, i - 1, i, ERROR_ANGLE, i, points[i], safe_acos(dot) * RAD_TO_DEG, points.size());
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

  Point3 sc = calc_spline_center(points);

  real planeD = -sc.x * planeNorm.x - sc.y * planeNorm.y - sc.z * planeNorm.z;
  plane = Point4(planeNorm.x, planeNorm.y, planeNorm.z, planeD);

  return true;
}

bool PolygonGenEntity::triangulate_poly(Geom &polyGeom, dag::ConstSpan<Point3> poly_pts, landclass::AssetData *land, const Props &,
  const char *poly_name, const BezierSpline3d *flattenBySpline)
{
  PolygonGenEntity::clear_poly_geom(polyGeom);

  int n = poly_pts.size();
  if (n < 3)
    return false;

  if (!land || !land->genGeom)
    return false;
  if (land->genGeom->waterSurface)
    return false;

  landclass::PolyGeomGenData &genGeom = *land->genGeom;
  bool border = (fabs(genGeom.borderWidth) > 1e-3);

  if (flattenBySpline)
  {
    Tab<Point3> points(poly_pts);
    for (int i = 0; i < points.size(); i++)
    {
      Point3 ddest = find_closest_point_on_spline(*flattenBySpline, points[i], nullptr);

      Point3 npos = points[i];
      npos.y = ddest.y;
      points[i] = npos;
    }

    Point3 first = points[0];
    Point3 second = points[1];

    Point3 last1 = first, last2 = find_closest_point_on_spline(*flattenBySpline, first, nullptr);
    Point3 lastb1 = first;

    for (int i = 1; i < points.size(); i++)
    {
      if (!check_line_crosses_spline_xz(*flattenBySpline, first, second))
      {
        Point3 p1 = first;
        Point3 dpP1 = p1;

        real startT = 0;
        Point3 sp1 = find_closest_point_on_spline(*flattenBySpline, p1, &startT);

        if (border)
          move_point_towards_normal(poly_pts, land, p1, i, *flattenBySpline);

        Point3 p2 = second;
        Point3 dpP2 = p2;

        real endT = 0;
        Point3 sp2 = find_closest_point_on_spline(*flattenBySpline, p2, &endT);

        if (border)
        {
          int tid = (i == points.size() - 1 ? 0 : i + 1);
          bool extr = false;

          if (!extr)
            move_point_towards_normal(poly_pts, land, p2, tid, *flattenBySpline);
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
            Point3 pp2 = flattenBySpline->get_pt(t);

            Point3 newb1 = dpP1 * (1 - dt) + dpP2 * dt;

            int idx = polyGeom.verts.size();
            polyGeom.verts.push_back(last1);
            polyGeom.verts.push_back(last2);
            polyGeom.verts.push_back(pp2);
            polyGeom.verts.push_back(pp1);

            IPolygonGenObj::Geom::Triangle *ptri = &polyGeom.centerTri[append_items(polyGeom.centerTri, 2)];
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
      second = (i == points.size() - 1 ? points[0] : points[i + 1]);
    }
  }
  else
  {
    Tab<Point2> pts(tmpmem);
    pts.resize(n);
    for (int i = 0; i < n; i++)
      pts[i].set_xz(poly_pts[i]);

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
    if (!calc_poly_plane(poly_pts, plane, poly_name))
      return false;

    if (!border)
    {
      if (!triangulate_arbitrary_poly(pts, polyGeom.centerTri, poly_name))
      {
        if (pts.size() > 3)
          DAEDITOR3.conError("can't triangulate poly: %d pts", pts.size());
        return false;
      }
      for (int i = 0; i < poly_pts.size(); i++)
        polyGeom.verts.push_back(poly_pts[reverced ? poly_pts.size() - i - 1 : i]);
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
            return false;
          }
        }
      }

      // make border

      Tab<Point2> npts(tmpmem);
      process_poly(pts, genGeom.borderWidth, npts);

      if (!npts.size())
        return false;

      for (int i = 0; i < npts.size(); i++)
        polyGeom.verts.push_back(project_to_plane(plane, npts[i]));
      polyGeom.verts.resize(polyGeom.verts.size() + 1);
      polyGeom.verts.back() = polyGeom.verts[0];

      if (!triangulate_arbitrary_poly(npts, polyGeom.centerTri, poly_name))
      {
        if (pts.size() > 3)
          DAEDITOR3.conError("can't triangulate poly(border): %d pts", pts.size());
        return false;
      }

      int offs = npts.size() + 1;

      for (int i = 0; i < pts.size(); i++)
        polyGeom.verts.push_back(project_to_plane(plane, pts[i]));
      polyGeom.verts.resize(polyGeom.verts.size() + 1);
      polyGeom.verts.back() = polyGeom.verts[offs];

      int invalidTriangles = 0;

      for (int i = 0; i < pts.size(); i++)
      {
        int idx1 = i, idx2 = offs + i + 1, idx3 = offs + i;
        int idx4 = i, idx5 = i + 1, idx6 = offs + i + 1;
        int idx = is_open_poly_intersect_excluding(pts, Point2::xz(polyGeom.verts[idx1]), Point2::xz(polyGeom.verts[idx2]), i + 1);
        if (idx >= 0)
        {
          invalidTriangles++;
          logerr("selfcross %d: seg %d, " FMT_P2 "-" FMT_P2 "", invalidTriangles, idx, P2D(pts[idx]),
            P2D(pts[(idx + 1) % pts.size()]));
          // continue;
        }

        IPolygonGenObj::Geom::Triangle *ptri = &polyGeom.borderTri[append_items(polyGeom.borderTri, 2)];
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
          poly_name, invalidTriangles);
    }
  }

  create_meshes(polyGeom, poly_name, land);
  return (polyGeom.mainMesh || polyGeom.borderMesh);
}

bool PolygonGenEntity::build_inner_spline(Tab<Point3> &out_pts, dag::ConstSpan<Point3> poly_pts, float offset, float pts_y, float eps)
{
  int n = poly_pts.size();
  out_pts.clear();
  if (n < 3)
    return false;

  Tab<Point2> pts(tmpmem);

  n = poly_pts.size();
  pts.resize(n);
  for (int i = 0; i < n; i++)
    pts[i].set_xz(poly_pts[i]);

  bool rev = (triang_area(pts) < 0.0f);
  if (rev)
    for (int i = 0; i < n; i++)
      pts[i].set_xz(poly_pts[n - i - 1]);

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
        return false;
      }
    }
  }

  // make border
  Tab<Point2> npts(tmpmem);
  process_poly(pts, offset, npts);

  if (!npts.size())
    return false;

  if (eps < 1e-6)
  {
    out_pts.resize(npts.size());
    for (int i = 0; i < npts.size(); i++)
      out_pts[i].set_xVy(npts[i], pts_y);
    return false;
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
  return true;
}

static bool triangulate_arbitrary_poly(Tab<Point2> &pts, Tab<IPolygonGenObj::Geom::Triangle> &tris, const char *poly_name)
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

      IPolygonGenObj::Geom::Triangle &ptri = tris.push_back();
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
