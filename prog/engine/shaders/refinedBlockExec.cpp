// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <drv/3d/dag_bindless.h>
#include <shaders/shLimits.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <shaders/shFunc.h>
#include <EASTL/bit.h>

#include "shadersBinaryData.h"
#include "shRegs.h"
#include "shStateBlock.h"
#include "refinedBlockExec.h"

namespace refined_block
{


static ScopedRefinedBlock emptyScopedBlock{PassBlockHandle::invalid};
static eastl::vector<FlushedVar> emptyFlushedVars;

static thread_local ScopedRefinedBlock *g_rb_scoped = &emptyScopedBlock;
static thread_local eastl::vector<FlushedVar> *g_rb_flushed_vars = &emptyFlushedVars;

ScopedRefinedBlock::ScopedRefinedBlock(const PassBlockHandle &block) : block(block) { g_rb_scoped = this; }
ScopedRefinedBlock::~ScopedRefinedBlock() { g_rb_scoped = &emptyScopedBlock; }

ScopedFlushedVarsCollector::ScopedFlushedVarsCollector(eastl::vector<FlushedVar> &vars) { g_rb_flushed_vars = &vars; }
ScopedFlushedVarsCollector::~ScopedFlushedVarsCollector() { g_rb_flushed_vars = &emptyFlushedVars; }

void exec_refined_block_stcode(dag::ConstSpan<int> cod, const PassBlockHandle &block, dag::Span<float> out_buf,
  eastl::vector<FlushedVar> &out_vars)
{
  using namespace shaderopcode;
  alignas(16) real vregs[MAX_TEMP_REGS];
  char *regs = (char *)vregs;
  RawBufferConstSetter setter(out_buf);

  out_vars.reserve(cod.size() / 2); // ~2 codes per var

  const int *__restrict codp = cod.data(), *__restrict codp_end = codp + cod.size();

  for (uint32_t opc; codp < codp_end; codp++)
    switch (getOp(opc = *codp))
    {
      case SHCOD_GET_GVEC: color4_reg(regs, getOp2p1(opc)) = stcode_get_from_block<Color4>(block, getOp2p2(opc)); break;
      case SHCOD_GET_GMAT44: float4x4_reg(regs, getOp2p1(opc)) = stcode_get_from_block<TMatrix4>(block, getOp2p2(opc)); break;
      case SHCOD_GET_GREAL: real_reg(regs, getOp2p1(opc)) = stcode_get_from_block<real>(block, getOp2p2(opc)); break;
      case SHCOD_GET_GINT: int_reg(regs, getOp2p1(opc)) = stcode_get_from_block<int>(block, getOp2p2(opc)); break;
      case SHCOD_GET_GINT_TOREAL: real_reg(regs, getOp2p1(opc)) = (real)stcode_get_from_block<int>(block, getOp2p2(opc)); break;
      case SHCOD_GET_GIVEC: ipoint4_reg(regs, getOp2p1(opc)) = stcode_get_from_block<IPoint4>(block, getOp2p2(opc)); break;
      case SHCOD_SET_CONST_PACKED:
        setter.setConst(STAGE_VS, getOpStageSlot_Reg(opc), get_reg_ptr<float>(regs, getOpStageSlot_Stage(opc)),
          getOpStageSlot_Slot(opc));
        break;
      case SHCOD_GET_GTEX: set_tex_reg(stcode_get_from_block<BaseTexture *>(block, getOp2p2(opc)), regs, getOp2p1(opc)); break;
      case SHCOD_GET_GBUF: buf_reg(regs, getOp2p1(opc)) = stcode_get_from_block<Sbuffer *>(block, getOp2p2(opc)); break;
      case SHCOD_TEXTURE:
        out_vars.push_back({STAGE_PS, int(getOp2p1(opc)), eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))});
        break;
      case SHCOD_TEXTURE_VS:
        out_vars.push_back({STAGE_VS, int(getOp2p1(opc)), eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))});
        break;
      case SHCOD_TEXTURE_CS:
        out_vars.push_back({STAGE_CS, int(getOp2p1(opc)), eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))});
        break;
      case SHCOD_BUFFER:
        out_vars.push_back({int(getOpStageSlot_Stage(opc)), int(getOpStageSlot_Slot(opc)), buf_reg(regs, getOpStageSlot_Reg(opc))});
        break;
      case SHCOD_REG_BINDLESS:
      {
        G_ASSERTF(PLATFORM_HAS_BINDLESS, "Bindless texture operation found in refined block stcode, but driver doesn't support it.");
        const uint32_t ind = getOp2p1(opc);
        const uint32_t ofs = getOp2p2(opc);
        BaseTexture *tex = eastl::get<BaseTexture *>(get_tex_reg(regs, ofs));
        const uint32_t bindlessIdx = get_or_allocate_bindless_tex(tex);
        setter.setConst(STAGE_VS, ind, &bindlessIdx, 1);
        out_vars.push_back({0, (int)bindlessIdx, VarValue{BindlessTexVar{tex}}});
      }
      break;
      case SHCOD_GLOB_SAMPLER:
        out_vars.push_back({int(getOpStageSlot_Stage(opc)), int(getOpStageSlot_Slot(opc)),
          stcode_get_from_block<d3d::SamplerHandle>(block, getOpStageSlot_Reg(opc))});
        break;
      case SHCOD_REG_BINDLESS_SAMPLER:
      {
        G_ASSERTF(PLATFORM_HAS_BINDLESS, "Bindless sampler in refined block stcode, but driver doesn't support it.");
        const uint32_t ind = getOp2p1(opc);
        const d3d::SamplerHandle smp = stcode_get_from_block<d3d::SamplerHandle>(block, getOp2p2(opc));
        const uint32_t bindlessIdx = d3d::register_bindless_sampler(smp);
        setter.setConst(STAGE_VS, ind, &bindlessIdx, 1);
        out_vars.push_back({0, (int)bindlessIdx, BindlessSamplerVar{smp}});
      }
      break;
      case SHCOD_CONST_BUFFER:
        out_vars.push_back(
          {int(getOpStageSlot_Stage(opc)), int(getOpStageSlot_Slot(opc)), CbufVar{buf_reg(regs, getOpStageSlot_Reg(opc))}});
        break;
      case SHCOD_GET_GTLAS:
        tlas_reg(regs, getOp2p1(opc)) = stcode_get_from_block<RaytraceTopAccelerationStructure *>(block, getOp2p2(opc));
        break;
      case SHCOD_TLAS:
        out_vars.push_back({int(getOpStageSlot_Stage(opc)), int(getOpStageSlot_Slot(opc)), tlas_reg(regs, getOpStageSlot_Reg(opc))});
        break;
      case SHCOD_RWTEX_CS:
        out_vars.push_back({STAGE_CS, int(getOp2p1(opc)), RWTexVar{eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))}});
        break;
      case SHCOD_RWTEX_PS:
        out_vars.push_back({STAGE_PS, int(getOp2p1(opc)), RWTexVar{eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))}});
        break;
      case SHCOD_RWTEX_VS:
        out_vars.push_back({STAGE_VS, int(getOp2p1(opc)), RWTexVar{eastl::get<BaseTexture *>(get_tex_reg(regs, getOp2p2(opc)))}});
        break;
      case SHCOD_RWBUF_CS: out_vars.push_back({STAGE_CS, int(getOp2p1(opc)), RWBufVar{buf_reg(regs, getOp2p2(opc))}}); break;
      case SHCOD_RWBUF_PS: out_vars.push_back({STAGE_PS, int(getOp2p1(opc)), RWBufVar{buf_reg(regs, getOp2p2(opc))}}); break;
      case SHCOD_RWBUF_VS: out_vars.push_back({STAGE_VS, int(getOp2p1(opc)), RWBufVar{buf_reg(regs, getOp2p2(opc))}}); break;

      // Arithmetic opcodes generated by computed #rb var expressions.
      case SHCOD_IMM_REAL1: int_reg(regs, getOp2p1_8(opc)) = int(getOp2p2_16(opc)) << 16; break;
      case SHCOD_IMM_SVEC1:
      {
        int *reg = get_reg_ptr<int>(regs, getOp2p1_8(opc));
        int v = int(getOp2p2_16(opc)) << 16;
        reg[0] = reg[1] = reg[2] = reg[3] = v;
      }
      break;
      case SHCOD_IMM_REAL:
        int_reg(regs, getOp1p1(opc)) = codp[1];
        codp++;
        break;
      case SHCOD_IMM_VEC:
        memcpy(get_reg_ptr<real>(regs, getOp1p1(opc)), &codp[1], sizeof(real) * 4);
        codp += 4;
        break;
      case SHCOD_MAKE_VEC:
      {
        real *reg = get_reg_ptr<real>(regs, getOp3p1(opc));
        reg[0] = real_reg(regs, getOp3p2(opc));
        reg[1] = real_reg(regs, getOp3p3(opc));
        reg[2] = real_reg(regs, getData2p1(codp[1]));
        reg[3] = real_reg(regs, getData2p2(codp[1]));
        codp++;
      }
      break;
      case SHCOD_MUL_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) * real_reg(regs, getOp3p3(opc)); break;
      case SHCOD_DIV_REAL:
        if (real rval = real_reg(regs, getOp3p3(opc)))
          real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) / rval;
        else
          DAG_FATAL("exec_refined_block_stcode: divide by zero [real] at index %d", codp - cod.data());
        break;
      case SHCOD_ADD_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) + real_reg(regs, getOp3p3(opc)); break;
      case SHCOD_SUB_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) - real_reg(regs, getOp3p3(opc)); break;
      case SHCOD_MUL_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) * color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_DIV_VEC:
      {
        Color4 rval = color4_reg(regs, getOp3p3(opc));
        for (int j = 0; j < 4; j++)
          if (rval[j] == 0.f)
            DAG_FATAL("exec_refined_block_stcode: divide by zero [color4[%d]] at index %d", j, codp - cod.data());
        color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) / rval;
      }
      break;
      case SHCOD_ADD_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) + color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_SUB_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) - color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_INVERSE:
      {
        real *r = get_reg_ptr<real>(regs, getOp2p1(opc));
        r[0] = -r[0];
        if (getOp2p2(opc) == 4)
          r[1] = -r[1], r[2] = -r[2], r[3] = -r[3];
      }
      break;
      case SHCOD_COPY_REAL: int_reg(regs, getOp2p1(opc)) = int_reg(regs, getOp2p2(opc)); break;
      case SHCOD_COPY_VEC: color4_reg(regs, getOp2p1(opc)) = color4_reg(regs, getOp2p2(opc)); break;
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
      case SHCOD_CALL_FUNCTION:
      {
        int functionName = getOpFunctionCall_FuncId(opc);
        int rOut = getOpFunctionCall_OutReg(opc);
        int paramCount = getOpFunctionCall_ArgCount(opc);
        functional::callFunction((functional::FunctionId)functionName, rOut, codp + 1, regs, &shBinDumpOwner());
        codp += paramCount;
      }
      break;

      default:
        DAG_FATAL("exec_refined_block_stcode: illegal instruction %u %s (index=%d)", getOp(opc), ShUtils::shcod_tokname(getOp(opc)),
          codp - cod.data());
    }
}

