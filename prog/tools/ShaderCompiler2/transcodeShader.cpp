// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <3d/parseShaders.h>
#include <libTools/util/makeBindump.h>
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <debug/dag_debug.h>
#include "shLog.h"
#include "transcodeShader.h"
#include <util/dag_hierBitArray.h>

#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX11
#include "hlsl11transcode/asmShaders11.h"
#endif

dag::ConstSpan<uint32_t> transcode_vertex_shader(dag::ConstSpan<uint32_t> vpr)
{
  if (!vpr.size())
    return {};
  G_ASSERT(vpr.size() > 1);

  if (vpr[0] == _MAKE4C('DX9v') || vpr[0] == _MAKE4C('DX11'))
    return make_span_const((const uint32_t *)&vpr[1], vpr.size() - 1);
  G_ASSERTF(0, "obsolete/corrupt vertex shader? %c%c%c%c", DUMP4C(vpr[0]));
  return {};
}

dag::ConstSpan<uint32_t> transcode_pixel_shader(dag::ConstSpan<uint32_t> fsh)
{
  if (!fsh.size())
    return {};
  G_ASSERT(fsh.size() > 1);

  if (fsh[0] == _MAKE4C('DX9p') || fsh[0] == _MAKE4C('D11c'))
    return dag::ConstSpan<uint32_t>((const uint32_t *)&fsh[1], fsh.size() - 1);

  G_ASSERT(0 && "obsolete/corrupt pixel shader?");
  return {};
}

dag::ConstSpan<int> transcode_stcode(dag::ConstSpan<int> stcode)
{
#if BINDUMP_TARGET_BE
  static Tab<int> st(tmpmem);
  st.resize(stcode.size());

  // transcode all stcode
  for (int i = 0; i < stcode.size(); i++)
  {
    int op = shaderopcode::getOp(stcode[i]);

    st[i] = mkbindump::le2be32(stcode[i]);
    switch (op)
    {
      case SHCOD_INVERSE:
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_G_TM:
      case SHCOD_FSH_CONST:
      case SHCOD_VPR_CONST:
      case SHCOD_CS_CONST:
      case SHCOD_GLOB_SAMPLER:
      case SHCOD_SAMPLER:
      case SHCOD_TEXTURE:
      case SHCOD_TEXTURE_VS:
      case SHCOD_REG_BINDLESS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_TLAS:
      case SHCOD_GET_GBUF:
      case SHCOD_GET_GTLAS:
      case SHCOD_GET_GINT:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GVEC:
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      case SHCOD_GET_CHANNEL:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
      case SHCOD_GET_TEX:
      case SHCOD_GET_INT:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_INT_TOREAL:
      case SHCOD_IVEC_TOREAL:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC: break;

      case SHCOD_IMM_REAL:
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        break;

      case SHCOD_IMM_VEC:
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        break;

      case SHCOD_MAKE_VEC:
      case SHCOD_STATIC_BLOCK:
        i++;
        st[i] = mkbindump::le2be32(stcode[i]);
        break;

      case SHCOD_CALL_FUNCTION:
      {
        int pnum = shaderopcode::getOp3p3(stcode[i]);
        while (pnum > 0)
        {
          i++;
          st[i] = mkbindump::le2be32(stcode[i]);
          pnum--;
        }
      }
      break;

      default: debug("stcode: %d '%s' not processed!", stcode[i], ShUtils::shcod_tokname(op)); G_ASSERT(0);
    }
  }

  return st;
#else
  return stcode;
#endif
}
