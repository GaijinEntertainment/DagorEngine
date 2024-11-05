// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <unwrapMapping/autoUnwrap.h>
#include <math/dag_mesh.h>
#include <generic/dag_qsort.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint3.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <generic/dag_sort.h>
#include <util/dag_bitArray.h>
#include <util/dag_string.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/svgWrite.h>
#include <direct.h>
#include <stdio.h>

#define DEBUG_SHOW_FINAL_MAPPING 0

#define SVG_LMSZ LIGHTMAPSZ / 2


#define USE_OWN_UNWRAP_MESH 0


static void print_mapping_stat();
static void clear_mapping_stat();

namespace LightMapMapper
{
struct LightMapObject;
}
DAG_DECLARE_RELOCATABLE(LightMapMapper::LightMapObject);

namespace LightMapMapper
{
enum
{
  MAX_LIGHTMAPSZ = 2048,
  MINMAPSZ = 6,
  FACEMAPSZ = 4,
};
static int LIGHTMAPSZ = 0;

static uint16_t *lmid;
static int lmid_stride;

static inline void set_lmid(int fi, int lm) { *(uint16_t *)((char *)lmid + fi * lmid_stride) = lm; }

static inline uint16_t get_lmid(int fi) { return *(uint16_t *)((char *)lmid + fi * lmid_stride); }

// Building Face groups

struct FaceGroup
{
  int objid; // Lightmap object id
  int fgid;
  int lmid; // lightmap id
  Tab<int> faces;
  int wd, ht, ox, oy;
  short *lower;
  short *upper;
  short *leftmost;
  short *rightmost;
  int transf;
  bool finalized;
  int uvDataPos;
  int minS, maxS;

  Tab<Point2> uv;
  Tab<IPoint3> tfaces;
  int sv;

  FaceGroup() : faces(tmpmem), uv(tmpmem), tfaces(tmpmem), sv(-1), finalized(false), transf(0)
  {
    lower = NULL;
    upper = NULL;
    leftmost = NULL;
    rightmost = NULL;
    uvDataPos = -1;
    minS = 0;
    maxS = 0;
  }
  ~FaceGroup() { clear(); }

  void saveOriginalUv(IGenSave &cwr)
  {
    uvDataPos = cwr.tell();
    cwr.writeTab(uv);
  }

  void loadOriginalUv(IGenLoad &crd)
  {
    if (uvDataPos == -1)
      return;

    crd.seekto(uvDataPos);
    crd.readTab(uv);
  }

  // translates and shrinks uv-coords to fit into (0..LTMAPSZ)
  void normalize_uv(real sample_size);

  void calc_histograms();
  bool get_transformed(int t, short *up, short *dn, int &w, int &h) const;

  // transforms texture coordinates
  void at_last_transform();
  int adduv(const Point2 &p);

  void clear()
  {
    del_it(upper);
    del_it(lower);
    del_it(leftmost);
    del_it(rightmost);
  }
  void measure()
  {
    minS = 0;
    maxS = 0;
    if (!upper || !lower || !leftmost || !rightmost)
      return;

    for (int i = 0; i < LIGHTMAPSZ; i++)
    {
      if (upper[i] >= lower[i])
        minS += upper[i] - lower[i] + 1;
      if (rightmost[i] >= leftmost[i])
        maxS += rightmost[i] - leftmost[i] + 1;
    }

    if (minS > maxS)
    {
      int t = minS;
      minS = maxS;
      maxS = t;
    }
  }
};

struct LightMapObject
{
  Tab<FaceGroup *> fgrps;
  Face *face;
  Point3 *vert;
  real sampleSize;
  int fnum, vnum;
  int minS, maxS, avgS;
  bool perobj;
  int unwrapScheme;

  LightMapObject() : fgrps(tmpmem)
  {
    face = NULL;
    vert = NULL;
    fnum = 0;
    vnum = 0;
    perobj = false;
    sampleSize = 0.0;
    minS = 0;
    maxS = 0;
    avgS = 0;
    unwrapScheme = -1;
  }
  ~LightMapObject()
  {
    for (int i = fgrps.size() - 1; i >= 0; i--)
      delete fgrps[i];
    clear_and_shrink(fgrps);
  }

  void prepareForPacking(real sample_scale)
  {
    for (int i = 0; i < fgrps.size(); ++i)
    {
      FaceGroup &g = *fgrps[i];
      g.normalize_uv(sampleSize * sample_scale);
      g.calc_histograms();
    }
  }

  void measure()
  {
    minS = 0;
    maxS = 0;
    avgS = 0;
    for (int i = 0; i < fgrps.size(); ++i)
    {
      FaceGroup &g = *fgrps[i];
      g.measure();
      minS += g.minS;
      maxS += g.maxS;
      avgS += g.minS + g.maxS;
      // debug ( "fg %p.%d: S = %d, %d", this, i, g.minS, g.maxS );
    }

    if (avgS)
      avgS /= 2 * fgrps.size();
    // debug ( "obj %p: S = %d, %d (avg %d)", this, minS, maxS, avgS );
  }

  void recalcHist()
  {
    for (int i = 0; i < fgrps.size(); ++i)
      fgrps[i]->calc_histograms();
  }

