// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/shLimits.h>
#include "shadersBinaryData.h"
#include "mapBinarySearch.h"
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_stcode.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shFunc.h>
#include <shaders/shUtils.h>
#include <shaders/dag_shaderMesh.h>
#include "constRemap.h"
#include "constSetter.h"
#include <3d/dag_render.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_renderStates.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4more.h>
#include <generic/dag_smallTab.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_statDrv.h>
#include "shRegs.h"
#include "shStateBlock.h"
#include <generic/dag_tab.h>
#include <EASTL/array.h>
#include <generic/dag_relocatableFixedVector.h>

#include "stcode/compareStcode.h"
#include "profileStcode.h"

static Tab<ShaderStateBlockId> block_init_code(inimem);

static eastl::array<UniqueBuf, 3> globalConstBuffers;

class D3dConstSetter : public shaders_internal::ConstSetter
{
public:
  virtual bool setConst(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs) override
  {
    return d3d::set_const(stage, reg_base, data, num_regs);
  }
};

static constexpr const char *stageNames[] = {"cs", "ps", "vs"};

class ArrayConstSetter : public shaders_internal::ConstSetter
{
public:
  eastl::array<dag::RelocatableFixedVector<Color4, 4, true, framemem_allocator, uint32_t, false>, 3> consts;

  virtual bool setConst(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs) override
  {
    auto &values = consts[stage];
    if (values.size() < reg_base + num_regs)
      values.resize(reg_base + num_regs);
    memcpy(&values[reg_base], data, sizeof(Color4) * num_regs);
    return true;
  }

  void upload()
  {
    for (int stage = STAGE_CS; stage <= STAGE_VS; stage++)
    {
      const auto &constData = consts[stage];
      if (constData.empty() && (stage != STAGE_CS || consts[STAGE_PS].empty()))
        continue;
      UniqueBuf &constBuffer = globalConstBuffers[stage];
      const int bufferSize = stage == STAGE_CS ? consts[STAGE_PS].size() + consts[stage].size() : consts[stage].size();
      if (!constBuffer)
      {
        String bufferName(32, "global_const_buf_%s", stageNames[stage]);
        constBuffer = dag::buffers::create_one_frame_cb(bufferSize, bufferName.c_str());
      }
      if (auto destinationData = lock_sbuffer<Color4>(constBuffer.getBuf(), 0, bufferSize, VBLOCK_DISCARD | VBLOCK_WRITEONLY))
      {
        int updateOffset = 0;
        if (stage == STAGE_CS && !consts[STAGE_PS].empty())
        {
          updateOffset = consts[STAGE_PS].size();
          destinationData.updateDataRange(0, consts[STAGE_PS].data(), updateOffset);
        }
        if (!constData.empty())
          destinationData.updateDataRange(updateOffset, constData.data(), constData.size());
      }
      d3d::set_const_buffer(stage, 2, constBuffer.getBuf());
    }
  }

  void dump()
  {
    for (int stage = STAGE_CS; stage <= STAGE_VS; stage++)
    {
      debug("%s consts (%d)", stageNames[stage], consts[stage].size());
      dump_consts((ShaderStage)stage);
    }
  }

private:
  void dump_consts(ShaderStage stage)
  {
    for (const Color4 &reg : consts[stage])
      debug("  REG: {%f}, {%f}, {%f}, {%f}\n", reg.r, reg.g, reg.b, reg.a);
  }
};

static void exec_stcode(dag::ConstSpan<int> cod, int block_id, shaders_internal::ConstSetter *const_setter);
static ShaderStateBlockId record_state_block(const int *__restrict, dag::ConstSpan<int> cod, char *regs);

static eastl::array<int, ShaderGlobal::LAYER_OBJECT + 1> current_blocks = {-1, -1, -1};
static int current_global_const = -1;

void shaders_internal::close_shader_block_stateblocks(bool final)
{
  // debug("clear_state_blocks for ShaderBlocks: %d", block_init_code.size());
  shaders_internal::BlockAutoLock autoLock;
  for (int i = 0; i < block_init_code.size(); i++)
  {
    if (block_init_code[i] != ShaderStateBlockId::Invalid)
      ShaderStateBlock::delBlock(block_init_code[i]);
  }
  if (final)
    clear_and_shrink(block_init_code);
}

