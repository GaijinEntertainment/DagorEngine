// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <templt.h>
// #include "debug.h"

typedef float real;
#define rabs(a) (fabsf(a))

static int mapw, maph, mapxmsk, mapymsk;
static real mapstep;

struct LMface
{
  Point2 t[3];
};

static void map_face(char *m, Point2 o, Point2 *t)
{
  int p0, p1, p2;
  if (t[0].y < t[1].y)
  {
    if (t[0].y < t[2].y)
    {
      p0 = 0;
      if (t[1].y < t[2].y)
      {
        p1 = 1;
        p2 = 2;
      }
      else
      {
        p1 = 2;
        p2 = 1;
      }
    }
    else
    {
      p0 = 2;
      p1 = 0;
      p2 = 1;
    }
  }
  else
  {
    if (t[1].y < t[2].y)
    {
      p0 = 1;
      if (t[0].y < t[2].y)
      {
        p1 = 0;
        p2 = 2;
      }
      else
      {
        p1 = 2;
        p2 = 0;
      }
    }
    else
    {
      p0 = 2;
      p1 = 1;
      p2 = 0;
    }
  }
  Point2 t0 = (t[p0] - o), t1 = (t[p1] - o), t2 = (t[p2] - o);
  t0.x *= mapw;
  t1.x *= mapw;
  t2.x *= mapw;
  t0.y *= maph;
  t1.y *= maph;
  t2.y *= maph;
  Point2 d1, d2;
  real xl, xr, y1, y2, ox1, ox2;
  int iy, ny;
  if (floorf(t0.y) == floorf(t1.y))
  {
    if (floorf(t2.y) == floorf(t0.y))
    {
      real x0, x1;
      x0 = x1 = t0.x;
      if (t1.x < x0)
        x0 = t1.x;
      else if (t1.x > x1)
        x1 = t1.x;
      if (t2.x < x0)
        x0 = t2.x;
      else if (t2.x > x1)
        x1 = t2.x;
      iy = int(floorf(t0.y)) & mapymsk;
      ny = (iy + 1) & mapymsk;
      int x = floorf(x0), xe = ceilf(x1);
      for (; x <= xe; ++x)
      {
        m[iy * mapw + (x & mapxmsk)] = 1;
        m[ny * mapw + (x & mapxmsk)] = 1;
      }
      return;
    }
    d1 = t2 - t1;
    d2 = t2 - t0;
    ox1 = t1.x;
    ox2 = t0.x;
    y1 = floorf(t1.y) + 1 - t1.y;
    y2 = floorf(t0.y) + 1 - t0.y;
    xl = xr = t0.x;
    if (t1.x < xl)
      xl = t1.x;
    else if (t1.x > xr)
      xr = t1.x;
  }
  else
  {
    d1 = t1 - t0;
    d2 = t2 - t0;
    ox1 = ox2 = t0.x;
    y1 = floorf(t0.y) + 1 - t0.y;
    y2 = y1;
    xl = xr = t0.x;
  }
  iy = int(floorf(t0.y)) & mapymsk;
  ny = (iy + 1) & mapymsk;
  while (y2 < d2.y)
  {
    int txs = ceilf(xl), txe = floorf(xr);
    if (y1 > d1.y)
    {
      d1 = t2 - t1;
      y1 = floorf(t1.y) + 1 - t1.y;
      ox1 = t1.x;
      if (t1.x < xl)
        xl = t1.x;
      else if (t1.x > xr)
        xr = t1.x;
    }
    real x1 = d1.x * y1 / d1.y + ox1;
    real x2 = d2.x * y2 / d2.y + ox2;
    if (x1 > x2)
    {
      real a = x1;
      x1 = x2;
      x2 = a;
    }
    if (x1 < xl)
      xl = x1;
    if (x2 > xr)
      xr = x2;
    int xs = floorf(xl), xe = ceilf(xr), bxs = ceilf(x1), bxe = floorf(x2), ix;
    for (ix = xs; ix < txs; ++ix)
      m[iy * mapw + (ix & mapxmsk)] = 1;
    for (; ix <= txe; ++ix)
      m[iy * mapw + (ix & mapxmsk)] = 1;
    for (; ix <= xe; ++ix)
      m[iy * mapw + (ix & mapxmsk)] = 1;
    for (ix = xs; ix < bxs; ++ix)
      m[ny * mapw + (ix & mapxmsk)] = 1;
    for (; ix <= bxe; ++ix)
      m[ny * mapw + (ix & mapxmsk)] = 1;
    for (; ix <= xe; ++ix)
      m[ny * mapw + (ix & mapxmsk)] = 1;
    xl = x1;
    xr = x2;
    y1 += 1;
    y2 += 1;
    iy = ny;
    ny = (ny + 1) & mapymsk;
  }
  int txs = ceilf(xl), txe = floorf(xr);
  if (floorf(t1.y) == floorf(t2.y))
  {
    if (t1.x < xl)
      xl = t1.x;
    else if (t1.x > xr)
      xr = t1.x;
  }
  if (t2.x < xl)
    xl = t2.x;
  else if (t2.x > xr)
    xr = t2.x;
  int xs = floorf(xl), xe = ceilf(xr), ix;
  for (ix = xs; ix < txs; ++ix)
    m[iy * mapw + (ix & mapxmsk)] = 1;
  for (; ix <= txe; ++ix)
    m[iy * mapw + (ix & mapxmsk)] = 1;
  for (; ix <= xe; ++ix)
    m[iy * mapw + (ix & mapxmsk)] = 1;
  for (ix = xs; ix <= xe; ++ix)
    m[ny * mapw + (ix & mapxmsk)] = 1;
}