  void finishPacking()
  {
    for (int i = 0; i < fgrps.size(); ++i)
      fgrps[i]->clear();
  }
};

static Tab<LightMapObject> ltmobjs(tmpmem);

bool FaceGroup::get_transformed(int t, short *up, short *dn, int &w, int &h) const
{
  int i;
  switch (t)
  {
    case 0: // no changes
      w = wd;
      h = ht;
      for (i = 0; i < wd; i++)
      {
        up[i] = lower[i];
        dn[i] = upper[i];
      }
      return true;
    case 1: // flip vertical
      w = wd;
      h = ht;
      for (i = 0; i < wd; i++)
        if (upper[i] >= lower[i])
        {
          up[i] = h - upper[i] - 1;
          dn[i] = h - lower[i] - 1;
        }
        else
        {
          up[i] = lower[i];
          dn[i] = upper[i];
        }
      return true;
    case 2: // flip horizontal
      w = wd;
      h = ht;
      for (i = 0; i < wd; i++)
      {
        up[i] = lower[wd - i - 1];
        dn[i] = upper[wd - i - 1];
      }
      return true;
    case 3: // flip x<->y (rotate +270 deg and flip)
      w = ht;
      h = wd;
      for (i = 0; i < ht; i++)
      {
        up[i] = leftmost[i];
        dn[i] = rightmost[i];
      }
      return true;
    case 4: // rotate 90 deg
      w = ht;
      h = wd;
      for (i = 0; i < ht; i++)
        if (rightmost[i] >= leftmost[i])
        {
          up[i] = h - rightmost[i] - 1;
          dn[i] = h - leftmost[i] - 1;
        }
        else
        {
          up[i] = leftmost[i];
          dn[i] = rightmost[i];
        }
      return true;
    case 5: // rotate +90 deg and horizontal flip
      w = ht;
      h = wd;
      for (i = 0; i < ht; i++)
        if (rightmost[ht - i - 1] >= leftmost[ht - i - 1])
        {
          up[i] = h - rightmost[ht - i - 1] - 1;
          dn[i] = h - leftmost[ht - i - 1] - 1;
        }
        else
        {
          up[i] = leftmost[ht - i - 1];
          dn[i] = rightmost[ht - i - 1];
        }
      return true;
    case 6: // rotate +270 deg
      w = ht;
      h = wd;
      for (i = 0; i < ht; i++)
      {
        up[i] = leftmost[ht - i - 1];
        dn[i] = rightmost[ht - i - 1];
      }
      return true;
    case 7: // rotate +180 deg
      w = wd;
      h = ht;
      for (i = 0; i < wd; i++)
        if (upper[wd - i - 1] >= lower[wd - i - 1])
        {
          up[i] = h - upper[wd - i - 1] - 1;
          dn[i] = h - lower[wd - i - 1] - 1;
        }
        else
        {
          up[i] = lower[wd - i - 1];
          dn[i] = upper[wd - i - 1];
        }
      return true;
  }
  return false;
}

void FaceGroup::normalize_uv(real sample_size)
{
  BBox2 b;
  int i;
  real su, sv;
  Point2 ofs = Point2(0, 0);

  b.setempty();
  for (i = 0; i < uv.size(); i++)
  {
    b += uv[i] / sample_size;
    // debug ( "uv[%d]=(%.3f,%.3f)", i, uv[i].x, uv[i].y );
  }

  ofs.x = b[0].x - floorf(b[0].x / sample_size) * sample_size;
  ofs.y = b[0].y - floorf(b[0].y / sample_size) * sample_size;

  su = b.width().x;
  sv = b.width().y;
  // debug ( "sz=%.3f,%.3f", su, sv );

  if (su > LIGHTMAPSZ - 1)
    su = (LIGHTMAPSZ - 1) / su;
  else
  {
    if (su < (MINMAPSZ - 5) && su > 0.0001)
      su = (MINMAPSZ - 5) / su;
    else
      su = 1.0;
    ofs.x += 1.0;
  }
  if (sv > LIGHTMAPSZ - 1)
    sv = (LIGHTMAPSZ - 1) / sv;
  else
  {
    if (sv < (MINMAPSZ - 5) && sv > 0.0001)
      sv = (MINMAPSZ - 5) / sv;
    else
      sv = 1.0;
    ofs.y += 1.0;
  }

  // debug ( "scale=%.3f,%.3f", su, sv );
  for (i = 0; i < uv.size(); i++)
  {
    uv[i] = uv[i] / sample_size - b[0];
    uv[i].x *= su;
    uv[i].y *= sv;
    uv[i] += ofs;
    // debug ( "->uv[%d]=(%.3f,%.3f)", i, uv[i].x, uv[i].y );
  }
}
void FaceGroup::calc_histograms()
{
  if (!lower)
    lower = new short[LIGHTMAPSZ];
  if (!upper)
    upper = new short[LIGHTMAPSZ];
  if (!leftmost)
    leftmost = new short[LIGHTMAPSZ];
  if (!rightmost)
    rightmost = new short[LIGHTMAPSZ];

  for (int hist_i = 0; hist_i < LIGHTMAPSZ; hist_i++)
  {
    lower[hist_i] = LIGHTMAPSZ * 2;
    upper[hist_i] = -LIGHTMAPSZ * 2;
    leftmost[hist_i] = LIGHTMAPSZ * 2;
    rightmost[hist_i] = -LIGHTMAPSZ * 2;
  }
  wd = 0;
  ht = 0;

  for (int f = 0; f < faces.size(); ++f)
  {
    for (int vi = 0; vi < 3; vi++)
    {
      int v0 = tfaces[f][vi], v1 = tfaces[f][(vi + 1) % 3];
      Point2 p0 = uv[v0];
      Point2 p1 = uv[v1];
      // real len;

      p0.x += 0.5;
      p0.y += 0.5;
      p1.x += 0.5;
      p1.y += 0.5;

      real llen = length(p1 - p0), la, rxi;
#define DESIRED_BORDER 1.5
      for (la = 0; la < llen + 0.25; la += 0.25)
      {
        Point2 p = llen > 0.1 ? p1 * (la / llen) + p0 * (1 - la / llen) : p0;
        if (p.x < -10 || p.x > LIGHTMAPSZ + 10)
        {
          // debug ( "!!! la/llen=%.3f/%.3f  p=" FMT_P2 "  p0=" FMT_P2 " p1=" FMT_P2 "", la, llen, P2D(p), P2D(p0), P2D(p1));
          continue;
        }
        // p.x = floor ( p.x );
        // p.y = floor ( p.y );
        for (rxi = p.x - DESIRED_BORDER; rxi <= p.x + DESIRED_BORDER; rxi += 0.5)
        {
          int xi = rxi;
          if (xi < 0 || xi >= LIGHTMAPSZ)
            continue;
          int lwr = floor(p.y - DESIRED_BORDER), upr = ceil(p.y + DESIRED_BORDER);
          if (lwr < 0)
            lwr = 0;
          else if (lwr >= LIGHTMAPSZ)
            lwr = LIGHTMAPSZ - 1;
          if (upr < 0)
            upr = 0;
          else if (upr >= LIGHTMAPSZ)
            upr = LIGHTMAPSZ - 1;

          if (xi + 1 > wd)
            wd = xi + 1;
          if (lower[xi] > lwr)
            lower[xi] = lwr;
          if (upper[xi] < upr)
            upper[xi] = upr;
        }
        for (rxi = p.y - DESIRED_BORDER; rxi <= p.y + DESIRED_BORDER; rxi += 0.5)
        {
          int xi = rxi;
          if (xi < 0 || xi >= LIGHTMAPSZ)
            continue;
          int lwr = floor(p.x - DESIRED_BORDER), upr = ceil(p.x + DESIRED_BORDER);
          if (lwr < 0)
            lwr = 0;
          else if (lwr >= LIGHTMAPSZ)
            lwr = LIGHTMAPSZ - 1;
          if (upr < 0)
            upr = 0;
          else if (upr >= LIGHTMAPSZ)
            upr = LIGHTMAPSZ - 1;

          if (xi + 1 > ht)
            ht = xi + 1;
          if (leftmost[xi] > lwr)
            leftmost[xi] = lwr;
          if (rightmost[xi] < upr)
            rightmost[xi] = upr;
        }
      }
    }
  }

  if (wd > LIGHTMAPSZ || ht > LIGHTMAPSZ /*|| wd < MINMAPSZ || ht < MINMAPSZ*/)
    DAG_FATAL("out contracts failed: wd=%d ht=%d", wd, ht);
  // if ( wd < MINMAPSZ || ht < MINMAPSZ ) debug ( "*TS* w=%d h=%d", wd, ht );
  if (wd == 0 || ht == 0)
  {
    debug("wd=%d ht=%d, faces=%d, tfaces=%d uv=%d", wd, ht, faces.size(), tfaces.size(), uv.size());
    if (wd == 0)
      wd = 1;
    if (ht == 0)
      ht = 1;
  }
  finalized = false;
  transf = 0;
}
void FaceGroup::at_last_transform()
{
  int i;
  if (finalized)
    return;

  switch (transf)
  {
    case 0: // original (no transformation needed)
      return;
    case 1: // flip vertical
      for (i = 0; i < uv.size(); i++)
      {
        uv[i].y = ht - 1 - uv[i].y;
      }
      break;
    case 2: // flip horizontal
      for (i = 0; i < uv.size(); i++)
      {
        uv[i].x = wd - 1 - uv[i].x;
      }
      break;
    case 3: // flip x<->y (rotate +270 deg and horizontal flip)
      for (i = 0; i < uv.size(); i++)
      {
        Point2 p;
        p.x = uv[i].y;
        p.y = uv[i].x;
        uv[i] = p;
      }
      break;
    case 4: // rotate +90 deg
      for (i = 0; i < uv.size(); i++)
      {
        Point2 p;
        p.x = uv[i].y;
        p.y = wd - 1 - uv[i].x;
        uv[i] = p;
      }
      break;
    case 5: // rotate +90 deg + horizontal flip
      for (i = 0; i < uv.size(); i++)
      {
        Point2 p;
        p.x = ht - 1 - uv[i].y;
        p.y = wd - 1 - uv[i].x;
        uv[i] = p;
      }
      break;
    case 6: // rotate +270 deg
      for (i = 0; i < uv.size(); i++)
      {
        Point2 p;
        p.x = ht - 1 - uv[i].y;
        p.y = uv[i].x;
        uv[i] = p;
      }
      break;
    case 7: // rotate +180 deg
      for (i = 0; i < uv.size(); i++)
      {
        Point2 p;
        p.x = wd - 1 - uv[i].x;
        p.y = ht - 1 - uv[i].y;
        uv[i] = p;
      }
      break;
  }
  finalized = true;
}
int FaceGroup::adduv(const Point2 &p)
{
  for (int i = 0; i < uv.size(); i++)
    if (p.x == uv[i].x && p.y == uv[i].y)
      return i;
  uv.push_back(p);
  return uv.size() - 1;
}

static void unwrap_faces(Face *_face, int _fnum, Point3 *_vert, int _vnum, Tab<FaceGroup *> &fgrps);
static bool dagor_unwrap_faces(Face *face, int fnum, Point3 *vert, int vnum, Tab<FaceGroup *> &fgrps);

static int add_lightmapobject(Face *face, int fnum, Point3 *vert, int vnum, real sample_size, bool perobj, int unwrap_scheme)
{
  int l = append_items(ltmobjs, 1);

  ltmobjs[l].face = face;
  ltmobjs[l].fnum = fnum;
  ltmobjs[l].vert = vert;
  ltmobjs[l].vnum = vnum;
  ltmobjs[l].perobj = perobj;
  ltmobjs[l].sampleSize = sample_size;
#if USE_OWN_UNWRAP_MESH
  ltmobjs[l].unwrapScheme = 0;
#else
  ltmobjs[l].unwrapScheme = unwrap_scheme > 0 ? unwrap_scheme : 0;
#endif

  return l;
}

struct Lightmap
{

