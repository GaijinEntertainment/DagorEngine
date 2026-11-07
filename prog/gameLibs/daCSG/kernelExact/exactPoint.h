// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "kernelExact/bigint.h"
#include "kernelExact/expansion.h"
#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>

namespace dacsg::exact
{
// Bounds |x| <= 2^bits from |grid coordinate| <= 2^MAX_GRID_BITS, spec 7.1
constexpr int MAX_GRID_BITS = 40;
constexpr int NORMAL_BITS = 2 * (MAX_GRID_BITS + 1) + 1;
constexpr int OFFSET_BITS = NORMAL_BITS + MAX_GRID_BITS + 2;
constexpr int CROSS_BITS = 2 * NORMAL_BITS + 1;
constexpr int LPI_W_BITS = NORMAL_BITS + MAX_GRID_BITS + 3;
constexpr int LPI_X_BITS = LPI_W_BITS + MAX_GRID_BITS + 2;
constexpr int TPI_W_BITS = NORMAL_BITS + CROSS_BITS + 2;
constexpr int TPI_X_BITS = OFFSET_BITS + CROSS_BITS + 2;

constexpr int limbs_for(int bits) { return (bits + 1) / 64 + 1; }

static_assert(limbs_for(OFFSET_BITS) == 2 && limbs_for(LPI_W_BITS) == 2 && limbs_for(CROSS_BITS) == 3);
static_assert(limbs_for(LPI_X_BITS) == 3 && limbs_for(TPI_X_BITS) == 5);

struct GridPoint
{
  int64_t c[3];
};

// The shewchuk backend keeps its expansions here; a chunk never moves, so a view stays valid
struct ExArena
{
  dag::Vector<eastl::unique_ptr<double[]>> chunks;
  uint32_t used = 0;

  const double *put(const double *e, int n);
  // takes other's chunks; a put continues in its last one
  void adopt(ExArena &other);
};

// a stored expansion of the arena: its double-double with the bound (Dd), then the n terms
struct ExView
{
  static constexpr int PREFIX = 3;
  const double *e;
  uint8_t n;

  const double *terms() const { return e + PREFIX; }
  int sign() const { return n == 0 ? 0 : terms()[n - 1] > 0 ? 1 : -1; }
};

// n . p + d = 0; the int64grid backend holds n and d as integers, the shewchuk backend (fp) as expansions
struct Plane //-V730 fp picks which union member is set
{
  union
  {
    BigInt<2> n[3];
    ExView fn[3];
  };
  union
  {
    BigInt<2> d;
    ExView fd;
  };
  double nd[3], dd; // within two ulps, for the filter
  bool fp = false;
  int8_t axis = 0; // int64grid: plane_axis
};

// an edge direction or the direction of a plane-plane line
struct ExactDir //-V730 fp picks which union member is set
{
  union
  {
    BigInt<3> c[3];
    ExS f[3];
  };
  double approx[3]; // within two ulps
  bool fp = false;
};

enum class PointKind : uint8_t
{
  Grid,
  Lpi,
  Ssi,
  Tpi,
};

// the point is c / w, w > 0
template <int N>
struct HomPoint
{
  BigInt<N> c[3], w;
};

struct FpHom
{
  ExView c[3], w;
};

// Grid: an input vertex, on the grid (approx exact) or a double triple (fp, approx exact); the others are constructed
struct ExactPoint
{
  PointKind kind = PointKind::Grid;
  bool fp = false;
  bool keyed = false;   // set_key ran: the key names the point within its arrangement
  uint32_t key[4] = {}; // see set_key
  DPoint3 approx;       // grid units, or the model's units for fp
  double err = 0;       // |coordinate - approx| <= err on every axis; 0: approx is exact
  union
  {
    GridPoint grid;
    HomPoint<3> lpi; // Lpi and Ssi
    HomPoint<5> tpi;
    FpHom fh; // every constructed fp point
  };
};

// The ExactPoint forms of make_plane, edge_dir, make_lpi and make_ssi take leaf points only (PointKind::Grid)
void cross(const BigInt<2> a[3], const BigInt<2> b[3], BigInt<3> out[3]);
// n is the triangle's cross product with the first non-zero component positive, so a plane has many forms.
// Returns the orientation of the triangle against pl.n (+1 or -1), 0 when it is degenerate.
int make_plane(const GridPoint &a, const GridPoint &b, const GridPoint &c, Plane &pl);
// the same for either backend; fp: n as it comes, the first non-zero component positive, kept in the arena
int make_plane(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, ExArena &arena, Plane &pl);
int plane_sign(const Plane &pl, int i);
int plane_axis(const Plane &pl); // the largest normal component
// a total order in which equal planes compare equal: projective on either backend
int plane_cmp(const Plane &a, const Plane &b);

ExactDir edge_dir(const GridPoint &from, const GridPoint &to);
ExactDir edge_dir(const ExactPoint &from, const ExactPoint &to);
ExactDir line_dir(const Plane &a, const Plane &b);
int dir_axis(const ExactDir &d); // the largest component
int dir_sign(const ExactDir &d, int i);
void dir_negate(ExactDir &d);

ExactPoint make_grid_point(const GridPoint &g);
ExactPoint make_fp_point(const DPoint3 &p);
// false when the edge is parallel to the plane; the grid forms are the int64grid backend alone
bool make_lpi(const GridPoint &p, const GridPoint &q, const Plane &pl, ExactPoint &out);
bool make_lpi(const ExactPoint &p, const ExactPoint &q, const Plane &pl, ExArena &arena, ExactPoint &out);
// edge pq crossing the coplanar edge rs, as the LPI of pq with the plane through rs along the projection axis
bool make_ssi(const GridPoint &p, const GridPoint &q, const GridPoint &r, const GridPoint &s, int axis, ExactPoint &out);
bool make_ssi(const ExactPoint &p, const ExactPoint &q, const ExactPoint &r, const ExactPoint &s, int axis, ExArena &arena,
  ExactPoint &out);
// false when the three planes have no unique intersection
bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExactPoint &out);
bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExArena &arena, ExactPoint &out);

