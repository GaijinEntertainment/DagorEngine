//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <gameRes/dag_collResDecl.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMathDecl.h>

class GeomNodeTree;

// The unit side axes across the unit dir. The volumetric traces place their pattern rays on them: a change moves every pattern ray.
VECTORCALL void make_ray_side_axes(vec3f dir, vec3f &out_side_x, vec3f &out_side_y);

// Samples the mesh nodes of a collision resource once on a 16x16 grid across one axis into per-node surfaces, then answers the
// all-hits queries of the rays parallel to the axis from the surface samples: a hit is interpolated, not exact. Box, sphere and
// capsule nodes are not sampled. A build stops after 65535 recorded triangles and does not sample the rest of the geometry.
// The surfaces live in framemem: keep a grid on the thread and inside the framemem region where it was built.
class CollisionSurfaceGrid
{
  static constexpr int GRID_SIZE = 16;

public:
  static constexpr int SAMPLE_COUNT = GRID_SIZE * GRID_SIZE;

  using SurfaceNoList = dag::RelocatableFixedVector<int, 64, true, framemem_allocator>;

  // False when the resource has box, sphere or capsule nodes, which the grid would miss.
  static bool supports(const CollisionResource &coll_res);

  // dir is unit length. The grid spans half_width to each side of the axis. Back faces are kept only on SOLID and TRACE_TWO_SIDED
  // nodes and on the nodes of no_cull_node_mask: on the others they are the second side of a paired sheet. SOLID nodes are sampled
  // to their far side, the others to len.
  void build(const CollisionResource &coll_res, const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir, float len,
    float half_width, const CollisionNodeMask &collision_node_mask, const CollisionNodeMask *no_cull_node_mask,
    TraceCollisionResourceStats *out_stats);

  // The offset is in half_width units along the two side axes of the grid, which spans [-1, 1] on each.
  Point3 getRayStart(const Point2 &offset) const;
  // Replaces the list with the unsorted hits of the ray from getRayStart(offset) along the axis, closer than in_t, and
  // out_surface_nos with the surface of each hit. Each covering sample answers with the plane of its triangle at the offset, held to
  // the t range of that triangle, also for an offset outside [-1, 1]. A surface is hit when the samples it covers carry at least half
  // of the bilinear weight at the offset. The normal of a back face hit has a positive projection on the axis. The triRef of a hit is
  // the triangle of its heaviest sample.
  bool traceRay(const Point2 &offset, float in_t, CollResIntersectionsType &intersected_nodes_list,
    SurfaceNoList &out_surface_nos) const;
  // The offset of sample sample_no, in the units of getRayStart.
  static Point2 sampleOffset(int sample_no);
  // Replaces the list with the unsorted hits of the ray from getRayStart(sampleOffset(sample_no)) along the axis, closer than in_t,
  // and out_surface_nos with the surface of each hit: one hit for each surface that covers the sample, on the plane of its triangle.
  // The normal of a back face hit has a positive projection on the axis.
  bool traceSample(int sample_no, float in_t, CollResIntersectionsType &intersected_nodes_list, SurfaceNoList &out_surface_nos) const;

  int surfaceCount() const { return surfaces.size(); }
  // SAMPLE_COUNT bits, bit n set where the surface covers sample n.
  const uint32_t *coverage(int surface_no) const { return surfaces[surface_no].coverage; }

private:
  static constexpr int SAMPLE_BLOCK_COUNT = GRID_SIZE / 4;

  enum class Facing : uint8_t
  {
    FRONT,
    BACK
  };

  struct NodeTraits
  {
    bool solid = false;
    bool keepBackFaces = false;
  };

  struct Crossing
  {
    float t;
    uint16_t triangleNo;
    uint8_t sampleNo;
    Facing facing;
  };

  struct SurfaceTriangle
  {
    Point3 normal;
    float tAtAxis;
    float tSlopeX;
    float tSlopeY;
    float tMin;
    float tMax;
    tri_ref_t triRef;
  };

  struct NodeRaster
  {
    mat44f gridTm;
    float detSign;
    float length;
    bool keepBackFaces;
  };

  struct Surface
  {
    uint32_t coverage[SAMPLE_COUNT / 32] = {};
    uint16_t triangleNo[SAMPLE_COUNT]; //-V730_NOINIT

    bool covers(int sample_no) const { return (coverage[sample_no >> 5] >> (sample_no & 31)) & 1u; }
  };

  struct SamplePoint
  {
    float x, y;
    int sampleNo[4];
    float weight[4];
  };

  float sampleCoord(int index) const { return -halfWidth + float(index) * sampleStep; }
  SamplePoint makeSamplePoint(const Point2 &offset) const;
  bool rasterizeNode(const CollisionResource &coll_res, const CollisionNode &node, const mat44f &origin_relative_node_tm,
    const NodeTraits &traits, dag::Vector<Crossing, framemem_allocator> &crossings);
  void rasterizeTriangle(const NodeRaster &raster, vec3f p0, vec3f p1, vec3f p2, tri_ref_t tri_ref,
    dag::Vector<Crossing, framemem_allocator> &crossings);
  void buildNodeSurfaces(dag::Vector<Crossing, framemem_allocator> &crossings);
  bool interpolateSurface(const Surface &surface, const SamplePoint &point, float &out_t, Point3 &out_normal,
    tri_ref_t &out_tri_ref) const;

  vec3f origin, axisDir, sideX, sideY;
  vec4f sampleX[SAMPLE_BLOCK_COUNT];
  float halfWidth = 0.f;
  float sampleStep = 0.f;
  float axisLength = 0.f;
  dag::Vector<Surface, framemem_allocator> surfaces;
  dag::Vector<SurfaceTriangle, framemem_allocator> triangles;
};
