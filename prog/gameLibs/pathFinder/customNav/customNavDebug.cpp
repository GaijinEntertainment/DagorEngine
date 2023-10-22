#include <pathFinder/pathFinder.h>
#include <pathFinder/customNav.h>
#include <detourNavMesh.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <util/dag_string.h>

namespace pathfinder
{

void CustomNav::drawDebug() const
{
  if (!getNavMeshPtr())
    return;
  for (const auto &it : areas)
  {
    draw_debug_box(it.second.oobb, it.second.tm,
      (it.second.isCylinder ? E3DCOLOR_MAKE(0, 0, 200, 200) : E3DCOLOR_MAKE(0, 200, 0, 200)));
    String text1(128, "id=%u, tileCnt=%d", it.second.id, it.second.tileCount);
    String text2(128, "gen=%u", it.second.generation);
    add_debug_text_mark(it.second.getAABB().center(), text1.str(), -1, 0, E3DCOLOR(0, 0, 200, 200));
    add_debug_text_mark(it.second.getAABB().center(), text2.str(), -1, 1, E3DCOLOR(0, 0, 200, 200));
  }
}
} // namespace pathfinder
