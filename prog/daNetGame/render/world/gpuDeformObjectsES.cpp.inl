// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_hlsl_floatx.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_math3d.h>
#include <math/dag_vecMathCompatibility.h>
#include "gpuDeformObjects.h"
#include "shaders/obstacleStruct.hlsli"
#include <gamePhys/phys/rendinstFloating.h>
#include <ecs/core/attributeEx.h>

enum
{
  MAX_DEFORMS = 256
};

template <typename Callable>
inline void process_deformables_ecs_query(Callable c);

void GpuDeformObjectsManager::setOrigin(const Point3 &p) { origin = p; }

void GpuDeformObjectsManager::fillEmpty()
{
  if (isEmpty())
    return;
  deformsCB.getBuf()->updateData(0, sizeof(Point4), ZERO_PTR<float>(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  setEmpty();
}

void GpuDeformObjectsManager::updateDeforms()
{
  TIME_PROFILE(update_deforms);
  if (!indices.getBuf())
    init(16, 16.f);

  G_STATIC_ASSERT(MAX_OBSTACLES <= (1 << (sizeof(uint8_t) * 8)));
  Tab<uint16_t> counters(framemem_ptr());
  int gridCounter = 0;
  counters.resize(width * width);
  mem_set_0(counters);
  struct GridCrd
  {
    uint8_t left, top, right, bottom;
  };
  carray<GridCrd, MAX_OBSTACLES> gridCrd;
  const float dist = float(width) * 0.5f * cellSize;
  Point2 indirection_lt;
  indirection_lt = Point2::xz(origin) - Point2(dist, dist);
  // box[1] = indirection_lt + Point2(totalDist, totalDist);
  ObstaclesData *deformsCBPtr = 0;
  if (!deformsCB.getBuf()->lock(0, sizeof(ObstaclesData), (void **)&deformsCBPtr, VBLOCK_WRITEONLY | VBLOCK_DISCARD) || !deformsCBPtr)
  {
    return;
  }

  deformsCBPtr->indirection_wd = width;
  deformsCBPtr->indirection_lt = indirection_lt;
  deformsCBPtr->indirection_cell = cellSize;
  Obstacle *obstacles = deformsCBPtr->obstacles;
  int usedObstaclesCount = 0;
  process_deformables_ecs_query([&](const TMatrix &transform, const Point3 &deform_bbox__bmin, const Point3 &deform_bbox__bmax,
                                  const TMatrix *transform_lastFrame = nullptr) {
    // Note: `transform_lastFrame` is copied on act, but not in all games. Do so to avoid colliding with ParallelUpdateFrameDelayed
    const TMatrix &tm = transform_lastFrame ? *transform_lastFrame : transform;
    if (usedObstaclesCount >= MAX_OBSTACLES || lengthSq(tm.getcol(3) - origin) > maxDistPreCheckSq)
      return;
    BBox3 box(deform_bbox__bmin, deform_bbox__bmax);
    BBox3 inflatedBox = box;
    Point3 pt = box.center() + orthonormalized_inverse(tm).getcol(1);
    inflatedBox += pt;
    inflatedBox[0] -= Point3(0.25, 0.25, 0.25);
    inflatedBox[1] += Point3(0.25, 0.25, 0.25);
    // intenipnally instead of scale, use fixed grass size. Regardless of a size of objet, grass should collide with soft offset
    BBox3 oBox = tm * inflatedBox;
    IPoint2 lt = ipoint2(floor((Point2::xz(oBox[0]) - indirection_lt - Point2(0.5, 0.5)) / cellSize));
    IPoint2 rb = ipoint2(floor((Point2::xz(oBox[1]) - indirection_lt + Point2(0.5, 0.5)) / cellSize)); // Point2(0.5,0.5) is grass size
    if (lt.x >= width || lt.y >= width || rb.x < 0 || rb.y < 0)
      return;

    lt.x = clamp(lt.x, (int)0, (int)width - 1);
    rb.x = clamp(rb.x, (int)0, (int)width - 1);
    lt.y = clamp(lt.y, (int)0, (int)width - 1);
    rb.y = clamp(rb.y, (int)0, (int)width - 1);
    for (int y = lt.y; y <= rb.y; ++y)
      for (int x = lt.x; x <= rb.x; ++x)
      {
        counters[x + y * width]++;
        gridCounter++;
      }

    gridCrd[usedObstaclesCount].left = lt.x;
    gridCrd[usedObstaclesCount].top = lt.y;
    gridCrd[usedObstaclesCount].right = rb.x;
    gridCrd[usedObstaclesCount].bottom = rb.y;
    Point3 boxCenter = tm * inflatedBox.center(), boxWidth = inflatedBox.width();
    obstacles[usedObstaclesCount].dir_x = Point4::xyzV(tm.getcol(0), boxCenter.x);
    obstacles[usedObstaclesCount].dir_y = Point4::xyzV(tm.getcol(1), boxCenter.y);
    obstacles[usedObstaclesCount].dir_z = Point4::xyzV(tm.getcol(2), boxCenter.z);
    obstacles[usedObstaclesCount].box = Point4::xyzV(boxWidth, 0);
    usedObstaclesCount++;
  });

  if (usedObstaclesCount == 0)
    memset(deformsCBPtr, 0, sizeof(*deformsCBPtr));

  deformsCB.getBuf()->unlock();

  if (usedObstaclesCount == 0)
  {
    setEmpty();
    return;
  }


  G_ASSERT(gridCounter < 65536); // sizeof(uint16_t)
  if (gridCounter > maxIndices)
  {
    logerr("reallocate obstacles array from %d to %d", maxIndices, gridCounter);
    initIndicesBuffer(gridCounter);
  }

  Tab<uint16_t> offsets(framemem_ptr());
  offsets.resize(counters.size() + 1);
  offsets[0] = 0;
  for (int i = 1; i < offsets.size(); ++i)
    offsets[i] = offsets[i - 1] + counters[i - 1];
  G_ASSERT(offsets.back() == gridCounter);
  mem_set_0(counters);
  Tab<uint32_t> indicesList(framemem_ptr());
  indicesList.resize(gridCounter);
  for (int i = 0; i < usedObstaclesCount; ++i)
  {
    for (int y = gridCrd[i].top; y <= gridCrd[i].bottom; ++y)
      for (int x = gridCrd[i].left; x <= gridCrd[i].right; ++x)
      {
        const int gridInd = x + y * width;
        indicesList[offsets[gridInd] + counters[gridInd]] = i;
        counters[gridInd]++;
      }
  }

  uint32_t *indicesPtr = 0;
  if (!indices.getBuf()->lock(0, 0, (void **)&indicesPtr, VBLOCK_WRITEONLY | VBLOCK_DISCARD) || !indicesPtr)
  {
    if (!d3d::is_in_device_reset_now())
      logerr("can't lock obstacles indices buffer");
    fillEmpty();
    return;
  }

  for (int i = 0; i < counters.size(); ++i)
  {
    indicesPtr[i] = ((offsets[i] + width * width) << OBSTACLE_OFFSET_BIT_SHIFT) | (counters[i]);
    G_ASSERT(counters[i] < (1 << OBSTACLE_OFFSET_BIT_SHIFT));
    G_ASSERT(offsets[i + 1] - offsets[i] == counters[i]);
  }
  memcpy(indicesPtr + width * width, indicesList.data(), data_size(indicesList));

  indices.getBuf()->unlock();
}

void GpuDeformObjectsManager::close()
{
  indices.close();
  deformsCB.close();
}

void GpuDeformObjectsManager::init(uint32_t wd, float cell)
{
  origin = Point3(0, 0, 0);
  width = wd;
  cellSize = cell;
  maxDistPreCheckSq = float(width * width) * (cellSize * cellSize) * 1.1f;
  initIndicesBuffer(width * width * 2);
  deformsCB = dag::buffers::create_persistent_cb(dag::buffers::cb_struct_reg_count<ObstaclesData>(), "obstacles_buf");
}

void GpuDeformObjectsManager::initIndicesBuffer(uint32_t max_indices)
{
  maxIndices = max_indices;
  // Like `create_persistent_sr_structured` but with SBCF_DYNAMIC for VBLOCK_DISCARD to work
  unsigned sbcf = SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED;
  indices = dag::create_sbuffer(sizeof(uint32_t), width * width + max_indices, sbcf, 0, "obstacle_indices_buf");
}
