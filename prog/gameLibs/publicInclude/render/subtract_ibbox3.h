//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IBBox3.h>
#include <generic/dag_span.h>
// for toroidal updates
inline void split_by_y(IBBox3 box1, IBBox3 box2, dag::Span<IBBox3> &result, int &count)
{
  if (box1[0].y < box2[0].y)
    result[count++] = IBBox3{{box1[0].x, box1[0].y, box1[0].z}, {box1[1].x, box2[0].y, box1[1].z}};
  else if (box1[1].y > box2[1].y)
    result[count++] = IBBox3{{box1[0].x, box2[1].y, box1[0].z}, {box1[1].x, box1[1].y, box1[1].z}};
}

inline void split_by_x(IBBox3 box1, IBBox3 box2, dag::Span<IBBox3> &result, int &count)
{
  if (box1[0].x < box2[0].x)
  {
    result[count++] = IBBox3{{box1[0].x, box1[0].y, box1[0].z}, {box2[0].x, box1[1].y, box1[1].z}};
    if (box2[0].x < box1[1].x)
      split_by_y(IBBox3{{box2[0].x, box1[0].y, box1[0].z}, {box1[1].x, box1[1].y, box1[1].z}}, box2, result, count);
  }
  else if (box1[1].x > box2[1].x)
  {
    result[count++] = IBBox3{{box2[1].x, box1[0].y, box1[0].z}, {box1[1].x, box1[1].y, box1[1].z}};
    if (box1[0].x < box2[1].x)
      split_by_y(IBBox3{{box1[0].x, box1[0].y, box1[0].z}, {box2[1].x, box1[1].y, box1[1].z}}, box2, result, count);
  }
  else
  {
    split_by_y(box1, box2, result, count);
  }
}

inline void split_by_z(IBBox3 box1, IBBox3 box2, dag::Span<IBBox3> &result, int &count)
{
  if (box1[0].z < box2[0].z)
  {
    result[count++] = IBBox3{{box1[0].x, box1[0].y, box1[0].z}, {box1[1].x, box1[1].y, box2[0].z}};
    if (box2[0].z < box1[1].z)
      split_by_x(IBBox3{{box1[0].x, box1[0].y, box2[0].z}, {box1[1].x, box1[1].y, box1[1].z}}, box2, result, count);
  }
  else if (box1[1].z > box2[1].z)
  {
    result[count++] = IBBox3{{box1[0].x, box1[0].y, box2[1].z}, {box1[1].x, box1[1].y, box1[1].z}};
    if (box1[0].z < box2[1].z)
      split_by_x(IBBox3{{box1[0].x, box1[0].y, box1[0].z}, {box1[1].x, box1[1].y, box2[1].z}}, box2, result, count);
  }
  else
  {
    split_by_x(box1, box2, result, count);
  }
}

inline int same_size_box_subtraction(const IBBox3 &box1, const IBBox3 &box2, IBBox3 &intersection, dag::Span<IBBox3> &result)
{
  intersection = box2;
  box1.clipBox(intersection);
  if (intersection.isEmpty())
  {
    result[0] = box1;
    return 1;
  }
  int count = 0;
  split_by_z(box1, box2, result, count);
  return count;
}

inline int move_box_toroidal(const IPoint3 &new_lt, const IPoint3 &old_lt, const IPoint3 &sz, dag::Span<IBBox3> &retSpan)
{
  IBBox3 oldWorldBox = {old_lt, old_lt + sz};
  IBBox3 newWorldBox = {new_lt, new_lt + sz};
  IBBox3 in;
  int cnt = same_size_box_subtraction(newWorldBox, oldWorldBox, in, retSpan);
#if DAGOR_DBGLEVEL > 0
  for (auto &ci : dag::Span<IBBox3>(retSpan.data(), cnt))
  {
    IPoint3 v = ci.width();
    G_ASSERT(v.x > 0 && v.y > 0 && v.z > 0);
  }
#endif
  return cnt;
}
