//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>
#include <math/integer/dag_IBBox2.h>
#include <generic/dag_tab.h>

// returns index of box region closest to helper.origin, and split it so there is no wrapping (all other boxes will be added to same
// array)
inline int get_closest_region_and_split(const ToroidalHelper &helper, Tab<IBBox2> &regions);

// get_texture_box returns box around origin [helper.curOrigin - helper.texSize/2, helper.curOrigin - helper.texSize/2 - 1]
inline IBBox2 get_texture_box(const ToroidalHelper &helper);

// split box so there is no wrapping, for up to 4 boxes.
// it is assumed, that box is clipped with texture box
// returns true if cnt>1
// boxes[0] is always written
inline bool split_toroidal(const IBBox2 &ibox, const ToroidalHelper &helper, IBBox2 *boxes, int &cnt);

// returns index of box region closest to point in array
inline int select_closest_region(const IPoint2 &p, dag::ConstSpan<IBBox2> boxes);

// split region alongs it's longest side/axis into two. origin used to determ closest region.
// only will be split if total box area is bigger than 2*max_texels_area (so bestRegion area is max_texels_area, and restRegion is at
// least of same area) return number of restRegions, 0 if not splitted
inline int split_region_to_size(int max_texels_area, const IBBox2 &region, IBBox2 &best, IBBox2 restRegion[2], const IPoint2 &origin);

// add new box to array of non intersected boxes
// box will be either:
//    added (if it is not intersecting any of boxes),
//    ignored (if it is completely within any of current boxes)
//    splitted in boxes that are not intersecting any of current boxes
// called recursively
inline void add_non_intersected_box(Tab<IBBox2> &boxes, const IBBox2 &ibox);

inline IBBox2 get_texture_box(const ToroidalHelper &helper)
{
  const IPoint2 halfTexSize(helper.texSize >> 1, helper.texSize >> 1);
  const IPoint2 lt = helper.curOrigin - halfTexSize;
  return IBBox2(lt, lt + IPoint2(helper.texSize - 1, helper.texSize - 1));
}

// clip region, so it is inside texture box
inline void toroidal_clip_region(const ToroidalHelper &helper, IBBox2 &region)
{
  const IBBox2 textureBox = get_texture_box(helper);
  textureBox.clipBox(region);
}

// clip array of regions, so they are all inside texture box
inline void toroidal_clip_regions(const ToroidalHelper &helper, Tab<IBBox2> &regions)
{
  const IBBox2 textureBox = get_texture_box(helper);
  for (int i = regions.size() - 1; i >= 0; --i)
  {
    textureBox.clipBox(regions[i]);
    if (regions[i].isEmpty())
      erase_items(regions, i, 1);
  }
}

inline bool split_toroidal(const IBBox2 &ibox, const ToroidalHelper &helper, IBBox2 *boxes, int &cnt)
{
  const IPoint2 halfTexSize(helper.texSize >> 1, helper.texSize >> 1), texSize(helper.texSize, helper.texSize);
  const IBBox2 textureBox = get_texture_box(helper);
  IBBox2 cbox = ibox;
  textureBox.clipBox(cbox);
  G_ASSERTF(cbox == ibox, "cbox = %@, ibox= %@ textureBox=%@", cbox, ibox, textureBox);
  const IPoint2 wrappedOriginOfs = (helper.curOrigin - helper.mainOrigin) % helper.texSize;
  const IPoint2 wrapPtViewport = (wrappedOriginOfs + texSize) % helper.texSize;
  const IPoint2 splitPt = textureBox[1] - wrapPtViewport;
  // const IPoint2 wrapPt = (wrapPtViewport - wrappedOriginOfs + texSize)%texSize + textureBox[0];
  // if (ibox[1].x < wrapPt.x || ibox[0].x > wrapPt.x)
  const IPoint2 iboxLT = transform_point_to_viewport(cbox[0], helper), iboxRB = transform_point_to_viewport(cbox[1], helper);

  boxes[0] = cbox;
  cnt = 1;

  if (iboxLT.x < iboxRB.x && iboxLT.y < iboxRB.y) // totally inside on quadrant
  {
    // debug("cbox %@, viewport= %@..%@", cbox, iboxLT, iboxRB);
    return false;
  }
  // debug("curOrigin = %@ mainOrigin=%@ cbox %@, viewport= %@..%@", helper.curOrigin, helper.mainOrigin, cbox, iboxLT, iboxRB);

#define SPLIT(attr)                      \
  if (iboxLT.attr > iboxRB.attr)         \
  {                                      \
    const int splitLine = splitPt.attr;  \
    for (int i = 0, e = cnt; i < e; ++i) \
    {                                    \
      boxes[cnt] = boxes[i];             \
      boxes[cnt][1].attr = splitLine;    \
      boxes[i][0].attr = splitLine + 1;  \
      cnt++;                             \
    }                                    \
  }
  SPLIT(x) // split horizontal
  SPLIT(y) // split vertical
#undef SPLIT
  IBBox2 box = boxes[0];
  for (int i = 1; i < cnt; ++i)
    box += boxes[i];
  G_ASSERT(box == cbox);
  return cnt > 1;
}

