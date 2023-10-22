#include "riGen/riGridDebug.h"
#include "riGen/riGrid.h"
#include <ADT/linearGrid.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <cstdio>

struct size_stats_t
{
  float from, to;
  int count;
};

void rigrid_debug_pos(const RiGrid &grid, const Point3 &pos, const Point3 &camera_pos)
{
  begin_draw_cached_debug_lines();
  IPoint2 coords;
  if (const auto cell = grid.getCellByPosDebug(pos, coords))
  {
    char buf[1024];
    uint32_t mainObjects, subObjects;
    grid.countCellObjectsDebug(*cell, mainObjects, subObjects);
    snprintf(buf, sizeof(buf), "Cell [%i; %i] objects: %u; main layer: %u", coords.y, coords.x, mainObjects + subObjects, mainObjects);
    buf[sizeof(buf) - 1] = '\0';
    add_debug_text_mark(pos, buf);

    if (cell->rootLeaf != EMPTY_LEAF)
    {
      leaf_iterate_branch_boxes(&grid, cell->rootLeaf, cell->getBBox(), 10000, [](bbox3f bb, unsigned count, leaf_id_t) {
        BBox3 b;
        v_stu_bbox3(b, bb);
        draw_cached_debug_box(b, E3DCOLOR_MAKE(64, 255, 64, 255));
        char buf[64];
        snprintf(buf, sizeof(buf), "%i", count);
        buf[sizeof(buf) - 1] = '\0';
        add_debug_text_mark(b.center(), buf);
      });
    }

    size_stats_t stats[] = {{0.f, 0.5f}, {0.5f, 1.0f}, {1.f, 2.f}, {2.f, 3.f}, {3.f, 4.f}, {4.f, 6.f}, {6.f, 8.f}, {8.f, 12.f},
      {12.f, 16.f}, {16.f, 24.f}, {24.f, 32.f}, {32.f, 64.f}, {64.f, 128.f}, {128.f, 256.f}, {256.f, 50000.f}};

    grid.foreachObjectInCellDebug(*cell, [&](RiGridObject obj) {
      for (auto &ss : stats)
      {
        if (v_extract_w(obj.getWBSph()) >= ss.from && v_extract_w(obj.getWBSph()) < ss.to)
          ss.count++;
      }
      return false;
    });

    float offs = 1.2f;
    for (auto &ss : stats)
    {
      if (ss.count)
      {
        snprintf(buf, sizeof(buf), "Size %.1f-%.1f: %u", ss.from, ss.to, ss.count);
        buf[sizeof(buf) - 1] = '\0';
        add_debug_text_mark(pos, buf, -1, offs);
        offs += 1.2f;
      }
    }

    if (const LinearSubGrid<RiGridObject> *subGrid = grid.getSubGrid(cell->subGridIdx))
    {
      offs += 1.2f;
      snprintf(buf, sizeof(buf), "Subgrid objects: %u (%.1f%%)", subObjects, (subObjects / double(mainObjects + subObjects)) * 100.0);
      buf[sizeof(buf) - 1] = '\0';
      add_debug_text_mark(pos, buf, -1, offs);
      offs += 1.2f;

      Point3 cameraDir;
      v_stu_p3(&cameraDir.x, v_sub(v_bbox3_center(cell->getBBox()), v_ldu(&camera_pos.x)));
      int xScale = abs(cameraDir.x) > abs(cameraDir.z) ? 8 : 1;
      int zScale = abs(cameraDir.x) > abs(cameraDir.z) ? 1 : 8;
      int startX = ((xScale == 8 ? (cameraDir.x < 0) : (cameraDir.z >= 0)) ? 0 : (LINEAR_GRID_SUBGRID_WIDTH - 1));
      int startZ = ((xScale == 8 ? (cameraDir.x < 0) : (cameraDir.z < 0)) ? 0 : (LINEAR_GRID_SUBGRID_WIDTH - 1));
      int stepX = startX == 0 ? 1 : -1;
      int stepZ = startZ == 0 ? 1 : -1;
      for (int z = startZ; z >= 0 && z <= LINEAR_GRID_SUBGRID_WIDTH - 1; z += stepZ)
      {
        int len = 0;
        for (int x = startX; x >= 0 && x <= LINEAR_GRID_SUBGRID_WIDTH - 1; x += stepX)
        {
          const LinearGridSubCell<RiGridObject> &subCell = subGrid->getCellByOffset(z * zScale + x * xScale);
          int count = subCell.rootLeaf != EMPTY_LEAF ? leaf_count_objects(&grid, subCell.rootLeaf) : 0;
          len += snprintf(buf + len, sizeof(buf) - len, "[%s%s%u]", count > 99 ? "" : "_", count > 9 ? "" : "_", count);
          draw_cached_debug_box(BBox3(subCell.bboxMin, subCell.bboxMax), E3DCOLOR_MAKE(64, 255, 255, 255));
        }
        buf[sizeof(buf) - 1] = '\0';
        add_debug_text_mark(pos, buf, -1, offs);
        offs += 1.2f;
      }
    }
  }
  end_draw_cached_debug_lines();
}