void shaders_internal::close_global_constbuffers()
{
  for (UniqueBuf &buf : globalConstBuffers)
    buf.close();
}

int ShaderGlobal::getBlockId(const char *block_name, int layer)
{
  using shaderbindump::blockStateWord;
  using shaderbindump::nullBlock;

  int id = mapbinarysearch::bfindStrId(shBinDump().blockNameMap, block_name);
  if (id != -1 && layer != -1)
  {
    G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
    G_ASSERT(nullBlock[layer]);
    if (nullBlock[layer]->uidMask != shBinDump().blocks[id].uidMask)
      DAG_FATAL("block <%s> doesn't belong to layer #%d, block mask=%04X, required=%04X", block_name, layer,
        shBinDump().blocks[id].uidMask, nullBlock[layer]->uidMask);
  }
  if (!block_init_code.size() && shBinDump().blockNameMap.size())
    block_init_code.assign(shBinDump().blockNameMap.size(), ShaderStateBlockId::Invalid);
  return id;
}

const char *ShaderGlobal::getBlockName(int block_id)
{
  if (block_id == -1)
    return "NULL";
  else
  {
    const int blockNameId = shBinDump().blocks[block_id].nameId;
    const char *blockName = shBinDump().blockNameMap[blockNameId].c_str();
    return blockName;
  }
}

static void execute_chosen_stcode(uint16_t stcodeId, int blockId, shaders_internal::ConstSetter *c_setter)
{
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV("execute_chosen_stcode__block");

  G_ASSERT(stcodeId < shBinDump().stcode.size());

#if CPP_STCODE

#if VALIDATE_CPP_STCODE
  stcode::dbg::reset();
#endif

#if STCODE_RUNTIME_CHOICE
  if (stcode::execution_mode() == stcode::ExecutionMode::BYTECODE)
    exec_stcode(shBinDump().stcode[stcodeId], blockId, c_setter);
  else
#endif
  {
    stcode::ScopedCustomConstSetter csetOverride(c_setter);
    stcode::run_routine(stcodeId, nullptr, false);
  }

#if VALIDATE_CPP_STCODE
  // Collect records
  if (stcode::execution_mode() == stcode::ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    exec_stcode(shBinDump().stcode[stcodeId], blockId, c_setter);

  stcode::dbg::validate_accumulated_records(stcodeId, "<block>");
#endif

#else

  exec_stcode(shBinDump().stcode[stcodeId], blockId, c_setter);

#endif
}

static void setBlockPrivate(int block_id, int layer)
{
  using shaderbindump::blockStateWord;
  using shaderbindump::nullBlock;

#if DAGOR_DBGLEVEL > 0
#define LOGERR DAG_FATAL
#elif DAGOR_FORCE_LOGS
#define LOGERR logerr
#else
#define LOGERR(...) \
  do                \
    ;               \
  while (0)
#endif

  if (layer == ShaderGlobal::LAYER_GLOBAL_CONST)
  {
    if (block_id == -1)
    {
      current_global_const = -1;
      for (UniqueBuf &buf : globalConstBuffers)
        buf.close();
    }
    else
    {
      const shaderbindump::ShaderBlock &b = shBinDump().blocks[block_id];
#if DAGOR_DBGLEVEL > 0
      const char *bName = (const char *)shBinDump().blockNameMap[b.nameId];
      if (strcmp(bName, "global_const_block") != 0)
        LOGERR("block <%s> is not the 'global_const_block'!\n", bName);
#endif
      if (b.stcodeId != -1)
      {
        ArrayConstSetter setter;
        execute_chosen_stcode(b.stcodeId, block_id, &setter);
        setter.upload();
      }
      current_global_const = block_id;
    }
  }
  else
  {
    if (block_id == -1)
    {
      G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
      G_ASSERT(nullBlock[layer]);
      current_blocks[layer] = -1;
      for (; layer < shaderbindump::MAX_BLOCK_LAYERS; layer++)
      {
        current_blocks[layer] = -1;
        blockStateWord = nullBlock[layer]->modifyBlockStateWord(blockStateWord);
      }
    }
    else
    {
      const shaderbindump::ShaderBlock &b = shBinDump().blocks[block_id];

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
      if (!b.isBlockSupported(blockStateWord))
      {
        int b_frame = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_FRAME);
        int b_scene = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_SCENE);
        int b_obj = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_OBJECT);
        LOGERR("block <%s> cannot be set when stateWord=%04X\n\ncurrent blocks=(%s:%s:%s)\n",
          (const char *)shBinDump().blockNameMap[block_id], blockStateWord,
          b_frame >= 0 ? (const char *)shBinDump().blockNameMap[b_frame] : "NULL",
          b_scene >= 0 ? (const char *)shBinDump().blockNameMap[b_scene] : "NULL",
          b_obj >= 0 ? (const char *)shBinDump().blockNameMap[b_obj] : "NULL");
      }
#endif

      blockStateWord = b.modifyBlockStateWord(blockStateWord);
      if (b.stcodeId != -1)
      {
        D3dConstSetter setter;
        execute_chosen_stcode(b.stcodeId, block_id, &setter);
      }
      current_blocks[layer] = block_id;

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
      for (int i = 0; i < shaderbindump::MAX_BLOCK_LAYERS; i++)
        if (nullBlock[i] && nullBlock[i]->uidMask > b.uidMask)
          blockStateWord |= nullBlock[i]->uidMask;

      if (layer != -1)
      {
        if (!(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS))
          LOGERR("shader block layer is invalid = %d", layer);
        else if (!nullBlock[layer])
          LOGERR("shader block no nullblock in layer = %d", layer);
        else if (nullBlock[layer]->uidMask != b.uidMask)
          LOGERR("block <%s> doesn't belong to layer #%d, block mask=%04X, required=%04X",
            (const char *)shBinDump().blockNameMap[block_id], layer, b.uidMask, nullBlock[layer]->uidMask);
      }
#endif
    }
  }
