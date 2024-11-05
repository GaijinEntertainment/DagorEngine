// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <meshadj.h>
#include <spline3d.h>
#include <splshape.h>
#include "enumnode.h"

void extrude_spline(TimeValue time, INode &node, BezierShape &bshp, Mesh &m, float ht, float tile, float grav, int segs, int steps,
  bool up, bool usetang)
{
  Matrix3 wtm = node.GetObjectTM(time);
  if (segs < 1)
    segs = 1;
  PolyShape sh;
  bshp.MakePolyShape(sh, steps, TRUE);
  Point3 dir;
  if (up)
    dir = Point3(0, 0, 1);
  else
    dir = Normalize(node.GetNodeTM(time).GetRow(2));
  ht /= segs;
  grav /= segs;
  m.setMapSupport(1);
  for (int ln = 0; ln < sh.numLines; ++ln)
  {
    PolyLine &pl = sh.lines[ln];
    int np = pl.numPts;
    if (np < 2)
      continue;
    int f0 = m.numFaces;
    int v0 = m.numVerts;
    m.setNumFaces(f0 + (np - 1) * segs * 2, TRUE);
    m.setMapSupport(1);
    TVFace *tf = m.mapFaces(1);
    assert(tf);
    int i;
    for (i = 0; i < np - 1; ++i)
    {
      for (int j = 0; j < segs; ++j)
      {
        int v = v0 + i * (segs + 1) + j;
        int f = f0 + (i * segs + j) * 2;
        m.faces[f].v[0] = tf[f].t[0] = v;
        m.faces[f].v[1] = tf[f].t[1] = v + segs + 1;
        m.faces[f].v[2] = tf[f].t[2] = v + 1;
        m.faces[f].smGroup = 1;
        m.faces[f].flags = EDGE_A | EDGE_C;
        m.faces[f + 1].v[0] = tf[f + 1].t[0] = v + segs + 1;
        m.faces[f + 1].v[1] = tf[f + 1].t[1] = v + segs + 1 + 1;
        m.faces[f + 1].v[2] = tf[f + 1].t[2] = v + 1;
        m.faces[f + 1].smGroup = 1;
        m.faces[f + 1].flags = EDGE_A | EDGE_B;
      }
    }
    m.setNumVerts(v0 + np * (segs + 1), TRUE);
    m.setNumMapVerts(1, v0 + np * (segs + 1), TRUE);
    Tab<Point3> pt;
    Tab<float> tu;
    pt.SetCount(np);
    tu.SetCount(np);
    for (i = 0; i < np; ++i)
      pt[i] = wtm * pl.pts[i].p;
    tu[0] = 0;
    for (i = 1; i < np; ++i)
      tu[i] = tu[i - 1] + Length(pt[i] - pt[i - 1]) / tile;
    Point3 *tv = m.mapVerts(1);
    assert(tv);
    for (i = 0; i < np; ++i)
    {
      Point3 p = pt[i];
      Point3 dh;
      if (usetang)
      {
        Point3 tg;
        if (i == 0)
          tg = pt[1] - p;
        else if (i == np - 1)
          tg = p - pt[i - 1];
        else
          tg = pt[i + 1] - pt[i - 1];
        tg = Normalize(tg);
        Point3 bn = Normalize(CrossProd(dir, tg));
        dh = Normalize(CrossProd(tg, bn));
        if (dh == Point3(0, 0, 0))
          dh = dir;
      }
      else
        dh = dir;
      for (int j = 0; j <= segs; ++j)
      {
        int v = v0 + i * (segs + 1) + j;
        m.verts[v] = p;
        tv[v] = Point3(tu[i], float(j) / segs, 0.f);
        dh = Normalize(dh + Point3(0.f, 0.f, -grav));
        p += dh * ht;
      }
    }
  }
}

struct GetMeshesCB : public ENodeCB
{
  TimeValue time;
  INode *snode;
  Box3 sbox;
  struct Obj
  {
    Mesh m;
    AdjFaceList *af;
    Box3 box;
    Obj(Mesh &sm, Matrix3 &tm, Box3 &b) : m(sm), box(b)
    {
      for (int i = 0; i < m.numVerts; ++i)
        m.verts[i] = tm * m.verts[i];
      m.InvalidateGeomCache();
      AdjEdgeList ae(m);
      af = new AdjFaceList(m, ae);
      assert(af);
    }
    ~Obj()
    {
      if (af)
        delete af;
    }
  };
  Tab<Obj *> mesh;

