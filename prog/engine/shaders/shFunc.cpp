// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/shFunc.h>
#include "shadersBinaryData.h"
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <debug/dag_debug.h>
#include <math/dag_math3d.h>
#include <osApiWrappers/dag_localConv.h>
#include "shRegs.h"

#include <shaders/dag_stcode.h>
#include <shaders/commonStcodeFunctions.h>

#include "stcode/compareStcode.h"

namespace functional
{

#define ARG0     real_reg(regs, in_regs[0])
#define ARG1     real_reg(regs, in_regs[1])
#define ARG2     real_reg(regs, in_regs[2])
#define ARG3     real_reg(regs, in_regs[3])
#define OUT      real_reg(regs, out_reg)
#define OUT_INT  int_reg(regs, out_reg)
#define COLOROUT color4_reg(regs, out_reg).set

void callFunction(FunctionId id, int out_reg, const int *in_regs, char *regs)
{
  switch (id)
  {
    case BF_SRGBREAD:
    {
      color4_reg(regs, out_reg) = srgb_read(color4_reg(regs, in_regs[0]));
      break;
    }

    case BF_TIME_PHASE: OUT = get_shader_global_time_phase(ARG0, ARG1); break;

    case BF_SIN: OUT = sinf(ARG0); break;

    case BF_COS: OUT = cosf(ARG0); break;

    case BF_VECPOW:
    {
      const Color4 &arg0 = color4_reg(regs, in_regs[0]);
      float arg1 = ARG1;
      COLOROUT(powf(arg0.r, arg1), powf(arg0.g, arg1), powf(arg0.b, arg1), powf(arg0.a, arg1));
      break;
    }

    case BF_POW: OUT = powf(ARG0, ARG1); break;

    case BF_SQRT: OUT = safe_sqrt(ARG0); break;

    case BF_MIN: OUT = min(ARG0, ARG1); break;

    case BF_MAX: OUT = max(ARG0, ARG1); break;

    case BF_FSEL: OUT = fsel(ARG0, ARG1, ARG2); break;

    case BF_ANIM_FRAME: color4_reg(regs, out_reg) = anim_frame(ARG0, ARG1, ARG2, ARG3); break;

    case BF_WIND_COEFF: color4_reg(regs, out_reg) = wind_coeff(get_shader_global_time_phase(ARG0, ARG1)); break;

    case BF_FADE_VAL: OUT = fade_val(get_shader_global_time_phase(ARG0, ARG1), ARG2); break;

    case BF_GET_DIMENSIONS:
    {
      TEXTUREID tex_id = tex_reg(regs, in_regs[0]);
      auto tex = acquire_managed_tex(tex_id);
      TextureInfo info;
      if (tex)
      {
        tex->getinfo(info, (int)ARG1);
        switch (info.type)
        {
          case D3DResourceType::ARRTEX: info.d = info.a; break;
          case D3DResourceType::CUBEARRTEX: info.d = info.a / 6; break;

          case D3DResourceType::TEX:
          case D3DResourceType::CUBETEX:
          case D3DResourceType::VOLTEX:
          case D3DResourceType::SBUF: break;
        }
      }
      else
      {
        info.w = info.h = info.d = info.mipLevels = 0; //-V1048
      }
      COLOROUT(info.w, info.h, info.d, info.mipLevels);
      release_managed_tex(tex_id);
      break;
    }

    case BF_GET_SIZE:
    {
      Sbuffer *buf = buf_reg(regs, in_regs[0]);
      int size = 0;
      if (buf)
        size = buf->getNumElements();
      OUT = size;
      break;
    }

    case BF_GET_VIEWPORT:
    {
      union
      {
        vec4i v;
        int i[4];
      } vp; // Intentionally not inited for perf reasons
      float _1, _2;
      G_VERIFY(d3d::getview(vp.i[0], vp.i[1], vp.i[2], vp.i[3], _1, _2));
      set_vec_reg(v_cvti_vec4f(vp.v), regs, out_reg);
      break;
    }

    case BF_REQUEST_SAMPLER:
    {
      int sampler_id = out_reg;
      auto *dump = shBinDumpOwner().getDumpV3();
      G_ASSERTF(dump, "Used incompatible version of shader dump for call 'request_sampler' intrinsic");
      d3d::SamplerInfo info = dump->samplers[sampler_id];
      if (in_regs[1] >= 0)
      {
        Color4 border_color = color4_reg(regs, in_regs[1]);
        uint32_t rgb = border_color.r || border_color.g || border_color.b ? 0xFFFFFF : 0;
        uint32_t a = border_color.a ? 0xFF000000 : 0;
        info.border_color = static_cast<d3d::BorderColor::Color>(a | rgb);
      }
      if (in_regs[2] >= 0)
        info.anisotropic_max = ARG2;
      if (in_regs[3] >= 0)
        info.mip_map_bias = ARG3;

      G_ASSERTF_BREAK(in_regs[0] < dump->globVars.size(), "Invalid sampler var id %d used for call 'request_sampler' intrinsic",
        in_regs[0]);
      G_ASSERTF_BREAK(dump->globVars.getType(in_regs[0]) == SHVT_SAMPLER,
        "Invalid sampler var id %d type used for call 'request_sampler' intrinsic", in_regs[0]);
      auto &smp = shBinDumpOwner().globVarsState.get<d3d::SamplerHandle>(in_regs[0]);
      smp = d3d::request_sampler(info);

      stcode::dbg::record_request_sampler(stcode::dbg::RecordType::REFERENCE, info);
      break;
    }

    case BF_EXISTS_TEX:
    {
      OUT_INT = 0;
      TEXTUREID tex_id = tex_reg(regs, in_regs[0]);
      auto tex = acquire_managed_tex(tex_id);
      if (tex)
        OUT_INT = 1;
      release_managed_tex(tex_id);
      break;
    }

    case BF_EXISTS_BUF:
    {
      OUT_INT = 0;
      Sbuffer *buf = buf_reg(regs, in_regs[0]);
      if (buf)
        OUT_INT = 1;
      break;
    }

    default: DAG_FATAL("Invalid stcode function id %d", id);
  }
}

#undef ARG0
#undef ARG1
#undef ARG2
#undef ARG3
#undef OUT
#undef COLOROUT

} // namespace functional
