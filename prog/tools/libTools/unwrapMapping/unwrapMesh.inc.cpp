//
// Mesh unwrap tool
//
#include <supp/dag_math.h>
#include <math/dag_mesh.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/svgWrite.h>
#include <debug/dag_debug.h>
#include <stdio.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTICE: expected that this file is included at the bottom of ltmpmap.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define __TRY_REMAP_VERTS__     0
#define OLD_ALIGNMENT_OPTIMIZER 0
#define EXCESSIVE_DEBUG         0

namespace MeshUnwrap
{

__forceinline Point2 floor(const Point2 &a) { return Point2(floorf(a.x), floorf(a.y)); }
__forceinline Point2 __round(const Point2 &a) { return Point2(floorf(a.x + 0.5), floorf(a.y + 0.5)); }
__forceinline real operator%(const Point2 &a, const Point2 &b) { return a.x * b.y - a.y * b.x; }

// forward declarations
struct MU_Edge;
struct MU_Face;
struct MU_OpenEdge;

// core structures
struct MU_Edge
{
  real v2_x,          // coordinate of 3rd point projected on edge
    v2_y;             // coordinate of 3rd point projected on perpendicular edge
  int fIndex, eIndex; // =-1 when no face attached
  real len;
};

struct MU_Face
{
  MU_Edge e[3];
  Point2 uv[3];
  int v[3];
  int flags;
  int open_no;
  real square, av_len;
};

struct MU_OpenEdge
{
  Point2 v0, v1;
  int fIndex, eIndex;
  real len, wt;
};

enum
{
  MUFF_INVALID = 0x0001,
  MUFF_PROCESSED = 0x0002,
  MUFF_CANT_UNFOLD0 = 0x0004,
  MUFF_CANT_UNFOLD1 = 0x0008,
  MUFF_CANT_UNFOLD2 = 0x0010,
  MUFF_SKIPPED = 0x0020,
  MUFF_MARKED_OLD = 0x0040,
};

// statistics
int stat_objnum = 0, stat_total_fnum = 0, stat_fnum = 0, stat_fnum_bad = 0, stat_flinked = 0, stat_grpnum = 0, stat_fnum_p = 0,
    stat_snapped = 0;

// given we have OpenEdge, next (3rd) point is calculated as:
//   v2 = v0 + (v1-v0)*v2_x + perp(v1-v0)*v2_y

//
// implementation
//
class MeshUnwrapProcessor
{
  Tab<MU_Face> mf;
  Tab<MU_OpenEdge> moe;
  int start_face;
  int mf_left, mf_linked, open_no;
  Point2 rot[2];
  Point2 c_min, c_max;

public:
  MeshUnwrapProcessor(Face *face, int fnum, Point3 *vert, int vnum) : mf(tmpmem), moe(tmpmem)
  {
    mf.resize(fnum);
    moe.reserve(128);
    stat_objnum++;
    stat_total_fnum += fnum;

    // debug ( "face_num=%d, vert_num=%d", fnum, vnum );
    debug_flush(false);

    // precalculate data
    int i, j;
    real longest_edge = 0;
    start_face = -1;
    mf_left = 0;
    mf_linked = 0;
    open_no = 0;

#if __TRY_REMAP_VERTS__
    int *vremap = 0;
#endif

    // calculate helper vectors
    for (i = 0; i < mf.size(); i++)
    {
      Point3 v0 = vert[face[i].v[0]], v1 = vert[face[i].v[1]], v2 = vert[face[i].v[2]], en;
      real e0_l2 = lengthSq(v1 - v0), e1_l2 = lengthSq(v2 - v1), e2_l2 = lengthSq(v0 - v2);

      if (e0_l2 == 0 || e1_l2 == 0 || e2_l2 == 0)
      {
        mf[i].flags = MUFF_INVALID | MUFF_PROCESSED | MUFF_MARKED_OLD;
        {
          debug("inv.face: %.3f,%.3f,%.3f lmid=%d; v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 " face.v=%d,%d,%d", e0_l2, e1_l2, e2_l2,
            lmid, P3D(v0), P3D(v1), P3D(v2), face[i].v[0], face[i].v[1], face[i].v[2]);

          // ban invalid face
          stat_fnum_bad++;

#if __TRY_REMAP_VERTS__
          // fix
          if (!vremap)
          {
            vremap = new (tmpmem) int[vnum];
            memset(vremap, 0xFF, 4 * vnum);
          }
          int vn0 = face[i].v[0], vn1 = face[i].v[1], vn2 = face[i].v[2];
          if (e0_l2 == 0 && vn0 != vn1)
          {
            if (vn0 < vn1)
              vremap[vn1] = vn0;
            else
              vremap[vn0] = vn1;
          }
          if (e1_l2 == 0 && vn1 != vn2)
          {
            if (vn2 < vn1)
              vremap[vn1] = vn2;
            else
              vremap[vn2] = vn1;
          }
          if (e2_l2 == 0 && vn0 != vn2)
          {
            if (vn0 < vn2)
              vremap[vn2] = vn0;
            else
              vremap[vn0] = vn2;
          }
#endif
        }
        continue;
      }

      mf[i].square = sqrt(length((v2 - v1) % (v0 - v1)) / 2);
      mf[i].v[0] = face[i].v[0];
      mf[i].v[1] = face[i].v[1];
      mf[i].v[2] = face[i].v[2];

      mf[i].e[0].fIndex = -1;
      mf[i].e[0].eIndex = -1;
      mf[i].e[0].len = sqrt(e0_l2);
      mf[i].e[0].v2_x = ((v2 - v1) * (v0 - v1)) / e0_l2;
      en = normalize((v0 - v1) % ((v2 - v1) % (v0 - v1)));
      mf[i].e[0].v2_y = ((v2 - v1) * en) / mf[i].e[0].len;
      if (mf[i].e[0].v2_y < 0.0001)
      {
        debug("face %d, edge 0: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 " face.v=%d,%d,%d", i, mf[i].e[0].v2_y, P3D(v0),
          P3D(v1), P3D(v2), face[i].v[0], face[i].v[1], face[i].v[2]);
        // DAG_FATAL("face %d, edge 0: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 "",
        //   i, mf[i].e[0].v2_y, P3D(v0), P3D(v1), P3D(v2));
      }

      mf[i].e[1].fIndex = -1;
      mf[i].e[1].eIndex = -1;
      mf[i].e[1].len = sqrt(e1_l2);
      mf[i].e[1].v2_x = ((v0 - v2) * (v1 - v2)) / e1_l2;
      en = normalize((v1 - v2) % ((v0 - v2) % (v1 - v2)));
      mf[i].e[1].v2_y = ((v0 - v2) * en) / mf[i].e[1].len;
      if (mf[i].e[1].v2_y < 0.0001)
      {
        debug("face %d, edge 1: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 " face.v=%d,%d,%d", i, mf[i].e[0].v2_y, P3D(v0),
          P3D(v1), P3D(v2), face[i].v[0], face[i].v[1], face[i].v[2]);
        // DAG_FATAL("face %d, edge 1: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 "",
        //   i, mf[i].e[0].v2_y, P3D(v0), P3D(v1), P3D(v2));
      }
      mf[i].e[2].fIndex = -1;
      mf[i].e[2].eIndex = -1;
      mf[i].e[2].len = sqrt(e2_l2);
      mf[i].e[2].v2_x = ((v1 - v0) * (v2 - v0)) / e2_l2;
      en = normalize((v2 - v0) % ((v1 - v0) % (v2 - v0)));
      mf[i].e[2].v2_y = ((v1 - v0) * en) / mf[i].e[2].len;
      if (mf[i].e[2].v2_y < 0.0001)
      {
        debug("face %d, edge 2: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 " face.v=%d,%d,%d", i, mf[i].e[0].v2_y, P3D(v0),
          P3D(v1), P3D(v2), face[i].v[0], face[i].v[1], face[i].v[2]);
        // DAG_FATAL("face %d, edge 2: v2_y=%.3f, v0=" FMT_P3 ", v1=" FMT_P3 ", v2=" FMT_P3 "",
        //   i, mf[i].e[0].v2_y, P3D(v0), P3D(v1), P3D(v2));
      }

      mf[i].av_len = (mf[i].e[0].len + mf[i].e[1].len + mf[i].e[2].len) / 3;
      mf[i].flags = 0;

      real face_maxedge = mf[i].e[0].len;
      if (face_maxedge < mf[i].e[1].len)
        face_maxedge = mf[i].e[1].len;
      if (face_maxedge < mf[i].e[2].len)
        face_maxedge = mf[i].e[2].len;

      if (start_face == -1 || longest_edge < face_maxedge)
      {
        start_face = i;
        longest_edge = face_maxedge;
      }
    }
#if __TRY_REMAP_VERTS__
    if (vremap)
    {
      // remap identical vertices
      for (i = 0; i < mf.size(); i++)
      {
        if (vremap[face[i].v[0]] != -1)
          face[i].v[0] = vremap[face[i].v[0]];
        if (vremap[face[i].v[1]] != -1)
          face[i].v[1] = vremap[face[i].v[1]];
        if (vremap[face[i].v[2]] != -1)
          face[i].v[2] = vremap[face[i].v[2]];
      }
      delete vremap;
    }
#endif

    // Build vert to face map
    F2V_Map map;
    MeshData::buildVertToFaceVertMap(map, face, fnum, vnum, 4, 1);
    // build connections
    for (i = 0; i < mf.size(); i++)
    {
      if (mf[i].flags & MUFF_INVALID)
        continue;

      for (int vi = 0; vi < 3; ++vi)
      {
        int vind = mf[i].v[vi];
        int fnum = map.get_face_num(vind);
        for (int neighb = 0; neighb < fnum; neighb++)
        {
          int fid = map.get_face_ij(vind, neighb);
          int fi = fid >> 2, fiv = fid & 3;
          if (fi == i)
            continue;
          if (mf[fi].flags & MUFF_INVALID)
            continue;
          if ((face[i].smgr & face[fi].smgr) == 0)
            continue;

          for (int e1 = 0; e1 < 3; e1++)
            for (int e2 = 0; e2 < 3; e2++)
              if (mf[i].v[e1] == mf[fi].v[(e2 + 1) % 3] && mf[i].v[(e1 + 1) % 3] == mf[fi].v[e2])
              {
                mf[i].e[e1].fIndex = fi;
                mf[i].e[e1].eIndex = e2;

                mf[fi].e[e2].fIndex = i;
                mf[fi].e[e2].eIndex = e1;

                mf_linked += 2;
                break;
              }
        }
      }
    }
    stat_flinked += mf_linked;
    // debug ( "::mf_linked=%d longest_edge=%3.f (face %d)", mf_linked, longest_edge, start_face );
  }