#undef LOGERR
  // debug("ShaderGlobal::setBlock(%d,%d) -> %08X", block_id, layer, blockStateWord);
  // debug_dump_stack(NULL, 2);
}

int ShaderGlobal::getBlock(int layer)
{
  if (!shBinDump().blocks.size())
    return -1;

  return current_blocks[layer];
}

void ShaderGlobal::setBlock(int block_id, int layer, bool invalidate_cached_stblk)
{
  if (!shBinDump().blocks.size())
    return;

  if (layer < LAYER_GLOBAL_CONST && invalidate_cached_stblk)
  {
    d3d::set_program(BAD_PROGRAM); // hot fix crashes on RX5700XT
    ShaderElement::invalidate_cached_state_block();
  }
  setBlockPrivate(block_id, layer);
}

bool ShaderGlobal::checkBlockCompatible(int block_id, int layer)
{
  using shaderbindump::blockStateWord;
  using shaderbindump::nullBlock;

  if (block_id == -1)
    return true;

  const shaderbindump::ShaderBlock &b = shBinDump().blocks[block_id];

  if (layer != -1)
  {
    G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
    G_ASSERT(nullBlock[layer]);
    if (nullBlock[layer]->uidMask != b.uidMask)
      return false;
  }
  return b.isBlockSupported(blockStateWord);
}

unsigned ShaderGlobal::getCurBlockStateWord() { return shaderbindump::blockStateWord; }

int ShaderGlobal::decodeBlock(unsigned block_state_word, int layer)
{
  using shaderbindump::nullBlock;

  G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
  if (!nullBlock[layer])
    return -2;

  int mask = nullBlock[layer]->uidMask, val = (block_state_word & mask);
  if (val == nullBlock[layer]->uidVal)
    return -1;

  dag::ConstSpan<shaderbindump::ShaderBlock> b = shBinDump().blocks;
  for (int i = 0; i < b.size(); i++)
    if (b[i].uidMask == mask && b[i].uidVal == val)
      return i;
  return -3;
}

