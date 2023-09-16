#include "resourceActivationGeneric.h"

void res_act_generic::activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline)
{
  G_ASSERTF_RETURN(buf != nullptr, , "Trying to activate a nullptr buffer!");

  using RAA = ResourceActivationAction;
  switch (action)
  {
    case RAA::DISCARD_AS_RTV_DSV:
    case RAA::REWRITE_AS_RTV_DSV:
    case RAA::CLEAR_AS_RTV_DSV: G_ASSERTF(false, "Tried to activate a buffer with a render target action!"); break;

    case RAA::DISCARD_AS_UAV:
    case RAA::REWRITE_AS_UAV:
    case RAA::REWRITE_AS_COPY_DESTINATION:
      // No action required: generic implementations have no aliases
      break;

    case RAA::CLEAR_F_AS_UAV: d3d::clear_rwbuff(buf, value.asFloat); break;

    case RAA::CLEAR_I_AS_UAV: d3d::clear_rwbufi(buf, value.asUint); break;
  }
}

void res_act_generic::activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline)
{
  G_ASSERTF_RETURN(tex != nullptr, , "Trying to activate a nullptr texture!");

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
      SCOPE_RENDER_TARGET;

      if (!(d3d::get_texformat_usage(info.cflg) & d3d::USAGE_DEPTH))
      {
        auto ithToChar = [value](uint32_t i) { return static_cast<unsigned char>(value.asFloat[i] * 255); };

        auto color = E3DCOLOR(ithToChar(0), ithToChar(1), ithToChar(2), ithToChar(3));

        for (uint32_t l = 0; l < info.a; ++l)
        {
          for (uint32_t m = 0; m < info.mipLevels; ++m)
          {
            // No, we cannot construct an initializer list of a
            // dynamic size with all the layers and mips at once.
            d3d::set_render_target({}, false, {{tex, m, l}});
            d3d::clearview(CLEAR_TARGET, color, 0, 0);
          }
        }
      }
      else
      {
        for (uint32_t l = 0; l < info.a; ++l)
        {
          for (uint32_t m = 0; m < info.mipLevels; ++m)
          {
            d3d::set_render_target({tex, m, l}, false, {});
            d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(), value.asDepth, value.asStencil);
          }
        }
      }
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
  G_ASSERTF(buf != nullptr, "Tried to deactivate a nullptr buffer!");
}

void res_act_generic::deactivate_texture(BaseTexture *tex, GpuPipeline)
{
  G_ASSERTF(tex != nullptr, "Tried to deactivate a nullptr texture!");
}
