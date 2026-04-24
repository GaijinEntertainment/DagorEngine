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
#include <drv/3d/dag_rwResource.h>
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
#include <generic/dag_sort.h>
#include <generic/dag_enumerate.h>
#include <util/dag_oaHashNameMap.h>

#include "stcode/compareStcode.h"
#include "profileStcode.h"

static OAHashNameMap<false> g_sh_block_name_map;
static eastl::array<UniqueBuf, 3> global_const_buffers;

void ScriptedShadersGlobalData::initBlockIndexMap()
{
  auto const *dump = backing->getDump();
  G_ASSERT(dump->blocks.size() <= INT16_MAX);

  shaderBlockIndexMap.resize(max<size_t>(shaderBlockIndexMap.size(), dump->blocks.size()));
  eastl::fill(shaderBlockIndexMap.begin(), shaderBlockIndexMap.end(), -1);

  for (size_t internalId = 0; const auto &b : dump->blocks)
  {
    const int blockId = g_sh_block_name_map.addNameId(dump->blockNameMap[b.nameId].c_str());
    if (blockId >= shaderBlockIndexMap.size())
      shaderBlockIndexMap.resize(blockId + 1, -1);
    shaderBlockIndexMap[blockId] = int16_t(internalId);
    ++internalId;
  }
}

class D3dConstSetter : public shaders_internal::ConstSetter
{
public:
  virtual bool setConst(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs) override
  {
    return d3d::set_const(stage, reg_base, data, num_regs);
  }
};

static constexpr const char *STAGE_NAMES[] = {"cs", "ps", "vs"};

class ArrayConstSetter : public shaders_internal::ConstSetter
{
public:
  eastl::array<dag::RelocatableFixedVector<Color4, 4, true, framemem_allocator, uint32_t, false>, 3> consts;

  // @TODO: replace 2 calls to setConstImpl with having one global cbuf for the graphics pipeline.

  virtual bool setConst(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs) final
  {
    setConstImpl(stage, reg_base, data, num_regs);

    // @HACK-ish: global const buffer hlsl declaration is shared between ps and vs stage. After the compiler update the preshader
    // statements are filtered (relying on ODR) -- if a const is compiled in one stage, it is not compiled in the other. This makes it
    // necessary to flush all writes to both stages' constbuffers.
    if (stage != STAGE_CS)
    {
      ShaderStage otherStage = stage == STAGE_VS ? STAGE_PS : STAGE_VS;
      setConstImpl(otherStage, reg_base, data, num_regs);
    }

    return true;
  }

