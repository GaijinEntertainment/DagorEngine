// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/cables.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_resetDevice.h>

eastl::unique_ptr<Cables> cables_mgr;

constexpr int CABLES_GRID_SIZE_X = 32;
constexpr int CABLES_GRID_SIZE_Y = 32;

static int cables_render_passVarId = -1;
static int pixel_scaleVarId = -1;

Cables *get_cables_mgr() { return cables_mgr.get(); }
void init_cables_mgr() { cables_mgr = eastl::make_unique<Cables>(); }
void close_cables_mgr() { cables_mgr.reset(); }

Cables::TiledArea::Tile &Cables::TiledArea::getTile(int x, int y) { return grid[y * gridSize.x + x]; }

void Cables::generateTiles()
{
  G_ASSERT(maxCables * 2 <= (uint16_t)-1);
  tiledArea.gridSize = IPoint2(CABLES_GRID_SIZE_X, CABLES_GRID_SIZE_Y);
  int tilesNum = tiledArea.gridSize.x * tiledArea.gridSize.y;
  tiledArea.grid.resize(tilesNum);
  tiledArea.cables_points.resize(maxCables * 2);

  tiledArea.gridBoundMin = Point2(cables[0].point1_rad.x, cables[0].point1_rad.z);
  Point2 gridBoundMax = tiledArea.gridBoundMin;
  for (int i = 0; i < cables.size(); ++i)
  {
    for (auto &point : {cables[i].point1_rad, cables[i].point2_sag})
    {
      tiledArea.gridBoundMin.x = min(point.x, tiledArea.gridBoundMin.x);
      tiledArea.gridBoundMin.y = min(point.z, tiledArea.gridBoundMin.y);
      gridBoundMax.x = max(point.x, gridBoundMax.x);
      gridBoundMax.y = max(point.z, gridBoundMax.y);
    }
  }
  tiledArea.tileSize = gridBoundMax - tiledArea.gridBoundMin;
  tiledArea.tileSize.x /= tiledArea.gridSize.x;
  tiledArea.tileSize.y /= tiledArea.gridSize.y;
  for (int idx = 0; idx < cables.size(); ++idx) // determine amount of cables ends in each tile
  {
    for (auto &point : {cables[idx].point1_rad, cables[idx].point2_sag})
    {
      int i = (point.x - tiledArea.gridBoundMin.x) / tiledArea.tileSize.x;
      int j = (point.z - tiledArea.gridBoundMin.y) / tiledArea.tileSize.y;
      if (i == tiledArea.gridSize.x) // for points lying on max bound
        --i;
      if (j == tiledArea.gridSize.y)
        --j;
      tiledArea.getTile(i, j).count++;
    }
  }
  int start = 0;
  for (int i = 0; i < tilesNum; ++i)
  {
    tiledArea.grid[i].start = start;
    start += tiledArea.grid[i].count;
    tiledArea.grid[i].count = 0;
  }
  for (int idx = 0; idx < cables.size(); ++idx) // fill array with cables ends
  {
    int cableEnd = 0;
    for (auto &point : {cables[idx].point1_rad, cables[idx].point2_sag})
    {
      int i = (point.x - tiledArea.gridBoundMin.x) / tiledArea.tileSize.x;
      int j = (point.z - tiledArea.gridBoundMin.y) / tiledArea.tileSize.y;
      if (i == tiledArea.gridSize.x) // for points lying on max bound
        --i;
      if (j == tiledArea.gridSize.y)
        --j;
      TiledArea::Tile &tile = tiledArea.getTile(i, j);
      tiledArea.cables_points[tile.start + tile.count].cableEnd = cableEnd;
      tiledArea.cables_points[tile.start + tile.count].cableIndex = idx;
      tile.count++;
      cableEnd++;
    }
  }
}

Cables::Cables()
{
  cablesRenderer.init("wires", nullptr, 0, "wires", true);
  cables_render_passVarId = ::get_shader_variable_id("cables_render_pass", true);
  pixel_scaleVarId = ::get_shader_variable_id("pixel_scale", true);
}

void Cables::loadCables(IGenLoad &crd)
{
  int cablesCount = crd.readInt();
  setMaxCables(cablesCount);
  cables.resize(cablesCount);
  crd.readTabData(cables);
  dirtyCables.clear();
  for (int i = 0; i < cablesCount; ++i)
    dirtyCables.emplace(i);
  changed = true;
  generateTiles();
}

void Cables::setMaxCables(unsigned int max_cables)
{
  maxCables = max_cables;
  changed = true;
  cablesBuf = dag::buffers::create_persistent_sr_structured(sizeof(CableData), maxCables, "cables_buf");
}

int Cables::addCable(const Point3 &p1, const Point3 &p2, float rad, float sag)
{
  changed = true;
  CableData cable = {
    Point4::xyzV(p1, rad),
    Point4::xyzV(p2, sag),
  };
  dirtyCables.emplace(cables.size());
  cables.push_back(cable);
  return cables.size() - 1;
}

