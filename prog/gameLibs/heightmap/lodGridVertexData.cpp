// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lodGridVertexDataPool.h"
#include <osApiWrappers/dag_spinlock.h>
#include <heightmap/lodGrid.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaders.h>

LodGridVertexData lod_grid_vdata[MAX_VDATA];
// Guards the pool's refCnt and index buffer lifetime: renderers are created/closed from loading
// threads while the render thread may rebuild buffers after a device reset.
static OSSpinlock vdata_cs;

template <class It>
static inline int generate_patch_indices_quads(int dim, It &&indices)
{
  size_t index = 0;
  for (int y = 0; y < dim; ++y)
    for (int x = 0; x < dim; ++x)
    {
      const int topleft = y * (dim + 1) + x;
      const int downleft = topleft + (dim + 1);
      const int downright = topleft + (dim + 1) + 1;
      const int topright = topleft + 1;
      indices[index + 0] = topleft;
      indices[index + 1] = downleft;
      indices[index + 2] = downright;
      indices[index + 3] = topright;
      index += 4;
    }
  return index;
}

void LodGridVertexData::close()
{
  OSSpinlockScopedLock lock(vdata_cs);
  if (!refCnt)
    return;
  if (--refCnt > 0)
    return;
  del_d3dres(ib);
  del_d3dres(quadsIb);
  patchDim = 0;
}

bool LodGridVertexData::init(int dim)
{
  // Creation must finish before a second user can return from init(): the old
  // interlocked-only check let another thread proceed to render with buffers
  // still being generated.
  OSSpinlockScopedLock lock(vdata_cs);
  if (refCnt++ > 0)
  {
    G_ASSERT(patchDim == dim);
    return true;
  }
  patchDim = dim;

  return createBuffers();
}

bool LodGridVertexData::createBuffers()
{
  del_d3dres(ib);
  del_d3dres(quadsIb);

  recreateBuffers = false;
  const int indexSize = 2;
  int indicesCnt = patchDim * patchDim * 6;
  int totalIndicesCnt = indicesCnt * 4;
  G_ASSERT(!ib);
  ib = d3d::create_ib(totalIndicesCnt * indexSize, (indexSize == 4 ? SBCF_INDEX32 : 0), "lod_grid_vdata_ib", RESTAG_LAND);
  d3d_err(ib);
  if (!ib)
    return false;
  if (auto lockedIndices = lock_sbuffer<uint16_t>(ib, 0, 0, VBLOCK_WRITEONLY))
  {
    generate_patch_indices(patchDim, lockedIndices, 0);
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt), 1);
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt * 2), 1, 0); // REGULAR_RTLB
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt * 3), 0, 0); // REGULAR_LTRB
  }
  else
  {
    recreateBuffers = true;
    logwarn("heightmap lock failed, reset device?");
    return true;
  }

  if (d3d::get_driver_desc().caps.hasQuadTessellation)
  {
    int quadsIndicesCnt = indicesCnt / 6 * 4;
    G_ASSERT(!quadsIb);
    quadsIb = d3d::create_ib(quadsIndicesCnt * indexSize, (indexSize == 4 ? SBCF_INDEX32 : 0), "lod_grid_vdata_quadsIb", RESTAG_LAND);
    d3d_err(quadsIb);
    if (!quadsIb)
      return false;
    if (auto lockedIndices = lock_sbuffer<uint16_t>(quadsIb, 0, 0, VBLOCK_WRITEONLY))
      generate_patch_indices_quads(patchDim, lockedIndices);
    else
    {
      recreateBuffers = true;
      logwarn("heightmap lock failed, reset device?");
      return true;
    }
  }

  debug("heightmap will be rendered using instanceId instancing");
  ShaderGlobal::set_int(get_shader_variable_id("heightmap_use_instancing", true), 1);
  return true;
}

void LodGridVertexData::beforeResetDevice()
{
  OSSpinlockScopedLock lock(vdata_cs);
  if (patchDim <= 0)
    return;

  recreateBuffers = true;
  del_d3dres(ib);
  del_d3dres(quadsIb);
}

void LodGridVertexData::afterResetDevice()
{
  OSSpinlockScopedLock lock(vdata_cs);
  if (recreateBuffers)
    createBuffers();
}

void lod_grid_vdata_before_reset_device()
{
  for (int i = 0; i < MAX_VDATA; ++i)
    lod_grid_vdata[i].beforeResetDevice();
}

void lod_grid_vdata_after_reset_device()
{
  for (int i = 0; i < MAX_VDATA; ++i)
    lod_grid_vdata[i].afterResetDevice();
}