  GetMeshesCB(Interface *ip, INode *sn, Box3 sb)
  {
    time = ip->GetTime();
    snode = sn;
    sbox = sb;
  }
  ~GetMeshesCB()
  {
    for (int i = 0; i < mesh.Count(); ++i)
      if (mesh[i])
        delete (mesh[i]);
  }
  int proc(INode *n)
  {
    if (!n)
      return ECB_CONT;
    if (n == snode)
      return ECB_CONT;
    if (n->IsNodeHidden())
      return ECB_CONT;
    Object *obj = n->EvalWorldState(time).obj;
    if (!obj)
      return ECB_CONT;
    if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
      return ECB_CONT;
    TriObject *tri = (TriObject *)obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
    if (!tri)
      return ECB_CONT;
    Matrix3 wtm = n->GetObjectTM(time);
    Box3 b = tri->mesh.getBoundingBox(&wtm);
    if (b.pmax.x < sbox.pmin.x || b.pmin.x > sbox.pmax.x || b.pmax.y < sbox.pmin.y || b.pmin.y > sbox.pmax.y)
      return ECB_CONT;
    Obj *o = new Obj(tri->mesh, wtm, b);
    assert(o);
    mesh.Append(1, &o);
    if (tri != obj)
      delete tri;
    return ECB_CONT;
  }
  int isectray(Ray &ray, float &at, int &mi, int &fi, Point3 &bary)
  {
    int isect = 0;
    for (int i = 0; i < mesh.Count(); ++i)
      if (mesh[i])
      {
        Box3 &b = mesh[i]->box;
        if (ray.p.x < b.pmin.x || ray.p.x > b.pmax.x || ray.p.y < b.pmin.y || ray.p.y > b.pmax.y)
          continue;
        float t;
        Point3 norm, bc;
        DWORD f;
        if (mesh[i]->m.IntersectRay(ray, t, norm, f, bc))
        {
          if (!isect || t < at)
          {
            isect = 1;
            at = t;
            mi = i;
            fi = f;
            bary = bc;
          }
        }
      }
    return isect;
  }
};

struct ProjPt
{
  Point3 p;
  int mi, fi;
};

static bool segs_crossing(Point3 &a, Point3 &b, Point3 &c, Point3 &d, Point3 &p)
{
  double den = (d.x - c.x) * (b.y - a.y) - (b.x - a.x) * (d.y - c.y);
  if (den == 0)
    return false;
  double t = (b.y * (a.x - c.x) + a.y * (c.x - b.x) + c.y * (b.x - a.x)) / den;
  if (t < 0 || t > 1)
    return false;
  double s = (a.y * (c.x - d.x) + c.y * (d.x - a.x) + d.y * (a.x - c.x)) / den;
  if (s < 0 || s > 1)
    return false;
  p = (d - c) * float(t) + c;
  return true;
}

static DWORD WINAPI dummyprogressfn(LPVOID arg) { return 0; }

