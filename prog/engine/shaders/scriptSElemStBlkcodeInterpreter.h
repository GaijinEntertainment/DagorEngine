// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_renderStates.h>
#include <EASTL/optional.h>
#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shFunc.h>
#include <shaders/shUtils.h>
#include <util/dag_finally.h>

#include "shStateBlock.h"
#include "shRegs.h"
#include "profileStcode.h"
#include "stcode/compareStcode.h"
#include "stcode/bindlessStblkcodeContext.h"

#if !_TARGET_STATIC_LIB
#define SHOW_ERROR(fmt, ...) G_ASSERTF(0, fmt, ##__VA_ARGS__);
#else
#define SHOW_ERROR(fmt, ...) logerr(fmt, ##__VA_ARGS__);
#endif

static __forceinline eastl::optional<ShaderStateBlock> execute_st_block_code(const int *__restrict &codp,
  const int *__restrict codp_end, const char *context, const uint8_t *vars, int stcode_id)
{
  ShaderStateBlock result;
  G_UNUSED(context);
#if DAGOR_DBGLEVEL > 0
  const int *__restrict codpStart = codp;
#endif
  using namespace shaderopcode;
  const uint32_t opc_ = *codp;
  G_ASSERT(getOp(opc_) == SHCOD_STATIC_BLOCK || getOp(opc_) == SHCOD_STATIC_MULTIDRAW_BLOCK);

  STBLKCODE_PROFILE_BEGIN();
  FINALLY([] { STBLKCODE_PROFILE_END(); });

  char *regs = POW2_ALIGN_PTR(alloca(shBinDump().maxRegSize * 4 + 16), 16, char);

  bool multidrawCbuf = getOp(opc_) == SHCOD_STATIC_MULTIDRAW_BLOCK;
  int constCnt = getOp1p1(opc_);
  auto [vsTexBase, vsSamplerBaseAndExtentPacked, psTexBase, psSamplerBaseAndExtentPacked] = unpackData4(codp[1]);
  codp += 2;

  stcode::dbg::record_multidraw_support(stcode::dbg::RecordType::REFERENCE, multidrawCbuf);

  auto vsSlotRange =
    SlotTexturesRangeInfo{vsTexBase, uint8_t(vsSamplerBaseAndExtentPacked >> 4), uint8_t(vsSamplerBaseAndExtentPacked & 0xF)};
  auto psSlotRange =
    SlotTexturesRangeInfo{psTexBase, uint8_t(psSamplerBaseAndExtentPacked >> 4), uint8_t(psSamplerBaseAndExtentPacked & 0xF)};

  TEXTUREID *psTex = (TEXTUREID *)alloca(psSlotRange.count * sizeof(TEXTUREID)), //-V630 (alloca for classes)
    *vsTex = (TEXTUREID *)alloca(vsSlotRange.count * sizeof(TEXTUREID));         //-V630 (alloca for classes)
  Point4 *consts = (Point4 *)alloca(constCnt * sizeof(Point4));                  //-V630 (alloca for classes)

  BindlessStblkcodeContext bindlessCtx{};

  for (; codp < codp_end; codp++)
  {
    const uint32_t opc = *codp;
    switch (getOp(opc))
    {
      case SHCOD_VPR_CONST:
      case SHCOD_FSH_CONST:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= 0 && ind < constCnt);
        consts[ind] = *get_reg_ptr<Point4>(regs, ofs);
        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE,
          STAGE_VS /* arbitrary, but has to match stcodeCallbacksImpl.cpp */, ind, &consts[ind], 1);
      }
      break;
      case SHCOD_REG_BINDLESS:
      {
        G_ASSERTF(PLATFORM_HAS_BINDLESS, "Bindless texture operation was found in stcode, while current driver doesn't support it.");
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= 0 && ind < constCnt);
        BindlessTexId texId = find_or_add_bindless_tex(tex_reg(regs, ofs), bindlessCtx.addedBindlessTextures);
        bindlessCtx.bindlessResources.push_back(BindlessConstParams{ind, texId});
        consts[ind] = Point4(bitwise_cast<float, int>(-1), bitwise_cast<float, int>(-1), 0, 0);
        stcode::dbg::record_reg_bindless(stcode::dbg::RecordType::REFERENCE, ind, tex_reg(regs, ofs));
        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE,
          STAGE_VS /* arbitrary, but has to match stcodeCallbacksImpl.cpp */, ind, &consts[ind], 1);
      }
      break;
      case SHCOD_TEXTURE:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= psSlotRange.texBase && ind < psSlotRange.texBase + psSlotRange.count);
        psTex[ind - psSlotRange.texBase] = tex_reg(regs, ofs);
        stcode::dbg::record_set_static_tex(stcode::dbg::RecordType::REFERENCE, STAGE_PS, ind, psTex[ind - psSlotRange.texBase]);
      }
      break;
      case SHCOD_TEXTURE_VS:
      {
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        G_ASSERT(ind >= vsSlotRange.texBase && ind < vsSlotRange.texBase + vsSlotRange.count);
        vsTex[ind - vsSlotRange.texBase] = tex_reg(regs, ofs);
        stcode::dbg::record_set_static_tex(stcode::dbg::RecordType::REFERENCE, STAGE_VS, ind, vsTex[ind - vsSlotRange.texBase]);
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

      case SHCOD_GET_INT:
      {
        const uint32_t ro = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        int_reg(regs, ro) = *(int *)&vars[ofs];
      }
      break;
      case SHCOD_GET_VEC:
      {
        const uint32_t ro = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        real *reg = get_reg_ptr<real>(regs, ro);
        memcpy(reg, &vars[ofs], sizeof(real) * 4);
      }
      break;

      case SHCOD_GET_REAL:
      {
        const uint32_t ro = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        real_reg(regs, ro) = *(real *)&vars[ofs];
      }
      break;

      case SHCOD_GET_TEX:
      {
        const uint32_t reg = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        shaders_internal::Tex &t = *(shaders_internal::Tex *)&vars[ofs];
        tex_reg(regs, reg) = t.texId == D3DRESID(D3DRESID::INVALID_ID2) ? BAD_TEXTUREID : t.texId;
        t.get();
      }
      break;

      case SHCOD_INT_TOREAL:
      {
        const uint32_t regDst = getOp1p1(opc);
        const uint32_t regSrc = getOp2p2(opc);
        real_reg(regs, regDst) = real(int_reg(regs, regSrc));
      }
      break;
      case SHCOD_IVEC_TOREAL:
      {
        const uint32_t regDst = getOp1p1(opc);
        const uint32_t regSrc = getOp2p2(opc);
        real *reg = get_reg_ptr<real>(regs, regDst);
        const int *src = get_reg_ptr<int>(regs, regSrc);
        reg[0] = src[0];
        reg[1] = src[1];
        reg[2] = src[2];
        reg[3] = src[3];
      }
      break;

      case SHCOD_GET_INT_TOREAL:
      {
        const uint32_t ro = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        real_reg(regs, ro) = *(int *)&vars[ofs];
      }
      break;
      case SHCOD_GET_IVEC_TOREAL:
      {
        const uint32_t ro = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        real *reg = get_reg_ptr<real>(regs, ro);
        reg[0] = *(int *)&vars[ofs];
        reg[1] = *(int *)&vars[ofs + 1];
        reg[2] = *(int *)&vars[ofs + 2];
        reg[3] = *(int *)&vars[ofs + 3];
      }
      break;


      default:
#if DAGOR_DBGLEVEL > 0
        logerr("recordEmulatedStateBlock: illegal instruction %u %s (index=%d)", getOp(opc), ShUtils::shcod_tokname(getOp(opc)),
          codp - codpStart);
#endif
        return eastl::nullopt;
    }
  }

  if (PLATFORM_HAS_BINDLESS)
  {
    return create_bindless_state(bindlessCtx.bindlessResources.data(), bindlessCtx.bindlessResources.size(), consts, constCnt,
      make_span(bindlessCtx.addedBindlessTextures.data(), bindlessCtx.addedBindlessTextures.size()), constCnt > 0,
      multidrawCbuf ? stcode_id : -1);
  }
  else
  {
    return create_slot_textures_state(psTex, psSlotRange, vsTex, vsSlotRange, consts, constCnt, constCnt > 0);
  }
}
