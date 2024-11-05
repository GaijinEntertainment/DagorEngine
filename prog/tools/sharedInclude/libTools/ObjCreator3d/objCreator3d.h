// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <math/dag_mesh.h>

#include <util/dag_globDef.h>
#include <generic/dag_tab.h>
#include <math/dag_math3d.h>


class Point2;
class IPoint3;
class StaticGeometryContainer;
class MaterialDataList;
class StaticGeometryMaterial;
class Mesh;
class BezierSpline3d;

class ObjCreator3d
{
public:
  // Creates (or recreates) plane in "geom" container as a node with 0 index
  static bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom);
  static bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, MaterialDataList *material);
  static bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material);

  // Creates (or recreates) sphere in "geom" container as a node with 0 index
  static bool generateSphere(int segments, StaticGeometryContainer &geom);
  static bool generateSphere(int segments, StaticGeometryContainer &geom, MaterialDataList *material);
  static bool generateSphere(int segments, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    Color4 col = Color4(1, 1, 1, 1));

  // Creates (or recreates) box in "geom" container as a node with 0 index
  static bool generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom);
  static bool generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom, MaterialDataList *material);
  static bool generateBox(const IPoint3 &segments_count, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    Color4 col = Color4(1, 1, 1, 1));

  // Creates (or recreates) cylinder in "geom" container as a node with 0 index
  static bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom);
  static bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom,
    MaterialDataList *material);
  static bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom,
    StaticGeometryMaterial *material);

  // Creates (or recreates) capsule in "geom" container as a node with 0 index
  static bool generateCapsule(int sides, int segments, StaticGeometryContainer &geom);
  static bool generateCapsule(int sides, int segments, StaticGeometryContainer &geom, MaterialDataList *material);
  static bool generateCapsule(int sides, int segments, StaticGeometryContainer &geom, StaticGeometryMaterial *material);

  // Creates (or recreates) polymesh in "geom" container as a node with 0 index
  static bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom);
  static bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom,
    MaterialDataList *material);
  static bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom,
    StaticGeometryMaterial *material);

  // Creates (or recreates) stairs in "geom" container as a node with 0 index
  static bool generateBoxStair(int steps, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateBoxStair(int steps, StaticGeometryContainer &geom);
  static bool generateBoxStair(int steps, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateClosedStair(int steps, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateClosedStair(int steps, StaticGeometryContainer &geom);
  static bool generateClosedStair(int steps, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom);
  static bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom,
    StaticGeometryMaterial *material, const Color4 col = Color4(1, 1, 1, 1));
  static bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom);

  static bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom);

  static bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, StaticGeometryMaterial *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom, MaterialDataList *material,
    const Color4 col = Color4(1, 1, 1, 1));
  static bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom);

  static bool generateLoftObject(BezierSpline3d &path, const Tab<Point3 *> &sections, int points_count, real step,
    StaticGeometryContainer &geom, bool closed = true);

  static bool generateLoftObject(const Tab<Point3> &path, const Tab<Point3 *> &sections, int points_count, real step,
    StaticGeometryContainer &geom, bool closed = true);

  static bool addNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom);
  static bool addNode(const char *name, Mesh *mesh, StaticGeometryMaterial *material, StaticGeometryContainer &geom);

private:
  static unsigned planeIdx;
  static unsigned sphereIdx;
  static unsigned boxIdx;

  static Mesh *generatePlaneMesh(const Point2 &cell_size);
  static Mesh *generateSphereMesh(int segments, Color4 col = Color4(1, 1, 1, 1));
  static Mesh *generateBoxMesh(const IPoint3 &segments_count, Color4 col = Color4(1, 1, 1, 1));

  static Mesh *generateBoxStair(int steps, const Color4 col = Color4(1, 1, 1, 1));
  static Mesh *generateClosedStair(int steps, const Color4 col = Color4(1, 1, 1, 1));
  static Mesh *generateOpenStair(int steps, real h, const Color4 col = Color4(1, 1, 1, 1), bool for_spiral = false);
  static Mesh *generateStair(int steps, int add_verts, int add_faces, bool for_spiral = false);
  static Mesh *generateBoxSpiralStair(int steps, real w, real arc, const Color4 col = Color4(1, 1, 1, 1));
  static Mesh *generateClosedSpiralStair(int steps, real w, real arc, const Color4 col = Color4(1, 1, 1, 1));
  static Mesh *generateOpenSpiralStair(int steps, real w, real h, real arc, const Color4 col = Color4(1, 1, 1, 1));
  static void setColorAndOrientation(Mesh *mesh, const Color4 col, bool for_spiral);

  static void generateRing(Tab<Point3> &vert, Tab<Point2> &tvert, int segments, real y);
  static void generateRing(int sides, float y, float radius, Tab<Point3> &vert);

  static void appendTriangle(int x1, int x2, int x3, Tab<Face> &face, int &face_id, bool is_extern);
  static void appendQuad(int x1, int x2, int x3, int x4, Tab<Face> &face, int &face_id, bool is_extern);
  static void appendBoxRingVertex(real z_offset, const IPoint2 &count, Tab<Point3> &vert);
  static void appendBoxRingFaces(int ring1_vert_offset, int ring2_vert_offset, int count, Tab<Face> &face, int &face_id);
  static void appendBoxPlaneFaces(const Point3 &f, int ring_vert_offset, int vert_in_ring_count, int plane_vert_offset,
    int vert_in_plane_count, Tab<Face> &face, int &face_id, bool is_extern);

  static Mesh *generateCylinderMesh(int sides, int height_segments, int cap_segments);

  static void generateCap(int cap_vertex_offset, int ring_vertex_offset, int sides, Tab<Face> &face, int &face_id, bool is_extern);

  static Mesh *generatePolyMesh(const Tab<Point2> &points, int height_segments);

  static void generatePolyPlaneFaces(const Tab<Point2> &poses, int vertex_offset, Tab<Face> &face, int &face_id, bool is_extern);

  static Mesh *generateLoftMesh(BezierSpline3d &path, const Tab<Point3 *> &sections, int points_count, real step, bool closed);

  static Mesh *generateCapsuleMesh(int sides, int segments);
};