bool ShaderGlobal::enableAutoBlockChange(bool enable)
{
  bool prev = shaderbindump::autoBlockStateWordChange;
  shaderbindump::autoBlockStateWordChange = enable;
  return prev;
}

void ShaderGlobal::changeStateWord(unsigned new_block_state_word)
{
  using shaderbindump::nullBlock;
  dag::ConstSpan<shaderbindump::ShaderBlock> b = shBinDump().blocks;

  for (int layer = 0; layer < shaderbindump::MAX_BLOCK_LAYERS; layer++)
  {
    if (!nullBlock[layer])
      return;

    int mask = nullBlock[layer]->uidMask;
    if ((shaderbindump::blockStateWord & mask) == (new_block_state_word & mask))
      continue;

    int val = (new_block_state_word & mask);
    if (val == nullBlock[layer]->uidVal)
    {
      setBlockPrivate(-1, layer);
      return;
    }

    for (int i = 0; i < b.size(); i++)
      if (b[i].uidMask == mask && b[i].uidVal == val)
      {
        setBlockPrivate(i, layer);
        break;
      }
  }
}

static const char *find_block(dag::ConstSpan<int> cod)
{
  for (int i = 0; i < shBinDump().blockNameMap.size(); i++)
    if (shBinDump().blocks[i].stcodeId != -1 && shBinDump().stcode[shBinDump().blocks[i].stcodeId].begin() == cod.begin() &&
        shBinDump().stcode[shBinDump().blocks[i].stcodeId].size() == cod.size())
      return (const char *)shBinDump().blockNameMap[shBinDump().blocks[i].nameId];
  return "unknown block";
}

#if DAGOR_DBGLEVEL > 0
static void shader_block_default_before_resource_used_callback(const D3dResource *, const char *){};
void (*shader_block_on_before_resource_used)(const D3dResource *, const char *) = &shader_block_default_before_resource_used_callback;
#else
static void shader_block_on_before_resource_used(const D3dResource *, const char *){};
#endif