  struct LightmapPart
  {
    short used[MAX_LIGHTMAPSZ], upper[MAX_LIGHTMAPSZ];
    int S, left, right;

    void init_main()
    {
      left = 0;
      right = LIGHTMAPSZ - 1;
      S = LIGHTMAPSZ * LIGHTMAPSZ;
      for (int i = 0; i < LIGHTMAPSZ; i++)
      {
        used[i] = 0;
        upper[i] = LIGHTMAPSZ - 1;
      }
    }
    LightmapPart *create_copy()
    {
      LightmapPart *lmp = new (tmpmem) LightmapPart;
      memcpy(lmp, this, sizeof(LightmapPart));
      return lmp;
    }

    void init_empty()
    {
      left = 0;
      right = LIGHTMAPSZ - 1;
      S = 0;
      memset(used, 0, sizeof(used));     //  0
      memset(upper, 0xFF, sizeof(used)); // -1
    }

    bool find_bestxy(int w, int &best_transf, int &bestx, int &besty, const FaceGroup &fg, int &opt)
    {
      int i, j, k, best_S, dS, dy;
      short t_lwr[MAX_LIGHTMAPSZ], t_upr[MAX_LIGHTMAPSZ];
      const short *lwr = fg.lower, *upr = fg.upper;
      bool found = false;

      if (w == 0)
        DAG_FATAL("w=0");

      // debug ( "%p.find_bestxy", this );
      bestx = -1;
      besty = LIGHTMAPSZ;
      best_transf = -1;

      // check measure first
      best_S = 0;
      for (j = 0; j < w; ++j)
        if (upr[j] >= lwr[j])
          best_S += upr[j] - lwr[j] + 1;
      //*-debug*/ if ( S == LIGHTMAPSZ*LIGHTMAPSZ ) debug ( "fig.S=%d S=%d opt=%d", best_S, S, opt );
      if (best_S > S)
        return false;

      // detect small blocks (upper measure < 100)
      best_S = 0;
      for (j = 0; j < w; ++j)
        if (upr[j] >= lwr[j])
          best_S += upr[j] + 1;

      if (best_S < 100)
      {
        for (i = left; i <= right + 1 - w; ++i)
        {
          // find bottom line
          dy = 0;
          for (j = 0; j < w; ++j)
            if (upr[j] >= lwr[j])
            {
              if (used[i + j] - lwr[j] >= besty)
                break;
              if (used[i + j] > upper[i + j])
                break; // empty bar; no place

              if (used[i + j] - lwr[j] > dy)
                dy = used[i + j] - lwr[j];
            }
          if (j == w)
          {
            dS = (dy + fg.ht) / 2 + best_S * 2 / w;
            if (dS > opt)
              continue;

            bool ok = true;
            for (j = 0; j < w; ++j)
              if (upr[j] >= lwr[j])
                if (upr[j] + dy > upper[i + j])
                {
                  ok = false;
                  break;
                }
            if (!ok)
              continue; // search further...
            bestx = i;
            besty = dy;
            found = true;
            opt = dS;
          }
        }
        best_transf = 0;
        return found;
      }

      // for big block find optimal place and transformation
      lwr = t_lwr;
      upr = t_upr;
      best_S = opt;
      for (k = 0; k < 8; k++)
      {
        int mh;
        if (!fg.get_transformed(k, t_lwr, t_upr, w, mh))
          break;

        for (i = left; i <= right + 1 - w; ++i)
        {
          // find bottom line
          dy = 0;
          for (j = 0; j < w; j++)
            if (upr[j] >= lwr[j])
            {
              if (used[i + j] > upper[i + j])
              {
                dy = LIGHTMAPSZ;
                break;
              }

              if (used[i + j] - lwr[j] > dy)
                dy = used[i + j] - lwr[j];
            }

          if (dy >= LIGHTMAPSZ)
            continue;

          // calc optimization function
          dS = 0;
          for (j = 0; j < w; j++)
            if (upr[j] >= lwr[j])
              dS += dy + upr[j] - used[i + j];
          dS = (dy + mh) / 2 + dS * 2 / w;

          // get new place if (new measure < old measure) or
          // (new place is lower then old AND new measure not much greater)
          //*-debug-*/ if ( mh == LIGHTMAPSZ ) debug ( "dS=%d best_S=%d dy=%d besty=%d", dS, best_S, dy, besty );

          if (dS < best_S - 2 || (dS < best_S + 4 && dy < besty))
          {
            bool ok = true;
            for (j = 0; j < w; ++j)
              if (upr[j] >= lwr[j])
                if (upr[j] + dy > upper[i + j])
                {
                  //*-debug-*/ if ( mh == LIGHTMAPSZ ) debug ( "upr[%d]=%d + dy=%d > upper[%d]=%d", j, upr[j], dy, i+j, upper[i+j] );
                  ok = false;
                  break;
                }
            //*-debug-*/ if ( mh == LIGHTMAPSZ ) debug ( "ok=%d", ok );
            if (!ok)
              continue; // search further...

            bestx = i;
            besty = dy;
            best_S = dS;
            best_transf = k;
            found = true;
            opt = dS;
          }
        }
      }
      return found;
    }
    void mark_used_area(Tab<LightmapPart *> &lmp, const FaceGroup &fg, int ox, int oy, int transf)
    {
      short t_lwr[MAX_LIGHTMAPSZ], t_upr[MAX_LIGHTMAPSZ];
      const short *lwr = fg.lower, *upr = fg.upper;
      int w, h, i;
      LightmapPart *np = NULL;

      if (transf != 0)
      {
        lwr = t_lwr;
        upr = t_upr;
        fg.get_transformed(transf, t_lwr, t_upr, w, h);
      }
      else
      {
        w = fg.wd;
        h = fg.ht;
      }

      int new_s = 0;
      for (i = 0; i < w; ++i)
        if (upr[i] >= lwr[i] && oy + lwr[i] - 1 - used[ox + i] > 2)
          new_s += oy + lwr[i] - 1 - used[ox + i];

      if (new_s > 100)
      {
        np = new (midmem) LightmapPart;
        np->init_empty();
        np->left = ox;
        np->right = ox + w - 1;
        for (i = 0; i < w; ++i)
          if (upr[i] >= lwr[i] && oy + lwr[i] - 1 - used[ox + i] > 2)
          {
            np->used[ox + i] = used[ox + i];
            np->upper[ox + i] = oy + lwr[i] - 1;
            np->S += np->upper[ox + i] - np->used[ox + i];
          }
        np->recalc();
      }

      for (i = 0; i < w; i++)
        if (upr[i] >= lwr[i])
        {
          S -= oy + upr[i] - used[ox + i];
          used[ox + i] = oy + upr[i];
        }

      recalc();
      if (np)
      {
        if (np->try_merge(lmp))
          delete np;
        else
          append_items(lmp, 1, &np);
      }
    }