void Cables::setCable(int id, const Point3 &p1, const Point3 &p2, float rad, float sag)
{
  changed = true;
  CableData cable = {
    Point4::xyzV(p1, rad),
    Point4::xyzV(p2, sag),
  };
  if (id < cables.size())
  {
    cables[id] = cable;
    dirtyCables.emplace(id);
  }
  else
  {
    cables.push_back(cable);
    dirtyCables.emplace(cables.size());
  }
}

bool Cables::beforeRender(float pixel_scale)
{
  ShaderGlobal::set_real_fast(pixel_scaleVarId, pixel_scale);
  return beforeRender();
}

bool Cables::beforeRender()
{
  TIME_D3D_PROFILE(cables_beforeRender)
  destroyCables();
  bool wasChanged = changed;
  if (changed && cables.size())
  {
    G_ASSERT(cablesRenderer.shader != nullptr);
    G_ASSERT(cables.size() <= maxCables);
    cablesBuf->updateData(0, sizeof(cables[0]) * cables.size(), cables.data(), 0);
    changed = false;
  }
  return wasChanged;
}

void Cables::render(int render_pass)
{
  if (!cables.size())
    return;
  ShaderGlobal::set_int_fast(cables_render_passVarId, render_pass);
  TIME_D3D_PROFILE(render_cables);
  if (cablesRenderer.shader)
  {
    cablesRenderer.shader->setStates(0, true);
    d3d::setvsrc(0, 0, 0);
    d3d::draw(PRIM_TRISTRIP, 0, (TRIANGLES_PER_CABLE + 4) * cables.size() - 2); // 4 degenerate triangle between cables
  }
}

void Cables::afterReset() { changed = true; }

void Cables::onRIExtraDestroyed(const TMatrix &tm, const BBox3 &box) { destroyedRIExtra.push_back({tm, box}); }

void Cables::destroyCables()
{
  G_ASSERTF_RETURN(!tiledArea.grid.empty() || cables.empty() || destroyedRIExtra.empty(), ,
    "tiledArea.grid.size()=%d cables.size()=%d destroyedRIExtra.size()=%d", //
    tiledArea.grid.size(), cables.size(), destroyedRIExtra.size());
  for (RIExtraInfo &riex : destroyedRIExtra)
  {
    mat44f riTm;
    bbox3f riBBox;
    v_mat44_make_from_43cu_unsafe(riTm, riex.tm.array);
    v_bbox3_init(riBBox, riTm, v_ldu_bbox3(riex.box));
    G_ASSERT_CONTINUE(!isnan(v_extract_x(riBBox.bmin)) && !isnan(v_extract_x(riBBox.bmax)) && !isnan(v_extract_z(riBBox.bmin)) &&
                      !isnan(v_extract_z(riBBox.bmax)));
    // inverse matrix usage is more accurate, but seems it works well as is
    // TMatrix invTm = inverse(riex.tm);
    int i_start = max<int>((v_extract_x(riBBox.bmin) - tiledArea.gridBoundMin.x) / tiledArea.tileSize.x, 0);
    int i_end = min<int>((v_extract_x(riBBox.bmax) - tiledArea.gridBoundMin.x) / tiledArea.tileSize.x + 1, tiledArea.gridSize.x);
    int j_start = max<int>((v_extract_z(riBBox.bmin) - tiledArea.gridBoundMin.y) / tiledArea.tileSize.y, 0);
    int j_end = min<int>((v_extract_z(riBBox.bmax) - tiledArea.gridBoundMin.y) / tiledArea.tileSize.y + 1, tiledArea.gridSize.y);
    for (int i = i_start; i < i_end; ++i)
      for (int j = j_start; j < j_end; ++j)
      {
        TiledArea::Tile &tile = tiledArea.getTile(i, j);
        for (int k = 0; k < tile.count; ++k)
        {
          CablePointInfo &info = tiledArea.cables_points[tile.start + k];
          float4 &point = info.cableEnd ? cables[info.cableIndex].point2_sag : cables[info.cableIndex].point1_rad;
          // Point3 localPoint = invTm % (Point3::xyz(point) - riex.tm.getcol(3));
          // if (riex.box & localPoint)
          if (v_bbox3_test_pt_inside(riBBox, v_ldu(&point.x)))
            setCable(info.cableIndex, Point3(0, 0, 0), Point3(0, 0, 0), 0, 0);
        }
      }
  }
  destroyedRIExtra.resize(0);
}

uint32_t Cables::getTranglesPerCable() const { return TRIANGLES_PER_CABLE; }

void reset_cables(bool)
{
  if (Cables *cableMgr = get_cables_mgr())
    cableMgr->afterReset();
}
REGISTER_D3D_AFTER_RESET_FUNC(reset_cables);