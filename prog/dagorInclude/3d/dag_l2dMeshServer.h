//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <memory/dag_mem.h>
#include <generic/dag_hierGrid.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN
#pragma warning(disable : 6201) // warning 6201| Index '3' is out of valid index range '0' to '2' for possibly stack allocated buffer
                                // 'h_level'
#endif

// class LtData interface:
// Brief desc: this sturcture is returned for getLighting request
//
//   void clear() - clears data
//

// class StLtData interface:
// Brief desc: this is internal lighting data storage format
//
// converts from internal to public format:
//   void copyTo(LtData &lt)
// checks whether light differs enough:
//   bool differs(StLtData &lt, real thres)
// interpolates between 2 lights:
//   void interpolate(const StLtData &lt1, const StLtData &lt2, real a)
// interpolate in triangle:
//   void interpolate(const StLtData &lt0, const StLtData &lt1, const StLtData &lt2, real a10, real 20 )
//

template <class LtData, class StLtData, int NUM_HT_LEV>
class Lt2DMeshServer
{
public:
  struct Leaf
  {
    DAG_DECLARE_NEW(midmem)

    Tab<int> faces;
    Leaf() : faces(midmem) {}
  };

  // --- public constraints and settings ---

  //! Size of leaf for hierarchical grid
  real grid_leaf_sz;
  //! Upward bias for altitude
  real __bias;
  //! Default Lighting outside of mesh
  LtData outer_light;

  // --- public interface ---

  Lt2DMeshServer();
  ~Lt2DMeshServer();

  real getAltitudeL(int level);

  bool loadBinary(IGenLoad &crd);
  bool load(const char *fname)
  {
    file_ptr_t fp = df_open(fname, DF_READ);
    // /*-debug*/debug_ctx ( "loading '%s', fp=%p", fname, fp);
    if (!fp)
      return false;

    LFileGeneralLoadCB crd(fp);
    bool ret = loadBinary(crd);

    df_close(fp);
    return ret;
  }

  bool getLight(const Point3 &pos, LtData &lt);
  real getAltitude(const Point3 &p);

  void dump();

protected:
  //! Face object optimized for Lt2D
  struct Lt2DFace
  {
    //! Vertex indices
    int v_ind[3];
    //! 2D circle (X,Z) radius
    real cr2;
    //! 2D center (X,Z) and level height
    Point3 c;
    //! Transformation coeffs: [REL] v0->(0,0), v1->(0,1), v2->(1,0)
    real A11, A12, A21, A22;
  };
  //! Vertex object optimized for Lt2D
  struct Lt2DVertex
  {
    //! Vertex coordinates
    Point3 v;
    //! Lighting data for levels
    StLtData lights[NUM_HT_LEV];
  };

  __forceinline real __getAltitude(const Point3 &p);

  // -- Variables --

  //! vertices array
  Tab<Lt2DVertex> vertices;
  //! faces array
  Tab<Lt2DFace> faces;
  //! Altitudes for leves; invariant for Lt2D
  real h_level[NUM_HT_LEV];
  //! grid to localize face processing
  HierGrid2<Leaf, 0> grid;

  //! temporary variables
  int __face_n;
  real __a1, __a2;

private:
  Lt2DMeshServer(Lt2DMeshServer &);
  Lt2DMeshServer &operator=(Lt2DMeshServer &);
};

//
// Template implementation
//
template <class LtData, class StLtData, int NUM_HT_LEV>
Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::Lt2DMeshServer() : vertices(midmem), faces(midmem), grid(5)
{
  __bias = 0.1;
  memset(h_level, 0, sizeof(h_level));

  grid_leaf_sz = 128;

  __face_n = -1;
  __a1 = 0;
  __a2 = 0;

  // We use simple light when mesh is not added
  outer_light.clear();
}

template <class LtData, class StLtData, int NUM_HT_LEV>
Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::~Lt2DMeshServer()
{}

template <class LtData, class StLtData, int NUM_HT_LEV>
real Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::getAltitudeL(int level)
{
  if (level < 0 || level >= NUM_HT_LEV)
    return realSNaN;
  return h_level[level];
}