    void recalc()
    {
      // recalc boundaries
      left = 0;
      right = LIGHTMAPSZ - 1;

      while (left < LIGHTMAPSZ && used[left] >= upper[left])
        left++;
      while (right > 0 && used[right] >= upper[right])
        right--;

      S = 0;
      for (int i = left; i <= right; i++)
        if (used[i] < upper[i])
          S += upper[i] - used[i];
    }

    bool try_merge(Tab<LightmapPart *> &lmp)
    {
      for (int i = 0; i < lmp.size(); i++)
      {
        if (left == lmp[i]->right + 1)
        {
          //*-debug-*/ debug ( "merged %d+%d=%d", S, lmp[i]->S, S+lmp[i]->S );
          lmp[i]->S += S;
          lmp[i]->right = right;
          for (int j = left; j <= right; j++)
          {
            lmp[i]->used[j] = used[j];
            lmp[i]->upper[j] = upper[j];
          }
          return true;
        }
        if (right == lmp[i]->left - 1)
        {
          //*-debug-*/ debug ( "merged %d+%d=%d", S, lmp[i]->S, S+lmp[i]->S );
          lmp[i]->S += S;
          lmp[i]->left = left;
          for (int j = left; j <= right; j++)
          {
            lmp[i]->used[j] = used[j];
            lmp[i]->upper[j] = upper[j];
          }
          return true;
        }
      }
      return false;
    }

    void dbglog()
    {
      debug("   S=%d left=%d right=%d", S, left, right);
      for (int i = left; i <= right; i++)
        debug("   %d: %d %d", i, used[i], upper[i]);
    }

    void draw(FILE *svg, float scale, const Point2 &ofs)
    {
      recalc();
      if (left >= right)
        return;

      int l = left, r = right, i;

      do
      {
        svg_start_poly(svg, Point2(l, used[l]) * scale + ofs);

        for (i = l; i <= r; i++)
          if (used[i] < upper[i])
            svg_add_poly_point(svg, Point2(i, used[i]) * scale + ofs);
          else
            break;

        r = i + 1;
        i--;

        for (; i >= l; i--)
          svg_add_poly_point(svg, Point2(i, upper[i]) * scale + ofs);

        svg_add_poly_point(svg, Point2(l, used[l]) * scale + ofs);

        svg_end_poly(svg);

        l = r;
        r = right;
        while (l < r && used[l] >= upper[l])
          l++;
      } while (l < right);
    }
  };

  Tab<LightmapPart *> lmp;
  bool bad, copy;
  int states_used;

  Tab<Lightmap *> state_stack;

  Lightmap() : state_stack(tmpmem), lmp(tmpmem)
  {
    bad = false;
    copy = false;
    states_used = 0;

    lmp.reserve(16);
    lmp.resize(1);
    lmp[0] = new (midmem) LightmapPart;
    lmp[0]->init_main();
    // debug("creating lightmap mapping");
    // DEBUG_CTX("memory used=%d", get_max_mem_used());
  }
  Lightmap(Lightmap *lm) : state_stack(tmpmem), lmp(tmpmem)
  {
    lmp.resize(lm->lmp.size());
    for (int i = 0; i < lmp.size(); i++)
      lmp[i] = lm->lmp[i]->create_copy();
    bad = lm->bad;
    copy = true;
  }
  ~Lightmap()
  {
    int i;
    // if ( !copy )
    //   debug ( "lightmap %p: states_used=%d state_stack.count=%d",
    //           this, states_used, state_stack.size());

    for (i = 0; i < lmp.size(); i++)
      delete lmp[i];
    clear_and_shrink(lmp);
  }

  void restore_from_copy(Lightmap *lm)
  {
    if (!lm->copy)
      DAG_FATAL("copy!= true!!!");

    bad = lm->bad;
    int i;
    for (i = 0; i < lmp.size(); i++)
      delete lmp[i];
    lmp.resize(lm->lmp.size());
    for (i = 0; i < lmp.size(); i++)
      lmp[i] = lm->lmp[i];
    clear_and_shrink(lm->lmp);

    delete lm;
  }