  bool Process(Tab<FaceGroup *> &fgrps)
  {
    int i, j;

    // unwrap
    // prepare unwrapping
    Prepare();
    if (start_face == -1)
    {
      debug("start_face=-1, skipping %d faces", mf.size());
      return false;
    }

    StartFromFace(start_face);

    do
    {
      // perform possible unwrapping
      while (mf_left > 0 && moe.size() > 0)
      {
        // find longest edge
        real wt;
        int ei = -1;

        for (int i = moe.size() - 1; i >= 0; i--)
        {
          if (mf[moe[i].fIndex].flags & MUFF_PROCESSED)
          {
            erase_items(moe, i, 1);
            if (ei > i)
              ei--;
            continue;
          }
          if (ei == -1 || wt <= moe[i].wt)
          {
            wt = moe[i].wt;
            ei = i;
          }
        }
        if (ei == -1)
        {
          // debug ( ":: no valid edge found moe.size()=%d!", moe.size());
          break;
        }

        // unfold this edge
        // debug ( ":: unfolding edge %d:%d (len %.3f)  moe.size()=%d mf_left=%d", moe[ei].fIndex, moe[ei].eIndex, moe[ei].len,
        // moe.size(), mf_left );

        MU_Face &f = mf[moe[ei].fIndex];
        int eIndex = moe[ei].eIndex, eIndex_p1 = (eIndex + 1) % 3, eIndex_p2 = (eIndex + 2) % 3;
        Point2 v0 = moe[ei].v0, v1 = moe[ei].v1, v2, edge;

        edge = normalize(v1 - v0) * moe[ei].len;
        v2 = v0 + edge * f.e[eIndex].v2_x + Point2(-edge.y, edge.x) * f.e[eIndex].v2_y;
        // debug ( "open_no=%d,f.e[eIndex].v2=%.3f,%.3f v0=%.3f,%.3f v1=%.3f,%.3f v2=%.3f,%.3f", open_no, f.e[eIndex].v2_x,
        // f.e[eIndex].v2_y, v0.x, v0.y, v1.x, v1.y, v2.x, v2.y );

        if (TestAddPoint(v2))
        {
          //        if ( !EdgeOverlaps ( __round(v0), __round(v2)) && !EdgeOverlaps ( __round(v1), __round(v2))) {
          TrySnapV2(moe[ei].fIndex, v2, 0.5 * 0.5, v0, v1, eIndex);
          if (!EdgeOverlaps(v0, v2) && !EdgeOverlaps(v1, v2))
          {
            // we can unfold without overlap
            f.uv[eIndex] = v1;
            f.uv[eIndex_p1] = v0;
            f.uv[eIndex_p2] = v2;
            f.flags |= MUFF_PROCESSED;
            f.open_no = open_no;
            mf_left--;
            open_no++;
            AddPtMinMax(v2);

            if (f.e[eIndex_p1].fIndex != -1 && !(mf[f.e[eIndex_p1].fIndex].flags & MUFF_PROCESSED))
            {
              int l = append_items(moe, 1);
              moe[l].fIndex = f.e[eIndex_p1].fIndex;
              moe[l].eIndex = f.e[eIndex_p1].eIndex;
              moe[l].v0 = v0;
              moe[l].v1 = v2;
              moe[l].len = f.e[eIndex_p1].len;
              moe[l].wt = moe[l].len + f.square * 1.5 + f.av_len;
              if (fabs(length(moe[l].v1 - moe[l].v0) - moe[l].len) > 1.0)
                debug("err1. v0=%.3f,%.3f v1=%.3f,%.3f len=%.3f != %.3f", moe[l].v0.x, moe[l].v0.y, moe[l].v1.x, moe[l].v1.y,
                  length(moe[l].v1 - moe[l].v0), moe[l].len);
              // debug ( ":: openedge %d:%d, %.3f", moe[l].fIndex, moe[l].eIndex, moe[l].len );
            }

            if (f.e[eIndex_p2].fIndex != -1 && !(mf[f.e[eIndex_p2].fIndex].flags & MUFF_PROCESSED))
            {
              int l = append_items(moe, 1);
              moe[l].fIndex = f.e[eIndex_p2].fIndex;
              moe[l].eIndex = f.e[eIndex_p2].eIndex;
              moe[l].v0 = v2;
              moe[l].v1 = v1;
              moe[l].len = f.e[eIndex_p2].len;
              moe[l].wt = moe[l].len + f.square * 1.5 + f.av_len;
              if (fabs(length(moe[l].v1 - moe[l].v0) - moe[l].len) > 1.0)
                debug("err2. v0=%.3f,%.3f v1=%.3f,%.3f len=%.3f != %.3f", moe[l].v0.x, moe[l].v0.y, moe[l].v1.x, moe[l].v1.y,
                  length(moe[l].v1 - moe[l].v0), moe[l].len);
              // debug ( ":: openedge %d:%d, %.3f", moe[l].fIndex, moe[l].eIndex, moe[l].len );
            }
          }
        }
        erase_items(moe, ei, 1); // we remove this edge anyway...
      }
      // debug ( ":: mf_left=%d moe.size()=%d", mf_left, moe.size());
      CalcMainLine();
      RotateUV();

      FaceGroup *fg = new (midmem) FaceGroup;
      fgrps.push_back(fg);
      AddRecentFaceGroup(*fg);
    } while (SelectNextStartFace());

    return true;
  }
  void DrawOutput(char *out_fname, const FaceGroup &g, bool round_uv = false)
  {
    // output result picture
    FILE *fp = svg_open(out_fname);
    Point2 minv = Point2(FLT_MAX, FLT_MAX), maxv = Point2(-FLT_MAX, -FLT_MAX);
    real scale = 1;
    int i;

    for (i = 0; i < g.faces.size(); i++)
    {
      MU_Face &f = mf[g.faces[i]];

      if (minv.x > f.uv[0].x)
        minv.x = f.uv[0].x;
      if (minv.x > f.uv[1].x)
        minv.x = f.uv[1].x;
      if (minv.x > f.uv[2].x)
        minv.x = f.uv[2].x;
      if (minv.y > f.uv[0].y)
        minv.y = f.uv[0].y;
      if (minv.y > f.uv[1].y)
        minv.y = f.uv[1].y;
      if (minv.y > f.uv[2].y)
        minv.y = f.uv[2].y;

      if (maxv.x < f.uv[0].x)
        maxv.x = f.uv[0].x;
      if (maxv.x < f.uv[1].x)
        maxv.x = f.uv[1].x;
      if (maxv.x < f.uv[2].x)
        maxv.x = f.uv[2].x;
      if (maxv.y < f.uv[0].y)
        maxv.y = f.uv[0].y;
      if (maxv.y < f.uv[1].y)
        maxv.y = f.uv[1].y;
      if (maxv.y < f.uv[2].y)
        maxv.y = f.uv[2].y;
    }
    if (maxv.x - minv.x < maxv.y - minv.y)
    {
      if (maxv.x - minv.x < 900)
        scale = 900 / (maxv.x - minv.x);
      else if (maxv.x - minv.x > 2000)
        scale = 900 / (maxv.x - minv.x);
    }
    else
    {
      if (maxv.y - minv.y < 900)
        scale = 900 / (maxv.y - minv.y);
      else if (maxv.y - minv.y > 2000)
        scale = 900 / (maxv.y - minv.y);
    }
    if (round_uv)
    {
      maxv = __round(maxv);
      minv = __round(minv);
    }

    svg_begin_group(fp, "stroke=\"red\" stroke-width=\"0.7\"");
    for (i = minv.x; i <= maxv.x; i++)
      svg_draw_line(fp, Point2(i - minv.x, 0) * scale, Point2(i - minv.x, maxv.y - minv.y) * scale, "");
    for (i = minv.y; i <= maxv.y; i++)
      svg_draw_line(fp, Point2(0, i - minv.y) * scale, Point2(maxv.x - minv.x, i - minv.y) * scale, "");
    svg_end_group(fp);
    svg_draw_line(fp, Point2(0, -minv.y) * scale, Point2(maxv.x - minv.x, -minv.y) * scale);
    svg_draw_line(fp, Point2(-minv.x, 0) * scale, Point2(-minv.x, maxv.y - minv.y) * scale);

    svg_begin_group(fp, "opacity=\"0.6\" fill=\"yellow\" stroke=\"blue\" stroke-width=\"0.5\"");
    for (i = 0; i < g.faces.size(); i++)
    {
      MU_Face &f = mf[g.faces[i]];
      if (round_uv)
        svg_draw_face(fp, __round(f.uv[0] - minv) * scale, __round(f.uv[1] - minv) * scale, __round(f.uv[2] - minv) * scale);
      else
        svg_draw_face(fp, (f.uv[0] - minv) * scale, (f.uv[1] - minv) * scale, (f.uv[2] - minv) * scale);
      svg_draw_number(fp, ((f.uv[0] + f.uv[1] + f.uv[2]) / 3 - minv) * scale, f.open_no);
    }
    svg_end_group(fp);
    svg_close(fp);
  }

protected:
  void Prepare()
  {
    mf_left = 0;
    open_no = 0;
    for (int i = 0; i < mf.size(); i++)
    {
      if (mf[i].flags & MUFF_INVALID)
        continue;
      mf[i].flags &= ~(MUFF_PROCESSED | MUFF_CANT_UNFOLD0 | MUFF_CANT_UNFOLD1 | MUFF_CANT_UNFOLD2 | MUFF_SKIPPED);
      mf_left++;
    }
    stat_fnum += mf_left;
  }
  void StartFromFace(int fi)
  {
    moe.clear();
    if (fi < 0 || fi >= mf.size() || (mf[fi].flags & MUFF_INVALID))
      DAG_FATAL("IN contract failed in MU_StartFromFace, fi=%d, facenum=%d", fi, mf.size());
    MU_Face &f = mf[fi];

    Point2 v0, v1, v2;
    // debug ( ":: first face: %.3f,%.3f,%.3f", f.e[0].len, f.e[1].len, f.e[2].len );
    int hor_edge = 0;

    // first, take as basis the logest edge
    if (f.e[0].len >= f.e[1].len && f.e[0].len >= f.e[2].len)
    {
      // first egde is the longest
      hor_edge = 0;
    }
    else if (f.e[1].len >= f.e[0].len && f.e[1].len >= f.e[2].len)
    {
      // second egde is the longest
      hor_edge = 1;
    }
    else if (f.e[2].len >= f.e[0].len && f.e[2].len >= f.e[0].len)
    {
      // third egde is the longest
      hor_edge = 2;
    }

    // now check whether l.e. is hypot in sqr triangle, if so, take longest catet
    if (hor_edge == 2 && fabs(1 - (f.e[0].len * f.e[0].len + f.e[1].len * f.e[1].len) / (f.e[2].len * f.e[2].len)) < 0.1)
    {
      if (f.e[0].len >= f.e[1].len)
        hor_edge = 0;
      else
        hor_edge = 1;
    }
    else if (hor_edge == 1 && fabs(1 - (f.e[0].len * f.e[0].len + f.e[2].len * f.e[2].len) / (f.e[1].len * f.e[1].len)) < 0.1)
    {
      if (f.e[0].len >= f.e[2].len)
        hor_edge = 0;
      else
        hor_edge = 2;
    }
    else if (hor_edge == 0 && fabs(1 - (f.e[2].len * f.e[2].len + f.e[1].len * f.e[1].len) / (f.e[0].len * f.e[0].len)) < 0.1)
    {
      if (f.e[2].len >= f.e[1].len)
        hor_edge = 2;
      else
        hor_edge = 1;
    }

    // now calc coords for chosen layout
    switch (hor_edge)
    {
      case 0:
        v0 = Point2(0, 0);
        v1 = Point2(f.e[0].len, 0);
        v2 = v1 - (v1 - v0) * f.e[0].v2_x - Point2(0, f.e[0].len) * f.e[0].v2_y;
        break;
      case 1:
        v1 = Point2(0, 0);
        v2 = Point2(f.e[1].len, 0);
        v0 = v2 - (v2 - v1) * f.e[1].v2_x - Point2(0, f.e[1].len) * f.e[1].v2_y;
        break;
      case 2:
        v2 = Point2(0, 0);
        v0 = Point2(f.e[2].len, 0);
        v1 = v0 - (v0 - v2) * f.e[2].v2_x - Point2(0, f.e[2].len) * f.e[2].v2_y;
        break;
    }

    if (f.e[0].fIndex != -1)
    {
      int l = append_items(moe, 1);
      moe[l].fIndex = f.e[0].fIndex;
      moe[l].eIndex = f.e[0].eIndex;
      moe[l].v0 = v0;
      moe[l].v1 = v1;
      moe[l].len = f.e[0].len;
      moe[l].wt = moe[l].len + f.square * 1.5 + f.av_len;

      // debug ( "::start:: openedge %d:%d, %.3f", moe[l].fIndex, moe[l].eIndex, moe[l].len );
    }

    if (f.e[1].fIndex != -1)
    {
      int l = append_items(moe, 1);
      moe[l].fIndex = f.e[1].fIndex;
      moe[l].eIndex = f.e[1].eIndex;
      moe[l].v0 = v1;
      moe[l].v1 = v2;
      moe[l].len = f.e[1].len;
      moe[l].wt = moe[l].len + f.square * 1.5 + f.av_len;
      // debug ( "::start:: openedge %d:%d, %.3f", moe[l].fIndex, moe[l].eIndex, moe[l].len );
    }

    if (f.e[2].fIndex != -1)
    {
      int l = append_items(moe, 1);
      moe[l].fIndex = f.e[2].fIndex;
      moe[l].eIndex = f.e[2].eIndex;
      moe[l].v0 = v2;
      moe[l].v1 = v0;
      moe[l].len = f.e[2].len;
      moe[l].wt = moe[l].len + f.square * 1.5 + f.av_len;
      // debug ( "::start:: openedge %d:%d, %.3f", moe[l].fIndex, moe[l].eIndex, moe[l].len );
    }

    f.uv[0] = v0;
    f.uv[1] = v1;
    f.uv[2] = v2;
    f.flags |= MUFF_PROCESSED;
    f.open_no = 0;

    // calc first minmax
    c_min = v0;
    c_max = v0;
    AddPtMinMax(v1);
    AddPtMinMax(v2);

    // if ( moe.size() == 0 ) debug ( "warning: isolated face was starting!" );

    mf_left--;
    open_no++;
  }
  void AddRecentFaceGroup(FaceGroup &g)
  {
    int i;

    for (i = 0; i < mf.size(); i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;
      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;
      g.faces.push_back(i);
      mf[i].flags |= MUFF_MARKED_OLD;
      stat_fnum_p++;
    }

    if (!g.faces.size())
      DAG_FATAL("faces count = 0");
    g.tfaces.resize(g.faces.size());
    for (i = 0; i < g.faces.size(); i++)
    {
      MU_Face &f = mf[g.faces[i]];
      g.tfaces[i][0] = g.adduv(f.uv[0]);
      g.tfaces[i][1] = g.adduv(f.uv[1]);
      g.tfaces[i][2] = g.adduv(f.uv[2]);
    }
#if EXCESSIVE_DEBUG
    if (g.faces.size() > 2)
    {
      char fname[260];
      sprintf(fname, "dbg/fg%05d.svg", stat_grpnum);
      DrawOutput(fname, g, false);
      sprintf(fname, "dbg/fg%05dr.svg", stat_grpnum);
      DrawOutput(fname, g, true);
    }
#endif
    stat_grpnum++;
  }
  void AddPtMinMax(const Point2 &v)
  {
    if (v.x < c_min.x)
      c_min.x = v.x;
    else if (v.x > c_max.x)
      c_max.x = v.x;
    if (v.y < c_min.y)
      c_min.y = v.y;
    else if (v.y > c_max.y)
      c_max.y = v.y;
  }
  bool TestAddPoint(const Point2 &v)
  {
    Point2 l_min = c_min, l_max = c_max;
    if (v.x < c_min.x)
      l_min.x = v.x;
    else if (v.x > c_max.x)
      l_max.x = v.x;
    if (v.y < c_min.y)
      l_min.y = v.y;
    else if (v.y > c_max.y)
      l_max.y = v.y;

    if (l_max.x - l_min.x <= LIGHTMAPSZ && l_max.y - l_min.y <= LIGHTMAPSZ)
      return true;
    if ((l_max.x - l_min.x) - (c_max.x - c_min.x) >= LIGHTMAPSZ || (l_max.x - l_min.x) - (c_max.x - c_min.x) >= LIGHTMAPSZ)
      return true;
    return false;
  }

