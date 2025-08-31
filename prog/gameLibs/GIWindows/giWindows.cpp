// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <memory/dag_framemem.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <daGI/daGI.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
// #include "global_vars.h"
#include <scene/dag_tiledScene.h>
#include "shaders/gi_windows.hlsli"
#include <GIWindows/GIWindows.h>

#define GLOBAL_VARS_OPTIONAL_LIST \
  VAR(windowsBoxLt)               \
  VAR(windowsBoxRb)               \
  VAR(windowsBoxRange)            \
  VAR(windowsGridLT)              \
  VAR(windowsGridInv)             \
  VAR(current_windows_count)

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR

void GIWindows::updatePos(const Point3 &pos_)
{
  if (!windows)
    return;
  const float cellSize = dist / (WINDOW_GRID_XZ / 2);
  const Point3 pos = floor(pos_ / cellSize) * cellSize;
  if (lengthSq(centerPos - pos) < dist * dist * 0.01f) // up to 10%
    return validate();
  TIME_PROFILE(update_windows);
  centerPos = pos;
  bbox3f box;
  box.bmin = v_ldu(&centerPos.x);
  box.bmax = v_add(box.bmin, v_splats(dist));
  box.bmin = v_sub(box.bmin, v_splats(dist));
  activeListBox.resize(0);
  activeList.resize(0);
  windows->boxCull<false, true>(box, 0, 0, [&](scene::node_index ni, mat44f_cref nm_) {
    mat44f nm;
    v_mat44_inverse43(nm, nm_);
    nm.col3 = nm_.col3;
    mat43f m;
    v_mat44_transpose_to_mat43(m, nm);
    activeList.push_back(m);
    activeListBox.push_back(windows->getBaseSimpleScene().calcNodeBox(ni)); // it is safe to call getBaseSimpleScene() here, as we are
                                                                            // inside readlock
  });
  // debug("cWindows.size() = %d", cur.size());
  // if (activeList.size() == cur.size() && memcmp(activeList.data(), cur.data(), cur.size()*sizeof(mat43f)) == 0)
  //   return validate();
  if (currentCount == 0 && activeList.size() == 0)
    return;
  currentCount = -1;
  if (activeList.size() && bufferCount < activeList.size())
  {
    bufferCount = max((int)256, max((int)activeList.size(), (int)bufferCount * 2));

    currentWindowsSB.close();
    currentWindowsSB =
      UniqueBufHolder(dag::buffers::create_persistent_sr_structured(sizeof(Window), bufferCount, "currentWindowsList"), "windows");
    currentWindowsSB.setVar();
  }
  return validate();
}