  void upload()
  {
    for (int stage = STAGE_CS; stage <= STAGE_VS; stage++)
    {
      const auto &constData = consts[stage];
      if (constData.empty() && (stage != STAGE_CS || consts[STAGE_PS].empty()))
        continue;
      UniqueBuf &constBuffer = global_const_buffers[stage];
      const int bufferSize = stage == STAGE_CS ? consts[STAGE_PS].size() + consts[stage].size() : consts[stage].size();
      if (!constBuffer)
      {
        String bufferName(32, "global_const_buf_%s", STAGE_NAMES[stage]);
        constBuffer = dag::buffers::create_one_frame_cb(bufferSize, bufferName.c_str(), RESTAG_ENGINE);
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
      debug("%s consts (%d)", STAGE_NAMES[stage], consts[stage].size());
      dump_consts((ShaderStage)stage);
    }
  }

private:
  void setConstImpl(ShaderStage stage, unsigned reg_base, const void *data, unsigned num_regs)
  {
    auto &values = consts[stage];
    if (values.size() < reg_base + num_regs)
      values.resize(reg_base + num_regs);
    memcpy(&values[reg_base], data, sizeof(Color4) * num_regs);
  }

  void dump_consts(ShaderStage stage)
  {
    for (const Color4 &reg : consts[stage])
      debug("  REG: {%f}, {%f}, {%f}, {%f}\n", reg.r, reg.g, reg.b, reg.a);
  }
};

static void exec_stcode(dag::ConstSpan<int> cod, int block_id, shaders_internal::ConstSetter *const_setter);

static eastl::array<ShaderBlockIds, ShaderGlobal::LAYER_OBJECT + 1> current_blocks{};
static ShaderBlockIds current_global_const{};

void shaders_internal::close_global_constbuffers()
{
  for (UniqueBuf &buf : global_const_buffers)
    buf.close();
}

static ShaderBlockIds get_block_ids_private(const char *block_name, int layer)
{
  using shaderbindump::blockStateWord;
  using shaderbindump::nullBlock;

  auto &gdata = shGlobalData();
  if (!gdata.backing)
    return {};

  auto *dump = gdata.backing->getDump();
  if (!dump)
    return {};

  int blockId = g_sh_block_name_map.getNameId(block_name);
  if (blockId == -1)
    return {};
  int internalId = gdata.shaderBlockIndexMap[blockId];
  if (internalId == -1)
    return {};

  if (layer != -1)
  {
    G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
    G_ASSERT(nullBlock[layer]);
    if (nullBlock[layer]->uidMask != dump->blocks[internalId].uidMask)
      DAG_FATAL("block <%s> doesn't belong to layer #%d, block mask=%04X, required=%04X", block_name, layer,
        dump->blocks[internalId].uidMask, nullBlock[layer]->uidMask);
  }
  return {blockId, internalId};
}

int ShaderGlobal::getBlockId(const char *block_name, int layer) { return get_block_ids_private(block_name, layer).blockId; }

const char *ShaderGlobal::getBlockName(int block_id)
{
  if (block_id == -1)
    return "NULL";
  else
    return g_sh_block_name_map.getName(block_id);
}

static void execute_chosen_stcode(uint16_t stcode_id, uint16_t ext_stcode_id, int internal_block_id,
  shaders_internal::ConstSetter *c_setter)
{
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV("execute_chosen_stcode__block");

  G_ASSERT(stcode_id < shBinDump().stcode.size());

#if CPP_STCODE

  auto const &ctx = shBinDumpOwner().stcodeCtx;

#if VALIDATE_CPP_STCODE
  stcode::dbg::reset();
#endif

  if (DAGOR_UNLIKELY(ctx.dynstcodeExMode == stcode::ExecutionMode::BYTECODE))
  {
    exec_stcode(shBinDump().stcode[stcode_id], internal_block_id, c_setter);
  }
  else
  {
    stcode::ScopedCustomConstSetter csetOverride(c_setter);
    stcode::run_dyn_routine(ctx, ext_stcode_id, nullptr);
  }

#if VALIDATE_CPP_STCODE
  // Collect records
  if (ctx.dynstcodeExMode == stcode::ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    exec_stcode(shBinDump().stcode[stcode_id], internal_block_id, c_setter);

  stcode::dbg::validate_accumulated_records(ext_stcode_id, stcode_id,
    shBinDump().blockNameMap[shBinDump().blocks[internal_block_id].nameId], true);
#endif

#else

  exec_stcode(shBinDump().stcode[stcode_id], internal_block_id, c_setter);

#endif
}

static void set_block_private(ShaderBlockIds block_ids, int layer)
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
    if (block_ids.internalBlockId == -1)
    {
      current_global_const = {};
      for (UniqueBuf &buf : global_const_buffers)
        buf.close();
    }
    else
    {
      const shaderbindump::ShaderBlock &b = shBinDump().blocks[block_ids.internalBlockId];
#if DAGOR_DBGLEVEL > 0
      const char *bName = (const char *)shBinDump().blockNameMap[b.nameId];
      if (strcmp(bName, "global_const_block") != 0)
        LOGERR("block <%s> is not the 'global_const_block'!\n", bName);
#endif
      if (b.stcodeId != -1)
      {
        ArrayConstSetter setter;
        execute_chosen_stcode(b.stcodeId, b.cppStcodeId, block_ids.internalBlockId, &setter);
        setter.upload();
      }
      current_global_const = block_ids;
    }
  }
  else
  {
    if (block_ids.internalBlockId == -1)
    {
      G_ASSERT(layer >= 0 && layer < shaderbindump::MAX_BLOCK_LAYERS);
      G_ASSERT(nullBlock[layer]);
      current_blocks[layer] = {};
      for (; layer < shaderbindump::MAX_BLOCK_LAYERS; layer++)
      {
        current_blocks[layer] = {};
        blockStateWord = nullBlock[layer]->modifyBlockStateWord(blockStateWord);
      }
    }
    else
    {
      const shaderbindump::ShaderBlock &b = shBinDump().blocks[block_ids.internalBlockId];

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
      if (!b.isBlockSupported(blockStateWord))
      {
        int b_frame = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_FRAME);
        int b_scene = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_SCENE);
        int b_obj = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_OBJECT);
        LOGERR("block <%s> cannot be set when stateWord=%04X\n\ncurrent blocks=(%s:%s:%s)\n",
          (const char *)shBinDump().blockNameMap[b.nameId], blockStateWord,
          b_frame >= 0 ? (const char *)shBinDump().blockNameMap[b_frame] : "NULL",
          b_scene >= 0 ? (const char *)shBinDump().blockNameMap[b_scene] : "NULL",
          b_obj >= 0 ? (const char *)shBinDump().blockNameMap[b_obj] : "NULL");
      }