inline int select_closest_region(const IPoint2 &p, dag::ConstSpan<IBBox2> boxes)
{
  int id = -1, maxDist = 0x7FFFFFFF;
  for (int i = 0; i < boxes.size(); ++i)
  {
    int dist = sq_distance_ipoint_to_ibox2(p, boxes[i]);
    if (dist == 0)
      return i;
    if (dist < maxDist)
    {
      id = i;
      maxDist = dist;
    }
  }
  return id;
}

inline int get_closest_region_and_split(const ToroidalHelper &helper, Tab<IBBox2> &regions)
{
  if (!regions.size())
    return -1;
  const int id = select_closest_region(helper.curOrigin, regions);

  // split box if we can't render in one quad (if texture border is splitting our quad)
  IBBox2 splitBoxes[4];
  int cnt;
  split_toroidal(regions[id], helper, splitBoxes, cnt);

  if (cnt > 1) // select best sub box
  {
    const int splitId = select_closest_region(helper.curOrigin, dag::ConstSpan<IBBox2>(splitBoxes, cnt));
    for (int i = 0; i < cnt; ++i)
    {
      if (i != splitId)
        regions.push_back(splitBoxes[i]);
    }
    regions[id] = splitBoxes[splitId];
  }
  return id;
}

// split along longest box axis
#define SPLIT(attr)                                           \
  {                                                           \
    const int splitWidth = int(regionWidth.attr * texRegion); \
    const int splitLine0 = region[0].attr + splitWidth;       \
    restRegion[0][0].attr = splitLine0 + 1;                   \
    bestRegion[0][1].attr = splitLine0;                       \
    const int splitLine1 = region[1].attr - splitWidth;       \
    bestRegion[1][0].attr = splitLine1 + 1;                   \
    restRegion[1][1].attr = splitLine1;                       \
  }

inline int split_region_to_size(int max_texels, const IBBox2 &region, IBBox2 &best, IBBox2 restRegions[2], const IPoint2 &origin)
{
  const IPoint2 regionWidth = region.width() + IPoint2(1, 1);
  const int regionTexSize = regionWidth.x * regionWidth.y;
  // check if invalid region is way too big for one update
  if (regionTexSize < max_texels * 2) // split if invalid region is too big
  {
    best = region;
    return 0;
  }

  const float texRegion = max_texels / float(regionTexSize);
  bool outX = (origin.x < region[0].x || origin.x > region[1].x), outY = (origin.y < region[0].y || origin.y > region[1].y);

  if (regionTexSize < max_texels * 3 || (outX && outY))
  {
    IBBox2 bestRegion[2], restRegion[2];
    bestRegion[1] = bestRegion[0] = restRegion[0] = restRegion[1] = region;
    if (regionWidth.x > regionWidth.y)
    {
      SPLIT(x)
    }
    else
    {
      SPLIT(y)
    }
    const int bestId = select_closest_region(origin, dag::ConstSpan<IBBox2>(bestRegion, 2));
    best = bestRegion[bestId];
    restRegions[0] = restRegion[bestId];
    G_ASSERT(!best.isEmpty() && !restRegions[0].isEmpty());
    return 1;
  }
  IBBox2 bestRet;
  bestRet = restRegions[0] = restRegions[1] = region;
  if ((regionWidth.x > regionWidth.y || outY) && !outX)
  {
    const int splitWidth = max(2, int(regionWidth.x * texRegion) / 2);
    bestRet[0].x = max(region[0].x, origin.x - splitWidth);
    bestRet[1].x = min(region[1].x, origin.x + splitWidth - 1);
    restRegions[0][1].x = bestRet[0].x - 1;
    restRegions[1][0].x = bestRet[1].x + 1;
  }
  else
  {
    const int splitWidth = max(2, int(regionWidth.y * texRegion) / 2);
    bestRet[0].y = max(region[0].y, origin.y - splitWidth);
    bestRet[1].y = min(region[1].y, origin.y + splitWidth - 1);
    restRegions[0][1].y = bestRet[0].y - 1;
    restRegions[1][0].y = bestRet[1].y + 1;
  }
  int cnt = 2;
  if (restRegions[1].isEmpty())
    cnt--;
  if (restRegions[0].isEmpty())
  {
    restRegions[0] = restRegions[1];
    cnt--;
  }

  G_ASSERTF(!bestRet.isEmpty() && !restRegions[0].isEmpty() && !restRegions[cnt - 1].isEmpty() && cnt,
    "cnt = %d region =%@ origin=%@ best=%@ rest = %@ %@ texRegion=%@", cnt, region, origin, bestRet, restRegions[0], restRegions[1],
    texRegion);
  best = bestRet;
  return cnt;
}

