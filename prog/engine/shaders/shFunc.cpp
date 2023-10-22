// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <shaders/shFunc.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <debug/dag_debug.h>
#include <math/dag_math3d.h>
#include <osApiWrappers/dag_localConv.h>
#include "shRegs.h"

namespace functional
{
// wind variables - can be refactored in future (when times & power will totally corrected)
#define TIMES_NUM 11
const float times[] = {1.0f, 0.66f, 0.8f, 1.0f, 0.66f, 0.8f, 1.0f, 0.66f, 0.8f, 1.0f, 1.0f};
const float maxPowers[] = {1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.0f};
const float minPowers[] = {1.0f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f, 1.5f, 1.25f, 1.0f};

static Color4 wind_coeff(float t)
{
  float _times[TIMES_NUM];

  float totF = 0.0f;
  for (int i = 0; i < TIMES_NUM; ++i)
    totF += times[i];

  float f = 0.0f;
  for (int i = 0; i < TIMES_NUM; ++i)
  {
    f += times[i] / totF;
    _times[i] = f;
  }

  int ind = 0;
  for (int i = 0; i < TIMES_NUM; ++i)
  {
    if (t <= _times[i])
    {
      ind = i;
      break;
    }
  }

  if (ind <= 0)
    t = t / _times[0];
  else
    t = (t - _times[ind - 1]) / (_times[ind] - _times[ind - 1]);

  return Color4(t, minPowers[ind] * (1.0f - t) + maxPowers[ind] * t, 0.f, 0.f);
}


static float fade_val(float t, float arg2)
{
  if (t > 0.5f)
  {
    t = 1.0f - (t - 0.5f) * 2.0f;
    t = 1.0f - t * t;
  }
  else
  {
    t *= 2.0f;
    t = 1.0f - t * t;
  }
  t = arg2 + t * (1.0f - arg2);

  return t;
}


static Color4 anim_frame(float arg0, float arg1, float arg2, float arg3)
{
  // arg0 - 0..1 - frame index
  int x = (int)arg1;     // 1 - frames by x
  int y = (int)arg2;     // 2 - frames by y
  int total = (int)arg3; // 3 - total frame count
  if (!x || !y || !total)
  {
    fatal("invalid arguments in shader function 'anim_frame(%.4f, %d, %d, %d)'", arg0, x, y, total);
  }
  int picture = (int)(arg0 * (total - 1));

  // return frame texture coords
  return Color4(real(picture % x) / x, real(int(picture / y)) / y, 0.f, 0.f);
}

#define ARG0     real_reg(regs, in_regs[0])
#define ARG1     real_reg(regs, in_regs[1])
#define ARG2     real_reg(regs, in_regs[2])
#define ARG3     real_reg(regs, in_regs[3])
#define OUT      real_reg(regs, out_reg)
#define COLOROUT color4_reg(regs, out_reg).set

void callFunction(FunctionId id, int out_reg, const int *in_regs, char *regs)
{
  switch (id)
  {
    case BF_SRGBREAD:
    {
      const Color4 &arg = color4_reg(regs, in_regs[0]);
      COLOROUT(powf(arg.r, 2.2f), powf(arg.g, 2.2f), powf(arg.b, 2.2f), arg.a);
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
        switch (info.resType)
        {
          case RES3D_ARRTEX: info.d = info.a; break;
          case RES3D_CUBEARRTEX: info.d = info.a / 6; break;
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

    case BF_GET_VIEWPORT:
    {
      int x = 0, y = 0, w = 0, h = 0;
      float zn = 0, zf = 0;
      d3d::getview(x, y, w, h, zn, zf);
      COLOROUT(x, y, w, h);
      break;
    }

    default: fatal("Invalid stcode function id %d", id);
  }
}

#undef ARG0
#undef ARG1
#undef ARG2
#undef ARG3
#undef OUT
#undef COLOROUT

} // namespace functional