// sorts each construction: Grid {vertex}, Lpi {edge v0, v1, plane}, Ssi {edge v0, v1, edge v0, v1}, Tpi {three planes}
void set_key(ExactPoint &p, uint32_t a, uint32_t b = 0, uint32_t c = 0, uint32_t d = 0);
bool key_less(const ExactPoint &a, const ExactPoint &b);

// The shewchuk backend: the same constructions and exact signs on expansions of the input doubles (fp.cpp)
namespace fp
{
// 0 for a degenerate triangle, or an expansion overflow, which leaves ex_overflow set
int make_plane(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, ExArena &arena, Plane &pl);
int plane_cmp(const Plane &a, const Plane &b);
int plane_axis(const Plane &pl);
int dir_axis(const ExactDir &d);
ExactDir edge_dir(const DPoint3 &from, const DPoint3 &to);
ExactDir line_dir(const Plane &a, const Plane &b);
// reads only pl.fn and pl.fd, as make_ssi's plane holds
bool make_lpi(const DPoint3 &p, const DPoint3 &q, const Plane &pl, ExArena &arena, ExactPoint &out);
bool make_ssi(const DPoint3 &p, const DPoint3 &q, const DPoint3 &r, const DPoint3 &s, int axis, ExArena &arena, ExactPoint &out);
bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExArena &arena, ExactPoint &out);
int point_plane(const ExactPoint &p, const Plane &pl);

// the three points by kind and key, the dropped axis, then the sign
struct MemoSlot
{
  uint32_t key[3][4];
  uint8_t kind[3], axis;
  int8_t sign;
  bool used;
};

// one worker's orient2d signs of one run: the keys name a point within one arrangement, so a memo lives one run
struct Memo
{
  dag::Vector<MemoSlot> slots; // direct-mapped, a power of two; orient2d makes it on the first keyed sign

  void init() { slots.assign(1 << 16, MemoSlot{}); }
};

int orient2d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, int u, int v, Memo *memo);
int compare(const ExactPoint &a, const ExactPoint &b, int axis);
int orient3d(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, const DPoint3 &d);
int wedge_order(const Plane &a, const Plane &b, const ExactDir &d);
int sign_dot(const Plane &a, const Plane &b);
// the axis coordinate of plane t at q minus that of f, the (e, e^2) move on a tie (ray.cpp)
int beyond_sign(const Plane &t, const Plane &f, const DPoint3 &q, int axis);
} // namespace fp
} // namespace dacsg::exact
