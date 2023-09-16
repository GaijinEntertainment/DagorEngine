#pragma once
#include <math/integer/dag_IBBox2.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <render/toroidal_update_regions.h>
#include <generic/dag_tab.h>

static bool intersects(const IBBox2 &ib, const IBBox2 *begin, const IBBox2 *end)
{
  for (; begin != end; ++begin)
    if (ib & *begin)
      return true;
  return false;
}

inline bool non_intersects(const Tab<IBBox2> &list)
{
  for (int i = 0; i < list.size(); ++i)
    for (int j = i + 1; j < list.size(); ++j)
      if (list[i] & list[j])
        return false;
  return true;
}

static void clip_list_to(Tab<IBBox2> &list, const IBBox2 &ib)
{
  for (int i = list.size() - 1; i >= 0; --i)
  {
    ib.clipBox(list[i]);
    if (list[i].isAreaEmpty())
      erase_items_fast(list, i, 1);
  }
  G_ASSERT(non_intersects(list));
}

static void subtract_region_from_non_intersected_list(const IBBox2 &b_, Tab<IBBox2> &list)
{
  for (int i = list.size() - 1; i >= 0; --i)
  {
    const IBBox2 lbx = list[i];
    if (!(lbx & b_)) // boxes not intersect
      continue;
    erase_items_fast(list, i, 1);
    if (is_box_inside_other(lbx, b_)) // box from list is completely inside other box
      continue;
    IBBox2 box = b_;
    lbx.clipBox(box); // we clip the source box by list box
    // boxes are intersected, we need to split list[i] by new box into 1..4
    IBBox2 add;
    add[0] = lbx[0];
    add[1] = IPoint2(box[0].x - 1, lbx[1].y);
    if (!add.isAreaEmpty())
      list.push_back(add);
    add[0] = IPoint2(box[0].x, lbx[0].y);
    add[1] = IPoint2(box[1].x, box[0].y - 1);
    if (!add.isAreaEmpty())
      list.push_back(add);

    add[0] = IPoint2(box[0].x, box[1].y + 1);
    add[1] = IPoint2(box[1].x, lbx[1].y);
    if (!add.isAreaEmpty())
      list.push_back(add);

    add[0] = IPoint2(box[1].x + 1, lbx[0].y);
    add[1] = lbx[1];
    if (!add.isAreaEmpty())
      list.push_back(add);
  }
}
