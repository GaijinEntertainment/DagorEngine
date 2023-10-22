#include <phys/dag_fastPhys.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug3d.h>
#include <EASTL/set.h>

eastl::set<eastl::string> debugAnimCharsSet;

void FastPhys::toggleDebugAnimChar(eastl::string &str)
{
  auto it = debugAnimCharsSet.find(str);
  if (it != debugAnimCharsSet.end())
    debugAnimCharsSet.erase(it);
  else
    debugAnimCharsSet.insert(str);
}
bool FastPhys::checkDebugAnimChar(eastl::string &str) { return debugAnimCharsSet.find(str) != debugAnimCharsSet.end(); }
void FastPhys::resetDebugAnimChars() { debugAnimCharsSet.clear(); }


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