template <class LtData, class StLtData, int NUM_HT_LEV>
__forceinline real Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::__getAltitude(const Point3 &p)
{
  Point2 p2d;
  p2d.x = p.x;
  p2d.y = p.z;
  real py = p.y + __bias;

  Leaf *l = grid.get_leaf((int)floorf(p.x / grid_leaf_sz), (int)floorf(p.z / grid_leaf_sz));
  //*-debug-*/ debug ( "GA: p=%.3f,%.3f,%.3f, leaf_sz=%.3f l=%p", p.x, p.y, p.z, grid_leaf_sz, &l );
  if (l == NULL)
  {
    __face_n = -1;
    return -100;
  }

  int fn = l->faces.size();
  for (int i = 0; i < fn; i++)
  {
    Lt2DFace &f = faces[l->faces[i]];

    // check whether p is above the face
    if (py < f.c.y)
      continue;

    // check whether p _c_a_n_ be in face range
    real r2 = (p.x - f.c.x) * (p.x - f.c.x) + (p.z - f.c.z) * (p.z - f.c.z);
    if (r2 > f.cr2)
      continue;
    //*-debug-*/ debug ( "reached here: face %d, r2=%.3f, f.cr2=%.3f", l.faces[i], r2, f.cr2 );

    // finally, check whether p IS n face
    Point2 v0, v1, v2, _p;
    real a1, a2;

    v0.x = vertices[f.v_ind[0]].v.x;
    v0.y = vertices[f.v_ind[0]].v.z;
    v1.x = vertices[f.v_ind[1]].v.x;
    v1.y = vertices[f.v_ind[1]].v.z;
    v2.x = vertices[f.v_ind[2]].v.x;
    v2.y = vertices[f.v_ind[2]].v.z;
    v1 -= v0;
    v2 -= v0;
    _p = p2d - v0;

    Point2 v1_, v2_, _p_;
    v1_.x = v1.x * f.A11 + v1.y * f.A12;
    v1_.y = v1.x * f.A21 + v1.y * f.A22;
    v2_.x = v2.x * f.A11 + v2.y * f.A12;
    v2_.y = v2.x * f.A21 + v2.y * f.A22;
    _p_.x = _p.x * f.A11 + _p.y * f.A12;
    _p_.y = _p.x * f.A21 + _p.y * f.A22;

    a1 = v1_ * _p_;
    a2 = v2_ * _p_;

    //*-debug-*/ debug ( "reached here: face %d, a1=%.3f, a2=%.3f, a1+a2=%.3f", l.faces[i], a1, a2, a1+a2 );
    if (a1 < -0.01 || a2 < -0.01 || (a1 + a2 > 1.02))
      continue;

    __face_n = l->faces[i];
    __a1 = a1;
    __a2 = a2;
    return vertices[f.v_ind[0]].v.y + (vertices[f.v_ind[1]].v.y - vertices[f.v_ind[0]].v.y) * a1 +
           (vertices[f.v_ind[2]].v.y - vertices[f.v_ind[0]].v.y) * a2;
  }

  // we are outside of mapped region...
  __face_n = -1;
  return -100;
}