bool GIWindows::calc()
{
  if (!activeList.size())
  {
    d3d::zero_rwbufi(currentWindowsGridSB.getBuf());
    d3d::resource_barrier({currentWindowsGridSB.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    return true;
  }

  TIME_D3D_PROFILE(windows_clustered_grid_cs);
  const IPoint3 res(WINDOW_GRID_XZ, WINDOW_GRID_Y, WINDOW_GRID_XZ);
  const int subDivX = 2, subDivZ = 2;
  const float cellSize = dist / (WINDOW_GRID_XZ / 2);
  const Point3_vec4 lt(centerPos - point3(res / 2) * cellSize);
  const IPoint3 subRes((res.x + subDivX - 1) / subDivX, res.y, (res.z + subDivZ - 1) / subDivZ);
  const Point3_vec4 wd(point3(subRes) * cellSize);
  eastl::vector<uint32_t, framemem_allocator> cBox;
  cBox.reserve(activeListBox.size() * 3 / 2);
  vec4f ltV = v_ld(&lt.x), wdV = v_ld(&wd.x);
  struct SubData
  {
    Point3_vec4 box[2];
    uint32_t first, count;
  };
  eastl::vector<SubData, framemem_allocator> fullList(subDivX * subDivZ);
  for (int z = 0, ci = 0; z < subDivZ; ++z)
    for (int x = 0; x < subDivX; ++x, ci++)
    {
      SubData &cData = fullList[ci];
      cData.first = cBox.size();
      bbox3f subBox;
      subBox.bmin = v_madd(v_make_vec4f(x, 0, z, 0), wdV, ltV);
      subBox.bmax = v_add(subBox.bmin, wdV);
      bbox3f wholeBox;
      v_bbox3_init_empty(wholeBox);
      vec4f maxSize = v_zero();
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
      cData.box[1].resv = v_extract_x(v_length3(maxSize)) * 0.5;
    }

  if (currentIndSize < cBox.size() || !currentWindowsSBInd.getBuf())
  {
    currentIndSize = max((int)256, max((int)cBox.size(), (int)currentIndSize * 2));
    currentWindowsSBInd.close();
    currentWindowsSBInd = UniqueBufHolder(
      dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), currentIndSize, "currentWindowsListInd"), "windows_grid_ind");
    currentWindowsSBInd.setVar();
  }

  if (cBox.empty() ||
      !currentWindowsSBInd.getBuf()->updateDataWithLock(0, cBox.size() * sizeof(uint32_t), cBox.data(), VBLOCK_WRITEONLY))
  {
    d3d::zero_rwbufi(currentWindowsGridSB.getBuf());
    d3d::resource_barrier({currentWindowsGridSB.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    return false;
  }

  d3d::zero_rwbufi(gridCntSB.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 0, currentWindowsGridSB.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 1, gridCntSB.getBuf());
  auto fun = [&](auto Condition, auto &shader, int sx, int sy, int sz) {
    for (int ci = 0, z = 0; z < subDivZ; ++z)
      for (int x = 0; x < subDivX; ++x, ++ci)
      {
        const SubData &cData = fullList[ci];
        if (!Condition(cData.count))
          continue;
        d3d::resource_barrier({currentWindowsGridSB.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        d3d::resource_barrier({gridCntSB.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        ShaderGlobal::set_color4(windowsBoxLtVarId, cData.box[0].x, cData.box[0].y, cData.box[0].z, 0);
        ShaderGlobal::set_color4(windowsBoxRbVarId, cData.box[1].x, cData.box[1].y, cData.box[1].z, cData.box[1].resv);
        ShaderGlobal::set_color4(windowsBoxRangeVarId, x * subRes.x, z * subRes.z, cData.first, cData.count);
        shader->dispatch((subRes.x + sx - 1) / sx, (subRes.y + sy - 1) / sy, (subRes.z + sz - 1) / sz);
      }
  };
  G_ASSERT(subRes.x % 2 == subRes.z % 2 && subRes.x % 2 == 0);
  fun([](uint cnt) -> auto { return cnt == 0; }, clear_windows_grid_range, 4, 4, 4);
  fun([](uint cnt) -> auto { return cnt > 0 && cnt <= 16; }, fill_windows_grid_range_fast, 2, 1, 2);
  fun([](uint cnt) -> auto { return cnt > 16; }, fill_windows_grid_range, 2, 1, 2);
  // full:
  d3d::set_rwbuffer(STAGE_CS, 0, 0);
  d3d::set_rwbuffer(STAGE_CS, 1, 0);
  d3d::resource_barrier({currentWindowsGridSB.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  /*uint *data;
  if (gridCntSB->lock(0, 0, (void**)&data, VBLOCK_READONLY))
  {
    debug("gridCntSB = %d", *data);
    gridCntSB->unlock();
  }*/
  return true;
}

void GIWindows::validate()
{
  if (currentCount == activeList.size())
  {
    // calc();
    return;
  }
  if (activeList.size() &&
      currentWindowsSB.getBuf()->updateDataWithLock(0, activeList.size() * sizeof(Window), activeList.data(), VBLOCK_WRITEONLY))
  {
    const float cellSize = dist / (WINDOW_GRID_XZ / 2);
    const IPoint3 res(WINDOW_GRID_XZ, WINDOW_GRID_Y, WINDOW_GRID_XZ);
    Point4 windowsGridLT = Point4::xyzV(centerPos - point3(res / 2) * cellSize, cellSize);
    Point4 windowsGridInv = Point4(-windowsGridLT.x, -windowsGridLT.y, -windowsGridLT.z, 1.f) / windowsGridLT.w;

    ShaderGlobal::set_int(current_windows_countVarId, activeList.size());
    ShaderGlobal::set_color4(windowsGridLTVarId, windowsGridLT.x, windowsGridLT.y, windowsGridLT.z, windowsGridLT.w);
    ShaderGlobal::set_color4(windowsGridInvVarId, windowsGridInv.x, windowsGridInv.y, windowsGridInv.z, windowsGridInv.w);

    currentCount = calc() ? activeList.size() : 0;
  }
  else
  {
    calc(); // clear grid
    currentCount = 0;
  }

  if (!currentCount) // explicitly empty
  {
    ShaderGlobal::set_int(current_windows_countVarId, 0);
    const Point4 windowsGridLT(-10000, -100000, -10000, 1), windowsGridInv(-1000, -1000, -1000, 0);
    ShaderGlobal::set_color4(windowsGridLTVarId, windowsGridLT.x, windowsGridLT.y, windowsGridLT.z, windowsGridLT.w);
    ShaderGlobal::set_color4(windowsGridInvVarId, windowsGridInv.x, windowsGridInv.y, windowsGridInv.z, windowsGridInv.w);
  }
}

void GIWindows::init(eastl::unique_ptr<class scene::TiledScene> &&s)
{
  eastl::swap(windows, s);
  centerPos = Point3(-10000, 1000, -10000);
  currentCount = 0;
  if (!windows)
    return;
  windows->rearrange(128.f); // todo: may be better tile heuresitcs
  if (!gridCntSB)
    gridCntSB = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 1, "gridCntSB");
  currentGridSize = WINDOW_GRID_XZ * WINDOW_GRID_Y * WINDOW_GRID_XZ * 2;
  if (!currentWindowsGridSB.getBuf())
    currentWindowsGridSB =
      UniqueBufHolder(dag::buffers::create_ua_sr_structured(sizeof(uint32_t), currentGridSize, "currentWindowsGrid"), "windowsGrid");
  currentWindowsGridSB.setVar();
  fill_windows_grid_range.reset(new_compute_shader("fill_windows_grid_range_cs"));
  fill_windows_grid_range_fast.reset(new_compute_shader("fill_windows_grid_range_fast_cs"));
  clear_windows_grid_range.reset(new_compute_shader("clear_windows_grid_range_cs"));
}

GIWindows::~GIWindows()
{
  ShaderGlobal::set_color4(windowsGridLTVarId, -10000, -10000, -10000, 1);
  ShaderGlobal::set_int(current_windows_countVarId, 0);
}