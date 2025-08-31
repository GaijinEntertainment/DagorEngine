// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "scriptSElem.h"

#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/shLimits.h>
#include <shaders/shOpcode.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shFunc.h>
#include <shaders/shUtils.h>

#include "profileStcode.h"
#include "shRegs.h"
#include "stcode/compareStcode.h"


#ifndef S_DEBUG
#if defined(DEBUG_RENDER)
#define S_DEBUG debug
#else
__forceinline bool DEBUG_F(...) { return false; };
#define S_DEBUG 0 && DEBUG_F
#endif
#endif

#define VEC_ALIGN(v, a)
// #define VEC_ALIGN(v, a)  G_ASSERT((v & (a-1)) == 0)

#if MEASURE_STCODE_PERF
extern bool enable_measure_stcode_perf;
#include <perfMon/dag_cpuFreq.h>
#define MEASURE_STCODE_PERF_START volatile __int64 startTime = enable_measure_stcode_perf ? ref_time_ticks() : 0;
#define MEASURE_STCODE_PERF_END   \
  if (enable_measure_stcode_perf) \
  shaderbindump::add_exec_stcode_time(this_elem.shClass, get_time_usec(startTime))
#else
#define MEASURE_STCODE_PERF_START
#define MEASURE_STCODE_PERF_END
#endif

#if DAGOR_DBGLEVEL > 0
static void scripted_shader_element_default_before_resource_used_callback(const ShaderElement *selem, const D3dResource *,
  const char *) {};
void (*scripted_shader_element_on_before_resource_used)(const ShaderElement *selem, const D3dResource *,
  const char *) = &scripted_shader_element_default_before_resource_used_callback;
#else
static void scripted_shader_element_on_before_resource_used(const ShaderElement *, const D3dResource *, const char *) {}
#endif

