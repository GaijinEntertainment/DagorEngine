//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>
#include <generic/dag_relocatableFixedVector.h>
#include <render/subtract_ibbox3.h>

inline IPoint3 world_to_voxel(Point3 world_pos, Point3 voxel_size)
{
  return ipoint3(floor(Point3::xzy(div(world_pos, voxel_size)) + 0.5));
}

inline IPoint3 get_thresholded_move(IPoint3 clip_size, IPoint3 move, int move_threshold)
{
  IPoint3 absMove = abs(move);
  int maxMove = max(max(absMove.x, absMove.y), absMove.z);

  if (maxMove <= move_threshold)
    return {};

  IPoint3 useMove{0, 0, 0};
  Point3 relMove = div(point3(absMove), clip_size);

  if (relMove.x + relMove.y + relMove.z >= 1) // trigger full update, as we have moved enough. This decrease number of
                                              // spiked frames after teleport
    useMove = move;
  else if (absMove.x == maxMove)
    useMove.x = move.x;
  else if (absMove.y == maxMove)
    useMove.y = move.y;
  else
    useMove.z = move.z;

  return useMove;
}

struct VoxelClip
{
  static constexpr int maxChangedBboxes = 7;
  static inline IPoint3 get_invalid_lt() { return {-100000, -100000, 100000}; }

  IPoint3 lt = get_invalid_lt();
  IBBox3 invalidClipBox;

  void invalidate()
  {
    lt = get_invalid_lt();
    invalidClipBox.setEmpty();
  }

  template <int align_w = 1, int align_d = 1>
  void invalidateBox(const BBox3 &box, uint32_t clip_w, uint32_t clip_d, float voxel_size)
  {
    IPoint3 clipSize(clip_w, clip_w, clip_d);
    IPoint3 alignment(align_w, align_w, align_d);
    Point3 alignedVoxelSize = Point3::xzy(alignment) * voxel_size;

    IBBox3 addInvalidClipBox;
    addInvalidClipBox[0] = mul(world_to_voxel(box[0] - alignedVoxelSize * 0.5f, alignedVoxelSize), alignment);
    addInvalidClipBox[1] = mul(world_to_voxel(box[1] + alignedVoxelSize * 0.5f, alignedVoxelSize), alignment);
    addInvalidClipBox = get_intersection(addInvalidClipBox, {lt, lt + clipSize});
    invalidClipBox += addInvalidClipBox;
  }

  bool isValid() const { return lt != get_invalid_lt() && invalidClipBox.isEmpty(); }

  template <int move_threshold = 1, int align_w = 1, int align_d = 1>
  dag::RelocatableFixedVector<IBBox3, VoxelClip::maxChangedBboxes, false> updatePos(const Point3 &world_pos, uint32_t clip_w,
    uint32_t clip_d, float voxel_size)
  {
    IPoint3 clipSize(clip_w, clip_w, clip_d);
    IPoint3 alignment(align_w, align_w, align_d);
    Point3 alignedVoxelSize = Point3::xzy(alignment) * voxel_size;
    IPoint3 alignedLt = div(lt, alignment);
    IPoint3 alignedClipSize = div(clipSize, alignment);

    IPoint3 movedAlignedLt = world_to_voxel(world_pos, alignedVoxelSize) - alignedClipSize / 2;
    IPoint3 useMove = mul(get_thresholded_move(alignedClipSize, movedAlignedLt - alignedLt, move_threshold), alignment);

    dag::RelocatableFixedVector<IBBox3, maxChangedBboxes, false> result;
    result.push_back_uninitialized(maxChangedBboxes);
    int changedCount = 0;

    IPoint3 prevLt = lt;
    lt += useMove;
    IBBox3 prevClipBox = {prevLt, prevLt + clipSize};
    IBBox3 newClipBox = {lt, lt + clipSize};

    if (useMove != IPoint3{0, 0, 0})
      changedCount += same_size_box_subtraction(newClipBox, prevClipBox, {result.data(), intptr_t(result.size())});

    IBBox3 toInvalidateBox = get_intersection(invalidClipBox, newClipBox);
    if (!toInvalidateBox.isEmpty())
      result[changedCount++] = toInvalidateBox;
    invalidClipBox.setEmpty();

    result.resize(changedCount);

    return result;
  }
};
