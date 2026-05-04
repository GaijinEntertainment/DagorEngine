// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driverDesc.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include "render.h"
#include "drv_assert_defs.h"
#include "drv_log_defs.h"

#include "indicesManager.h"

using namespace drv3d_metal;

VPROG d3d::create_vertex_shader(const ShaderSource &data)
{
  Tab<uint8_t> tmpmem(framemem_ptr());
  const uint32_t *native_code = data.uncompress(tmpmem);
  return render.createVertexShader((const uint8_t*)native_code, data.metadata.data());
}

void d3d::delete_vertex_shader(VPROG vs)
{
  render.deleteVertexShader(vs);
}

FSHADER d3d::create_pixel_shader(const ShaderSource &data)
{
  Tab<uint8_t> tmpmem(framemem_ptr());
  const uint32_t *native_code = data.uncompress(tmpmem);
  return render.createPixelShader((const uint8_t*)native_code, data.metadata.data());
}

void d3d::delete_pixel_shader(FSHADER ps)
{
  render.deletePixelShader(ps);
}

// Combined programs
PROGRAM d3d::create_program(VPROG vp, FSHADER ps, VDECL vdecl,
              unsigned *strides, unsigned streams)
{
  return render.createProgram(vp, ps, vdecl, strides, streams);
}

//if strides & streams are unset, will get them from VDECL
PROGRAM d3d::create_program_cs(const ShaderSource &data, CSPreloaded)
{
  Tab<uint8_t> tmpmem(framemem_ptr());
  const uint32_t *cs_native = data.uncompress(tmpmem);
  return render.createComputeProgram((const uint8_t*)cs_native, data.metadata.data());
}

//sets both pixel and vertex shader and vertex declaration
bool d3d::set_program(PROGRAM prog)
{
  render.setProgram(prog);
  return true;
}

//deletes vprog and fshader. VDECL should be deleted independently
void d3d::delete_program(PROGRAM prog)
{
  render.deleteProgram(prog);
}

int d3d::set_vs_constbuffer_register_count(int required_count) { return min(Render::MAX_CBUFFER_SIZE / 16, required_count); }

bool d3d::set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs)
{
  D3D_CONTRACT_ASSERT_RETURN(stage < STAGE_MAX, false);
  render.setConst(stage, reg_base, (const float*)data, num_regs);

  return true;
}

int d3d::set_cs_constbuffer_register_count(int required_count) { return min(Render::MAX_CBUFFER_SIZE / 16, required_count); }

bool d3d::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  render.dispatch(thread_group_x, thread_group_y, thread_group_z);

  return true;
}

bool d3d::dispatch_indirect(Sbuffer* buffer, uint32_t offset, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  render.dispatch_indirect(buffer, offset);

  return true;
}

void d3d::dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  render.dispatchMesh(thread_group_x, thread_group_y, thread_group_z);
}

void d3d::dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  render.dispatchMeshIndirect((Buffer *)args, dispatch_count, stride_bytes, byte_offset);
}

void d3d::dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  if (!d3d::get_driver_desc().caps.hasMeshShader || !d3d::get_driver_desc().caps.hasMeshShaderIndirectCount)
  {
    D3D_ERROR("Metal: dispatch_mesh_indirect_count is unsupported");
    return;
  }
  G_UNUSED(args);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(count);
  G_UNUSED(count_byte_offset);
  G_UNUSED(max_count);
}

bool d3d::zero_rwbufi(Sbuffer *buf)
{
  uint32_t val[4] = {0, 0, 0, 0};
  render.clearRwBufferi(buf, val);
  return true;
}