#endif

      blockStateWord = b.modifyBlockStateWord(blockStateWord);
      if (b.stcodeId != -1)
      {
        D3dConstSetter setter;
        execute_chosen_stcode(b.stcodeId, b.cppStcodeId, block_ids.internalBlockId, &setter);
      }
      current_blocks[layer] = block_ids;

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
            (const char *)shBinDump().blockNameMap[b.nameId], layer, b.uidMask, nullBlock[layer]->uidMask);
      }
#endif
    }
  }
#undef LOGERR
}

static ShaderBlockIds get_block_private(int layer)
{
  if (!shBinDump().blocks.size())
    return {};

  if (layer == ShaderGlobal::LAYER_GLOBAL_CONST)
    return current_global_const;
  else
    return current_blocks[layer];
}

int ShaderGlobal::getBlock(int layer) { return get_block_private(layer).blockId; }

void ShaderGlobal::setBlock(int block_id, int layer, bool invalidate_cached_stblk)
{
  if (shBinDump().blocks.empty())
    return;

  if (layer < LAYER_GLOBAL_CONST && invalidate_cached_stblk)
  {
    d3d::set_program(BAD_PROGRAM); // hot fix crashes on RX5700XT
    ShaderElement::invalidate_cached_state_block();
  }

  ShaderBlockIds ids{block_id, -1};
  if (block_id >= 0)
  {
    ids.internalBlockId = shGlobalData().shaderBlockIndexMap[block_id];
    G_ASSERTF_AND_DO(ids.internalBlockId >= 0, return, "Trying to set stale block <%s> with id=%d to layer #%d",
      getBlockName(block_id), block_id, layer);
  }

  set_block_private(ids, layer);
}

bool ShaderGlobal::checkBlockCompatible(int block_id, int layer)
{
  using shaderbindump::blockStateWord;
  using shaderbindump::nullBlock;

  if (block_id == -1)
    return true;

  int internalBlockId = shGlobalData().shaderBlockIndexMap[block_id];
  G_ASSERTF_RETURN(internalBlockId >= 0, false, "Trying to check stale block compat <%s> with id=%d to layer #%d",
    getBlockName(block_id), block_id, layer);

  const shaderbindump::ShaderBlock &b = shBinDump().blocks[internalBlockId];

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

  const auto &gdata = shGlobalData();
  const auto *dump = gdata.backing->getDump();

  for (auto [blockId, internalId] : enumerate(gdata.shaderBlockIndexMap))
  {
    if (internalId == -1)
      continue;
    const auto &b = dump->blocks[internalId];
    if (b.uidMask == mask && b.uidVal == val)
      return blockId;
  }

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
  const auto &gdata = shGlobalData();
  const auto *dump = gdata.backing->getDump();

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
      set_block_private({}, layer);
      return;
    }

    for (auto [blockId, internalId] : enumerate(gdata.shaderBlockIndexMap))
    {
      if (internalId == -1)
        continue;
      const auto &b = dump->blocks[internalId];
      if (b.uidMask == mask && b.uidVal == val)
      {
        set_block_private({int(blockId), internalId}, layer);
        break;
      }
    }
  }
}

