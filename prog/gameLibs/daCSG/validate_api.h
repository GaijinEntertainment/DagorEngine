// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/solid_api.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>

namespace dacsg
{
struct AngleHistogram
{
  static constexpr int BINS = 8;
  static constexpr double binUpperDeg[BINS - 1] = {0.5, 1, 2, 5, 10, 20, 30}; // the top bin is open
  uint32_t bin[BINS] = {};

  void add(double min_angle_deg)
  {
    int i = 0;
    while (i < BINS - 1 && min_angle_deg >= binUpperDeg[i])
      ++i;
    ++bin[i];
  }
};

enum ValidateFlags : uint32_t
{
  VALIDATE_SELF_INTERSECTION = 1u << 0,
};

struct FacePairIntersection
{
  uint32_t faceA = 0, faceB = 0;
  DPoint3 from, to; // crossing segment; coplanar: the face centers
};

struct ValidationReport
{
  static constexpr uint32_t MAX_REPORTED_INTERSECTIONS = 8;

  bool valid = false;
  uint32_t nonFiniteVerts = 0;
  uint32_t unpairedHalfedges = 0;
  uint32_t misorientedEdges = 0;
  uint32_t duplicateFaces = 0;
  uint32_t zeroAreaFaces = 0;
  uint32_t misalignedArrays = 0;       // a provenance or facePlaneKey array of the wrong size
  uint32_t strayInwardShells = 0;      // inward shells that no other shell holds
  uint32_t extraFans = 0;              // face fans beyond one per used vertex: shells that touch at a vertex index
  bool selfIntersectionTested = false; // false too when the coordinates pass the float range
  uint32_t selfIntersectingPairs = 0;
  dag::Vector<FacePairIntersection> firstIntersections; // in face pair order
  double signedVolume = 0;
  int eulerCharacteristic = 0;
  eastl::string error;
};

struct CompareParams
{
  double maxVolumeRelDiff = 1e-4;
  double maxAreaRelDiff = 1e-3;
  uint32_t samples = 10000;
  uint32_t maxDisagreements = 0;
  uint32_t maxListedDisagreements = 4096; // the first ones are kept
  uint32_t seed = 1;
  double tolerance = 0; // 0: auto over the joint bbox
};

struct Disagreement
{
  Point3 p;
  uint8_t side = 0; // 0: inside a only, 1: inside b only
};

struct CompareReport
{
  bool equal = false;
  double volumeA = 0, volumeB = 0, volumeRelDiff = 0;
  double areaA = 0, areaB = 0, areaRelDiff = 0;
  uint32_t samples = 0;
  uint32_t disagreements = 0;
  dag::Vector<Disagreement> disagreementPoints; // in sample order
  double surfaceDistMax = 0;                    // DBL_MAX: no samples, or only one solid has faces
  double surfaceDistMean = 0;
  eastl::string error;
};

struct QualityReport
{
  uint32_t triangles = 0;
  AngleHistogram minAngle;
  uint32_t slivers = 0;
  uint32_t degenerateFaces = 0;
  uint32_t nonManifoldEdges = 0; // edges with one halfedge or more than two, and halfedges from a vertex to itself
  uint32_t duplicateVerts = 0;
  uint32_t skinFaces = 0; // in pairs with the same vertices and opposite orientation
  double shortestEdge = 0;
};

ValidationReport validate(const Solid &s, uint32_t flags = 0);
CompareReport compare(const Solid &a, const Solid &b, const CompareParams &p = CompareParams());
QualityReport quality(const Solid &s, const CleanupParams &p);
} // namespace dacsg