static __forceinline void exec_stcode(uint16_t stcodeId, const shaderbindump::ShaderCode::Pass *__restrict code_cp,
  const ScriptedShaderElement &__restrict this_elem)
{
  alignas(16) real vpr_const[64][4];
  alignas(16) real fsh_const[64][4];
  alignas(16) real vregs[MAX_TEMP_REGS];
  char *regs = (char *)vregs;
  uint64_t vpr_c_mask = 0, fsh_c_mask = 0;

#define SHL1(y) 1ull << (y)
#define SHLF(y) 0xFull << (y)

  DYNSTCODE_PROFILE_BEGIN();

  MEASURE_STCODE_PERF_START;
  const vec4f *tm_world_c = NULL, *tm_lview_c = NULL;

  const uint8_t *vars = this_elem.getVars();
  dag::ConstSpan<int> cod = shBinDump().stcode[stcodeId];
  const int *__restrict codp = cod.data(), *__restrict codp_end = codp + cod.size();
  for (; codp < codp_end; codp++)
  {
    const uint32_t opc = *codp;

    switch (shaderopcode::getOp(opc))
    {
      case SHCOD_GET_GVEC:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        color4_reg(regs, ro) = shBinDumpOwner().globVarsState.get<Color4>(index);
      }
      break;
      case SHCOD_GET_GMAT44:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        float4x4_reg(regs, ro) = shBinDumpOwner().globVarsState.get<TMatrix4>(index);
      }
      break;
      case SHCOD_VPR_CONST:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        VEC_ALIGN(ofs, 4);
        if (ind < countof(vpr_const))
        {
          v_st(&vpr_const[ind], v_ld(get_reg_ptr<float>(regs, ofs)));
          vpr_c_mask |= SHL1(ind);
        }
        else
        {
          d3d::set_vs_const(ind, get_reg_ptr<float>(regs, ofs), 1);
        }

        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_VS, ind,
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, ofs), 1);
      }
      break;
      case SHCOD_FSH_CONST:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        VEC_ALIGN(ofs, 4);
        if (ind < countof(fsh_const))
        {
          v_st(&fsh_const[ind], v_ld(get_reg_ptr<float>(regs, ofs)));
          fsh_c_mask |= SHL1(ind);
        }
        else
        {
          d3d::set_ps_const(ind, get_reg_ptr<float>(regs, ofs), 1);
        }

        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_PS, ind,
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, ofs), 1);
      }
      break;
      case SHCOD_CS_CONST:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        VEC_ALIGN(ofs, 4);
        d3d::set_cs_const(ind, get_reg_ptr<float>(regs, ofs), 1);

        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_CS, ind,
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, ofs), 1);
      }
      break;
      case SHCOD_IMM_REAL1: int_reg(regs, shaderopcode::getOp2p1_8(opc)) = int(shaderopcode::getOp2p2_16(opc)) << 16; break;
      case SHCOD_IMM_SVEC1:
      {
        int *reg = get_reg_ptr<int>(regs, shaderopcode::getOp2p1_8(opc));
        int v = int(shaderopcode::getOp2p2_16(opc)) << 16;
        reg[0] = reg[1] = reg[2] = reg[3] = v;
      }
      break;
      case SHCOD_GET_GREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        real_reg(regs, ro) = shBinDumpOwner().globVarsState.get<real>(index);
      }
      break;
      case SHCOD_MUL_REAL:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) * real_reg(regs, regR);
      }
      break;
      case SHCOD_MAKE_VEC:
      {
        const uint32_t ro = shaderopcode::getOp3p1(opc);
        const uint32_t r1 = shaderopcode::getOp3p2(opc);
        const uint32_t r2 = shaderopcode::getOp3p3(opc);
        const uint32_t r3 = shaderopcode::getData2p1(codp[1]);
        const uint32_t r4 = shaderopcode::getData2p2(codp[1]);
        real *reg = get_reg_ptr<real>(regs, ro);
        reg[0] = real_reg(regs, r1);
        reg[1] = real_reg(regs, r2);
        reg[2] = real_reg(regs, r3);
        reg[3] = real_reg(regs, r4);
        codp++;
      }
      break;
      case SHCOD_GET_VEC:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        set_vec_reg(v_ldu((const float *)&vars[ofs]), regs, ro);
      }
      break;
      case SHCOD_IMM_REAL:
      {
        const uint32_t reg = shaderopcode::getOp1p1(opc);
        real_reg(regs, reg) = *(const real *)&codp[1];
        codp++;
      }
      break;
      case SHCOD_GET_REAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        real_reg(regs, ro) = *(real *)&vars[ofs];
      }
      break;
      case SHCOD_TEXTURE:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        TEXTUREID tid = tex_reg(regs, ofs);
        mark_managed_tex_lfu(tid, this_elem.tex_level);
        S_DEBUG("ind=%d ofs=%d tid=0x%X", ind, ofs, unsigned(tid));
        BaseTexture *tex = D3dResManagerData::getBaseTex(tid);
        scripted_shader_element_on_before_resource_used(&this_elem, tex, this_elem.shClass.name.data());
        d3d::set_tex(this_elem.stageDest, ind, tex);

        stcode::dbg::record_set_tex(stcode::dbg::RecordType::REFERENCE, this_elem.stageDest, ind, tex);
      }
      break;
      case SHCOD_GLOB_SAMPLER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t id = shaderopcode::getOpStageSlot_Reg(opc);
        d3d::SamplerHandle smp = shBinDumpOwner().globVarsState.get<d3d::SamplerHandle>(id);

        d3d::set_sampler(stage, slot, smp);

        stcode::dbg::record_set_sampler(stcode::dbg::RecordType::REFERENCE, stage, slot, smp);
      }
      break;
      case SHCOD_SAMPLER:
      {
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t ofs = shaderopcode::getOpStageSlot_Reg(opc);
        d3d::SamplerHandle smp = *(d3d::SamplerHandle *)&vars[ofs];

        d3d::set_sampler(this_elem.stageDest, slot, smp);

        stcode::dbg::record_set_sampler(stcode::dbg::RecordType::REFERENCE, this_elem.stageDest, slot, smp);
      }
      break;
      case SHCOD_TEXTURE_VS:
      {
        TEXTUREID tid = tex_reg(regs, shaderopcode::getOp2p2(opc));
        mark_managed_tex_lfu(tid, this_elem.tex_level);
        BaseTexture *tex = D3dResManagerData::getBaseTex(tid);
        scripted_shader_element_on_before_resource_used(&this_elem, tex, this_elem.shClass.name.data());
        d3d::set_tex(STAGE_VS, shaderopcode::getOp2p1(opc), tex);

        stcode::dbg::record_set_tex(stcode::dbg::RecordType::REFERENCE, STAGE_VS, shaderopcode::getOp2p1(opc), tex);
      }
      break;
      case SHCOD_BUFFER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t ofs = shaderopcode::getOpStageSlot_Reg(opc);
        Sbuffer *buf = buf_reg(regs, ofs);
        S_DEBUG("buf: stage = %d slot=%d ofs=%d buf=%X", stage, slot, ofs, buf);
        scripted_shader_element_on_before_resource_used(&this_elem, buf, this_elem.shClass.name.data());
        d3d::set_buffer(stage, slot, buf);

        stcode::dbg::record_set_buf(stcode::dbg::RecordType::REFERENCE, stage, slot, buf);
      }
      break;
      case SHCOD_CONST_BUFFER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t ofs = shaderopcode::getOpStageSlot_Reg(opc);
        Sbuffer *buf = buf_reg(regs, ofs);
        S_DEBUG("cb: stage = %d slot=%d ofs=%d buf=%X", stage, slot, ofs, buf);
        scripted_shader_element_on_before_resource_used(&this_elem, buf, this_elem.shClass.name.data());
        d3d::set_const_buffer(stage, slot, buf);

        stcode::dbg::record_set_const_buf(stcode::dbg::RecordType::REFERENCE, stage, slot, buf);
      }
      break;
      case SHCOD_TLAS:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t ofs = shaderopcode::getOpStageSlot_Reg(opc);
        RaytraceTopAccelerationStructure *tlas = tlas_reg(regs, ofs);
        S_DEBUG("tlas: stage = %d slot=%d ofs=%d tlas=%X", stage, slot, ofs, tlas);