static void scan_map(char *m, char *mh, char *mv)
{
  int i, j;
  for (i = 0; i < maph; ++i)
  {
    for (j = 0; j < mapw; ++j)
      if (m[i * mapw + j])
        break;
    if (j < mapw)
      mh[i] = 1;
    else
      mh[i] = 0;
  }
  for (i = 0; i < mapw; ++i)
  {
    for (j = 0; j < maph; ++j)
      if (m[j * mapw + i])
        break;
    if (j < maph)
      mv[i] = 1;
    else
      mv[i] = 0;
  }
}

static int map_fits(char *m, char *map, char *mh, char *mv, int ox, int oy)
{
  int y, my, x, mx;
  for (y = 0, my = maph - oy; y < oy; ++y, ++my)
    if (mh[my])
    {
      for (x = 0, mx = mapw - ox; x < ox; ++x, ++mx)
        if (mv[mx])
          if (m[my * mapw + mx] && map[y * mapw + x])
            return 0;
      for (mx = 0; x < mapw; ++x, ++mx)
        if (mv[mx])
          if (m[my * mapw + mx] && map[y * mapw + x])
            return 0;
    }
  for (my = 0; y < maph; ++y, ++my)
    if (mh[my])
    {
      for (x = 0, mx = mapw - ox; x < ox; ++x, ++mx)
        if (mv[mx])
          if (m[my * mapw + mx] && map[y * mapw + x])
            return 0;
      for (mx = 0; x < mapw; ++x, ++mx)
        if (mv[mx])
          if (m[my * mapw + mx] && map[y * mapw + x])
            return 0;
    }
  return 1;
}

static void apply_map(char *m, char *map, int ox, int oy)
{
  int y, my, x, mx;
  for (y = 0, my = maph - oy; y < oy; ++y, ++my)
  {
    for (x = 0, mx = mapw - ox; x < ox; ++x, ++mx)
      if (m[my * mapw + mx])
        map[y * mapw + x] = 1;
    for (mx = 0; x < mapw; ++x, ++mx)
      if (m[my * mapw + mx])
        map[y * mapw + x] = 1;
  }
  for (my = 0; y < maph; ++y, ++my)
  {
    for (x = 0, mx = mapw - ox; x < ox; ++x, ++mx)
      if (m[my * mapw + mx])
        map[y * mapw + x] = 1;
    for (mx = 0; x < mapw; ++x, ++mx)
      if (m[my * mapw + mx])
        map[y * mapw + x] = 1;
  }
}

typedef Tab<int> Tabint;

static bool ispow2(int a)
{
  if (!a)
    return 0;
  unsigned m;
  for (m = 1 << 31; !(a & m); m >>= 1)
    ;
  return a == m;
}