__forceinline static void exec_stcode(dag::ConstSpan<int> cod, int block_id, shaders_internal::ConstSetter *const_setter)
{
  using namespace shaderopcode;
  alignas(16) real vregs[MAX_TEMP_REGS];
  char *regs = (char *)vregs;
  const vec4f *tm_world_c = NULL, *tm_lview_c = NULL;
  const int *__restrict codp = cod.data(), *__restrict codp_end = codp + cod.size();
  if (getOp(*codp) == SHCOD_BLK_ICODE_LEN)
  {
    int len = getOp1p1(*codp);
    codp++;
    unsigned opc = *codp;
    if (getOp(opc) == SHCOD_STATIC_BLOCK)
    {
      ShaderStateBlockId blockId = block_init_code[block_id];
      if (blockId == ShaderStateBlockId::Invalid)
        blockId = block_init_code[block_id] = record_state_block(codp, make_span_const(cod).subspan(3, len - 2), regs);

      if (blockId != ShaderStateBlockId::Invalid)
        ShaderStateBlock::blocks[blockId].apply();
    }
    codp += len;
  }

#if DAGOR_DBGLEVEL > 0
  const char *blockName = ShaderGlobal::getBlockName(block_id);
#else
  const char *blockName = "";
#endif

  STCODE_PROFILE_BEGIN();

  for (uint32_t opc; codp < codp_end; codp++)
    switch (getOp(opc = *codp))
    {
      case SHCOD_GET_GVEC: color4_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<Color4>(getOp2p2(opc)); break;
      case SHCOD_GET_GMAT44: float4x4_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<TMatrix4>(getOp2p2(opc)); break;
      case SHCOD_VPR_CONST:
        const_setter->setVsConst(getOp2p1(opc), get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_VS, getOp2p1(opc),
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        break;
      case SHCOD_FSH_CONST:
        const_setter->setPsConst(getOp2p1(opc), get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_PS, getOp2p1(opc),
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        break;
      case SHCOD_CS_CONST:
        const_setter->setCsConst(getOp2p1(opc), get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_CS, getOp2p1(opc),
          (stcode::cpp::float4 *)get_reg_ptr<float>(regs, getOp2p2(opc)), 1);
        break;
      case SHCOD_TEXTURE:
      {
        TEXTUREID tid = tex_reg(regs, getOp2p2(opc));
        mark_managed_tex_lfu(tid);
        BaseTexture *tex = D3dResManagerData::getBaseTex(tid);
        shader_block_on_before_resource_used(tex, blockName);
        d3d::set_tex(STAGE_PS, shaderopcode::getOp2p1(opc), tex);

        stcode::dbg::record_set_tex(stcode::dbg::RecordType::REFERENCE, STAGE_PS, shaderopcode::getOp2p1(opc), tex);
      }
      break;
      case SHCOD_GLOB_SAMPLER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t id = shaderopcode::getOpStageSlot_Reg(opc);
        d3d::SamplerHandle smp = shBinDump().globVars.get<d3d::SamplerHandle>(id);
        if (smp != d3d::INVALID_SAMPLER_HANDLE)
          d3d::set_sampler(stage, slot, smp);

        stcode::dbg::record_set_sampler(stcode::dbg::RecordType::REFERENCE, stage, slot, smp);
      }
      break;
      case SHCOD_TEXTURE_VS:
      {
        TEXTUREID tid = tex_reg(regs, getOp2p2(opc));
        mark_managed_tex_lfu(tid);
        BaseTexture *tex = D3dResManagerData::getBaseTex(tid);
        shader_block_on_before_resource_used(tex, blockName);
        d3d::set_tex(STAGE_VS, shaderopcode::getOp2p1(opc), tex);

        stcode::dbg::record_set_tex(stcode::dbg::RecordType::REFERENCE, STAGE_VS, shaderopcode::getOp2p1(opc), tex);
      }
      break;
      case SHCOD_BUFFER:
      {
        Sbuffer *buf = buf_reg(regs, getOpStageSlot_Reg(opc));
        shader_block_on_before_resource_used(buf, blockName);
        d3d::set_buffer(getOpStageSlot_Stage(opc), getOpStageSlot_Slot(opc), buf);

        stcode::dbg::record_set_buf(stcode::dbg::RecordType::REFERENCE, getOpStageSlot_Stage(opc), getOpStageSlot_Slot(opc), buf);
      }
      break;
      case SHCOD_CONST_BUFFER:
      {
        d3d::set_const_buffer(getOpStageSlot_Stage(opc), getOpStageSlot_Slot(opc), buf_reg(regs, getOpStageSlot_Reg(opc)));
        stcode::dbg::record_set_const_buf(stcode::dbg::RecordType::REFERENCE, getOpStageSlot_Stage(opc), getOpStageSlot_Slot(opc),
          buf_reg(regs, getOpStageSlot_Reg(opc)));
      }
      break;
      case SHCOD_TLAS:
      {
#if D3D_HAS_RAY_TRACING
        d3d::set_top_acceleration_structure(ShaderStage(getOpStageSlot_Stage(opc)), getOpStageSlot_Slot(opc),
          tlas_reg(regs, getOpStageSlot_Reg(opc)));
#else
        G_ASSERT_LOG(0, "%s: SHCOD_TLAS ignored", find_block(cod));
#endif
        stcode::dbg::record_set_tlas(stcode::dbg::RecordType::REFERENCE, getOpStageSlot_Stage(opc), getOpStageSlot_Slot(opc),
          tlas_reg(regs, getOpStageSlot_Reg(opc)));
        break;
      }
      case SHCOD_IMM_REAL1: int_reg(regs, getOp2p1_8(opc)) = int(getOp2p2_16(opc)) << 16; break;
      case SHCOD_IMM_SVEC1:
      {
        int *reg = get_reg_ptr<int>(regs, getOp2p1_8(opc));
        int v = int(getOp2p2_16(opc)) << 16;
        reg[0] = reg[1] = reg[2] = reg[3] = v;
      }
      break;
      case SHCOD_GET_GREAL: real_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<real>(getOp2p2(opc)); break;
      case SHCOD_MUL_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) * real_reg(regs, getOp3p3(opc)); break;
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
      case SHCOD_IMM_REAL:
        int_reg(regs, getOp1p1(opc)) = codp[1];
        codp++;
        break;
      case SHCOD_LVIEW:
        if (!tm_lview_c)
          tm_lview_c = &d3d::gettm_cref(TM_VIEW2LOCAL).col0;
        v_stu(get_reg_ptr<real>(regs, getOp2p1(opc)), tm_lview_c[getOp2p2(opc)]);
        break;
      case SHCOD_GET_GTEX: tex_reg(regs, getOp2p1(opc)) = shBinDump().globVars.getTex(getOp2p2(opc)).texId; break;
      case SHCOD_GET_GBUF: buf_reg(regs, getOp2p1(opc)) = shBinDump().globVars.getBuf(getOp2p2(opc)).buf; break;
      case SHCOD_GET_GTLAS:
        tlas_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<RaytraceTopAccelerationStructure *>(getOp2p2(opc));
        break;
      case SHCOD_G_TM:
      {
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
          default: G_ASSERTF(0, "SHCOD_G_TM(%d, %d)", getOp2p1_8(opc), getOp2p2_16(opc));
        }
        process_tm_for_drv_consts(gtm);
        d3d::set_vs_const(getOp2p2_16(opc), gtm[0], 4);

        stcode::dbg::record_set_const(stcode::dbg::RecordType::REFERENCE, STAGE_VS, getOp2p2_16(opc), (stcode::cpp::float4 *)gtm[0],
          4);
      }
      break;
      case SHCOD_DIV_REAL:
        if (real rval = real_reg(regs, getOp3p3(opc)))
          real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) / rval;
        else
          G_ASSERT_LOG(0, "%s: divide by zero [real] while exec shader code. stopped at operand #%d", find_block(cod),
            codp - cod.data());
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
      case SHCOD_TMWORLD:
        if (!tm_world_c)
          tm_world_c = &d3d::gettm_cref(TM_WORLD).col0;
        v_stu(get_reg_ptr<real>(regs, getOp2p1(opc)), tm_world_c[getOp2p2(opc)]);
        break;
      case SHCOD_COPY_REAL: int_reg(regs, getOp2p1(opc)) = int_reg(regs, getOp2p2(opc)); break;
      case SHCOD_COPY_VEC: color4_reg(regs, getOp2p1(opc)) = color4_reg(regs, getOp2p2(opc)); break;
      case SHCOD_SUB_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) - real_reg(regs, getOp3p3(opc)); break;
      case SHCOD_ADD_REAL: real_reg(regs, getOp3p1(opc)) = real_reg(regs, getOp3p2(opc)) + real_reg(regs, getOp3p3(opc)); break;
      case SHCOD_SUB_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) - color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_MUL_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) * color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_IMM_VEC:
        memcpy(get_reg_ptr<real>(regs, getOp1p1(opc)), &codp[1], sizeof(real) * 4);
        codp += 4;
        break;

      // infrequentally used opcodes
      case SHCOD_INVERSE:
      {
        real *r = get_reg_ptr<real>(regs, getOp2p1(opc));
        r[0] = -r[0];
        if (getOp2p2(opc) == 4)
          r[1] = -r[1], r[2] = -r[2], r[3] = -r[3];
      }
      break;
      case SHCOD_ADD_VEC: color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) + color4_reg(regs, getOp3p3(opc)); break;
      case SHCOD_DIV_VEC:
      {
        Color4 rval = color4_reg(regs, getOp3p3(opc));

        for (int j = 0; j < 4; j++)
          if (rval[j] == 0)
          {
            G_ASSERT_LOG(0, "%s: divide by zero [color4[%d]] while exec shader code. stopped at operand #%d", find_block(cod), j,
              codp - cod.data());
            break;
          }
        color4_reg(regs, getOp3p1(opc)) = color4_reg(regs, getOp3p2(opc)) / rval;
      }
      break;

      case SHCOD_GET_GINT: int_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<int>(getOp2p2(opc)); break;
      case SHCOD_GET_GINT_TOREAL: real_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<int>(getOp2p2(opc)); break;
      case SHCOD_GET_GIVEC_TOREAL:
      {
        const IPoint4 &ivec = shBinDump().globVars.get<IPoint4>(shaderopcode::getOp2p2(opc));
        color4_reg(regs, shaderopcode::getOp2p1(opc)) = Color4(ivec.x, ivec.y, ivec.z, ivec.w);
      }
      break;

      default:
        DAG_FATAL("%s: exec_stcode: illegal instruction %u %s (index=%d)", find_block(cod), getOp(opc),
          ShUtils::shcod_tokname(getOp(opc)), codp - cod.data());
    }

  STCODE_PROFILE_END();
}