  void TrySnapV2(int fi, Point2 &v2, real snap_dist2, const Point2 &v0, const Point2 &v1, int ei)
  {
    Point2 nv2 = v2;
    real best_dist2 = snap_dist2;
    int i, j, k;
    int vi = mf[fi].v[(ei + 2) % 3];

    for (i = mf.size() - 1; i >= 0; i--)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;
      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;
      for (j = 0; j < 3; j++)
        if (mf[i].v[j] == vi)
        {
          real dist2;
          dist2 = lengthSq(mf[i].uv[j] - v2);
          if (dist2 < best_dist2)
          {
            best_dist2 = dist2;
            nv2 = mf[i].uv[j];
          }
        }
    }
    if (best_dist2 < snap_dist2)
    {
#if EXCESSIVE_DEBUG
      debug("snap: fi=%d ei=%d [%d] (%.3f,%.3f)->(%.3f,%.3f), dist=%.3f v0=(%.3f,%.3f), v1=(%.3f,%.3f)", fi, ei, open_no, v2.x, v2.y,
        sqrt(best_dist2), nv2.x, nv2.y, v0.x, v0.y, v1.x, v1.y);
#endif
      stat_snapped++;
    }
    v2 = nv2;
  }

  bool SelectNextStartFace()
  {
    real longest_edge = 0;
    start_face = -1;
    if (mf_left <= 0)
      return false;

    for (int i = 0; i < mf.size(); i++)
    {
      if (mf[i].flags & MUFF_PROCESSED)
        continue;

      real face_maxedge = mf[i].e[0].len;
      if (face_maxedge < mf[i].e[1].len)
        face_maxedge = mf[i].e[1].len;
      if (face_maxedge < mf[i].e[2].len)
        face_maxedge = mf[i].e[2].len;

      if (start_face == -1 || longest_edge < face_maxedge)
      {
        start_face = i;
        longest_edge = face_maxedge;
      }
    }
    // debug ( "next group start_face=%d mf_left=%d", start_face, mf_left );
    if (start_face != -1)
    {
      StartFromFace(start_face);
      return true;
    }
    return false;
  }