void ShaderBlockIdHolder::resolve()
{
  if (!IShaderBindumpReloadListener::staticInitDone) // no sense in trying to resolve, shader dump is not loaded
    return;
  ids = get_block_ids_private(name, layer);
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
static void shader_block_default_before_resource_used_callback(const D3dResource *, const char *) {}
void (*shader_block_on_before_resource_used)(const D3dResource *, const char *) = &shader_block_default_before_resource_used_callback;
#else
static void shader_block_on_before_resource_used(const D3dResource *, const char *) {}
#endif

__forceinline static void exec_stcode(dag::ConstSpan<int> cod, int internal_block_id, shaders_internal::ConstSetter *const_setter)
{
  using namespace shaderopcode;
  alignas(16) real vregs[MAX_TEMP_REGS];
  char *regs = (char *)vregs;
  const vec4f *tm_world_c = NULL, *tm_lview_c = NULL;
  const int *__restrict codp = cod.data(), *__restrict codp_end = codp + cod.size();

#if DAGOR_DBGLEVEL > 0
  const char *blockName = shBinDump().blockNameMap[shBinDump().blocks[internal_block_id].nameId].c_str();
#else
  const char *blockName = "";
#endif

  DYNSTCODE_PROFILE_BEGIN();

  auto doSetTex = [&](uint32_t opc, ShaderStage stage) {
    const auto texData = get_tex_reg(regs, getOp2p2(opc));
    BaseTexture *tex = nullptr;
    if (const auto *tid = eastl::get_if<TEXTUREID>(&texData))
    {
      mark_managed_tex_lfu(*tid);
      tex = D3dResManagerData::getBaseTex(*tid);
    }
    else
    {
      tex = eastl::get<BaseTexture *>(texData);
    }

    shader_block_on_before_resource_used(tex, blockName);
    d3d::set_tex(stage, shaderopcode::getOp2p1(opc), tex);

    stcode::dbg::record_set_tex(stcode::dbg::RecordType::REFERENCE, stage, shaderopcode::getOp2p1(opc), tex);
  };

  auto doSetRwtex = [&](uint32_t opc, ShaderStage stage) {
    const uint32_t ind = shaderopcode::getOp2p1(opc);
    const uint32_t ofs = shaderopcode::getOp2p2(opc);
    const auto texData = get_tex_reg(regs, ofs);

    BaseTexture *tex = nullptr;
    const TEXTUREID *tid = eastl::get_if<TEXTUREID>(&texData);
    if (tid)
    {
      tex = D3dResManagerData::getBaseTex(*tid);
    }
    else
    {
      tex = eastl::get<BaseTexture *>(texData);
    }

    d3d::set_rwtex(stage, ind, tex, 0, 0);
    stcode::dbg::record_set_rwtex(stcode::dbg::RecordType::REFERENCE, stage, ind, tex);
  };
  auto doSetRwbuf = [&](uint32_t opc, ShaderStage stage) {
    const uint32_t ind = shaderopcode::getOp2p1(opc);
    const uint32_t ofs = shaderopcode::getOp2p2(opc);
    Sbuffer *buf = buf_reg(regs, ofs);
    d3d::set_rwbuffer(stage, ind, buf);
    stcode::dbg::record_set_rwbuf(stcode::dbg::RecordType::REFERENCE, stage, ind, buf);
  };

  auto const &gvarsState = shGlobalData().globVarsState;

  for (uint32_t opc; codp < codp_end; codp++)
    switch (getOp(opc = *codp))
    {
      case SHCOD_GET_GVEC: color4_reg(regs, getOp2p1(opc)) = gvarsState.get<Color4>(getOp2p2(opc)); break;
      case SHCOD_GET_GMAT44: float4x4_reg(regs, getOp2p1(opc)) = gvarsState.get<TMatrix4>(getOp2p2(opc)); break;
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
      case SHCOD_TEXTURE: doSetTex(opc, STAGE_PS); break;
      case SHCOD_GLOB_SAMPLER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(opc);
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t id = shaderopcode::getOpStageSlot_Reg(opc);
        d3d::SamplerHandle smp = gvarsState.get<d3d::SamplerHandle>(id);
        d3d::set_sampler(stage, slot, smp);

        stcode::dbg::record_set_sampler(stcode::dbg::RecordType::REFERENCE, stage, slot, smp);
      }
      break;
      case SHCOD_TEXTURE_VS: doSetTex(opc, STAGE_VS); break;
      case SHCOD_TEXTURE_CS: doSetTex(opc, STAGE_CS); break;
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
      case SHCOD_RWTEX_CS: doSetRwtex(opc, STAGE_CS); break;
      case SHCOD_RWTEX_PS: doSetRwtex(opc, STAGE_PS); break;
      case SHCOD_RWTEX_VS: doSetRwtex(opc, STAGE_VS); break;
      case SHCOD_RWBUF_CS: doSetRwbuf(opc, STAGE_CS); break;
      case SHCOD_RWBUF_PS: doSetRwbuf(opc, STAGE_PS); break;
      case SHCOD_RWBUF_VS: doSetRwbuf(opc, STAGE_VS); break;
      case SHCOD_IMM_REAL1: int_reg(regs, getOp2p1_8(opc)) = int(getOp2p2_16(opc)) << 16; break;
      case SHCOD_IMM_SVEC1:
      {
        int *reg = get_reg_ptr<int>(regs, getOp2p1_8(opc));
        int v = int(getOp2p2_16(opc)) << 16;
        reg[0] = reg[1] = reg[2] = reg[3] = v;
      }
      break;
      case SHCOD_GET_GREAL: real_reg(regs, getOp2p1(opc)) = gvarsState.get<real>(getOp2p2(opc)); break;
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
      case SHCOD_GET_GTEX:
      {
        const auto &tex = gvarsState.get<shaders_internal::Tex>(getOp2p2(opc));
        if (tex.isTextureManaged())
        {
          set_tex_reg(tex.texId, regs, getOp2p1(opc));
        }
        else
        {
          set_tex_reg(tex.tex, regs, getOp2p1(opc));
        }
      }
      break;
      case SHCOD_GET_GBUF: buf_reg(regs, getOp2p1(opc)) = gvarsState.get<shaders_internal::Buf>(getOp2p2(opc)).buf; break;
      case SHCOD_GET_GTLAS: tlas_reg(regs, getOp2p1(opc)) = gvarsState.get<RaytraceTopAccelerationStructure *>(getOp2p2(opc)); break;
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
        int functionName = getOpFunctionCall_FuncId(opc);
        int rOut = getOpFunctionCall_OutReg(opc);
        int paramCount = getOpFunctionCall_ArgCount(opc);
        functional::callFunction((functional::FunctionId)functionName, rOut, codp + 1, regs, &shBinDumpOwner());
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

      case SHCOD_GET_GINT: int_reg(regs, getOp2p1(opc)) = gvarsState.get<int>(getOp2p2(opc)); break;
      case SHCOD_GET_GINT_TOREAL: real_reg(regs, getOp2p1(opc)) = gvarsState.get<int>(getOp2p2(opc)); break;
      case SHCOD_GET_GIVEC_TOREAL:
      {
        const IPoint4 &ivec = gvarsState.get<IPoint4>(shaderopcode::getOp2p2(opc));
        color4_reg(regs, shaderopcode::getOp2p1(opc)) = Color4(ivec.x, ivec.y, ivec.z, ivec.w);
      }
      break;

      default:
        DAG_FATAL("%s: exec_stcode: illegal instruction %u %s (index=%d)", find_block(cod), getOp(opc),
          ShUtils::shcod_tokname(getOp(opc)), codp - cod.data());
    }

  DYNSTCODE_PROFILE_END();
}