stcode::cpp::float4 rb_get_f4(int32_t gid)
{
  Color4 v = g_rb_scoped->getVar<Color4>(gid);
  return {v.r, v.g, v.b, v.a};
}

float rb_get_real(int32_t gid) { return g_rb_scoped->getVar<float>(gid); }

int32_t rb_get_int(int32_t gid) { return g_rb_scoped->getVar<int>(gid); }

void rb_get_mat44(int32_t gid, stcode::cpp::float4x4 *out)
{
  TMatrix4 m = g_rb_scoped->getVar<TMatrix4>(gid);
  memcpy(out, &m, sizeof(TMatrix4));
}

void *rb_get_tex(int32_t gid) { return g_rb_scoped->getVar<BaseTexture *>(gid); }

void *rb_get_buf(int32_t gid) { return g_rb_scoped->getVar<Sbuffer *>(gid); }

void rb_flush_tex(int32_t stage, int32_t slot, void *tex)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, static_cast<BaseTexture *>(tex)}); //-V522
}

void rb_flush_buf(int32_t stage, int32_t slot, void *buf)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, static_cast<Sbuffer *>(buf)}); //-V522
}

uint32_t rb_alloc_bindless(void *tex) { return g_rb_scoped->getOrAllocateBindlessTex(static_cast<BaseTexture *>(tex)); }