  void push_state()
  {
    Lightmap *lm = new (tmpmem) Lightmap(this);
    append_items(state_stack, 1, &lm);
    states_used++;
  }
  void pop_state()
  {
    G_ASSERT(state_stack.size());
    restore_from_copy(state_stack.back());
    state_stack.pop_back();
  }
  void remove_state()
  {
    G_ASSERT(state_stack.size());
    delete state_stack.back();
    state_stack.pop_back();
  }

  LightmapPart *find_best_place(int w, int &transf, int &ox, int &oy, const FaceGroup &fg)
  {
    int opt = 4 * LIGHTMAPSZ, i;
    int best_transf, bestx, besty;
    LightmapPart *found = NULL;

    for (i = 0; i < lmp.size(); i++)
    {
      // debug ( "lmp[%d/%d]=%p", i, lmp.size(), lmp[i] );
      if (lmp[i]->S < fg.minS)
        continue;

      if (lmp[i]->find_bestxy(w, best_transf, bestx, besty, fg, opt))
      {
        found = lmp[i];
        transf = best_transf;
        ox = bestx;
        oy = besty;
      }
      else if (lmp[i]->S == LIGHTMAPSZ * LIGHTMAPSZ && opt == 4 * LIGHTMAPSZ)
      {
        debug("obj_per_lt_map: add block: wd=%d, ht=%d failed", fg.wd, fg.ht);
        debug("  figure dimensions:");
        int z;
        for (z = 0; z < fg.wd; z++)
          debug("   h%d: %d %d", z, fg.lower[z], fg.upper[z]);
        for (z = 0; z < fg.ht; z++)
          debug("   v%d: %d %d", z, fg.leftmost[z], fg.rightmost[z]);
        debug("  ltmap dimensions:");
        lmp[i]->dbglog();
        DAG_FATAL("stop point");
      }
    }
    return found;
  }
  bool add_block(int w, int h, int &transf, int &ox, int &oy, const FaceGroup &fg)
  {
    int bestx, besty = LIGHTMAPSZ, best_transf, i;
    LightmapPart *p = find_best_place(w, best_transf, bestx, besty, fg);
    if (!p)
      return false;

    ox = bestx;
    oy = besty;
    transf = best_transf;
    p->mark_used_area(lmp, fg, bestx, besty, best_transf);
    return true;
  }

  int getUnusedArea()
  {
    int s = 0;
    for (int i = 0; i < lmp.size(); i++)
      s += lmp[i]->S;
    return s;
  }

  void drawParts(FILE *svg, float scale, int ofs_x, int ofs_y)
  {
    for (int i = 0; i < lmp.size(); i++)
      lmp[i]->draw(svg, scale, Point2(ofs_x, ofs_y));
  }
};

static Tab<Lightmap *> ltmap(tmpmem_ptr());


static void put_mapping_on_facegroup(FaceGroup &g, TFace *tface, Tab<Point2> &tvert)
{
  if (g.sv >= 0)
  {
    int fi = g.faces[0];
    set_lmid(fi, g.lmid);
    int t0 = append_items(tvert, 3);

    tvert[t0 + 0].x = (g.ox + .5f) / real(LIGHTMAPSZ);
    tvert[t0 + 0].y = (g.oy + .5f) / real(LIGHTMAPSZ);
    tvert[t0 + 1].x = (g.ox + .5f) / real(LIGHTMAPSZ);
    tvert[t0 + 1].y = (FACEMAPSZ + g.oy + .5f) / real(LIGHTMAPSZ);
    tvert[t0 + 2].x = (FACEMAPSZ + g.ox + .5f) / real(LIGHTMAPSZ);
    tvert[t0 + 2].y = (g.oy + .5f) / real(LIGHTMAPSZ);
    tface[fi].t[0] = t0;
    tface[fi].t[1] = t0 + 1;
    tface[fi].t[2] = t0 + 2;
  }
  else
  {
    g.at_last_transform();

    int t0 = append_items(tvert, g.uv.size());

    for (int v = 0; v < g.uv.size(); ++v)
    {
      Point2 &p = g.uv[v];
      tvert[t0 + v].x = (p.x + g.ox + 0.5) / real(LIGHTMAPSZ);
      tvert[t0 + v].y = (p.y + g.oy + 0.5) / real(LIGHTMAPSZ);
    }
    for (int f = 0; f < g.faces.size(); ++f)
    {
      int fi = g.faces[f];

      set_lmid(fi, g.lmid);
      tface[fi].t[0] = t0 + g.tfaces[f][0];
      tface[fi].t[1] = t0 + g.tfaces[f][1];
      tface[fi].t[2] = t0 + g.tfaces[f][2];
    }
  }
}

static int cmp_ltmap_objs_ids(const int *a, const int *b)
{
  int res = ltmobjs[*b].avgS - ltmobjs[*a].avgS;
  if (!res)
    return ltmobjs[*b].maxS - ltmobjs[*a].maxS;
  return res;
}

struct FaceGroupCompare
{
  static int compare(FaceGroup *a, FaceGroup *b)
  {
    int res = b->maxS - a->maxS;
    if (!res)
      return b->minS - a->minS;
    else
      return res;
  }
};
} // namespace LightMapMapper


//
// public AutoUnwrapMapping interface
//
using namespace LightMapMapper;

static Tab<Point2> tmp_tvert(tmpmem_ptr());

#if DEBUG_SHOW_FINAL_MAPPING
static FILE *fp_svg = NULL;
#endif

void AutoUnwrapMapping::start_mapping(int lm_sz)
{
  clear_mapping_stat();
  for (int lm = 0; lm < ltmap.size(); lm++)
    delete ltmap[lm];
  clear_and_shrink(ltmap);

  LIGHTMAPSZ = 2048;
  for (int i = 32; i <= 2048; i *= 2)
    if (lm_sz <= i)
    {
      LIGHTMAPSZ = i;
      break;
    }
  debug("LM mapping: using lightmaps %dx%d", LIGHTMAPSZ, LIGHTMAPSZ);
}

int AutoUnwrapMapping::request_mapping(Face *face, int fnum, Point3 *vert, int vnum, float samplesize, bool perobj, int unwrap_scheme)
{
  lmid = NULL;
  lmid_stride = 0;
  return add_lightmapobject(face, fnum, vert, vnum, samplesize, perobj, unwrap_scheme);
}

void AutoUnwrapMapping::prepare_facegroups(IGenericProgressIndicator &pbar, SimpleProgressCB &progr_cb)
{
  FullFileSaveCB uv_orig_cwr("facegrp.orig.uv");

  pbar.setDone(0);
  pbar.setTotal(ltmobjs.size());
  for (int l = 0; l < ltmobjs.size(); ++l)
  {
    if (progr_cb.mustStop)
      return;

    // Mesh-unwrap Mapping
    if (ltmobjs[l].unwrapScheme == 0)
      dagor_unwrap_faces(ltmobjs[l].face, ltmobjs[l].fnum, ltmobjs[l].vert, ltmobjs[l].vnum, ltmobjs[l].fgrps);
    else if (ltmobjs[l].unwrapScheme == 1)
      unwrap_faces(ltmobjs[l].face, ltmobjs[l].fnum, ltmobjs[l].vert, ltmobjs[l].vnum, ltmobjs[l].fgrps);

    pbar.incDone();
    for (int i = 0; i < ltmobjs[l].fgrps.size(); ++i)
    {
      ltmobjs[l].fgrps[i]->objid = l;
      ltmobjs[l].fgrps[i]->saveOriginalUv(uv_orig_cwr);
    }
  }
}

