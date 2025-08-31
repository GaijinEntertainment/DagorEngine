// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceActivationGeneric.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include "drv_assert_defs.h"

void res_act_generic::activate_buffer(Sbuffer *buf, ResourceActivationAction action, GpuPipeline)
{
  D3D_CONTRACT_ASSERTF_RETURN(buf != nullptr, , "Trying to activate a nullptr buffer!");

  using RAA = ResourceActivationAction;
  switch (action)
  {
    case RAA::DISCARD_AS_RTV_DSV:
    case RAA::REWRITE_AS_RTV_DSV:
    case RAA::CLEAR_AS_RTV_DSV: D3D_CONTRACT_ASSERT_FAIL("Tried to activate a buffer with a render target action!"); break;

    case RAA::DISCARD_AS_UAV:
    case RAA::REWRITE_AS_UAV:
    case RAA::REWRITE_AS_COPY_DESTINATION:
      // No action required: generic implementations have no aliases
      break;

    case RAA::CLEAR_F_AS_UAV:
    case RAA::CLEAR_I_AS_UAV: d3d::zero_rwbufi(buf); break;
  }
}

void res_act_generic::activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline)
{
  D3D_CONTRACT_ASSERTF_RETURN(tex != nullptr, , "Trying to activate a nullptr texture!");

  TextureInfo info{};
  tex->getinfo(info);

  using RAA = ResourceActivationAction;
  switch (action)
  {
    case RAA::DISCARD_AS_RTV_DSV:
    case RAA::DISCARD_AS_UAV:
    case RAA::REWRITE_AS_COPY_DESTINATION:
    case RAA::REWRITE_AS_RTV_DSV:
    case RAA::REWRITE_AS_UAV:
      // No action required: generic implementations have no aliases
      break;

    case RAA::CLEAR_AS_RTV_DSV:
    {
      auto cv = d3d::get_texformat_usage(info.cflg) & d3d::USAGE_DEPTH
                  ? make_clear_value(value.asDepth, value.asStencil)
                  : make_clear_value(value.asFloat[0], value.asFloat[1], value.asFloat[2], value.asFloat[3]);

      for (uint32_t l = 0; l < info.a; ++l)
        for (uint32_t m = 0; m < info.mipLevels; ++m)
          d3d::clear_rt({tex, m, l}, cv);
    }
    break;

    case RAA::CLEAR_F_AS_UAV:
      for (uint32_t l = 0; l < info.a; ++l)
        for (uint32_t m = 0; m < info.mipLevels; ++m)
          d3d::clear_rwtexf(tex, value.asFloat, l, m);
      break;

    case RAA::CLEAR_I_AS_UAV:
      for (uint32_t l = 0; l < info.a; ++l)
        for (uint32_t m = 0; m < info.mipLevels; ++m)
          d3d::clear_rwtexi(tex, value.asUint, l, m);
      break;
  }
}

void res_act_generic::deactivate_buffer(Sbuffer *buf, GpuPipeline)
{
  D3D_CONTRACT_ASSERTF(buf != nullptr, "Tried to deactivate a nullptr buffer!");
}

void res_act_generic::deactivate_texture(BaseTexture *tex, GpuPipeline)
{
  D3D_CONTRACT_ASSERTF(tex != nullptr, "Tried to deactivate a nullptr texture!");
}
