#include <phys/dag_fastPhys.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug3d.h>


void FastPhysSystem::debugRender()
{
  real r = 0.03f;
  E3DCOLOR color(255, 20, 20);
  E3DCOLOR velColor(255, 20, 255);

  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  for (int i = 0; i < points.size(); ++i)
  {
    Point &pt = points[i];

    ::draw_cached_debug_line(pt.pos - Point3(r, 0, 0), pt.pos + Point3(r, 0, 0), color);
    ::draw_cached_debug_line(pt.pos - Point3(0, r, 0), pt.pos + Point3(0, r, 0), color);
    ::draw_cached_debug_line(pt.pos - Point3(0, 0, r), pt.pos + Point3(0, 0, r), color);

    ::draw_cached_debug_line(pt.pos, pt.pos - pt.vel * 0.1f, velColor);
  }
}