int AutoUnwrapMapping::repack_facegroups(float samplesize_scale, float *out_lm_area_usage_ratio, int max_lm_num,
  IGenericProgressIndicator &pbar, SimpleProgressCB &progr_cb)
{
  Bitarray fgrpsset;

  pbar.setDone(0);
  pbar.setTotal(ltmobjs.size() + 3);

  // clear exisiting lightmap to perform packing anew
  for (int lm = 0; lm < ltmap.size(); lm++)
    delete ltmap[lm];
  clear_and_shrink(ltmap);

  pbar.incDone();

  FullFileLoadCB uv_orig_crd("facegrp.orig.uv");

  // measure all objects/facegroups
  Tab<int> obj_order(tmpmem);
  obj_order.resize(ltmobjs.size());
  for (int i = 0; i < ltmobjs.size(); ++i)
  {
    if (progr_cb.mustStop)
      return -1;

    for (int j = 0; j < ltmobjs[i].fgrps.size(); ++j)
      ltmobjs[i].fgrps[j]->loadOriginalUv(uv_orig_crd);

    ltmobjs[i].prepareForPacking(samplesize_scale);
    ltmobjs[i].measure();
    ltmobjs[i].finishPacking();
    obj_order[i] = i;
  }
  sort(obj_order, &cmp_ltmap_objs_ids);

  pbar.incDone();

  // pack all objects
  for (int i = 0; i < obj_order.size(); ++i)
  {
    if (progr_cb.mustStop)
      return -1;

    LightMapObject &obj = ltmobjs[obj_order[i]];
    obj.recalcHist();

    SimpleQsort<FaceGroup *, FaceGroupCompare>::sort(&obj.fgrps[0], obj.fgrps.size());

    if (obj.perobj && obj.minS < LIGHTMAPSZ * LIGHTMAPSZ * 0.9)
    {
      fgrpsset.resize(obj.fgrps.size());
      fgrpsset.reset();

      // try to put all groups in one lightmap creating new or inserting to current,
      // until all groups will be placed
      for (int setcnt = 0; setcnt < obj.fgrps.size();)
      {
        int lm;

        // try to insert in current lightmap
        for (lm = 0; lm < ltmap.size(); ++lm)
        {
          ltmap[lm]->push_state();
          int j;
          for (j = 0; j < obj.fgrps.size(); ++j)
            if (!fgrpsset[j])
            {
              FaceGroup &g = *obj.fgrps[j];
              if (!ltmap[lm]->add_block(g.wd, g.ht, g.transf, g.ox, g.oy, g))
                break;
            }
          if (j < obj.fgrps.size())
            ltmap[lm]->pop_state();
          else
          {
            for (int j = 0; j < obj.fgrps.size(); ++j)
              if (!fgrpsset[j])
              {
                FaceGroup &g = *obj.fgrps[j];
                g.lmid = lm;
                fgrpsset.set(j);
                setcnt++;
              }
            ltmap[lm]->remove_state();
            break;
          }
        }

        // create new
        if (lm >= ltmap.size())
        {
          lm = append_items(ltmap, 1);

          ltmap[lm] = new (tmpmem) Lightmap;

          if (max_lm_num > 0 && ltmap.size() > max_lm_num)
          {
            obj.finishPacking();
            if (out_lm_area_usage_ratio)
              *out_lm_area_usage_ratio = 0.0;
            return ltmap.size();
          }

          // debug ( "obj_per_lt_map: add ltmap" );
          for (int j = 0; j < obj.fgrps.size(); ++j)
            if (!fgrpsset[j])
            {
              FaceGroup &g = *obj.fgrps[j];
              if (ltmap[lm]->add_block(g.wd, g.ht, g.transf, g.ox, g.oy, g))
              {
                // add to lm
                g.lmid = lm;
                fgrpsset.set(j);
                setcnt++;
              }
            }
        }
      }
    }
    else
      for (int j = 0; j < obj.fgrps.size(); ++j)
      {
        FaceGroup &g = *obj.fgrps[j];
        int lm;

        for (lm = 0; lm < ltmap.size(); ++lm)
          if (ltmap[lm]->add_block(g.wd, g.ht, g.transf, g.ox, g.oy, g))
          {
            g.lmid = lm;
            break;
          }

        if (lm >= ltmap.size())
        {
          // create another lightmap
          // debug ( "add ltmap after add block: wd=%d, ht=%d", g.wd, g.ht );
          lm = append_items(ltmap, 1);

          ltmap[lm] = new (tmpmem) Lightmap;

          if (max_lm_num > 0 && ltmap.size() > max_lm_num)
          {
            obj.finishPacking();
            if (out_lm_area_usage_ratio)
              *out_lm_area_usage_ratio = 0.0;
            return ltmap.size();
          }

          if (!ltmap[lm]->add_block(g.wd, g.ht, g.transf, g.ox, g.oy, g))
            DAG_FATAL("cannot fit %d,%d into newly added map", g.wd, g.ht);

          g.lmid = lm;
        }
      }


    obj.finishPacking();
    pbar.incDone();
  }

  if (out_lm_area_usage_ratio)
  {
    int unused_pix = 0;
    int total_area = ltmap.size() * LIGHTMAPSZ * LIGHTMAPSZ;

    for (int i = 0; i < ltmap.size(); i++)
      unused_pix += ltmap[i]->getUnusedArea();

    *out_lm_area_usage_ratio = double(total_area - unused_pix) / double(LIGHTMAPSZ * LIGHTMAPSZ);
  }

#if DEBUG_SHOW_FINAL_MAPPING
  if (fp_svg)
    svg_close(fp_svg);

  fp_svg = svg_open(String(64, "mapping_%0.1f.svg", samplesize_scale), (ltmap.size() + 1) / 2 * SVG_LMSZ, SVG_LMSZ * 2);

#if DEBUG_SHOW_FINAL_MAPPING >= 2
  svg_begin_group(fp_svg, "opacity=\"1.0\" fill=\"red\" stroke=\"black\" stroke-width=\"0.5\"");
  for (int i = 0; i < ltmap.size(); i++)
    ltmap[i]->drawParts(fp_svg, double(SVG_LMSZ) / LIGHTMAPSZ, (i / 2) * SVG_LMSZ, (i % 2) * SVG_LMSZ);
  svg_end_group(fp_svg);
#endif
#endif

  pbar.incDone();

  return ltmap.size();
}

