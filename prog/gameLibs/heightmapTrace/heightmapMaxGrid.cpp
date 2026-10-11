// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmapTrace/heightmapMaxGrid.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_rwResource.h>
#include <math/dag_adjpow2.h>

static ShaderVariableInfo heightmap_max_grid_ltVarId("heightmap_max_grid_lt", true);
static ShaderVariableInfo heightmap_max_grid_cellVarId("heightmap_max_grid_cell", true);
static ShaderVariableInfo heightmap_max_grid_updateVarId("heightmap_max_grid_update", true);
static ShaderVariableInfo tex_hmap_lowVarId("tex_hmap_low", true);

HeightmapMaxGrid::~HeightmapMaxGrid() { close(); }

bool HeightmapMaxGrid::init(uint32_t cells_, float cell_size)
{
  if (buf && cells == cells_ && cellSize == cell_size)
    return true;
  close();
  if (!cells_ || !is_pow_of2(cells_) || !(cell_size > 0))
    return false;
  updateCs.reset(new_compute_shader("heightmap_max_grid_update_cs", true));
  if (!updateCs)
    return false;
  buf = dag::create_sbuffer(sizeof(uint32_t), cells_ * cells_, SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0,
    "heightmap_max_grid");
  if (!buf)
  {
    close();
    return false;
  }
  // -1e9f, "no terrain", until a real fill lands: traces never sample an unready heightfield
  uint32_t *data = nullptr;
  if (buf.getBuf()->lock(0, cells_ * cells_ * sizeof(uint32_t), (void **)&data, VBLOCK_WRITEONLY) && data)
  {
    for (uint32_t i = 0, e = cells_ * cells_; i < e; ++i)
      data[i] = 0xCE6E6B28u;
    buf.getBuf()->unlock();
  }
  cells = cells_;
  cellSize = cell_size;
  ShaderGlobal::set_float4(heightmap_max_grid_cellVarId, cellSize, 1.f / cellSize, 0, 0);
  return true;
}

void HeightmapMaxGrid::close()
{
  buf.close();
  updateCs.reset();
  cells = 0;
  lt = IPoint2(INVALID_COORD, INVALID_COORD);
  pending.setempty();
  ShaderGlobal::set_int4(heightmap_max_grid_ltVarId, 0, 0, 0, 0);
}

void HeightmapMaxGrid::dispatchRect(const IPoint2 &rect_lt, const IPoint2 &rect_size)
{
  ShaderGlobal::set_int4(heightmap_max_grid_updateVarId, rect_lt.x, rect_lt.y, rect_size.x, rect_size.y);
  updateCs->dispatchThreads(rect_size.x, rect_size.y, 1);
}

void HeightmapMaxGrid::update(const Point3 &view_pos)
{
  if (!buf)
    return;
  if (tex_hmap_lowVarId.get_texture() == BAD_TEXTUREID)
  {
    // no heightfield: zero cells reads as off, the march skips; the window is
    // forgotten too, the next heightfield may be another one at the same place
    ShaderGlobal::set_int4(heightmap_max_grid_ltVarId, 0, 0, 0, 0);
    lt = IPoint2(INVALID_COORD, INVALID_COORD);
    return;
  }
  const IPoint2 newLt(int(floorf(view_pos.x / cellSize)) - int(cells / 2), int(floorf(view_pos.z / cellSize)) - int(cells / 2));
  ShaderGlobal::set_int4(heightmap_max_grid_ltVarId, newLt.x, newLt.y, int(cells), int(cells - 1));
  // no sync between the update's dispatches: overlapping rects store the same value
  const bool scrolled = newLt != lt;
  bool dispatched = scrolled;
  if (scrolled)
  {
    const IPoint2 d = newLt - lt;
    if (lt.x == INVALID_COORD || abs(d.x) >= int(cells) || abs(d.y) >= int(cells))
      dispatchRect(newLt, IPoint2(cells, cells));
    else
    {
      // the band that scrolled in on each moved axis
      if (d.x)
        dispatchRect(IPoint2(d.x > 0 ? lt.x + int(cells) : newLt.x, newLt.y), IPoint2(abs(d.x), cells));
      if (d.y)
      {
        if (d.x)
          d3d::resource_barrier({buf.getBuf(), RB_NONE});
        dispatchRect(IPoint2(newLt.x, d.y > 0 ? lt.y + int(cells) : newLt.y), IPoint2(cells, abs(d.y)));
      }
    }
    lt = newLt;
  }
  if (!pending.isempty())
  {
    // a cell's max reads a texel past its edges
    const IPoint2 from = max(IPoint2(int(floorf(pending[0].x / cellSize)) - 1, int(floorf(pending[0].y / cellSize)) - 1), newLt);
    const IPoint2 to =
      min(IPoint2(int(floorf(pending[1].x / cellSize)) + 2, int(floorf(pending[1].y / cellSize)) + 2), newLt + IPoint2(cells, cells));
    pending.setempty();
    if (from.x < to.x && from.y < to.y)
    {
      if (dispatched)
        d3d::resource_barrier({buf.getBuf(), RB_NONE});
      dispatchRect(from, to - from);
      dispatched = true;
    }
  }
  // fill inputs can change without a scroll (content streams in or is
  // re-rendered): a rolling row revisits the window over cells updates
  refreshRow = (refreshRow + 1) & (cells - 1);
  if (dispatched)
    d3d::resource_barrier({buf.getBuf(), RB_NONE});
  dispatchRect(IPoint2(newLt.x, newLt.y + int(refreshRow)), IPoint2(int(cells), 1));
  d3d::resource_barrier({buf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_SOURCE_STAGE_COMPUTE});
}
