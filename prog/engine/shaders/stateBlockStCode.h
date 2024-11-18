// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_renderStates.h>
#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shFunc.h>

#include "shStateBlock.h"
#include "shRegs.h"

#if !_TARGET_STATIC_LIB
#define SHOW_ERROR(fmt, ...) G_ASSERTF(0, fmt, ##__VA_ARGS__);
#else
#define SHOW_ERROR(fmt, ...) logerr(fmt, ##__VA_ARGS__);
#endif

template <typename Special>
__forceinline bool execute_st_block_code(const int *__restrict &codp, const int *__restrict codp_end, ShaderStateBlock &block,
  const char *context, int stcode_id, Special &&special, void (*lock_fn)())
{
  G_UNUSED(context);
#if DAGOR_DBGLEVEL > 0
  const int *__restrict codpStart = codp;
#endif
  using namespace shaderopcode;
  const uint32_t opc_ = *codp;
  G_ASSERT(getOp(opc_) == SHCOD_STATIC_BLOCK);

  char *regs = POW2_ALIGN_PTR(alloca(shBinDump().maxRegSize * 4 + 16), 16, char);

  int psTexCount = getOp3p1(opc_), vscCnt = getOp3p2(opc_), pscCnt = getOp3p3(opc_), psTexBase = (codp[1] >> 16) & 0xFF,
      vscBase = (codp[1] >> 8) & 0xFF, pscBase = codp[1] & 0xFF;
  bool static_cbuf = (vscBase == 0xFF && pscBase == 0xFF && pscCnt == 0);
  bool multidrawCbuf = false;
  if (static_cbuf)
  {
    vscBase = 0x800;
    pscBase = 0;
  }

  codp += 2;
  static constexpr int MAX_VS_TEX = 16;
  int minVsTex = MAX_VS_TEX, maxVsTex = -1;
  TEXTUREID *psTex = (TEXTUREID *)alloca(psTexCount * sizeof(TEXTUREID)), //-V630 (alloca for classes)
    *vsTex = (TEXTUREID *)alloca(MAX_VS_TEX * sizeof(TEXTUREID));         //-V630 (alloca for classes)
  Point4 *vsConsts = (Point4 *)alloca(vscCnt * sizeof(Point4)),           //-V630 (alloca for classes)
    *psConsts = (Point4 *)alloca(pscCnt * sizeof(Point4));                //-V630 (alloca for classes)

  eastl::fixed_vector<BindlessConstParams, 6, /*overflow*/ true, framemem_allocator> bindlessResources;
  added_bindless_textures_t addedBindlessTextures;

  shaders::RenderState state;

  for (; codp < codp_end; codp++)
  {
    const uint32_t opc = *codp;
    switch (getOp(opc))
    {
      case SHCOD_VPR_CONST:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= vscBase && ind < vscBase + vscCnt);
        vsConsts[ind - vscBase] = *get_reg_ptr<Point4>(regs, ofs);
      }
      break;
      case SHCOD_REG_BINDLESS:
      {
        G_ASSERTF(PLATFORM_HAS_BINDLESS, "Bindless texture operation was found in stcode, while current driver doesn't support it.");
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= vscBase && ind < vscBase + vscCnt);
        int texId = find_or_add_bindless_tex(tex_reg(regs, ofs), addedBindlessTextures);
        bindlessResources.push_back(BindlessConstParams{ind - vscBase, texId});
        vsConsts[ind - vscBase] = Point4(bitwise_cast<float, int>(-1), bitwise_cast<float, int>(-1), 0, 0);
      }
      break;
      case SHCOD_FSH_CONST:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);

        G_ASSERT(ind >= pscBase && ind < pscBase + pscCnt);
        psConsts[ind - pscBase] = *get_reg_ptr<Point4>(regs, ofs);
      }
      break;
      case SHCOD_TEXTURE:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= psTexBase && ind < psTexBase + psTexCount);
        psTex[ind - psTexBase] = tex_reg(regs, ofs);
      }
      break;
      case SHCOD_TEXTURE_VS:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= 0 && ind < MAX_VS_TEX);
        vsTex[ind] = tex_reg(regs, ofs);
        minVsTex = min<int>(ind, minVsTex);
        maxVsTex = max((int)ind, maxVsTex);
      }
      break;
      case SHCOD_IMM_REAL1: int_reg(regs, getOp2p1_8(opc)) = int(getOp2p2_16(opc)) << 16; break;
      case SHCOD_IMM_SVEC1:
      {
        int *reg = get_reg_ptr<int>(regs, getOp2p1_8(opc));
        int v = int(getOp2p2_16(opc)) << 16;
        reg[0] = reg[1] = reg[2] = reg[3] = v;
      }
      break;

      case SHCOD_MUL_REAL:
      {
        const uint32_t regDst = getOp3p1(opc);
        const uint32_t regL = getOp3p2(opc);
        const uint32_t regR = getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) * real_reg(regs, regR);
      }
      break;

      case SHCOD_MAKE_VEC:
      {
        const uint32_t ro = getOp3p1(opc);
        const uint32_t r1 = getOp3p2(opc);
        const uint32_t r2 = getOp3p3(opc);
        const uint32_t r3 = getData2p1(codp[1]);
        const uint32_t r4 = getData2p2(codp[1]);
        real *reg = get_reg_ptr<real>(regs, ro);
        reg[0] = real_reg(regs, r1);
        reg[1] = real_reg(regs, r2);
        reg[2] = real_reg(regs, r3);
        reg[3] = real_reg(regs, r4);
        codp++;
      }
      break;

      case SHCOD_IMM_REAL:
      {
        const uint32_t reg = getOp1p1(opc);
        real_reg(regs, reg) = *(const real *)&codp[1];
        codp++;
      }
      break;

      case SHCOD_SUB_REAL:
      {
        const uint32_t regDst = getOp3p1(opc);
        const uint32_t regL = getOp3p2(opc);
        const uint32_t regR = getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) - real_reg(regs, regR);
      }
      break;

      case SHCOD_ADD_REAL:
      {
        const uint32_t regDst = getOp3p1(opc);
        const uint32_t regL = getOp3p2(opc);
        const uint32_t regR = getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) + real_reg(regs, regR);
      }
      break;
      case SHCOD_COPY_REAL: int_reg(regs, getOp2p1(opc)) = int_reg(regs, getOp2p2(opc)); break;
      case SHCOD_COPY_VEC: color4_reg(regs, getOp2p1(opc)) = color4_reg(regs, getOp2p2(opc)); break;
      case SHCOD_ADD_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) + color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_SUB_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) - color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_MUL_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) * color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_IMM_VEC:
        memcpy(get_reg_ptr<real>(regs, getOp1p1(opc)), &codp[1], sizeof(real) * 4);
        codp += 4;
        break;
      case SHCOD_CALL_FUNCTION:
      {
        int functionName = getOp3p1(opc);
        int rOut = getOp3p2(opc);
        int paramCount = getOp3p3(opc);
        functional::callFunction((functional::FunctionId)functionName, rOut, codp + 1, regs);
        codp += paramCount;
      }
      break;
      case SHCOD_INVERSE:
      {
        real *r = get_reg_ptr<real>(regs, getOp2p1(opc));
        r[0] = -r[0];
        if (getOp2p2(opc) == 4)
          r[1] = -r[1], r[2] = -r[2], r[3] = -r[3];
      }
      break;
      case SHCOD_DIV_REAL:
        if (real rval = real_reg(regs, getOp3p3(opc)))
          real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) / rval;
