// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <daGI/daGI.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include "global_vars.h"
#include <scene/dag_tiledScene.h>
#include <EASTL/sort.h>
#include "shaders/gi_walls.hlsli"

namespace dagi
{

class GIWalls
{
public:
  const int max_visible_nodes = 8; // 4?
  void init(eastl::unique_ptr<class scene::TiledScene> &&s);
  void validate();
  void setVisibleWalls(mat44f_cref globtm, vec4f pos, Occlusion *occlusion);
  bool isValid() const { return currentCount == activeList.size(); }
  void invalidate() { centerPos += Point3(10000, -1000, 10000); }
  void updatePos(const Point3 &pos);
  eastl::unique_ptr<class scene::TiledScene> walls;
  Point3 centerPos = {-10000, 1000, -10000};
  float dist = 128.f;
  int bufferCount = 0, currentCount = 0, currentGridSize = 0, currentIndSize = 0, planeCount = 0;
  union Wall
  {
    struct
    {
      uint32_t at : 24;
      uint32_t cnt : 8;
    };
    uint32_t u;
    Wall(uint32_t a = 0) : u(a) {}
  };
  eastl::vector<Wall> activeList;
  eastl::vector<vec4f> planeList;
  eastl::vector<bbox3f> activeListBox;
  UniqueBufHolder currentWallsSB, currentWallsGridSB, currentWallsConvexCB;
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> frameWallsCB;
  // eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> gridCntSB;
  eastl::unique_ptr<ComputeShaderElement> fill_walls_grid_range;
  // eastl::unique_ptr<ComputeShaderElement> clear_walls_grid_range;
  bool calc();
};

void GI3D::GIWallsDestroyer::operator()(GIWalls *ptr) { del_it(ptr); }

void GIWalls::updatePos(const Point3 &pos_)
{
  if (!walls)
    return;
  const float cellSizeXZ = dist / (WALLS_GRID_XZ / 2), cellSizeY = dist / (WALLS_GRID_Y / 2);
  Point3 cellSize(cellSizeXZ, cellSizeY, cellSizeXZ);
  const Point3 pos = mul(floor(div(pos_, cellSize)), cellSize);
  if (lengthSq(centerPos - pos) < dist * dist * 0.01f) // up to 10%
    return validate();
  TIME_PROFILE(update_walls);
  centerPos = pos;
  bbox3f box;
  box.bmin = v_ldu(&centerPos.x);
  box.bmax = v_add(box.bmin, v_splats(dist));
  box.bmin = v_sub(box.bmin, v_splats(dist));
  activeList.resize(0);
  planeList.resize(0);
  activeListBox.resize(0);

  walls->boxCull<false, true>(box, 0, 0, [&](scene::node_index ni, mat44f_cref nm) {
    if (planeList.size() + 6 > 4096)
      return;
    Wall wall;
    wall.at = planeList.size();
    wall.cnt = 6;
    activeList.push_back(wall);
    auto addPlane = [&](vec3f col, vec3f c) {
      planeList.push_back(v_norm3(v_perm_xyzd(col, v_neg(v_dot3(col, v_madd(V_C_HALF, col, c))))));
      col = v_neg(col);
      planeList.push_back(v_norm3(v_perm_xyzd(col, v_neg(v_dot3(col, v_madd(V_C_HALF, col, c))))));
    };
    addPlane(nm.col0, nm.col3);
    addPlane(nm.col1, nm.col3);
    addPlane(nm.col2, nm.col3);
    activeListBox.push_back(walls->getBaseSimpleScene().calcNodeBox(ni)); // it is safe to call getBaseSimpleScene() here, as we are
                                                                          // inside readlock
  });
  // debug("cWalls.size() = %d (%d planes)", activeList.size(), planeList.size());
  // if (activeList.size() == cur.size() && memcmp(activeList.data(), cur.data(), cur.size()*sizeof(mat43f)) == 0)
  //   return validate();
  currentCount = -1;
  if (bufferCount < activeList.size())
  {
    bufferCount = max((int)256, max((int)activeList.size(), (int)bufferCount * 2));

    currentWallsSB.close();
    currentWallsSB =
      UniqueBufHolder(dag::buffers::create_persistent_sr_structured(sizeof(Wall), bufferCount, "currentWallsList"), "wallsList");
    currentWallsSB.setVar();
  }
  if (planeCount < planeList.size())
  {
    planeCount = d3d::get_driver_code()
                   .map(d3d::metal, 4096)
                   .map(d3d::any, min<int>(4096, max<int>(256, max<int>(planeList.size(), planeCount * 2))));
    G_ASSERT(planeList.size() < 4096);

    currentWallsConvexCB.close();
    currentWallsConvexCB = UniqueBufHolder(dag::buffers::create_persistent_cb(planeCount, "currentWallsListVB"), "PlanesCbuffer");
    currentWallsConvexCB.setVar();
  }
  return validate();
}

bool GIWalls::calc()
{
  const IPoint3 res(WALLS_GRID_XZ, WALLS_GRID_Y, WALLS_GRID_XZ);
  if (!planeList.size())
  {
    TIME_D3D_PROFILE(walls_clear_cs);
    uint v[4] = {0, 0, 0, 0};
    d3d::clear_rwbufi(currentWallsGridSB.getBuf(), v);
    d3d::resource_barrier({currentWallsGridSB.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    return true;
  }
  TIME_D3D_PROFILE(walls_clustered_grid_cs);
  static int grid_uav_no = ShaderGlobal::get_slot_by_name("fill_walls_grid_range_cs_grid_uav_no");
  d3d::set_rwbuffer(STAGE_CS, grid_uav_no, currentWallsGridSB.getBuf());
  fill_walls_grid_range->dispatchThreads(res.x, res.y, res.z);
  d3d::set_rwbuffer(STAGE_CS, grid_uav_no, NULL);
  d3d::resource_barrier({currentWallsGridSB.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  /*const int subDivX = 2, subDivZ = 2;
  const float cellSize = dist/(WALLS_GRID_XZ/2);
  const Point3_vec4 lt(centerPos - point3(res/2)*cellSize);
  const IPoint3 subRes((res.x+subDivX-1)/subDivX, res.y, (res.z+subDivZ-1)/subDivZ);
  const Point3_vec4 wd(point3(subRes)*cellSize);
  eastl::vector<uint32_t, framemem_allocator> cBox;
  cBox.reserve(activeListBox.size()*3/2);
  vec4f ltV = v_ld(&lt.x), wdV = v_ld(&wd.x);
  struct SubData
  {
    Point3_vec4 box[2];
    uint32_t first, count;
  };
  eastl::vector<SubData, framemem_allocator> fullList(subDivX*subDivZ);
  for (int z = 0, ci = 0; z < subDivZ; ++z)
    for (int x = 0; x < subDivX; ++x, ci++)
    {
      SubData &cData = fullList[ci];
      cData.first = cBox.size();
      bbox3f subBox;subBox.bmin = v_madd(v_make_vec4f(x, 0, z, 0), wdV, ltV);
      subBox.bmax = v_add(subBox.bmin, wdV);
      bbox3f wholeBox;v_bbox3_init_empty(wholeBox);vec4f maxSize = v_zero();
      for (int i = 0, ei = activeListBox.size(); i < ei; ++i)
      {
        bbox3f box = activeListBox[i];
        if (!v_bbox3_test_box_intersect(box, subBox))
          continue;
        v_bbox3_add_box(wholeBox, box);
        maxSize = v_max(maxSize, v_sub(box.bmax, box.bmin));
        cBox.push_back(i);
      }
      cData.count = cBox.size() - cData.first;
      v_st(&cData.box[0].x, wholeBox.bmin);
      v_st(&cData.box[1].x, wholeBox.bmax);
      cData.box[1].resv = v_extract_x(v_length3(maxSize))*0.5;
    }

  if (currentIndSize < cBox.size())
  {
    currentIndSize = max((int)256, max((int)cBox.size(), (int)currentIndSize*2));
    currentWallsSBInd.reset();
    currentWallsSBInd.reset(
      d3d::create_sbuffer(sizeof(uint32_t), currentIndSize,
        SBCF_DYNAMIC|SBCF_CPU_ACCESS_WRITE|SBCF_BIND_SHADER_RES|SBCF_MISC_STRUCTURED,
        0,
        "currentWallsListInd"));
  }
  if (currentIndSize &&
      !currentWallsSBInd->updateDataWithLock(0, cBox.size()*sizeof(uint), cBox.data(), VBLOCK_WRITEONLY|VBLOCK_DISCARD))
    return;

  d3d::set_buffer(STAGE_CS, 17, currentWallsSBInd.get());
  d3d::set_buffer(STAGE_CS, 16, currentWallsSB.get());
  d3d::set_const_buffer(STAGE_CS, 5, currentWallsConvexCB.get());//if we set to 4 slot it causes driver hung
  d3d::set_rwbuffer(STAGE_CS, 0, currentWallsGridSB.get(), 0);
  d3d::set_rwbuffer(STAGE_CS, 1, gridCntSB.get(), 0);
  auto fun = [&](auto Condition, auto &shader, int sx, int sy, int sz)
  {
    for (int ci = 0, z = 0; z < subDivZ; ++z)
      for (int x = 0; x < subDivX; ++x, ++ci)
      {
        const SubData &cData = fullList[ci];
        if (!Condition(cData.count))
          continue;
        ShaderGlobal::set_color4(wallsBoxLtVarId, cData.box[0].x, cData.box[0].y, cData.box[0].z, 0);
        ShaderGlobal::set_color4(wallsBoxRbVarId, cData.box[1].x, cData.box[1].y, cData.box[1].z, cData.box[1].resv);
        ShaderGlobal::set_color4(wallsBoxRangeVarId, x*subRes.x, z*subRes.z, cData.first, cData.count);
        shader->dispatch((subRes.x+sx-1)/sx, (subRes.y+sy-1)/sy, (subRes.z+sz-1)/sz);
      }
  };
  G_ASSERT(subRes.x%2 == subRes.z%2 && subRes.x%2 == 0);
  fun( [](uint cnt)->auto {return cnt==0;}, clear_walls_grid_range, 4, 4, 4);
  fun( [](uint cnt)->auto {return cnt>0;}, fill_walls_grid_range, 2, 1, 2);
  d3d::set_buffer(STAGE_CS, 17, 0);
  d3d::set_buffer(STAGE_CS, 16, 0);
  d3d::set_rwbuffer(STAGE_CS, 0, 0, 0);
  d3d::set_rwbuffer(STAGE_CS, 1, 0, 0);
  d3d::set_const_buffer(STAGE_CS, 5, NULL);//if we set to 4 slot it causes driver hung
  uint *data;
  if (gridCntSB->lock(0, 0, (void**)&data, VBLOCK_READONLY))
  {
    debug("gridCntSB = %d", *data);
    gridCntSB->unlock();
  }
  */
  return true;
}

void GIWalls::validate()
{
  if (currentCount == activeList.size())
  {
    // calc();
    return;
  }
  G_ASSERT(bufferCount >= activeList.size());
  if (!activeList.empty())
  {
    if (!currentWallsSB.getBuf()->updateDataWithLock(0, activeList.size() * sizeof(Wall), activeList.data(), VBLOCK_WRITEONLY))
      return;
  }
  if (planeList.size())
  {
    G_ASSERT(planeCount >= planeList.size());
    if (!currentWallsConvexCB.getBuf()->updateDataWithLock(0, planeList.size() * sizeof(plane3f), planeList.data(),
          VBLOCK_WRITEONLY | VBLOCK_DISCARD))
      return;
  }
  const float cellSizeXZ = dist / (WALLS_GRID_XZ / 2), cellSizeY = dist / (WALLS_GRID_Y / 2);
  const Point3 res(WALLS_GRID_XZ >> 1, WALLS_GRID_Y >> 1, WALLS_GRID_XZ >> 1);
  Point3 cellSize(cellSizeXZ, cellSizeY, cellSizeXZ);
  Point3 wallsGridLT(centerPos - mul(res, cellSize));
  ShaderGlobal::set_color4(wallsGridLTVarId, wallsGridLT.x, wallsGridLT.y, wallsGridLT.z, 0.f);
  ShaderGlobal::set_color4(wallsGridCellSizeVarId, cellSizeXZ, cellSizeY);
  ShaderGlobal::set_int(current_walls_countVarId, activeList.size());
  if (calc())
    currentCount = activeList.size();
  else
    ShaderGlobal::set_int(current_walls_countVarId, 0);
}

void GIWalls::setVisibleWalls(mat44f_cref globtm, vec4f pos, Occlusion *occlusion)
{
  vec4f posDistScale = v_perm_xyzd(pos, v_splats(1.f)); // fixme: use some dist heurestics
  eastl::vector<mat44f, framemem_allocator> nodes;
  if (occlusion)
    walls->frustumCull<false, true, true>(globtm, posDistScale, 0, 0, occlusion, [&](scene::node_index, mat44f_cref nm_, vec4f d) {
      mat44f nm = nm_;
      nm.col3 = v_perm_xyzd(nm.col3, d);
      nodes.push_back(nm);
    });
  else
    walls->frustumCull<false, true, false>(globtm, posDistScale, 0, 0, NULL, [&](scene::node_index, mat44f_cref nm_, vec4f d) {
      mat44f nm = nm_;
      nm.col3 = v_perm_xyzd(nm.col3, d);
      nodes.push_back(nm);
    });

  if (nodes.size() > max_visible_nodes)
  {
    // sort and select best <= 8
    eastl::nth_element(nodes.begin(), nodes.begin() + max_visible_nodes, nodes.end(),
      [](mat44f_cref a, mat44f_cref b) -> bool { return v_test_vec_x_lt(v_splat_w(a.col3), v_splat_w(b.col3)); });
    nodes.resize(max_visible_nodes);
  }

  eastl::vector<plane3f> framePlaneList;
  framePlaneList.resize(0);
  for (int i = 0, ie = nodes.size(); i < ie; ++i)
  {
    mat44f nm = nodes[i];
    auto addPlane = [&](vec3f col, vec3f c) {
      framePlaneList.push_back(v_norm3(v_perm_xyzd(col, v_neg(v_dot3(col, v_madd(V_C_HALF, col, c))))));
      col = v_neg(col);
      framePlaneList.push_back(v_norm3(v_perm_xyzd(col, v_neg(v_dot3(col, v_madd(V_C_HALF, col, c))))));
    };
    addPlane(nm.col0, nm.col3);
    addPlane(nm.col1, nm.col3);
    addPlane(nm.col2, nm.col3);
  }

  if (!frameWallsCB->updateDataWithLock(0, framePlaneList.size() * sizeof(plane3f), framePlaneList.data(),
        VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    return;
  // set to cb
}

void GIWalls::init(eastl::unique_ptr<class scene::TiledScene> &&s)
{
  eastl::swap(walls, s);
  if (walls)
  {
    walls->rearrange(128.f); // todo: may be better tile heuresitcs
    currentGridSize = WALLS_GRID_XZ * WALLS_GRID_Y * WALLS_GRID_XZ;
    if (!currentWallsGridSB.getBuf())
    {
      currentWallsGridSB =
        UniqueBufHolder(dag::buffers::create_ua_sr_structured(sizeof(WallGridType), currentGridSize, // sizeof(plane3f)*6
                          "currentWallsGrid"),
          "wallsGridList");
      currentWallsGridSB.setVar();
    }
    if (!frameWallsCB)
      frameWallsCB.reset(d3d::buffers::create_persistent_cb(max_visible_nodes * 6, "currentWallsListVB")); // fixme: auto
                                                                                                           // resize

    // fill_walls_grid.reset(new_compute_shader("fill_walls_grid_cs"));
    fill_walls_grid_range.reset(new_compute_shader("fill_walls_grid_range_cs"));
    // fill_walls_grid_range_fast.reset(new_compute_shader("fill_walls_grid_range_fast_cs"));
    // clear_walls_grid_range.reset(new_compute_shader("clear_walls_grid_range_cs"));
  }
  centerPos = Point3(-10000, 1000, -10000);
  currentCount = 0;
}

void GI3D::initWalls(eastl::unique_ptr<class scene::TiledScene> &&s)
{
  if (!s && cWalls)
    cWalls.reset();
  if (!s)
  {
    ShaderGlobal::set_color4(wallsGridLTVarId, -10000, -10000, -10000, 1);
    ShaderGlobal::set_int(current_walls_countVarId, 0);
    return;
  }
  if (!cWalls)
    cWalls.reset(new GIWalls);
  cWalls->init(eastl::move(s));
}

void GI3D::updateWallsPos(const Point3 &pos)
{
  if (cWalls)
    cWalls->updatePos(pos);
}

/*void GI3D::setVisibleWalls(mat44f_cref globtm, vec4f pos, Occlusion *occlusion)
{
  if (cWalls)
    cWalls->setVisibleWalls(globtm, pos, occlusion);
}*/

void GI3D::invalidateWalls()
{
  if (cWalls)
    cWalls->invalidate();
}

eastl::unique_ptr<scene::TiledScene> &GI3D::getWallsScene() const
{
  static eastl::unique_ptr<scene::TiledScene> empty;
  return cWalls ? cWalls->walls : empty;
}

}; // namespace dagi