// Similar to the function above, but does the split based on desired region side dimensions instead of area
// Which allows to use it in a way that preserves more or less square regions
inline int split_region_to_size_linear(int desiredSide, const IBBox2 &region, IBBox2 &best, IBBox2 restRegions[2],
  const IPoint2 &origin)
{
  const IPoint2 regionWidth = region.width() + IPoint2(1, 1);
  bool outX = (origin.x < region[0].x || origin.x > region[1].x), outY = (origin.y < region[0].y || origin.y > region[1].y);

  int maxSide = max(regionWidth.x, regionWidth.y);
  if (maxSide < int(desiredSide) * 2)
  {
    best = region;
    return 0;
  }

  const float texRegion = float(desiredSide) / maxSide;

  if (
    maxSide < desiredSide * 3 || (outX && outY) || (regionWidth.x <= regionWidth.y && outY) || (regionWidth.x > regionWidth.y && outX))
  {
    IBBox2 bestRegion[2], restRegion[2];
    bestRegion[1] = bestRegion[0] = restRegion[0] = restRegion[1] = region;
    if (regionWidth.x > regionWidth.y)
    {
      SPLIT(x)
    }
    else
    {
      SPLIT(y)
    }
    const int bestId = select_closest_region(origin, dag::ConstSpan<IBBox2>(bestRegion, 2));
    best = bestRegion[bestId];
    restRegions[0] = restRegion[bestId];
    G_ASSERT(!best.isEmpty() && !restRegions[0].isEmpty());
    return 1;
  }
  IBBox2 bestRet;
  bestRet = restRegions[0] = restRegions[1] = region;
  if (regionWidth.x > regionWidth.y && !outX)
  {
    const int splitWidth = max(2, int(regionWidth.x * texRegion) / 2);
    bestRet[0].x = max(region[0].x, origin.x - splitWidth);
    bestRet[1].x = min(region[1].x, origin.x + splitWidth - 1);
    restRegions[0][1].x = bestRet[0].x - 1;
    restRegions[1][0].x = bestRet[1].x + 1;
  }
  else
  {
    const int splitWidth = max(2, int(regionWidth.y * texRegion) / 2);
    bestRet[0].y = max(region[0].y, origin.y - splitWidth);
    bestRet[1].y = min(region[1].y, origin.y + splitWidth - 1);
    restRegions[0][1].y = bestRet[0].y - 1;
    restRegions[1][0].y = bestRet[1].y + 1;
  }
  int cnt = 2;
  if (restRegions[1].isEmpty())
    cnt--;
  if (restRegions[0].isEmpty())
  {
    restRegions[0] = restRegions[1];
    cnt--;
  }

  G_ASSERTF(!bestRet.isEmpty() && !restRegions[0].isEmpty() && !restRegions[cnt - 1].isEmpty() && cnt,
    "cnt = %d region =%@ origin=%@ best=%@ rest = %@ %@ texRegion=%@", cnt, region, origin, bestRet, restRegions[0], restRegions[1],
    texRegion);
  best = bestRet;
  return cnt;
}

#undef SPLIT

inline void add_non_intersected_box(Tab<IBBox2> &boxes, const IBBox2 &ibox)
{
  // debug("add box = %@ to array of %d", ibox, boxes.size());
  for (int i = 0; i < boxes.size(); ++i)
  {
    if (!(boxes[i] & ibox))
      continue;
    if (is_box_inside_other(boxes[i], ibox)) // new box is bigger
    {
      // debug("new box =%@ is bigger than old one [%d] = %@", ibox, i, boxes[i]);
      erase_items(boxes, i, 1);
      i--;
      continue; // it can be bigger than other boxes as well
    }
    else if (is_box_inside_other(ibox, boxes[i])) // old box is bigger, we need completely ignore new one
    {
      // debug("new box =%@ is smaller than old one [%d] = %@", ibox, i, boxes[i]);
      return;
    }

    // boxes are intersected
    // we need split new box into one, two or three non-intersecting box
    const IBBox2 oldBox = boxes[i];
#define SPLIT(attr)                                         \
  if (ibox[0].x <= oldBox[1].x && ibox[1].x >= oldBox[0].x) \
  {                                                         \
    if (ibox[0].attr < oldBox[0].attr)                      \
    {                                                       \
      IBBox2 nbox = ibox;                                   \
      nbox[1].attr = oldBox[0].attr - 1;                    \
      add_non_intersected_box(boxes, nbox);                 \
      nbox = ibox;                                          \
      nbox[0].attr = oldBox[0].attr;                        \
      add_non_intersected_box(boxes, nbox);                 \
      return;                                               \
    }                                                       \
    else if (ibox[1].attr > oldBox[1].attr)                 \
    {                                                       \
      IBBox2 nbox = ibox;                                   \
      nbox[0].attr = oldBox[1].attr + 1;                    \
      add_non_intersected_box(boxes, nbox);                 \
      nbox = ibox;                                          \
      nbox[1].attr = oldBox[1].attr;                        \
      add_non_intersected_box(boxes, nbox);                 \
      return;                                               \
    }                                                       \
  }
    SPLIT(x)
    SPLIT(y)
#undef SPLIT
    G_ASSERT(0);
    return;
  }
  boxes.push_back(ibox);
}
