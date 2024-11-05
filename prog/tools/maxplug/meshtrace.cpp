// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "math3d.h"
#include "qsort.h"
#include "debug.h"

template <class T>
inline T *__memchk(T *o, char *f, int l)
{
  if (!o)
  {
    debug("Not enough memory in file <%s>, line <%d>", f, l);
    assert(false);
  }
  return o;
}

#define nomemchk(o) __memchk(o, __FILE__, __LINE__)

#ifndef nomem
#define nomem(a)                                                              \
  {                                                                           \
    if (!(a))                                                                 \
      fatal("Not enough memory in file <%s>, line <%d>", __FILE__, __LINE__); \
  }
#endif

#define bugchk(a) assert(a)

#define MAX_LEAF_FACES  16
#define MAX_LEAF_LENGTH MAX_REAL
#define MIN_LEAF_FACES  16

#define MAX_SCENE_LEAF_FACES 16


class StaticMeshRTracer : public StaticMeshRayTracer
{
public:
  static real a2, a4;
  struct RTface
  {
    Matrix33 bcm;
    Point3 sc;
    real sr2;
    Point3 n, en[3];
    real d, ed[3];
    int id;
  };
  struct Node
  {
    Point3 bsc;
    real bsr2;
    Node *sub0;

    Node() : sub0(NULL) {}
    bool traceray(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
    bool traceray_back(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
  };
  struct BNode : Node
  {
    Node *sub1;
    BNode() : sub1(NULL) {}
  };
  struct LNode : Node
  {
    Tab<RTface> face;

    LNode() {}
    bool traceray(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
    bool traceray_back(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
    void build(Mesh &m, int *fc, int numf);
  };
  void kill_node(Node *);
  Node *root;

  StaticMeshRTracer() : root(NULL) {}
  ~StaticMeshRTracer() { kill_node(root); }
  bool traceray(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
  bool traceray_back(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc);
  Node *build_node(Mesh &m, int *fc, int numf);
  void build(Mesh &m);
};


real StaticMeshRTracer::a2, StaticMeshRTracer::a4;

void StaticMeshRTracer::kill_node(Node *n)
{
  if (!n)
    return;
  if (n->sub0)
  {
    kill_node(n->sub0);
    kill_node(((BNode *)n)->sub1);
    delete ((BNode *)n);
  }
  else
  {
    delete ((LNode *)n);
  }
}

bool StaticMeshRTracer::LNode::traceray(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  bool hit = false;
  for (int fi = 0; fi < face.Count(); ++fi)
  {
    RTface &f = face[fi];
    real den = DotProd(f.n, dir);
    if (den <= 0)
      continue;
    Point3 pc = p - f.sc;
    real b = DotProd(pc, dir) * 2;
    real c = DotProd(pc, pc) - f.sr2;
    real d = b * b - a4 * c;
    if (d < 0)
      continue;
    real s = sqrtf(d);
    real t2 = (-b + s) * a2;
    if (t2 < 0)
      continue;
    real t1 = (-b - s) * a2;
    if (t1 >= mint)
      continue;
    // intersect face plane
    real t = -(DotProd(f.n, p) + f.d) / den;
    if (t < 0)
      continue;
    if (t >= mint)
      continue;
    Point3 pt = p + dir * t;
    int j;
    for (j = 0; j < 3; ++j)
      if (DotProd(f.en[j], pt) + f.ed[j] < 0)
        break;
    if (j < 3)
      continue;
    Point3 bc = f.bcm * pt;
    mint = t;
    fid = f.id;
    bary = bc;
    hit = true;
  }
  return hit;
}


bool StaticMeshRTracer::Node::traceray(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  Point3 pc = p - bsc;
  real b = DotProd(pc, dir) * 2;
  real c = DotProd(pc, pc) - bsr2;
  real d = b * b - a4 * c;
  if (d < 0)
    return false;
  real s = sqrtf(d);
  real t2 = (-b + s) * a2;
  if (t2 < 0)
    return false;
  real t1 = (-b - s) * a2;
  if (t1 >= mint)
    return false;
  if (sub0)
  {
    // branch node
    bool hit = sub0->traceray(p, dir, mint, fid, bary);
    if (((BNode *)this)->sub1->traceray(p, dir, mint, fid, bary))
      return true;
    return hit;
  }
  else
  {
    // leaf node
    return ((LNode *)this)->traceray(p, dir, mint, fid, bary);
  }
}


bool StaticMeshRTracer::traceray(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  if (!root)
    return false;
  a4 = lengthSq(dir) * 2;
  a2 = 1 / a4;
  a4 *= 2;
  return root->traceray(p, dir, mint, fid, bary);
}


// Back

bool StaticMeshRTracer::LNode::traceray_back(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  bool hit = false;
  for (int fi = 0; fi < face.Count(); ++fi)
  {
    RTface &f = face[fi];
    real den = DotProd(f.n, dir);
    if (den >= 0)
      continue;
    Point3 pc = p - f.sc;
    real b = DotProd(pc, dir) * 2;
    real c = DotProd(pc, pc) - f.sr2;
    real d = b * b - a4 * c;
    if (d < 0)
      continue;
    real s = sqrtf(d);
    real t2 = (-b + s) * a2;
    if (t2 < 0)
      continue;
    real t1 = (-b - s) * a2;
    if (t1 >= mint)
      continue;
    // intersect face plane
    real t = -(DotProd(f.n, p) + f.d) / den;
    if (t < 0)
      continue;
    if (t >= mint)
      continue;
    Point3 pt = p + dir * t;
    int j;
    for (j = 0; j < 3; ++j)
      if (DotProd(f.en[j], pt) + f.ed[j] < 0)
        break;
    if (j < 3)
      continue;
    Point3 bc = f.bcm * pt;
    mint = t;
    fid = f.id;
    bary = bc;
    hit = true;
  }
  return hit;
}


bool StaticMeshRTracer::Node::traceray_back(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  Point3 pc = p - bsc;
  real b = DotProd(pc, dir) * 2;
  real c = DotProd(pc, pc) - bsr2;
  real d = b * b - a4 * c;
  if (d < 0)
    return false;
  real s = sqrtf(d);
  real t2 = (-b + s) * a2;
  if (t2 < 0)
    return false;
  real t1 = (-b - s) * a2;
  if (t1 >= mint)
    return false;
  if (sub0)
  {
    // branch node
    bool hit = sub0->traceray_back(p, dir, mint, fid, bary);
    if (((BNode *)this)->sub1->traceray_back(p, dir, mint, fid, bary))
      return true;
    return hit;
  }
  else
  {
    // leaf node
    return ((LNode *)this)->traceray_back(p, dir, mint, fid, bary);
  }
}


bool StaticMeshRTracer::traceray_back(Point3 &p, Point3 &dir, real &mint, int &fid, Point3 &bary)
{
  if (!root)
    return false;
  a4 = lengthSq(dir) * 2;
  a2 = 1 / a4;
  a4 *= 2;
  return root->traceray_back(p, dir, mint, fid, bary);
}


static Tab<float> fgrp2;

StaticMeshRTracer::Node *StaticMeshRTracer::build_node(Mesh &m, int *fc, int numf)
{
  if (numf <= 0)
    return NULL;
  BBox3 box;
  int i, j;
  for (i = 0; i < numf; ++i)
    for (j = 0; j < 3; ++j)
      box += m.verts[m.faces[fc[i]].v[j]];
  Point3 c = box.center();
  real r = 0;
  for (i = 0; i < numf; ++i)
    for (j = 0; j < 3; ++j)
    {
      real d = lengthSq(m.verts[m.faces[fc[i]].v[j]] - c);
      if (d > r)
        r = d;
    }

  Point3 wd = box.width();
  int md = 0;
  real ms = wd[0];
  for (i = 1; i < 3; ++i)
    if (wd[i] > ms)
      ms = wd[md = i];

  if (numf <= MAX_LEAF_FACES && (ms <= MAX_LEAF_LENGTH || numf <= MIN_LEAF_FACES))
  {
    // build leaf node
    LNode *n = nomemchk(new LNode);
    n->bsc = c;
    n->bsr2 = r;
    n->build(m, fc, numf);
    return n;
  }
  else
  {
    // build branch
    BNode *n = nomemchk(new BNode);
    n->bsc = c;
    n->bsr2 = r;
    /*Point3 wd=box.width();
    int md=0;
    real ms=wd[0];
    for(i=1;i<3;++i) if(wd[i]>ms) ms=wd[md=i];*/
    real dp = c[md] * 3;
    for (i = 0; i < numf; ++i)
    {
      int f = fc[i];
      // fgrp[f]=((m.vert[m.face[f].v[0]][md]+m.vert[m.face[f].v[1]][md]+
      // m.vert[m.face[f].v[2]][md])>=dp);
      fgrp2[f] = ((m.verts[m.faces[f].v[0]][md] + m.verts[m.faces[f].v[1]][md] + m.verts[m.faces[f].v[2]][md]));
    }
    // DataSimpleQsort<int,MapSimpleAscentCompare<char>,char> qs;
    // qs.sort(fc,numf,fgrp);
    DataSimpleQsort<int, MapSimpleAscentCompare<float>, float> qs;
    qs.sort(fc, numf, fgrp2.Addr(0));
    int df = numf >> 1;
    n->sub0 = build_node(m, fc, df);
    n->sub1 = build_node(m, fc + df, numf - df);
    bugchk(n->sub0);
    return n;
  }
}

void StaticMeshRTracer::LNode::build(Mesh &m, int *fc, int numf)
{
  face.SetCount(numf);
  for (int i = 0; i < numf; ++i)
  {
    Matrix33 tm;
    tm.setcol(0, m.verts[m.faces[fc[i]].v[0]]);
    tm.setcol(1, m.verts[m.faces[fc[i]].v[1]]);
    tm.setcol(2, m.verts[m.faces[fc[i]].v[2]]);
    RTface &f = face[i];
    f.id = fc[i];
    f.sc = (tm.getcol(0) + tm.getcol(1) + tm.getcol(2)) / 3;
    f.sr2 = lengthSq(tm.getcol(0) - f.sc);
    real r = lengthSq(tm.getcol(1) - f.sc);
    if (r > f.sr2)
      f.sr2 = r;
    r = lengthSq(tm.getcol(2) - f.sc);
    if (r > f.sr2)
      f.sr2 = r;
    f.bcm = inverse(tm);
    f.n = CrossProd((tm.getcol(2) - tm.getcol(0)), (tm.getcol(1) - tm.getcol(0)));
    f.d = -DotProd(tm.getcol(0), f.n);
    for (int j = 0; j < 3; ++j)
    {
      f.en[j] = CrossProd((tm.getcol(j + 1 >= 3 ? 0 : j + 1) - tm.getcol(j)), f.n);
      f.ed[j] = -DotProd(f.en[j], tm.getcol(j));
    }
  }
}

void StaticMeshRTracer::build(Mesh &m)
{
  if (root)
  {
    kill_node(root);
    root = NULL;
  }
  Tab<int> fc;
  fc.SetCount(m.numFaces);
  // nomem(fgrp.resize(m.face.size()));
  fgrp2.SetCount(m.numFaces);
  for (int i = 0; i < fc.Count(); ++i)
    fc[i] = i;
  root = build_node(m, fc.Addr(0), fc.Count());
}

StaticMeshRayTracer *create_staticmeshraytracer()
{
  StaticMeshRTracer *rt = nomemchk(new StaticMeshRTracer);
  return rt;
}

StaticMeshRayTracer *create_staticmeshraytracer(Mesh &m)
{
  StaticMeshRTracer *rt = nomemchk(new StaticMeshRTracer);
  rt->build(m);
  return rt;
}
