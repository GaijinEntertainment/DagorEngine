// Copyright (C) Gaijin Games KFT.  All rights reserved.

// draw commands implementation
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_draw.h>
#include <perfMon/dag_graphStat.h>
#include "vulkan_api.h"
#include "globals.h"
#include "frontend.h"
#include "global_lock.h"
#include "global_const_buffer.h"
#include "translate_d3d_to_vk.h"
#include "device_context.h"
#include "buffer.h"
#include "frontend_pod_state.h"

using namespace drv3d_vulkan;

namespace
{

// stolen from dx11 backend
static inline uint32_t nprim_to_nverts(uint32_t prim_type, uint32_t numprim)
{
  // table look-up: 4 bits per entry [2b mul 2bit add]
  static const uint64_t table = (0x0ULL << (4 * PRIM_POINTLIST))          //*1+0 00/00
                                | (0x4ULL << (4 * PRIM_LINELIST))         //*2+0 01/00
                                | (0x1ULL << (4 * PRIM_LINESTRIP))        //*1+1 00/01
                                | (0x8ULL << (4 * PRIM_TRILIST))          //*3+0 10/00
                                | (0x2ULL << (4 * PRIM_TRISTRIP))         //*1+2 00/10
                                | (0x2ULL << (4 * PRIM_TRIFAN))           //*1+2 00/10
                                | (0xcULL << (4 * PRIM_4_CONTROL_POINTS)) //*4+0 11/00
    ;

  const uint32_t code = uint32_t((table >> (prim_type * 4)) & 0x0f);
  return numprim * ((code >> 2) + 1) + (code & 3);
}

static void update_draw_stats(int prims, int instances)
{
  Stat3D::updateDrawPrim();
  if (prims)
    Stat3D::updateTriangles(prims * instances);
  if (instances > 1)
    Stat3D::updateInstances(instances);
  ++Frontend::State::pod.drawsCount;
}

static VkPrimitiveTopology before_draw(int type)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  Frontend::GCB.flushGraphics(Globals::ctx);
  return translate_primitive_topology_to_vulkan(type);
}

} // anonymous namespace

bool d3d::draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  VkPrimitiveTopology topology = before_draw(type);
  uint32_t nverts = nprim_to_nverts(type, numprim);

  CmdDraw cmd{topology, (uint32_t)start, nverts, start_instance, num_instances};
  Globals::ctx.dispatchPipeline(cmd, "draw_base");

  update_draw_stats(numprim, num_instances);
  return true;
}

bool d3d::drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  G_ASSERT(num_instances > 0);
  VkPrimitiveTopology topology = before_draw(type);
  uint32_t nverts = nprim_to_nverts(type, numprim);

  CmdDrawIndexed cmd{topology, (uint32_t)startind, nverts, max(base_vertex, 0), start_instance, num_instances};
  Globals::ctx.dispatchPipeline(cmd, "drawind_base");

  update_draw_stats(numprim, num_instances);
  return true;
}

bool d3d::draw_up(int type, int numprim, const void *ptr, int stride_bytes)
{
  VkPrimitiveTopology topology = before_draw(type);
  uint32_t nverts = nprim_to_nverts(type, numprim);

  CmdDrawUserData cmd{topology, nverts, (uint32_t)stride_bytes, Globals::ctx.uploadToDeviceFrameMem(nverts * stride_bytes, ptr)};
  Globals::ctx.dispatchPipeline(cmd, "draw_up");

  update_draw_stats(numprim, 1);
  return true;
}

bool d3d::drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  G_UNUSED(minvert);
  VkPrimitiveTopology topology = before_draw(type);
  uint32_t nverts = nprim_to_nverts(type, numprim);

  CmdDrawIndexedUserData cmd{topology, nverts, (uint32_t)stride_bytes,
    Globals::ctx.uploadToDeviceFrameMem(numvert * stride_bytes, ptr),
    Globals::ctx.uploadToDeviceFrameMem(nverts * sizeof(uint16_t), ind)};
  Globals::ctx.dispatchPipeline(cmd, "drawind_up");

  update_draw_stats(numprim, 1);
  return true;
}

bool d3d::draw_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  return d3d::multi_draw_indirect(prim_type, args, 1, sizeof(VkDrawIndirectCommand), byte_offset);
}

bool d3d::draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  return d3d::multi_draw_indexed_indirect(prim_type, args, 1, sizeof(VkDrawIndexedIndirectCommand), byte_offset);
}

bool d3d::multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_ASSERTF(args != nullptr, "multi_draw_indirect with nullptr buffer is invalid");
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "multi_draw_indirect buffer is not usable as indirect buffer");
  VkPrimitiveTopology topology = before_draw(prim_type);
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;

  CmdDrawIndirect cmd{topology, draw_count, buffer->getBufferRef(), byte_offset, stride_bytes};
  Globals::ctx.dispatchPipeline(cmd, "multi_draw_indirect");

  update_draw_stats(0, 0);
  return true;
}

bool d3d::multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_ASSERTF(args != nullptr, "multi_draw_indexed_indirect with nullptr buffer is invalid");
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "multi_draw_indexed_indirect buffer is not usable as indirect buffer");
  VkPrimitiveTopology topology = before_draw(prim_type);
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;

  CmdDrawIndexedIndirect cmd{topology, draw_count, buffer->getBufferRef(), byte_offset, stride_bytes};
  Globals::ctx.dispatchPipeline(cmd, "multi_draw_indexed_indirect");

  update_draw_stats(0, 0);
  return true;
}