void AutoUnwrapMapping::get_final_mapping(int handle, unsigned short *lmids, int lmid_str, TFace *tface, Point2 *&tvptr, int &numtv)
{
  if (handle < 0 || handle >= ltmobjs.size())
    return;

  // calculate final mapping
  lmid = lmids;
  lmid_stride = lmid_str;
  tmp_tvert.clear();

  for (int i = 0; i < ltmobjs[handle].fnum; ++i)
    set_lmid(i, -1);

  for (int i = 0; i < ltmobjs[handle].fgrps.size(); i++)
    put_mapping_on_facegroup(*ltmobjs[handle].fgrps[i], tface, tmp_tvert);

  for (int i = 0; i < ltmobjs[handle].fnum; ++i)
    if (get_lmid(i) == uint16_t(-1))
      tface[i].t[0] = tface[i].t[1] = tface[i].t[2] = 0;

#if DEBUG_SHOW_FINAL_MAPPING
  svg_begin_group(fp_svg, "opacity=\"0.9\" fill=\"yellow\" stroke=\"blue\" stroke-width=\"0.5\"");

  svg_draw_line(fp_svg, Point2(0, 0), Point2((ltmap.size() + 1) / 2 * SVG_LMSZ, 0));

  svg_draw_line(fp_svg, Point2(0, SVG_LMSZ), Point2((ltmap.size() + 1) / 2 * SVG_LMSZ, SVG_LMSZ));

  svg_draw_line(fp_svg, Point2(0, SVG_LMSZ * 2), Point2((ltmap.size() + 1) / 2 * SVG_LMSZ, SVG_LMSZ * 2));

  for (int i = (ltmap.size() + 1) / 2; i >= 0; i--)
    svg_draw_line(fp_svg, Point2(i * SVG_LMSZ, 0), Point2(i * SVG_LMSZ, SVG_LMSZ * 2));

  for (int i = 0; i < ltmobjs[handle].fnum; i++)
  {
    Point2 ofs((get_lmid(i) / 2) * SVG_LMSZ, (get_lmid(i) % 2) * SVG_LMSZ);

    svg_draw_face(fp_svg, tmp_tvert[tface[i].t[0]] * SVG_LMSZ + ofs, tmp_tvert[tface[i].t[1]] * SVG_LMSZ + ofs,
      tmp_tvert[tface[i].t[2]] * SVG_LMSZ + ofs);
  }
  svg_end_group(fp_svg);
#endif

  tvptr = &tmp_tvert[0];
  numtv = tmp_tvert.size();
}

int AutoUnwrapMapping::num_lightmaps() { return ltmap.size(); }

void AutoUnwrapMapping::get_lightmap_size(int i, int &w, int &h)
{
  w = LIGHTMAPSZ;
  h = LIGHTMAPSZ;
}

void AutoUnwrapMapping::finish_mapping()
{
#if DEBUG_SHOW_FINAL_MAPPING
  if (fp_svg)
    svg_close(fp_svg);
  fp_svg = NULL;
#endif

  clear_and_shrink(tmp_tvert);
  print_mapping_stat();
  clear_and_shrink(ltmobjs);

  ::unlink("facegrp.orig.uv");
}


//
// mesh unwrapping
//
#include "unwrapMesh.inc.cpp"
static bool LightMapMapper::dagor_unwrap_faces(Face *face, int fnum, Point3 *vert, int vnum, Tab<FaceGroup *> &fgrps)
{
  MeshUnwrap::MeshUnwrapProcessor mup(face, fnum, vert, vnum);
  return mup.Process(fgrps);
}

#if USE_OWN_UNWRAP_MESH

static void LightMapMapper::unwrap_faces(Face *face, int fnum, Point3 *vert, int vnum, Tab<FaceGroup *> &fgrps)
{
  bool ret = dagor_unwrap_faces(face, fnum, vert, vnum, fgrps);
  DEBUG_CTX("memory used=%d", get_max_mem_used());
}
void AutoUnwrapMapping::set_graphite_folder(const char *) {}

#else

#include <generic/dag_smallTab.h>
#include <util/dag_simpleString.h>
#include <windows.h>
#include <stdlib.h>

static dag::ConstSpan<Face> face1;
static Tab<int> face1_gr(tmpmem_ptr());
static int face1_grpcnt = 0, call_no = 0;

static SimpleString graphiteWorkDir;

struct UnwrapRetData
{
  int tc[3];
  int gr;
};

static bool isAdjacent(int i, int j)
{
  int e = 0;

  if (face1[i].v[0] == face1[j].v[0] || face1[i].v[0] == face1[j].v[1] || face1[i].v[0] == face1[j].v[2])
    e++;

  if (face1[i].v[1] == face1[j].v[0] || face1[i].v[1] == face1[j].v[1] || face1[i].v[1] == face1[j].v[2])
    e++;

  if (face1[i].v[2] == face1[j].v[0] || face1[i].v[2] == face1[j].v[1] || face1[i].v[2] == face1[j].v[2])
    e++;
  return e == 2;
}

static void assignFaceGroups()
{
  Tab<int> idx(tmpmem);
  Tab<int> open_idx(tmpmem);

  idx.resize(face1.size() - 1);
  for (int i = 0; i < idx.size(); i++)
    idx[i] = i;

  int cur_face = face1.size() - 1, face_cnt = 1;
  face1_gr[cur_face] = face1_grpcnt;

  while (idx.size() > 0 || open_idx.size() > 0)
  {
    int smgr = face1[cur_face].smgr;
    for (int i = idx.size() - 1; i >= 0; i--)
      if (face1[idx[i]].smgr & smgr)
        if (isAdjacent(idx[i], cur_face))
        {
          open_idx.push_back(idx[i]);
          erase_items(idx, i, 1);
        }

    if (open_idx.size() == 0)
    {
      if (idx.size() > 0)
      {
        cur_face = idx.back();
        idx.pop_back();
        face1_grpcnt++;
        face1_gr[cur_face] = face1_grpcnt;
        face_cnt = 1;
      }
    }
    else
    {
      cur_face = open_idx.back();
      face1_gr[cur_face] = face1_grpcnt;
      open_idx.pop_back();
      face_cnt++;
    }
  }
  face1_grpcnt++;
}