#include "stateBlockStCode.h"
static ShaderStateBlockId record_state_block(const int *__restrict cod_p, dag::ConstSpan<int> cod, char * /*regs*/)
{
  using namespace shaderopcode;
  // debug("record_state_block(%p, (%p,%d))", &block, cod.data(), cod.size());
  const vec4f *tm_world_c = NULL, *tm_lview_c = NULL;

  auto maybeShStateBlock = execute_st_block_code(cod_p, cod.data() + cod.size(), find_block(cod), -1 /*stcodeId*/,
    [&](const int &codAt, char *regs, Point4 *vsConsts, int vscBase) {
      uint32_t opc = codAt;
      switch (getOp(opc))
      {
        case SHCOD_GET_GVEC: color4_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<Color4>(getOp2p2(opc)); break;
        case SHCOD_GET_GREAL: real_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<real>(getOp2p2(opc)); break;
        case SHCOD_LVIEW:
          if (!tm_lview_c)
            tm_lview_c = &d3d::gettm_cref(TM_VIEW2LOCAL).col0;
          v_stu(get_reg_ptr<real>(regs, getOp2p1(opc)), tm_lview_c[getOp2p2(opc)]);
          break;
        case SHCOD_GET_GTEX: tex_reg(regs, getOp2p1(opc)) = shBinDump().globVars.getTex(getOp2p2(opc)).texId; break;
        case SHCOD_TMWORLD:
          if (!tm_world_c)
            tm_world_c = &d3d::gettm_cref(TM_WORLD).col0;
          v_stu(get_reg_ptr<real>(regs, getOp2p1(opc)), tm_world_c[getOp2p2(opc)]);
          break;

        case SHCOD_GET_GINT: int_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<int>(getOp2p2(opc)); break;
        case SHCOD_GET_GINT_TOREAL: real_reg(regs, getOp2p1(opc)) = shBinDump().globVars.get<int>(getOp2p2(opc)); break;
        case SHCOD_GET_GIVEC_TOREAL:
        {
          const IPoint4 &ivec = shBinDump().globVars.get<IPoint4>(getOp2p2(opc));
          color4_reg(regs, getOp2p1(opc)) = Color4(ivec.x, ivec.y, ivec.z, ivec.w);
        }
        break;

        case SHCOD_G_TM:
        {
          TMatrix4_vec4 &gtm = *(TMatrix4_vec4 *)&vsConsts[getOp2p2_16(opc) - vscBase];
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
            default: G_ASSERTF(0, "SHCOD_G_TM(%d, %d)", getOp2p1_8(opc), getOp2p2_16(opc));
          }
          process_tm_for_drv_consts(gtm);
        }
        break;
        default:
          logerr("%s: exec_stcode: illegal instruction %u %s (index=%d)", find_block(cod), getOp(opc),
            ShUtils::shcod_tokname(getOp(opc)), &codAt - cod.data());
          return false;
      }
      return true;
    });

  if (!maybeShStateBlock.has_value())
    return DEFAULT_SHADER_STATE_BLOCK_ID;

  return ShaderStateBlock::addBlock(eastl::move(*maybeShStateBlock), {});
}