int apply_ltmap(Mesh &m, int mati, int mch, int wd, int ht, float step, int &useperc)
{
  mapw = wd;
  maph = ht;
  mapstep = step;
  if (!ispow2(wd) || !ispow2(ht))
    return 0;
  mapxmsk = wd - 1;
  mapymsk = ht - 1;
  Tab<Tabint> vrfs;
  vrfs.SetCount(m.numVerts);
  int i, j;
  memset(&vrfs[0], 0, vrfs.Count() * sizeof(Tabint));
  // for(i=0;i<vrfs.Count();++i) vrfs[i].Init();
  for (i = 0; i < m.numFaces; ++i)
  {
    vrfs[m.faces[i].v[0]].Append(1, &i, 8);
    vrfs[m.faces[i].v[1]].Append(1, &i, 8);
    vrfs[m.faces[i].v[2]].Append(1, &i, 8);
  }

  Tab<signed char> fct;
  Tab<LMface> lmfc;
  fct.SetCount(m.numFaces);
  lmfc.SetCount(m.numFaces);
  for (i = 0; i < lmfc.Count(); ++i)
    if (mati < 0 || m.faces[i].getMatID() == mati)
    {
      Point3 fn = CrossProd(m.verts[m.faces[i].v[1]] - m.verts[m.faces[i].v[0]], m.verts[m.faces[i].v[2]] - m.verts[m.faces[i].v[0]]);
      real mv = rabs(fn.x);
      int gc = 0;
      real a = rabs(fn.y);
      if (a > mv)
      {
        mv = a;
        gc = 1;
      }
      a = rabs(fn.z);
      if (a > mv)
      {
        mv = a;
        gc = 2;
      }
      int mc, sc;
      if (gc == 0)
      {
        mc = 1;
        sc = 2;
      }
      else if (gc == 1)
      {
        mc = 0;
        sc = 2;
      }
      else
      {
        mc = 0;
        sc = 1;
      }
      fct[i] = gc + 1;
      if (fn[gc] < 0)
        fct[i] += 3;
      for (j = 0; j < 3; ++j)
      {
        lmfc[i].t[j].x = m.verts[m.faces[i].v[j]][mc] / (mapw * mapstep);
        lmfc[i].t[j].y = m.verts[m.faces[i].v[j]][sc] / (maph * mapstep);
      }
    }
    else
    {
      lmfc[i].t[0] = Point2(0.f, 0.f);
      lmfc[i].t[1] = Point2(0.f, 0.f);
      lmfc[i].t[2] = Point2(0.f, 0.f);
      fct[i] = -1;
    }

  char *fmap = new char[mapw * maph];
  assert(fmap);
  memset(fmap, 0, mapw * maph);
  char *map = new char[mapw * maph];
  assert(map);
  char *hmap = new char[maph];
  assert(hmap);
  char *vmap = new char[mapw];
  assert(vmap);
  bool maperr = false;
  for (i = 0; i < lmfc.Count(); ++i)
    if (fct[i] > 0)
    {
      signed char fctype = fct[i];
      Point2 org;
      Box2D box;
      Tab<int> fc;
      fc.Append(1, &i);
      box += lmfc[i].t[0];
      box += lmfc[i].t[1];
      box += lmfc[i].t[2];
      org.x = floorf(box.min.x * mapw) / mapw;
      org.y = floorf(box.min.y * maph) / maph;
      memset(map, 0, mapw * maph);
      map_face(map, org, lmfc[i].t);
      fct[i] = 0;
      for (int cf = 0; cf < fc.Count(); ++cf)
      {
        // int smgr=m.face[fc[cf]].smgr;
        for (int vi = 0; vi < 3; ++vi)
        {
          Tabint &vt = vrfs[m.faces[fc[cf]].v[vi]];
          for (j = 0; j < vt.Count(); ++j)
            if (fct[vt[j]] == fctype /*&& (m.face[vt[j]].smgr&smgr)*/)
            {
              Box2D nb = box;
              nb += lmfc[vt[j]].t[0];
              nb += lmfc[vt[j]].t[1];
              nb += lmfc[vt[j]].t[2];
              if (nb.max.x - nb.min.x < 1 && nb.max.y - nb.min.y < 1)
              {
                box = nb;
                fc.Append(1, &vt[j]);
                map_face(map, org, lmfc[vt[j]].t);
                fct[vt[j]] = 0;
              }
            }
        }
      }
      // find place for surface
      scan_map(map, hmap, vmap);
      int ox = 0, oy = 0;
      bool ok = false;
      for (oy = 0; oy < maph && !ok; ++oy)
        for (ox = 0; ox < mapw; ++ox)
          if (map_fits(map, fmap, hmap, vmap, ox, oy))
          {
            ok = true;
            break;
          }
      if (!ok)
      {
        mesh_face_sel(m).ClearAll();
        for (j = 0; j < fc.Count(); ++j)
          mesh_face_sel(m).Set(fc[j]);
        for (j = 0; j < fct.Count(); ++j)
          if (fct[j] > 0)
            mesh_face_sel(m).Set(j);
        maperr = true;
        break;
      }
      apply_map(map, fmap, ox, oy);
      Point2 o = Point2(real(ox) / mapw, real(oy) / maph) - org;
      for (j = 0; j < fc.Count(); ++j)
      {
        lmfc[fc[j]].t[0] += o;
        lmfc[fc[j]].t[1] += o;
        lmfc[fc[j]].t[2] += o;
      }
    }
  char *p = fmap;
  int uc = 0;
  for (i = mapw * maph; i; --i, ++p)
    if (*p)
      ++uc;
  useperc = uc * 100 / (mapw * maph);
  delete[] vmap;
  delete[] hmap;
  delete[] map;
  delete[] fmap;

  for (i = 0; i < vrfs.Count(); ++i)
  {
    vrfs[i].SetCount(0);
    vrfs[i].Shrink();
  }

  Tab<Point2> tc;
  tc.Resize(m.numFaces * 3);
  m.setMapSupport(mch);
  m.setNumMapFaces(mch, m.numFaces);
  for (int f = 0; f < m.numFaces; ++f)
  {
    for (int v = 0; v < 3; ++v)
    {
      Point2 p = lmfc[f].t[v] + Point2(0.5f / mapw, 0.5f / maph);
      for (j = 0; j < tc.Count(); ++j)
        if (tc[j] == p)
          break;
      if (j >= tc.Count())
      {
        tc.Append(1, &p);
        m.setNumMapVerts(mch, j + 1, TRUE);
        m.setMapVert(mch, j, Point3(p.x, p.y, 0.f));
      }
      m.mapFaces(mch)[f].t[v] = j;
    }
  }

  return !maperr;
}