void rb_flush_bindless_tex(uint32_t bi, void *tex)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({0, (int)bi, VarValue{BindlessTexVar{static_cast<BaseTexture *>(tex)}}}); //-V522
}

stcode::cpp::int4 rb_get_ivec(int32_t gid)
{
  IPoint4 v = g_rb_scoped->getVar<IPoint4>(gid);
  stcode::cpp::int4 out;
  memcpy(&out, &v, sizeof(IPoint4));
  return out;
}


uint64_t rb_get_sampler(int32_t gid) { return eastl::to_underlying(g_rb_scoped->getVar<d3d::SamplerHandle>(gid)); }

void *rb_get_tlas(int32_t gid) { return g_rb_scoped->getVar<RaytraceTopAccelerationStructure *>(gid); }

void rb_flush_cbuf(int32_t stage, int32_t slot, void *buf)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, CbufVar{static_cast<Sbuffer *>(buf)}}); //-V522
}

void rb_flush_sampler(int32_t stage, int32_t slot, uint64_t handle)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, d3d::SamplerHandle(handle)}); //-V522
}

uint32_t rb_alloc_bindless_sampler(uint64_t handle) { return d3d::register_bindless_sampler(d3d::SamplerHandle(handle)); }

void rb_flush_bindless_sampler(uint32_t bi, uint64_t handle)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({0, (int)bi, BindlessSamplerVar{d3d::SamplerHandle(handle)}}); //-V522
}

void rb_flush_tlas(int32_t stage, int32_t slot, void *tlas)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, static_cast<RaytraceTopAccelerationStructure *>(tlas)}); //-V522
}

void rb_flush_rwtex(int32_t stage, int32_t slot, void *tex)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, RWTexVar{static_cast<BaseTexture *>(tex)}}); //-V522
}

void rb_flush_rwbuf(int32_t stage, int32_t slot, void *buf)
{
  G_ASSERT(g_rb_flushed_vars);
  g_rb_flushed_vars->push_back({stage, slot, RWBufVar{static_cast<Sbuffer *>(buf)}}); //-V522
}

} // namespace refined_block