static void saveSvg(const char *file_name, const FaceGroup &g)
{
  Point2 minv(MAX_REAL, MAX_REAL), maxv(-MAX_REAL, -MAX_REAL);
  double scale = 1.0;

  for (int i = 0; i < g.tfaces.size(); i++)
  {
    const Point2 &tc0 = g.uv[g.tfaces[i][0]];
    const Point2 &tc1 = g.uv[g.tfaces[i][1]];
    const Point2 &tc2 = g.uv[g.tfaces[i][2]];
    if (minv.x > tc0.x)
      minv.x = tc0.x;
    if (minv.x > tc1.x)
      minv.x = tc1.x;
    if (minv.x > tc2.x)
      minv.x = tc2.x;
    if (minv.y > tc0.y)
      minv.y = tc0.y;
    if (minv.y > tc1.y)
      minv.y = tc1.y;
    if (minv.y > tc2.y)
      minv.y = tc2.y;

    if (maxv.x < tc0.x)
      maxv.x = tc0.x;
    if (maxv.x < tc1.x)
      maxv.x = tc1.x;
    if (maxv.x < tc2.x)
      maxv.x = tc2.x;
    if (maxv.y < tc0.y)
      maxv.y = tc0.y;
    if (maxv.y < tc1.y)
      maxv.y = tc1.y;
    if (maxv.y < tc2.y)
      maxv.y = tc2.y;
  }

  if (maxv.x - minv.x < maxv.y - minv.y)
  {
    if (maxv.x - minv.x < 700)
      scale = 700 / (maxv.x - minv.x);
    else if (maxv.x - minv.x > 1500)
      scale = 1500 / (maxv.x - minv.x);
  }
  else
  {
    if (maxv.y - minv.y < 700)
      scale = 700 / (maxv.y - minv.y);
    else if (maxv.y - minv.y > 1500)
      scale = 1500 / (maxv.y - minv.y);
  }

  FILE *fp = svg_open(file_name, (maxv.x - minv.x) * scale, (maxv.y - minv.y) * scale);
  if (!fp)
    return;

  svg_begin_group(fp, "opacity=\"0.9\" fill=\"yellow\" stroke=\"blue\" stroke-width=\"0.5\"");

  for (int i = 0; i < g.tfaces.size(); i++)
  {
    const Point2 &tc0 = g.uv[g.tfaces[i][0]];
    const Point2 &tc1 = g.uv[g.tfaces[i][1]];
    const Point2 &tc2 = g.uv[g.tfaces[i][2]];

    svg_draw_face(fp, scale * (tc0 - minv), scale * (tc1 - minv), scale * (tc2 - minv));
  }
  svg_end_group(fp);

  svg_close(fp);
}

static void LightMapMapper::unwrap_faces(Face *face, int fnum, Point3 *vert, int vnum, Tab<FaceGroup *> &fgrps)
{
  int fgrp_base = fgrps.size();

  face1.set(face, fnum);
  face1_gr.resize(fnum);
  face1_grpcnt = 0;
  assignFaceGroups();

  char cwd[260];
  ::getcwd(cwd, 260);
  ::chdir(graphiteWorkDir);

  // write unwrapper input data
  FILE *fp = fopen("input.mobj", "wb");
  if (fp)
  {
    fwrite(&vnum, 4, 1, fp);
    fwrite(vert, vnum, sizeof(Point3), fp);

    fwrite(&fnum, 4, 1, fp);
    fwrite(&face1_grpcnt, 4, 1, fp);

    for (int i = 0; i < fnum; i++)
    {
      fwrite(face[i].v, 1, sizeof(face[i].v), fp);
      fwrite(&face1_gr[i], 1, 4, fp);
    }
    fclose(fp);

    ::unlink("output.mobj");


    // exec process
    STARTUPINFO cif;
    PROCESS_INFORMATION pi;

    ZeroMemory(&cif, sizeof(cif));
    ZeroMemory(&pi, sizeof(pi));
    cif.wShowWindow = SW_SHOWMINNOACTIVE;

    char cmdline[] = "/c graphite.exe /quiet >nul 2>nul";
    if (::CreateProcess(getenv("ComSpec"), cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &cif, &pi))
      while (::MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLEVENTS) != WAIT_OBJECT_0)
      {
        static MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
          if (!GetMessage(&msg, NULL, 0, 0))
            break;
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }


    // read output data (unwrapper results)
    fp = fopen("output.mobj", "rb");
    if (fp)
    {
      SmallTab<Point2, TmpmemAlloc> uv;
      SmallTab<UnwrapRetData, TmpmemAlloc> fc;
      int n;

      fread(&n, 4, 1, fp);
      clear_and_resize(uv, n);
      fread(&uv[0], sizeof(Point2), n, fp);

      fread(&n, 4, 1, fp);
      if (fnum != n)
      {
        fclose(fp);
        fp = NULL;
        goto grafite_fails;
      }

      clear_and_resize(fc, fnum);
      fread(&fc[0], sizeof(UnwrapRetData), fnum, fp);

      fclose(fp);

      // build face group data
      for (int i = 0; i < fnum; i++)
      {
        int gr = fgrp_base + fc[i].gr;

        while (gr >= fgrps.size())
          fgrps.push_back(nullptr);

        if (!fgrps[gr])
          fgrps[gr] = new (midmem) FaceGroup;

        fgrps[gr]->faces.push_back(i);
      }

      for (int i = fgrps.size() - 1; i >= fgrp_base; i--)
      {
        if (!fgrps[i])
        {
          erase_items(fgrps, i, 1);
          continue;
        }

        FaceGroup &g = *fgrps[i];

        g.tfaces.resize(g.faces.size());

        double tex_area = 0, world_area = 0;
        for (int j = 0; j < g.faces.size(); j++)
        {
          UnwrapRetData &f = fc[g.faces[j]];
          uint32_t *fv = face[g.faces[j]].v;
          g.tfaces[j][0] = g.adduv(uv[f.tc[0]]);
          g.tfaces[j][1] = g.adduv(uv[f.tc[1]]);
          g.tfaces[j][2] = g.adduv(uv[f.tc[2]]);

          world_area += length((vert[fv[1]] - vert[fv[0]]) % (vert[fv[2]] - vert[fv[0]]));
          Point2 e1 = uv[f.tc[1]] - uv[f.tc[0]];
          Point2 e2 = uv[f.tc[2]] - uv[f.tc[0]];
          tex_area += length(Point3(e1.x, e1.y, 0) % Point3(e2.x, e2.y, 0));
        }

        BBox2 fgbb;
        fgbb.setempty();
        for (int j = 0; j < g.uv.size(); j++)
          fgbb += g.uv[j];

        double lin_ratio = sqrt(world_area / tex_area);
        for (int j = 0; j < g.uv.size(); j++)
          g.uv[j] = (g.uv[j] - fgbb.lim[0]) * lin_ratio;

        DEBUG_CTX("facegroup %d: %d faces, ratio=%.5f (lin %.5f)", i, g.faces.size(), world_area / tex_area, lin_ratio);
      }
    }
    else
    {
    grafite_fails:
      DEBUG_CTX("could not read results, using internal unwrapper");
      dagor_unwrap_faces(face, fnum, vert, vnum, fgrps);
    }

    // for ( int i = fgrps.size()-1; i >= fgrp_base; i -- )
    //   saveSvg(String(128,"%d_fg%d.svg", call_no, i), *fgrps[i]);
  }
  ::chdir(cwd);
  call_no++;
}
void AutoUnwrapMapping::set_graphite_folder(const char *folder) { graphiteWorkDir = folder; }

#endif

static void print_mapping_stat()
{
  debug("\n\nltmap.count=%d", ltmap.size());
#if USE_OWN_UNWRAP_MESH
  MeshUnwrap::print_stat();
#endif
}
static void clear_mapping_stat()
{
#if USE_OWN_UNWRAP_MESH
  MeshUnwrap::clear_stat();
#endif
}