#if DAGOR_DBGLEVEL > 0
        else
          SHOW_ERROR("%s: divide by zero [real] while exec shader code. stopped at operand #%d", context, codp - codpStart);
#endif
        break;
      case SHCOD_DIV_VEC:
      {
        Color4 rval = color4_reg(regs, getOp3p3(opc));

        for (int j = 0; j < 4; j++)
          if (rval[j] == 0)
          {
#if DAGOR_DBGLEVEL > 0
            SHOW_ERROR("%s: divide by zero [color4[%d]] while exec shader code. stopped at operand #%d", context, j, codp - codpStart);
#endif
            break;
          }
        color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) / rval;
      }
      break;
      case SHCOD_PACK_MATERIALS: multidrawCbuf = true; break;
      default:
        if (!special(*codp, regs, vsConsts, vscBase))
          return false;
    }
  }
  if (PLATFORM_HAS_BINDLESS)
  {
    block = create_bindless_state(bindlessResources.data(), bindlessResources.size(), vsConsts, vscCnt,
      make_span(addedBindlessTextures.data(), addedBindlessTextures.size()), static_cbuf, multidrawCbuf ? stcode_id : -1);
  }
  else
  {
    block = create_slot_textures_state(psTex, psTexBase, psTexCount, vsTex + minVsTex, minVsTex, max((int)0, maxVsTex - minVsTex + 1),
      vsConsts, vscCnt, static_cbuf);
  }
  (*lock_fn)();
  block.stateIdx = shaders::render_states::create(state);
  return true;
}