  bool EdgeOverlaps(const Point2 &v0, const Point2 &v1)
  {
    Point2 en, dir, cr;
    real A, B, t;
    bool force_debug = (stat_grpnum == 319) && (open_no >= 64 && open_no <= 71);

    en.x = -(v1.y - v0.y);
    en.y = (v1.x - v0.x);

    for (int i = 0; i < mf.size(); i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;
      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;

      for (int j = 0; j < 3; j++)
      {
        // Point2 _v0 = __round(mf[i].uv[j]), _v1 = __round(mf[i].uv[(j+1)%3]);
        Point2 _v0 = mf[i].uv[j], _v1 = mf[i].uv[(j + 1) % 3];

        if (v0 == _v0 || v0 == _v1)
        {
          // alternative check: v1 must be outside triangle
          // this means that v1 and _v2 must lie on different sides of _v0-_v1
          Point2 _v2 = mf[i].uv[(j + 2) % 3];
          real sin1 = (v1 - _v0) % (_v1 - _v0);
          real sin2 = (_v2 - _v0) % (_v1 - _v0);
          if (sin2 * sin1 <= 0)
            break;
          sin1 = (v1 - _v1) % (_v2 - _v1);
          sin2 = (_v0 - _v1) % (_v2 - _v1);
          if (sin2 * sin1 <= 0)
            break;
          sin1 = (v1 - _v0) % (_v2 - _v0);
          sin2 = (_v1 - _v0) % (_v2 - _v0);
          if (sin2 * sin1 <= 0)
            break;

#if EXCESSIVE_DEBUG
          debug("stat_grpnum=%d open_no=%d sin1=%.3f sin2=%.3f v0=(%.3f,%.3f) v1=(%.3f,%.3f) _v0=(%.3f,%.3f) _v1=(%.3f,%.3f) "
                "_v2=(%.3f,%.3f)",
            stat_grpnum, open_no, sin1, sin2, v0.x, v0.y, v1.x, v1.y, _v0.x, _v0.y, _v1.x, _v1.y, _v2.x, _v2.y);
#endif
          return true;
        }

        dir = _v1 - _v0;
        B = dir * en;
        if (B != 0.0)
        {
          A = en * (_v0 - v0);
          t = -A / B;
          if (t < 0.001 || t > 0.999)
            continue;

          if (v1.x != v0.x)
          {
            cr.x = _v0.x + dir.x * t;
            if (v1.x > v0.x && (cr.x <= v0.x || cr.x >= v1.x))
              continue;
            if (v1.x < v0.x && (cr.x <= v1.x || cr.x >= v0.x))
              continue;
          }
          else if (v1.y != v0.y)
          {
            cr.y = _v0.y + dir.y * t;
            if (v1.y > v0.y && (cr.y <= v0.y || cr.y >= v1.y))
              continue;
            if (v1.y < v0.y && (cr.y <= v1.y || cr.y >= v0.y))
              continue;
          }
          return true;
        }
      }
    }
    return false;
  }
  void CalcMainLine()
  {
#if OLD_ALIGNMENT_OPTIMIZER
    double x2_av = 0, y2_av = 0, xy_av = 0, x_av = 0, y_av = 0;
    int n = 0;

    for (int i = 0; i < mf.size(); i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;
      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;

      Point2 *uv = mf[i].uv;
      x2_av += uv[0].x * uv[0].x + uv[1].x * uv[1].x + uv[2].x * uv[2].x;
      y2_av += uv[0].y * uv[0].y + uv[1].y * uv[1].y + uv[2].y * uv[2].y;
      xy_av += uv[0].x * uv[0].y + uv[1].x * uv[1].y + uv[2].x * uv[2].y;
      x_av += uv[0].x + uv[1].x + uv[2].x;
      y_av += uv[0].y + uv[1].y + uv[2].y;
      // debug ( "(%.3f,%.3f) (%.3f,%.3f) (%.3f,%.3f)", uv[0].x, uv[0].y, uv[1].x, uv[1].y, uv[2].x, uv[2].y );
      n += 3;
    }
    if (!n)
      return;
    x2_av /= real(n);
    y2_av /= real(n);
    xy_av /= real(n);
    x_av /= real(n);
    y_av /= real(n);

    double alpha = (x2_av - x_av * x_av - (y2_av - y_av * y_av));
    double beta = (x_av * y_av - xy_av);
    double A = 4 * beta * beta + alpha * alpha, C = beta * beta;
    double D = A * A - 4 * A * C, f_best = FLT_MAX;
    double a_best = 0, b_best = 1;

    if (D >= 0)
    {
      D = sqrt(D);
      double a2_1 = (A - D) / 2 / A;
      double a2_2 = (A + D) / 2 / A;
      double a, b, c, f;

      if (a2_1 >= 0 && a2_1 <= 1)
      {
        a = sqrt(a2_1);
        b = sqrt(1 - a2_1);
        c = -x_av * a - y_av * b;
        // debug ( "==1: a=+- %.4f, b=+- %.4f c=%.4f", a, b, c );

        f = x2_av * a * a + 2 * xy_av * a * b + y2_av * b * b + 2 * x_av * a * c + 2 * y_av * b * c + c * c;
        // debug ( "  a=%.4f, b=%.4f c=%.4f f=%.5f", a, b, c, f );
        if (f_best > f)
        {
          f_best = f;
          a_best = a;
          b_best = b;
        }

        a = -a;
        c = -x_av * a - y_av * b;
        f = x2_av * a * a + 2 * xy_av * a * b + y2_av * b * b + 2 * x_av * a * c + 2 * y_av * b * c + c * c;
        // debug ( "  a=%.4f, b=%.4f c=%.4f f=%.5f", a, b, c, f );
        if (f_best > f)
        {
          f_best = f;
          a_best = a;
          b_best = b;
        }
      }
      if (a2_2 >= 0 && a2_2 <= 1)
      {
        a = sqrt(a2_2);
        b = sqrt(1 - a2_2);
        c = -x_av * a - y_av * b;
        // debug ( "==2: a=+- %.4f, b=+- %.4f c=%.4f", a, b, c );

        f = x2_av * a * a + 2 * xy_av * a * b + y2_av * b * b + 2 * x_av * a * c + 2 * y_av * b * c + c * c;
        // debug ( "  a=%.4f, b=%.4f c=%.4f f=%.5f", a, b, c, f );
        if (f_best > f)
        {
          f_best = f;
          a_best = a;
          b_best = b;
        }

        a = -a;
        c = -x_av * a - y_av * b;
        f = x2_av * a * a + 2 * xy_av * a * b + y2_av * b * b + 2 * x_av * a * c + 2 * y_av * b * c + c * c;
        // debug ( "  a=%.4f, b=%.4f c=%.4f f=%.5f", a, b, c, f );
        if (f_best > f)
        {
          f_best = f;
          a_best = a;
          b_best = b;
        }
      }
    }
    else
    {
      // debug ( "alpha=%.4f beta=%.4f D=%.4f", alpha, beta, D );
    }

    rot[0] = Point2(b_best, -a_best);
    rot[1] = Point2(a_best, b_best);
#else
    int mf_start, mf_end;
    real angle;
    real optima = calcOptimaParamFirstPass(mf_start, mf_end);
    real optima_a = 0;

    for (angle = PI / 180.0 * 2.0; angle < PI; angle += PI / 180.0 * 2.0)
    {
      real o = calcOptimaParam(angle, mf_start, mf_end);
      if (o < optima)
      {
        optima = o;
        optima_a = angle;
      }
    }

    rot[0] = Point2(cos(optima_a), sin(optima_a));
    rot[1] = Point2(-rot[0].y, rot[0].x);
#endif
  }
  __forceinline real calcOptimaParamFirstPass(int &mf_start, int &mf_end)
  {
    Point2 minmax;
    real cy;
    int i;

    mf_start = mf.size();
    mf_end = -1;

    for (i = 0; i < mf.size(); i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;

      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;

      if (mf_end == -1)
      {
        cy = mf[i].uv[0].y;
        minmax[0] = cy;
        minmax[1] = cy;

        mf_start = i;
        mf_end = i;
      }
      else
      {
        if (i < mf_start)
          mf_start = i;
        if (i > mf_end)
          mf_end = i;
      }

      cy = mf[i].uv[1].y;
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;

      cy = mf[i].uv[2].y;
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;
    }

    return minmax[1] - minmax[0];
  }
  __forceinline real calcOptimaParam(real angle, int mf_start, int mf_end)
  {
    Point2 minmax;
    real cy;
    int i;

    for (i = mf_start; i <= mf_end; i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;

      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;

      rot[1] = Point2(-sin(angle), cos(angle));

      cy = rot[1] * mf[i].uv[0];
      minmax[0] = cy;
      minmax[1] = cy;

      cy = rot[1] * mf[i].uv[1];
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;

      cy = rot[1] * mf[i].uv[2];
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;

      i++;
      break;
    }

    for (; i <= mf_end; i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;

      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;
      cy = rot[1] * mf[i].uv[0];
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;

      cy = rot[1] * mf[i].uv[1];
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;

      cy = rot[1] * mf[i].uv[2];
      if (cy > minmax[1])
        minmax[1] = cy;
      else if (cy < minmax[0])
        minmax[0] = cy;
    }

    return minmax[1] - minmax[0];
  }
  void RotateUV()
  {
    for (int i = 0; i < mf.size(); i++)
    {
      if (!(mf[i].flags & MUFF_PROCESSED))
        continue;

      if (mf[i].flags & MUFF_MARKED_OLD)
        continue;
      Point2 nuv;
      nuv.x = rot[0] * mf[i].uv[0];
      nuv.y = rot[1] * mf[i].uv[0];
      mf[i].uv[0] = nuv;

      nuv.x = rot[0] * mf[i].uv[1];
      nuv.y = rot[1] * mf[i].uv[1];
      mf[i].uv[1] = nuv;

      nuv.x = rot[0] * mf[i].uv[2];
      nuv.y = rot[1] * mf[i].uv[2];
      mf[i].uv[2] = nuv;
    }
  }
};

void clear_stat()
{
  stat_objnum = 0;
  stat_total_fnum = 0;
  stat_fnum = 0;
  stat_fnum_p = 0;
  stat_flinked = 0;
  stat_grpnum = 0;
  stat_fnum_bad = 0;
  stat_snapped = 0;
}
void print_stat()
{
  debug("total obj_num=%d (total_face_num=%d face_num=%d/%d fnum_bad=%d face_linked=%d grp_num=%d)", stat_objnum, stat_total_fnum,
    stat_fnum_p, stat_fnum, stat_fnum_bad, stat_flinked, stat_grpnum);
  debug("snapped points: %d", stat_snapped);
  if (stat_objnum && stat_grpnum)
    debug("av. grp/obj=%.3f  av.face/grp=%.3f\n\n", double(stat_grpnum) / double(stat_objnum),
      double(stat_fnum_p) / double(stat_grpnum));
}

} // namespace MeshUnwrap