#if D3D_HAS_RAY_TRACING
        d3d::set_top_acceleration_structure(ShaderStage(stage), slot, tlas);
#else
        logerr("%s(%d): SHCOD_TLAS ignored for shader %s", __FUNCTION__, stcodeId, (const char *)this_elem.shClass.name);
#endif

        stcode::dbg::record_set_tlas(stcode::dbg::RecordType::REFERENCE, stage, slot, tlas);
      }
      break;
      case SHCOD_RWTEX:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        TEXTUREID tid = tex_reg(regs, ofs);
        BaseTexture *tex = D3dResManagerData::getBaseTex(tid);
        scripted_shader_element_on_before_resource_used(&this_elem, tex, this_elem.shClass.name.data());
        S_DEBUG("rwtex: ind=%d ofs=%d tex=%X", ind, ofs, tex);
        d3d::set_rwtex(this_elem.stageDest, ind, tex, 0, 0);

        stcode::dbg::record_set_rwtex(stcode::dbg::RecordType::REFERENCE, this_elem.stageDest, ind, tex);
      }
      break;
      case SHCOD_RWBUF:
      {
        const uint32_t ind = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        Sbuffer *buf = buf_reg(regs, ofs);
        scripted_shader_element_on_before_resource_used(&this_elem, buf, this_elem.shClass.name.data());
        S_DEBUG("rwbuf: ind=%d ofs=%d buf=%X", ind, ofs, buf);
        d3d::set_rwbuffer(this_elem.stageDest, ind, buf);

        stcode::dbg::record_set_rwbuf(stcode::dbg::RecordType::REFERENCE, this_elem.stageDest, ind, buf);
      }
      break;
      case SHCOD_LVIEW:
      {
        real *reg = get_reg_ptr<real>(regs, shaderopcode::getOp2p1(opc));
        if (!tm_lview_c)
          tm_lview_c = &d3d::gettm_cref(TM_VIEW2LOCAL).col0;
        v_stu(reg, tm_lview_c[shaderopcode::getOp2p2(opc)]);
      }
      break;
      case SHCOD_GET_GTEX:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        tex_reg(regs, reg) = shBinDumpOwner().globVarsState.get<shaders_internal::Tex>(index).texId;
      }
      break;
      case SHCOD_GET_GBUF:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        Sbuffer *&buf = buf_reg(regs, reg);
        buf = shBinDumpOwner().globVarsState.get<shaders_internal::Buf>(index).buf;
      }
      break;
      case SHCOD_GET_GTLAS:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        RaytraceTopAccelerationStructure *&tlas = tlas_reg(regs, reg);
        tlas = shBinDumpOwner().globVarsState.get<RaytraceTopAccelerationStructure *>(index);
      }
      break;
      case SHCOD_G_TM:
      {
        int ind = shaderopcode::getOp2p2_16(opc);
        TMatrix4_vec4 gtm;
        switch (shaderopcode::getOp2p1_8(opc))
        {
          case P1_SHCOD_G_TM_GLOBTM: d3d::getglobtm(gtm); break;
          case P1_SHCOD_G_TM_PROJTM: d3d::gettm(TM_PROJ, &gtm); break;
          case P1_SHCOD_G_TM_VIEWPROJTM:
          {
            TMatrix4_vec4 v, p;
            d3d::gettm(TM_VIEW, &v);
            d3d::gettm(TM_PROJ, &p);
            gtm = v * p;
          }
          break;
          default: G_ASSERTF(0, "SHCOD_G_TM(%d, %d)", shaderopcode::getOp2p1_8(opc), ind);
        }

        process_tm_for_drv_consts(gtm);

        if (ind <= countof(vpr_const) - 4)
        {
          memcpy(&vpr_const[ind], gtm[0], sizeof(real) * 4 * 4);
          vpr_c_mask |= SHLF(ind);
        }
        else
        {
          d3d::set_vs_const(ind, gtm[0], 4);
        }

        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_VS, ind, (stcode::cpp::float4 *)gtm[0], 4);
      }
      break;
      case SHCOD_DIV_REAL:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        if (int_reg(regs, regR) == 0)
        {
#if DAGOR_DBGLEVEL > 0
          debug("shclass: %s", (const char *)this_elem.shClass.name);
          ShUtils::shcod_dump(cod, &shBinDump().globVars, &shBinDumpOwner().globVarsState, &this_elem.shClass.localVars,
            this_elem.code.stVarMap);
          DAG_FATAL("divide by zero [real] while exec shader code. stopped at operand #%d", codp - cod.data());
#endif
          real_reg(regs, regDst) = real_reg(regs, regL);
        }
        else
          real_reg(regs, regDst) = real_reg(regs, regL) / real_reg(regs, regR);
      }
      break;
      case SHCOD_CALL_FUNCTION:
      {
        int functionName = shaderopcode::getOp3p1(opc);
        int rOut = shaderopcode::getOp3p2(opc);
        int paramCount = shaderopcode::getOp3p3(opc);
        functional::callFunction((functional::FunctionId)functionName, rOut, codp + 1, regs);
        codp += paramCount;
      }
      break;
      case SHCOD_GET_TEX:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        ScriptedShaderElement::Tex &t = *(ScriptedShaderElement::Tex *)&vars[ofs];
        tex_reg(regs, reg) = t.texId;
        t.get();
      }
      break;
      case SHCOD_TMWORLD:
      {
        real *reg = get_reg_ptr<real>(regs, shaderopcode::getOp2p1(opc));
        if (!tm_world_c)
          tm_world_c = &d3d::gettm_cref(TM_WORLD).col0;
        v_stu(reg, tm_world_c[shaderopcode::getOp2p2(opc)]);
      }
      break;
      case SHCOD_COPY_REAL: int_reg(regs, shaderopcode::getOp2p1(opc)) = int_reg(regs, shaderopcode::getOp2p2(opc)); break;
      case SHCOD_COPY_VEC: color4_reg(regs, shaderopcode::getOp2p1(opc)) = color4_reg(regs, shaderopcode::getOp2p2(opc)); break;
      case SHCOD_SUB_REAL:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) - real_reg(regs, regR);
      }
      break;
      case SHCOD_ADD_REAL:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        real_reg(regs, regDst) = real_reg(regs, regL) + real_reg(regs, regR);
      }
      break;
      case SHCOD_SUB_VEC:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        set_vec_reg(v_sub(get_vec_reg(regs, regL), get_vec_reg(regs, regR)), regs, regDst);
      }
      break;
      case SHCOD_MUL_VEC:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        set_vec_reg(v_mul(get_vec_reg(regs, regL), get_vec_reg(regs, regR)), regs, regDst);
      }
      break;
      case SHCOD_IMM_VEC:
      {
        const uint32_t ro = shaderopcode::getOp1p1(opc);
        set_vec_reg(v_ldu((const float *)&codp[1]), regs, ro);
        codp += 4;
      }
      break;
      case SHCOD_INT_TOREAL:
      {
        const uint32_t regDst = shaderopcode::getOp1p1(opc);
        const uint32_t regSrc = shaderopcode::getOp2p2(opc);
        real_reg(regs, regDst) = real(int_reg(regs, regSrc));
      }
      break;
      case SHCOD_IVEC_TOREAL:
      {
        const uint32_t regDst = shaderopcode::getOp1p1(opc);
        const uint32_t regSrc = shaderopcode::getOp2p2(opc);
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
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        real_reg(regs, ro) = *(int *)&vars[ofs];
      }
      break;
      case SHCOD_GET_IVEC_TOREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        real *reg = get_reg_ptr<real>(regs, ro);
        reg[0] = *(int *)&vars[ofs];
        reg[1] = *(int *)&vars[ofs + 1];
        reg[2] = *(int *)&vars[ofs + 2];
        reg[3] = *(int *)&vars[ofs + 3];
      }
      break;
      case SHCOD_GET_INT:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t ofs = shaderopcode::getOp2p2(opc);
        int_reg(regs, ro) = *(int *)&vars[ofs];
      }
      break;
      case SHCOD_INVERSE:
      {
        real *r = get_reg_ptr<real>(regs, shaderopcode::getOp2p1(opc));
        r[0] = -r[0];
        if (shaderopcode::getOp2p2(opc) == 4)
          r[1] = -r[1], r[2] = -r[2], r[3] = -r[3];
      }
      break;
      case SHCOD_ADD_VEC:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        set_vec_reg(v_add(get_vec_reg(regs, regL), get_vec_reg(regs, regR)), regs, regDst);
      }
      break;
      case SHCOD_DIV_VEC:
      {
        const uint32_t regDst = shaderopcode::getOp3p1(opc);
        const uint32_t regL = shaderopcode::getOp3p2(opc);
        const uint32_t regR = shaderopcode::getOp3p3(opc);
        vec4f lval = get_vec_reg(regs, regL);
        vec4f rval = get_vec_reg(regs, regR);
        rval = v_sel(rval, V_C_ONE, v_cmp_eq(rval, v_zero()));
        set_vec_reg(v_div(lval, rval), regs, regDst);

#if DAGOR_DBGLEVEL > 0
        Color4 rvalS = color4_reg(regs, regR);

        for (int j = 0; j < 4; j++)
          if (rvalS[j] == 0)
          {
            debug("shclass: %s", (const char *)this_elem.shClass.name);
            ShUtils::shcod_dump(cod, &shBinDump().globVars, &shBinDumpOwner().globVarsState, &this_elem.shClass.localVars,
              this_elem.code.stVarMap);
            DAG_FATAL("divide by zero [color4[%d]] while exec shader code. stopped at operand #%d", j, codp - cod.data());
          }
#endif
      }
      break;

      case SHCOD_GET_GINT:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        int_reg(regs, ro) = shBinDumpOwner().globVarsState.get<int>(index);
      }
      break;
      case SHCOD_GET_GINT_TOREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        real_reg(regs, ro) = shBinDumpOwner().globVarsState.get<int>(index);
      }
      break;
      case SHCOD_GET_GIVEC_TOREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        const IPoint4 &ivec = shBinDumpOwner().globVarsState.get<IPoint4>(index);
        color4_reg(regs, ro) = Color4(ivec.x, ivec.y, ivec.z, ivec.w);
      }
      break;
      default:
        DAG_FATAL("%s(%d): illegal instruction %u %s (index=%d)", __FUNCTION__, stcodeId, shaderopcode::getOp(opc),
          ShUtils::shcod_tokname(shaderopcode::getOp(opc)), codp - cod.data());
    }
  }

  while (fsh_c_mask)
  {
    auto start = __ctz_unsafe(fsh_c_mask);
    auto tmp = fsh_c_mask + (1ull << start);
    auto end = tmp ? __ctz_unsafe(tmp) : 64;
    fsh_c_mask &= tmp ? (eastl::numeric_limits<uint64_t>::max() << end) : 0;
    d3d::set_ps_const(start, fsh_const[start], end - start);
  }

  while (vpr_c_mask)
  {
    auto start = __ctz_unsafe(vpr_c_mask);
    auto tmp = vpr_c_mask + (1ull << start);
    auto end = tmp ? __ctz_unsafe(tmp) : 64;
    vpr_c_mask &= tmp ? (eastl::numeric_limits<uint64_t>::max() << end) : 0;
    d3d::set_vs_const(start, vpr_const[start], end - start);
  }

  MEASURE_STCODE_PERF_END;

  DYNSTCODE_PROFILE_END();
}
