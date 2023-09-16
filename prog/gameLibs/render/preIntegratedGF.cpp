#include <3d/dag_drvDecl.h>
#include <render/preIntegratedGF.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_drv3d.h>

TexPtr render_preintegrated_fresnel_GGX(const char *name, uint32_t preintegrate_type)
{
  SCOPE_RENDER_TARGET;

  uint32_t prefmt = TEXFMT_G16R16;
#if _TARGET_C1 | _TARGET_C2

#else
  const bool tryU16Format = true;
#endif

  // PS4 shader is hardwired to either u16 or f16 component output
  if (preintegrate_type & PREINTEGRATE_SPECULAR_DIFFUSE)
  {
    if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16) & d3d::USAGE_RTARGET) && tryU16Format)
      prefmt = TEXFMT_A16B16G16R16;
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_RTARGET))
      prefmt = TEXFMT_A16B16G16R16F;
    else if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & d3d::USAGE_RTARGET)) // 10 bits is a little bit not enough bits, but
                                                                                  // performance is higher. 11bit float is definetly
                                                                                  // not enough bits
      prefmt = TEXFMT_A2B10G10R10;
    else
      prefmt = TEXFMT_DEFAULT;
    if (!(preintegrate_type & PREINTEGRATE_QUALITY_MAX) && prefmt != TEXFMT_A2B10G10R10)
      if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & d3d::USAGE_RTARGET)) // 10 bits is a little bit not enough bits, but
                                                                               // performance is higher. 11bit float is definetly not
                                                                               // enough bits
        prefmt = TEXFMT_A2B10G10R10;
  }
  else
  {
    if ((d3d::get_texformat_usage(TEXFMT_G16R16) & d3d::USAGE_RTARGET) && tryU16Format)
      prefmt = TEXFMT_G16R16;
    else if ((d3d::get_texformat_usage(TEXFMT_G16R16F) & d3d::USAGE_RTARGET))
      prefmt = TEXFMT_G16R16F;
    else
      prefmt = TEXFMT_DEFAULT;
  }

  TexPtr preIntegratedGF = dag::create_tex(nullptr, 128, 32, prefmt | TEXCF_RTARGET, 1, name);
  preIntegratedGF->texaddr(TEXADDR_CLAMP);
  PostFxRenderer preintegrateEnvi;
  preintegrateEnvi.init("preintegrateEnvi");
  // d3d::clearview( CLEAR_TARGET, 0xffffffff, 0.f, 0 );
  d3d::set_render_target(preIntegratedGF.get(), 0);
  preintegrateEnvi.render();
  d3d::resource_barrier({preIntegratedGF.get(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  return preIntegratedGF;
}
