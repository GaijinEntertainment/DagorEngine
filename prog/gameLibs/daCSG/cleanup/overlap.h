// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cleanup/cleanup.h"
#include "mesh/solidUtil.h"

namespace dacsg
{
// faces in both orientations joined by chains of overlapping pairs, so not all within the tolerance of one plane;
// n is the normal of the lowest face, side A faces it
struct OverlapGroup
{
  dag::Vector<uint32_t> faces;
  DPoint3 n;
};

// the group's faces replaced by the symmetric difference of its two sides, spec 8 step 2
struct OverlapEdit
{
  dag::Vector<uint32_t> rename;    // pairs: a vertex of side B welded onto one of side A
  dag::Vector<uint32_t> tris;      // three ids each; a fan keeps its face's ids, so the rename applies to tris too
  dag::Vector<uint32_t> triSource; // the face each new face takes its attributes from
  dag::Vector<uint32_t> dead;      // the group's faces, and a face across a rim edge with seam vertices on it
};

// attribute records apply per leaf
inline bool same_look(const Solid &s, uint32_t f, uint32_t g, const ChartTest *charts)
{
  const FaceInfo &a = s.face[f], &b = s.face[g];
  return s.leafRef[f].leaf == s.leafRef[g].leaf && a.mat == b.mat && a.smgr == b.smgr && a.attrSet == b.attrSet &&
         (!charts || charts->sameChart(s.leafRef[f], s.leafRef[g]));
}

// a pair: opposite faces within the tolerance of the larger one's plane that overlap deeper than it; groups chain pairs
void find_overlap_groups(const Solid &s, dag::ConstSpan<uint8_t> live, double tolerance, dag::Vector<OverlapGroup> &groups);
// false leaves the group as it is: a pinched or open rim, a side with no rim or with adjacent faces of two looks,
// two rim vertices of one side within the tolerance, crossing rims, a cycle that does not close, sides that disagree,
// a failed triangulation; edit holds no usable result then
bool remove_overlap(const Solid &s, const OverlapGroup &g, double tolerance, const ChartTest *charts, OverlapEdit &edit);
} // namespace dacsg
