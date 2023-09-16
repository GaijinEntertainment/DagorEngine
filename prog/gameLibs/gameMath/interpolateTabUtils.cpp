#include <gameMath/interpolateTabUtils.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

bool read_interpolate_tab_float_p2(InterpolateTabFloat &tab, const DataBlock *blk)
{
  return read_interpolate_tab_as_params(tab, blk, [](const DataBlock *blk, int i, float &x, float &y) -> bool {
    Point2 p = blk->getPoint2(i);
    x = p.x;
    y = p.y;
    return true;
  });
}

bool read_interpolate_2d_tab_float_p2(Interpolate2DTabFloat &tab, const DataBlock *blk, const char *x_param_name)
{
  return read_interpolate_2d_tab_p2(
    tab, blk,
    [](const DataBlock *blk, int name_id, float &x) -> bool {
      x = blk->getRealByNameId(name_id, 0.f);
      return true;
    },
    [](const DataBlock *blk, int i, float &y, float &z) -> bool {
      Point2 p = blk->getPoint2(i);
      y = p.x;
      z = p.y;
      return true;
    },
    x_param_name);
}

bool read_interpolate_2d_tab_float_p3(Interpolate2DTabFloat &tab, const DataBlock *blk)
{
  return read_interpolate_2d_tab_p3(tab, blk, [](const DataBlock *blk, int i, float &x, float &y, float &z) -> bool {
    Point3 p = blk->getPoint3(i);
    x = p.x;
    y = p.y;
    z = p.z;
    return true;
  });
}