template <class LtData, class StLtData, int NUM_HT_LEV>
bool Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::loadBinary(IGenLoad &crd)
{
  static char _l2dllm_hdr[4] = {'2', 'D', 'L', 'M'};
  char hdr[sizeof(_l2dllm_hdr)];

  crd.read(hdr, sizeof(_l2dllm_hdr));
  if (memcmp(hdr, _l2dllm_hdr, sizeof(_l2dllm_hdr)) != 0)
    return false;

#if defined(_TARGET_CPU_BE)
#define SWAP_LE32_VALUES(p, n)                                                                                                \
  {                                                                                                                           \
    uint32_t *ptr = (uint32_t *)(void *)(p);                                                                                  \
    for (int cnt = (n); cnt > 0; --cnt, ++ptr)                                                                                \
      *ptr = (((*ptr) >> 24) & 0xFF) | (((*ptr) >> 8) & 0xFF00) | (((*ptr) << 8) & 0xFF0000) | (((*ptr) << 24) & 0xFF000000); \
  }
#else
#define SWAP_LE32_VALUES(p, n) \
  {}
#endif

  int vn, fn, ver, lev;
  crd.read(&ver, 4);
  SWAP_LE32_VALUES(&ver, 1);
  // /*-debug*/debug_ctx ( "ver=0x%X", ver);
  if (ver < 0x601)
    return false;

  crd.read(&lev, 4);
  crd.read(&vn, 4);
  crd.read(&fn, 4);
  SWAP_LE32_VALUES(&lev, 1);
  SWAP_LE32_VALUES(&vn, 1);
  SWAP_LE32_VALUES(&fn, 1);
  if (lev != NUM_HT_LEV || vn < 0 || fn < 0 || vn > 1024 * 1024 || fn > 1024 * 1024)
  {
    // /*-debug*/debug_ctx ( "lev=%d vn=%d fn=%d", lev, vn, fn);
    return false;
  }
  /// /*-debug*/debug_ctx ( "lev=%d vn=%d fn=%d", lev, vn, fn);

  crd.read(h_level, lev * sizeof(real));
  SWAP_LE32_VALUES(h_level, lev);
  crd.read(&outer_light, sizeof(outer_light));
  SWAP_LE32_VALUES(&outer_light, sizeof(outer_light) / 4); //== FIXME: dirty hack (template arg)

  clear_and_shrink(vertices);
  clear_and_shrink(faces);

  vertices.resize(vn);
  faces.resize(fn);

  crd.read(&vertices[0], vn * sizeof(Lt2DVertex));
  SWAP_LE32_VALUES(&vertices[0], sizeof(Lt2DVertex) / 4 * vn); //== FIXME: dirty hack (template arg)
  crd.read(&faces[0], fn * sizeof(Lt2DFace));
  SWAP_LE32_VALUES(&faces[0], sizeof(Lt2DFace) / 4 * fn);

  crd.read(&grid_leaf_sz, 4);
  SWAP_LE32_VALUES(&grid_leaf_sz, 1);
  /// /*-debug*/debug_ctx ( "grid_leaf_szv=%.3f", grid_leaf_sz);
  do
  {
    int u, v, n;
    crd.read(&u, 4);
    crd.read(&v, 4);
    crd.read(&n, 4);
    SWAP_LE32_VALUES(&u, 1);
    SWAP_LE32_VALUES(&v, 1);
    SWAP_LE32_VALUES(&n, 1);
    if (!u && !v && !n)
      break;

    Leaf &leaf = *grid.create_leaf(u, v);
    leaf.faces.resize(n);
    crd.read(&leaf.faces[0], 4 * n);
    SWAP_LE32_VALUES(&leaf.faces[0], n);
  } while (true);

#undef SWAP_LE32_VALUES

  return true;
}

template <class LtData, class StLtData, int NUM_HT_LEV>
bool Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::getLight(const Point3 &pos, LtData &lt)
{
  real dh, ah = 0.f;

  lt = outer_light;
  dh = __getAltitude(pos);

  if (__face_n == -1)
    return false;

  Lt2DFace &f = faces[__face_n];

  dh = pos.y - dh;
  int lev = 0;
  if (dh < h_level[0])
  {
    lev = 0;
    ah = 0.0;
  }
  else if (dh >= h_level[NUM_HT_LEV - 1])
  {
    lev = NUM_HT_LEV - 1;
    ah = 0.0;
  }
  else
  {
    for (lev = 1; lev < NUM_HT_LEV; lev++)
      if (dh < h_level[lev])
      {
        lev--;
        ah = (dh - h_level[lev]) / (h_level[lev + 1] - h_level[lev]);
        break;
      }
  }

  StLtData tmp1;
  tmp1.interpolate(vertices[f.v_ind[0]].lights[lev], vertices[f.v_ind[1]].lights[lev], vertices[f.v_ind[2]].lights[lev], __a1, __a2);


  if (ah == 0.0)
  {
    // enough calculations: no vertical interpolation
    tmp1.copyTo(lt);
    return true;
  }

  lev++;
  StLtData tmp2, tmp3;
  tmp2.interpolate(vertices[f.v_ind[0]].lights[lev], vertices[f.v_ind[1]].lights[lev], vertices[f.v_ind[2]].lights[lev], __a1, __a2);

  // final interpolation
  tmp3.interpolate(tmp1, tmp2, ah);

  tmp3.copyTo(lt);
  return true;
}

template <class LtData, class StLtData, int NUM_HT_LEV>
real Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::getAltitude(const Point3 &p)
{
  real h = __getAltitude(p);
  if (__face_n == -1)
    return h;
  return p.y - h;
}

template <class LtData, class StLtData, int NUM_HT_LEV>
void Lt2DMeshServer<LtData, StLtData, NUM_HT_LEV>::dump()
{
  for (int level = 0; level < NUM_HT_LEV; ++level)
  {
    debug("%d vertices, level %d", vertices.size(), level);
    for (int i = 0; i < vertices.size(); ++i)
    {
      debug("%d:", i);
      vertices[i].lights[level].dump();
    }
  }
}