void project_spline_on_scene(Interface *ip, INode &snode, BezierShape &shape, BezierShape &rshp, float maxseglen, float optang,
  float optseglen)
{
  float maxseglen2 = maxseglen * maxseglen;
  float optseglen2 = optseglen * optseglen;
  float optcos;
  if (optang < 0)
    optcos = 2;
  else
    optcos = cosf(optang);
  TimeValue time = ip->GetTime();
  Matrix3 wtm = snode.GetObjectTM(time);
  PolyShape shp;
  shape.MakePolyShape(shp, -1, FALSE);
  shp.Transform(wtm);
  GetMeshesCB cb(ip, &snode, shp.GetBoundingBox());
  enum_nodes(ip->GetRootNode(), &cb);
  for (int ln = 0; ln < shp.numLines; ++ln)
  {
    int i;
    PolyLine &pl = shp.lines[ln];
    if (pl.numPts < 2)
      continue;
    {
      TSTR s;
      s.printf(_T("spline %d/%d:"), ln + 1, shp.numLines);
      ip->ProgressStart(s, TRUE, dummyprogressfn, NULL);
      ip->ProgressUpdate(0, FALSE, _T("preparing points"));
    }
    // maxseglen
    Tab<Point3> spt;
    spt.Resize(pl.numPts * 2);
    spt.Append(1, &pl.pts[0].p);
    for (i = 1; i < pl.numPts; ++i)
    {
      float l = LengthSquared(pl.pts[i].p - pl.pts[i - 1].p);
      if (l > maxseglen2)
      {
        Point3 &p0 = pl.pts[i - 1].p;
        Point3 d = pl.pts[i].p - p0;
        l = sqrtf(l);
        int n = ceilf(l / maxseglen);
        for (int j = 1; j <= n; ++j)
        {
          Point3 p = p0 + d * (float(j) / n);
          spt.Append(1, &p, n * 2);
        }
      }
      else
        spt.Append(1, &pl.pts[i].p);
    }
    // project points
    Tab<ProjPt> pt;
    pt.SetCount(spt.Count());
    for (i = 0; i < pt.Count(); ++i)
    {
      if ((i & 63) == 0)
        ip->ProgressUpdate(i * 100 / pt.Count());
      if ((i & 31) == 0)
        if (ip->GetCancel())
          break;
      Ray r;
      r.p = spt[i];
      r.dir = Point3(0, 0, -1);
      float t;
      int mi, fi;
      Point3 bc;
      if (cb.isectray(r, t, mi, fi, bc))
      {
        pt[i].p = r.p + r.dir * t;
        pt[i].mi = mi;
        pt[i].fi = fi;
      }
      else
      {
        pt[i].p = r.p;
        pt[i].mi = -1;
        pt[i].fi = -1;
      }
    }
    // divide segments that cross edges
    ip->ProgressUpdate(0, FALSE, _T("crossing edges"));
    for (i = 1; i < pt.Count(); ++i)
    {
      if ((i & 31) == 0)
        if (ip->GetCancel())
          break;
      int mi = pt[i].mi;
      if (mi < 0)
        continue;
      if (pt[i - 1].mi != mi)
        continue;
      int f1 = pt[i].fi;
      if (f1 < 0)
        continue;
      int f2 = pt[i - 1].fi;
      if (f2 < 0)
        continue;
      Mesh &m = cb.mesh[mi]->m;
      AdjFaceList &af = *cb.mesh[mi]->af;
      int lv1 = -1, lv2 = -1;
      int added = 0;
      while (f1 != f2)
      {
        ProjPt p;
        int j;
        for (j = 0; j < 3; ++j)
        {
          int v1 = m.faces[f1].v[j];
          int v2 = m.faces[f1].v[j + 1 < 3 ? j + 1 : 0];
          if (v1 == lv1 && v2 == lv2)
            continue;
          if (v1 == lv2 && v2 == lv1)
            continue;
          if (segs_crossing(pt[i].p, pt[i - 1].p, m.verts[v1], m.verts[v2], p.p))
          {
            lv1 = v1;
            lv2 = v2;
            p.mi = mi;
            p.fi = -1;
            pt.Insert(i, 1, &p);
            ++added;
            DWORD f = af[f1].f[j];
            if (f == UNDEFINED)
              f1 = f2;
            else
              f1 = f;
            break;
          }
        }
        if (j >= 3)
          break;
      }
      i += added;
    }
    // optimize in-face segments
    ip->ProgressUpdate(0, FALSE, _T("optimizing"));
    for (i = 0; i < pt.Count() - 2; ++i)
    {
      if ((i & 31) == 0)
        if (ip->GetCancel())
          break;
      int mi = pt[i].mi;
      if (mi < 0)
        continue;
      int fi = pt[i].fi;
      int j;
      for (j = i + 1; j < pt.Count(); ++j)
      {
        if (pt[j].mi != mi)
          break;
        if (pt[j].fi < 0)
        {
          ++j;
          break;
        }
        if (fi < 0)
          fi = pt[j].fi;
        else if (pt[j].fi != fi)
          break;
      }
      if (fi < 0)
        continue;
      if (j - i < 3)
        continue;
      // delete extra points between i and j-1
      for (int k = i + 1; k < j - 1; ++k)
      {
        float ca = DotProd(Normalize(pt[k].p - pt[k - 1].p), Normalize(pt[k + 1].p - pt[k].p));
        if (ca > optcos || LengthSquared(pt[k].p - pt[k - 1].p) < optseglen2)
        {
          pt.Delete(k, 1);
          --j;
          --k;
        }
      }
      if (j - i > 2)
        if (LengthSquared(pt[j - 1].p - pt[j - 2].p) < optseglen2)
        {
          pt.Delete(j - 2, 1);
          --j;
        }
      i = j - 2;
    }
    // create spline
    if (ip->GetCancel())
    {
      ip->ProgressEnd();
      continue;
    }
    ip->ProgressUpdate(0, FALSE, _T("creating spline"));
    Spline3D &spl = *new Spline3D;
    assert(&spl);
    for (i = 0; i < pt.Count(); ++i)
      spl.AddKnot(SplineKnot(KTYPE_CORNER, LTYPE_LINE, pt[i].p, Point3(0, 0, 0), Point3(0, 0, 0)));
    spl.ComputeBezPoints();
    rshp.AddSpline(&spl);
    ip->ProgressEnd();
  }
  rshp.UpdateSels();
}
