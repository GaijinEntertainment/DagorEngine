// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/shFunc.h>
#include <debug/dag_debug.h>
#include <math/dag_color.h>
#include <math/dag_math3d.h>
#include <osApiWrappers/dag_localConv.h>

namespace functional
{
// prepare argument array
void prepareArgs(FunctionId func, ArgList &args)
{
  args.resize(getOpCount(func));
  prepareArgs(func, args.data(), args.size());
}

int prepareArgs(FunctionId func, FuncArgument *args, int /*num*/)
{
  switch (func)
  {
    case BF_TIME_PHASE:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_SIN: args[0].vt = shexpr::VT_REAL; return 1;
    case BF_COS: args[0].vt = shexpr::VT_REAL; return 1;
    case BF_POW:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_VECPOW:
      args[0].vt = shexpr::VT_COLOR4;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_SRGBREAD: args[0].vt = shexpr::VT_COLOR4; return 1;
    case BF_FSEL:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      args[2].vt = shexpr::VT_REAL;
      return 3;
    case BF_SQRT: args[0].vt = shexpr::VT_REAL; return 1;
    case BF_MIN:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_MAX:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_ANIM_FRAME:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      args[2].vt = shexpr::VT_REAL;
      args[3].vt = shexpr::VT_REAL;
      return 4;
    case BF_WIND_COEFF:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_FADE_VAL:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_REAL;
      args[2].vt = shexpr::VT_REAL;
      args[3].vt = shexpr::VT_REAL;
      return 4;
    case BF_GET_DIMENSIONS:
      args[0].vt = shexpr::VT_TEXTURE;
      args[1].vt = shexpr::VT_REAL;
      return 2;
    case BF_GET_SIZE: args[0].vt = shexpr::VT_BUFFER; return 1;
    case BF_GET_VIEWPORT: return 0;
    case BF_REQUEST_SAMPLER:
      args[0].vt = shexpr::VT_REAL;
      args[1].vt = shexpr::VT_COLOR4;
      args[2].vt = shexpr::VT_REAL;
      args[3].vt = shexpr::VT_REAL;
      return 4;
    case BF_EXISTS_TEX: args[0].vt = shexpr::VT_TEXTURE; return 1;
    case BF_EXISTS_BUF: args[0].vt = shexpr::VT_BUFFER; return 1;

    default: G_ASSERT(0);
  }
  return -1;
}

// return operand count for specified function
int getOpCount(FunctionId func)
{
  switch (func)
  {
    case BF_TIME_PHASE: return 2;
    case BF_SIN: return 1;
    case BF_COS: return 1;
    case BF_POW: return 2;
    case BF_SRGBREAD: return 1;
    case BF_VECPOW: return 2;
    case BF_FSEL: return 3;
    case BF_SQRT: return 1;
    case BF_MIN: return 2;
    case BF_MAX: return 2;
    case BF_ANIM_FRAME: return 4;
    case BF_WIND_COEFF: return 2;
    case BF_FADE_VAL: return 4;
    case BF_GET_DIMENSIONS: return 2;
    case BF_GET_SIZE: return 1;
    case BF_GET_VIEWPORT: return 0;
    case BF_REQUEST_SAMPLER: return 4;
    case BF_EXISTS_TEX: return 1;
    case BF_EXISTS_BUF: return 1;
    default: G_ASSERT(0);
  }
  return 0;
}

// return value type for specified function
shexpr::ValueType getValueType(FunctionId func)
{
  switch (func)
  {
    case BF_TIME_PHASE: return shexpr::VT_REAL;
    case BF_SIN: return shexpr::VT_REAL;
    case BF_COS: return shexpr::VT_REAL;
    case BF_POW: return shexpr::VT_REAL;
    case BF_FSEL: return shexpr::VT_REAL;
    case BF_SQRT: return shexpr::VT_REAL;
    case BF_MIN: return shexpr::VT_REAL;
    case BF_MAX: return shexpr::VT_REAL;
    case BF_ANIM_FRAME: return shexpr::VT_COLOR4;
    case BF_WIND_COEFF: return shexpr::VT_COLOR4;
    case BF_VECPOW: return shexpr::VT_COLOR4;
    case BF_SRGBREAD: return shexpr::VT_COLOR4;
    case BF_FADE_VAL: return shexpr::VT_REAL;
    case BF_GET_DIMENSIONS: return shexpr::VT_COLOR4;
    case BF_GET_SIZE: return shexpr::VT_REAL;
    case BF_GET_VIEWPORT: return shexpr::VT_COLOR4;
    case BF_REQUEST_SAMPLER: return shexpr::VT_UNDEFINED;
    case BF_EXISTS_TEX: return shexpr::VT_REAL;
    case BF_EXISTS_BUF: return shexpr::VT_REAL;
    default: G_ASSERT(0);
  }
  return shexpr::VT_UNDEFINED;
}

static Color4 anim_frame(float arg0, float arg1, float arg2, float arg3)
{
  // arg0 - 0..1 - frame index
  int x = (int)arg1;     // 1 - frames by x
  int y = (int)arg2;     // 2 - frames by y
  int total = (int)arg3; // 3 - total frame count
  if (!x || !y || !total)
  {
    DAG_FATAL("invalid arguments in shader function 'anim_frame(%.4f, %d, %d, %d)'", arg0, x, y, total);
  }
  int picture = (int)(arg0 * (total - 1));

  // return frame texture coords
  return Color4(real(picture % x) / x, real(int(picture / y)) / y, 0.f, 0.f);
}

// evaluate function - return false, if error
bool evaluate(FunctionId func, Color4 &res, const ArgList &args)
{
  switch (func)
  {
    case BF_SRGBREAD:
      res = Color4(powf(args[0].val.c[0], 2.2f), powf(args[0].val.c[1], 2.2f), powf(args[0].val.c[2], 2.2f), args[0].val.c[3]);
      break;
    case BF_SIN: res.r = sinf(args[0].val.r); break;
    case BF_COS: res.r = cosf(args[0].val.r); break;
    case BF_VECPOW:
      res = Color4(powf(args[0].val.c[0], args[1].val.r), powf(args[0].val.c[1], args[1].val.r), powf(args[0].val.c[2], args[1].val.r),
        powf(args[0].val.c[3], args[1].val.r));
      break;
    case BF_POW: res.r = powf(args[0].val.r, args[1].val.r); break;
    case BF_SQRT:
    {
      real x = args[0].val.r;

      if (x < 0)
      {
        DAG_FATAL("invalid arguments in shader function 'sqrt(%.4f)'", x);
      }

      res.r = sqrtf(x);
      break;
    }
    case BF_MIN: res.r = min(args[0].val.r, args[1].val.r); break;
    case BF_MAX: res.r = max(args[0].val.r, args[1].val.r); break;
    case BF_ANIM_FRAME: res = anim_frame(args[0].val.r, args[1].val.r, args[2].val.r, args[3].val.r); break;
    case BF_FSEL: res.r = fsel(args[0].val.r, args[1].val.r, args[2].val.r); break;
    default: DAG_FATAL("evaluate - no code for evaluate: id=%d name='%s'", func, getFuncName(func));
  }
  return true;
}

// return function id by name; return false, if invalid function
bool getFuncId(const char *name, FunctionId &ret_func)
{
  if (dd_stricmp(name, "time_phase") == 0)
    ret_func = BF_TIME_PHASE;
  else if (dd_stricmp(name, "sin") == 0)
    ret_func = BF_SIN;
  else if (dd_stricmp(name, "cos") == 0)
    ret_func = BF_COS;
  else if (dd_stricmp(name, "pow") == 0)
    ret_func = BF_POW;
  else if (dd_stricmp(name, "sRGBread") == 0)
    ret_func = BF_SRGBREAD;
  else if (dd_stricmp(name, "vecpow") == 0)
    ret_func = BF_VECPOW;
  else if (dd_stricmp(name, "fsel") == 0)
    ret_func = BF_FSEL;
  else if (dd_stricmp(name, "sqrt") == 0)
    ret_func = BF_SQRT;
  else if (dd_stricmp(name, "min") == 0)
    ret_func = BF_MIN;
  else if (dd_stricmp(name, "max") == 0)
    ret_func = BF_MAX;
  else if (dd_stricmp(name, "anim_frame") == 0)
    ret_func = BF_ANIM_FRAME;
  else if (dd_stricmp(name, "wind_coeff") == 0)
    ret_func = BF_WIND_COEFF;
  else if (dd_stricmp(name, "fade_val") == 0)
    ret_func = BF_FADE_VAL;
  else if (dd_stricmp(name, "get_dimensions") == 0)
    ret_func = BF_GET_DIMENSIONS;
  else if (dd_stricmp(name, "get_size") == 0)
    ret_func = BF_GET_SIZE;
  else if (dd_stricmp(name, "get_viewport") == 0)
    ret_func = BF_GET_VIEWPORT;
  else if (dd_stricmp(name, "request_sampler") == 0)
    ret_func = BF_REQUEST_SAMPLER;
  else if (dd_stricmp(name, "exists_tex") == 0)
    ret_func = BF_EXISTS_TEX;
  else if (dd_stricmp(name, "exists_buf") == 0)
    ret_func = BF_EXISTS_BUF;
  else
    return false;
  return true;
}

// return true, if function is dynamic always
bool isAlwaysDynamic(FunctionId id)
{
  switch (id)
  {
    case BF_TIME_PHASE: return true;
    case BF_WIND_COEFF: return true;
    case BF_FADE_VAL: return true;
    case BF_GET_VIEWPORT: return true;
    default: return false;
  }
}

} // namespace functional
