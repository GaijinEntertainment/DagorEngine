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
  G_ASSERT(0 && "obsolete/corrupt vertex shader?");
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

dag::ConstSpan<int> process_stblkcode(dag::ConstSpan<int> stcode, bool sh_blk)
{
  static Tab<int> st(tmpmem);
  if (sh_blk && (stcode.size() < 1 || shaderopcode::getOp(stcode[0]) != SHCOD_BLK_ICODE_LEN))
    return stcode;

  st.resize(stcode.size() + 2);
  mem_copy_to(stcode, &st[2]);
  if (sh_blk)
    st[0] = shaderopcode::makeOp1(SHCOD_BLK_ICODE_LEN, shaderopcode::getOp1p1(stcode[0]) + 2);
  ConstSizeBitArray<5> tx_bits;
  tx_bits.reset();
  ConstSizeBitArray<12> vs_bits;
  vs_bits.reset();
  ConstSizeBitArray<12> ps_bits;
  ps_bits.reset();

  int stcode_len = sh_blk ? shaderopcode::getOp1p1(stcode[0]) + 1 : stcode.size();
  for (int i = sh_blk ? 1 : 0; i < stcode_len; i++)
  {
    int op = shaderopcode::getOp(stcode[i]);

    switch (op)
    {
      case SHCOD_FSH_CONST:
      case SHCOD_CS_CONST: ps_bits.set(shaderopcode::getOp2p1(stcode[i])); break;
      case SHCOD_VPR_CONST:
      case SHCOD_REG_BINDLESS: vs_bits.set(shaderopcode::getOp2p1(stcode[i])); break;
      case SHCOD_TEXTURE: tx_bits.set(shaderopcode::getOp2p1(stcode[i])); break;
      case SHCOD_TEXTURE_VS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF: break;
      case SHCOD_G_TM:
        vs_bits.set(shaderopcode::getOp2p2_16(stcode[i]) + 0);
        vs_bits.set(shaderopcode::getOp2p2_16(stcode[i]) + 1);
        vs_bits.set(shaderopcode::getOp2p2_16(stcode[i]) + 2);
        vs_bits.set(shaderopcode::getOp2p2_16(stcode[i]) + 3);
        break;

      case SHCOD_INVERSE:
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      case SHCOD_GET_TEX:
      case SHCOD_GET_INT:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC:
      case SHCOD_PACK_MATERIALS: break;

      case SHCOD_IMM_REAL: i++; break;

      case SHCOD_IMM_VEC: i += 4; break;

      case SHCOD_MAKE_VEC: i++; break;

      case SHCOD_CALL_FUNCTION: i += shaderopcode::getOp3p3(stcode[i]); break;

      case SHCOD_GET_GINT:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
        if (sh_blk)
          break;
        G_ASSERT(0 && "ICE: global var access in static block code");
        break;

      default: debug("stcode: %d '%s' not processed!", stcode[i], ShUtils::shcod_tokname(op)); G_ASSERT(0);
    }
  }
  int tx_base, vs_base, ps_base, tx_num, vs_num, ps_num;
#define FIND_BOUNDS(x)                                                       \
  for (x##_base = 0; x##_base < x##_bits.SZ; x##_base++)                     \
    if (x##_bits.get(x##_base))                                              \
      break;                                                                 \
  for (x##_num = 0; x##_base + x##_num < x##_bits.SZ; x##_num++)             \
    if (!x##_bits.get(x##_base + x##_num))                                   \
      break;                                                                 \
  if (!x##_num)                                                              \
    x##_base = 0;                                                            \
  else                                                                       \
    for (int i = x##_base + x##_num; i < x##_bits.SZ; i++)                   \
      if (x##_bits.get(i))                                                   \
      {                                                                      \
        ShUtils::shcod_dump(stcode);                                         \
        debug_(#x " bits:");                                                 \
        for (int j = 0; j < x##_bits.SZ; j++)                                \
          debug_(" %d", x##_bits.get(j));                                    \
        debug("\n  base=%d, num=%d", x##_base, x##_num);                     \
        G_ASSERT(0 && "ICE: " #x " is not continuous in static block code"); \
        break;                                                               \
      }

  FIND_BOUNDS(tx);
  FIND_BOUNDS(vs);
  FIND_BOUNDS(ps);
#undef FIND_BOUNDS
  if (vs_base == 0x800 && ps_base == 0)
  {
    G_ASSERTF(ps_num == 0, "ICE: vs_base=%d ps_base=%d means STATIC_CBUF, but ps_num=%d (!=0)", vs_base, ps_base, ps_num);
    vs_base = 0xFF;
    ps_base = 0xFF;
  }

  st[sh_blk ? 1 : 0] = shaderopcode::makeOp3(SHCOD_STATIC_BLOCK, tx_num, vs_num, ps_num);
  st[sh_blk ? 2 : 1] = (tx_base << 16) | (vs_base << 8) | ps_base;
  return st;
}
