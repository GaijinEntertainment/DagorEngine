// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "clear.h"

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

#include "driver.h"
#include "texture.h"

namespace drv3d_dx11
{

extern eastl::vector_map<int, shaders::DriverRenderStateId> clear_view_states;

void clear_slow(int write_mask, const float (&color_value)[4], float z_value, uint32_t stencil_value)
{
  d3d::set_pixel_shader(g_default_clear_ps);
  d3d::set_vertex_shader(g_default_clear_vs);
  d3d::setvdecl(g_default_clear_vdecl);

  if (auto ins = clear_view_states.insert(write_mask); ins.second)
  {
    shaders::RenderState state;
    state.independentBlendEnabled = 0;
    for (auto &blendParam : state.blendParams)
    {
      blendParam.ablend = 0;
      blendParam.sepablend = 0;
    }
    state.ztest = static_cast<bool>(write_mask & CLEAR_ZBUFFER);
    state.zwrite = static_cast<bool>(write_mask & CLEAR_ZBUFFER);
    state.zFunc = D3D11_COMPARISON_ALWAYS;
    state.zBias = 0.0f;
    state.slopeZBias = 0.0f;
    state.colorWr = (write_mask & CLEAR_TARGET) ? WRITEMASK_ALL : 0;
    state.cull = CULL_NONE;
    state.zClip = 0;
    if (write_mask & CLEAR_STENCIL)
    {
      state.stencil.func = CMPF_ALWAYS;
      state.stencil.readMask = state.stencil.writeMask = 0xFF;
      state.stencilRef = stencil_value;
      state.stencil.fail = state.stencil.pass = state.stencil.zFail = STNCLOP_REPLACE;
    }
    ins.first->second = d3d::create_render_state(state);
  }
  d3d::setstencil(stencil_value);
  d3d::set_render_state(clear_view_states[write_mask]);

  struct ClearVertex
  {
    Point3 pos;
    Point4 color;
  } tri[3];

  tri[0].pos = Point3(-1.f, 1.f, z_value);
  tri[1].pos = Point3(-1.f, -3.f, z_value);
  tri[2].pos = Point3(3.f, 1.f, z_value);
  tri[0].color = tri[1].color = tri[2].color = Point4(color_value, Point4::CtorPtrMark{});

  RenderState &rs = g_render_state;
  const bool psUavsUsed = rs.texFetchState.uavState[STAGE_PS].uavsUsed;
  if (psUavsUsed)
  {
    // This shader uses all 8 RTs, so slots must be valid RTs or null. Make sure we don't set UAVs on flush.
    // Prevents validation error DEVICE_DRAW_UNORDEREDACCESSVIEW_RENDERTARGETVIEW_OVERLAP
    rs.texFetchState.uavState[STAGE_PS].uavModifiedMask = 0xFFFFFFFF;
    rs.texFetchState.uavState[STAGE_PS].uavsUsed = false;
    rs.modified = true;
  }

  d3d::draw_up(PRIM_TRISTRIP, 1, tri, sizeof(ClearVertex));

  if (psUavsUsed)
  {
    // Restore UAV state.
    rs.texFetchState.uavState[STAGE_PS].uavModifiedMask = 0xFFFFFFFF;
    rs.texFetchState.uavState[STAGE_PS].uavsUsed = true;
    rs.modified = true;
  }
}

} // namespace drv3d_dx11
