// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC
#undef _DEBUG_TAB_ // exec_stcode if called too frequently to allow any perfomance penalty there
#endif

#include "scriptSElem.h"
#include "scriptSMat.h"
#include "mapBinarySearch.h"
#include <shaders/shFunc.h>
#include "shaderRenderPassPrivate.h"
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shLimits.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderDbg.h>
#include "shStateBlk.h"
#include "shStateBlock.h"
#include "shRegs.h"
#include <3d/dag_render.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_driverDesc.h>
#include <3d/fileTexFactory.h>
#include <supp/dag_prefetch.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <generic/dag_tab.h>
#include <math/dag_intrin.h>
#include <math/dag_TMatrix4more.h>
#include <3d/dag_texIdSet.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_rwLock.h>
#include <supp/dag_cpuControl.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_renderStates.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shAssert.h>
#include <supp/dag_alloca.h>
#include <util/dag_convar.h>

#include <shaders/dag_stcode.h>
#include <shaders/stcode/stblkcodeContext.h>
#include <shaders/shader_name_format.h>
#include "stcode/bindlessStblkcodeContext.h"
#include "profileStcode.h"
#include "stcode/compareStcode.h"

#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
static CONSOLE_BOOL_VAL("render", shader_creation_perf_markers, false);
#endif

// #define DEBUG_RENDER
#if defined(DEBUG_RENDER)
#define S_DEBUG debug
#else
__forceinline bool DEBUG_F(...) { return false; };
#define S_DEBUG 0 && DEBUG_F
#endif

#include "scriptSElemStcodeInterpreter.h"
#include "scriptSElemStBlkcodeInterpreter.h"

#if DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0
extern void update_float_exceptions_this_thread(bool);
static void temporary_disable_fp_exceptions()
{
  if (is_float_exceptions_enabled())
    update_float_exceptions_this_thread(false);
}
static void restore_fp_exceptions_state()
{
  if (is_float_exceptions_enabled())
    update_float_exceptions_this_thread(true);
}
#else
static inline void temporary_disable_fp_exceptions() {}
static inline void restore_fp_exceptions_state() {}
#endif

#define ID_T ScriptedShaderElement::PackedPassId::Id

template <typename T>
void *void_ptr_cast(T val)
{
  return reinterpret_cast<void *>(intptr_t(val));
}

template <typename F>
static ShaderSourceExt make_shader_source(ScriptedShadersBinDumpOwner const &dumpOwner, int i, ShaderCodeType type, F &&get_debug_info)
{
  ShaderSourceExt src = dumpOwner.getCode(i, type);
#if DAGOR_DBGLEVEL > 0
  auto debugName = get_debug_info(type);
  src.debugName = debugName.data();
  src.debugNameLen = uint32_t(debugName.length());
#endif
  return src;
}

template <typename F>
static inline VPROG get_vprog(ScriptedShadersBinDumpOwner const &dumpOwner, int i, F &&get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (uint32_t(i) >= dumpOwner.vprId.size())
    return BAD_VPROG;

  VPROG vpr = interlocked_acquire_load(dumpOwner.vprId[i]);
  if (vpr != BAD_VPROG)
    return vpr;

  temporary_disable_fp_exceptions();
  if (&dumpOwner == &shBinDumpOwner()) // @NOTE: shader ids are only valid for the main dump
    d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::VERTEX), &vpr);
  if (vpr == BAD_VPROG)
  {
    ShaderSourceExt src = make_shader_source(dumpOwner, i, ShaderCodeType::VERTEX, get_debug_info);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
    if (DAGOR_UNLIKELY(shader_creation_perf_markers))
    {
      TIME_PROFILE_NAME(create_vs, src.debugName);
      vpr = d3d::create_vertex_shader(src);
    }
    else
#endif
      vpr = d3d::create_vertex_shader(src);
  }
  restore_fp_exceptions_state();

  if (vpr != BAD_VPROG)
  {
    VPROG pvpr = interlocked_compare_exchange((VPROG volatile &)dumpOwner.vprId[i], vpr, BAD_VPROG);
    if (DAGOR_UNLIKELY(pvpr != BAD_VPROG)) // unlikely case when other thread created and set this vprog slot first
    {
      d3d::delete_vertex_shader(vpr);
      vpr = pvpr;
    }
  }
  else
    DAG_FATAL("Cant create VS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(ShaderCodeType::VERTEX).data(), d3d::get_last_error());
  return vpr;
}

template <typename F>
static inline FSHADER get_fshader(ScriptedShadersBinDumpOwner const &dumpOwner, int i, F &&get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (i < 0 || i >= dumpOwner.fshId.size())
    return BAD_FSHADER;

  FSHADER fsh = interlocked_acquire_load(dumpOwner.fshId[i]);
  if (fsh != BAD_FSHADER)
    return fsh;

  temporary_disable_fp_exceptions();
  if (&dumpOwner == &shBinDumpOwner()) // @NOTE: shader ids are only valid for the main dump
    d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::PIXEL), &fsh);
  if (fsh == BAD_FSHADER)
  {
    ShaderSourceExt src = make_shader_source(dumpOwner, i, ShaderCodeType::PIXEL, get_debug_info);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
    if (DAGOR_UNLIKELY(shader_creation_perf_markers))
    {
      TIME_PROFILE_NAME(create_ps, src.debugName);
      fsh = d3d::create_pixel_shader(src);
    }
    else
#endif
      fsh = d3d::create_pixel_shader(src);
  }
  restore_fp_exceptions_state();

  if (fsh != BAD_FSHADER)
  {
    FSHADER pfsh = interlocked_compare_exchange((FSHADER volatile &)dumpOwner.fshId[i], fsh, BAD_FSHADER);
    if (DAGOR_UNLIKELY(pfsh != BAD_FSHADER)) // unlikely case when other thread created and set this fsh slot first
    {
      d3d::delete_pixel_shader(fsh);
      fsh = pfsh;
    }
  }
  else
    DAG_FATAL("Cant create PS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(ShaderCodeType::PIXEL).data(), d3d::get_last_error());

  return fsh;
}

template <typename F>
static PROGRAM get_compute_prg(ScriptedShadersBinDumpOwner const &dumpOwner, int i, F &&get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (i < 0 || i >= dumpOwner.cshId.size())
    return BAD_PROGRAM;

  PROGRAM csh = interlocked_acquire_load(dumpOwner.cshId[i]);
  if (csh != BAD_PROGRAM)
    return csh;

  bool const isMainDump = &dumpOwner == &shBinDumpOwner();

  temporary_disable_fp_exceptions();
  if (isMainDump) // @NOTE: shader ids are only valid for the main dump
    d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::COMPUTE), &csh);
  if (csh == BAD_PROGRAM)
  {
    auto preload = isMainDump ? CSPreloaded::Yes : CSPreloaded::No;
    ShaderSourceExt src = make_shader_source(dumpOwner, i, ShaderCodeType::COMPUTE, get_debug_info);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
    // @NOTE: any shader warmups/precompilations are not applicable to late-loaded additional dumps
    if (DAGOR_UNLIKELY(shader_creation_perf_markers))
    {
      TIME_PROFILE_NAME(create_cs, src.debugName ? src.debugName : "create_cs_unknown");
      csh = d3d::create_program_cs(src, preload);
    }
    else
#endif
      csh = d3d::create_program_cs(src, preload);
  }
  restore_fp_exceptions_state();

  if (csh != BAD_PROGRAM)
  {
    PROGRAM pcsh = interlocked_compare_exchange((PROGRAM volatile &)dumpOwner.cshId[i], csh, BAD_PROGRAM);
    if (DAGOR_UNLIKELY(pcsh != BAD_PROGRAM)) // unlikely case when other thread created and set this fsh slot first
    {
      d3d::delete_program(csh);
      csh = pcsh;
    }
  }
  else
    DAG_FATAL("Cant create CS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(ShaderCodeType::COMPUTE).data(), d3d::get_last_error());

  return csh;
}

__forceinline void push_int(Tab<uint8_t> &values, const int *v) { append_items(values, sizeof(int), (const uint8_t *)v); }

__forceinline void push_real(Tab<uint8_t> &values, const real *v) { append_items(values, sizeof(real), (const uint8_t *)v); }

__forceinline void push_vec(Tab<uint8_t> &values, const real *v) { append_items(values, sizeof(real) * 4, (const uint8_t *)v); }


// push 'count' vectors
__forceinline void push_vec(Tab<uint8_t> &values, const real *v, int count)
{
  append_items(values, sizeof(real) * 4 * count, (const uint8_t *)v);
}

#if DAGOR_DBGLEVEL > 0
static auto choose_dump_for_variant_name(ScriptedShadersBinDumpOwner const &dump_owner)
{
  const char *dumpName = "";
  const ScriptedShadersBinDumpV2 *intervalDump = ::shBinDumpOwner().getDumpV2();
  if (DAGOR_UNLIKELY(dump_owner.selfHandle == SEC_EXP_BINDUMP_HANDLE))
  {
    dumpName = "_exp_";
    intervalDump = ::shBinDumpExOwner(false).getDumpV2();
  }
  else if (dump_owner.selfHandle != MAIN_BINDUMP_HANDLE)
  {
    dumpName = dump_owner.name.c_str();
  }
  return eastl::make_pair(dumpName, intervalDump);
}
static void build_variant_name_for_element(const ScriptedShaderElement &self, auto &out, unsigned int variant_code, ShaderStage stage,
  ScriptedShadersBinDumpOwner const &dump_owner)
{
  G_ASSERT(self.selectedStaticVariantCode != uint32_t(-1));
  G_ASSERT(variant_code != unsigned(-1));
  out.clear();
  const auto [dumpName, intervalDump] = choose_dump_for_variant_name(dump_owner);
  shader_name_format::compile_human_readable_variant_name(out,
    shader_name_format::VariantIdentifierRef{.shClassName = self.shClass.name.data(),
      .shDumpName = dumpName,
      .stVarCode = int(self.selectedStaticVariantCode),
      .dynVarCode = int(variant_code),
      .stage = stage},
    *dump_owner.getDump(), *intervalDump, dump_owner.shaderNameFormatSettings);
}
static void build_variant_name_for_shader_assert(const ScriptedShaderElement &self, auto &out, unsigned int variant_code,
  ScriptedShadersBinDumpOwner const &dump_owner)
{
  G_ASSERT(self.selectedStaticVariantCode != uint32_t(-1));
  G_ASSERT(variant_code != unsigned(-1));
  out.clear();
  const auto [dumpName, _] = choose_dump_for_variant_name(dump_owner);
  shader_name_format::compile_minimal_variant_name(out,
    shader_name_format::VariantIdentifierRef{.shClassName = self.shClass.name.data(),
      .shDumpName = dumpName,
      .stVarCode = int(self.selectedStaticVariantCode),
      .dynVarCode = int(variant_code)},
    dump_owner.shaderNameFormatSettings);
}

struct FramememDebugInfoBuilder
{
  FRAMEMEM_REGION;
  eastl::basic_string<char, framemem_allocator> debugInfoStr = {};
};

#endif

__forceinline void ScriptedShaderElement::prepareShaderProgram(ID_T &pass_id, int variant, unsigned int variant_code,
  ScriptedShadersBinDumpOwner const &dump_owner) const
{
#if DAGOR_DBGLEVEL > 0
  auto get_debug_info = [&, builder = FramememDebugInfoBuilder()](ShaderCodeType type) mutable {
    build_variant_name_for_element(*this, builder.debugInfoStr, variant_code, type == ShaderCodeType::VERTEX ? STAGE_VS : STAGE_PS,
      dump_owner);
    return eastl::string_view{builder.debugInfoStr};
  };
#else
  auto get_debug_info = [](ShaderCodeType) { return eastl::string_view{}; };
  G_UNUSED(variant_code);
#endif

  VPROG vpr = get_vprog(dump_owner, code.passes[variant].rpass->vprId, get_debug_info);
  FSHADER fsh = get_fshader(dump_owner, code.passes[variant].rpass->fshId, get_debug_info);

  if (vpr != BAD_VPROG)
  {
    temporary_disable_fp_exceptions();
    PROGRAM prg = d3d::create_program(vpr, fsh, getEffectiveVDecl());
    restore_fp_exceptions_state();
    G_ASSERT(prg != BAD_PROGRAM);
    if (DAGOR_UNLIKELY(interlocked_compare_exchange(pass_id.pr, prg, -2) != -2)) // unlikely case when other thread created and set
                                                                                 // this pr first
      if (prg != BAD_PROGRAM)
        d3d::delete_program(prg);
  }
  else
  {
    logerr("RENDERING INVALID VARIANT shader_class=%s variant=%d", (const char *)shClass.name, variant);
    pass_id.pr = BAD_PROGRAM;
    G_ASSERT(vpr == BAD_VPROG && fsh == BAD_FSHADER);
  }
}

DAGOR_NOINLINE
void ScriptedShaderElement::preparePassIdOOL(ID_T &pass_id, int variant, unsigned int variant_code,
  ScriptedShadersBinDumpOwner const &dump_owner) const
{
  pass_id.v.store(recordStateBlock(*code.passes[variant].rpass, code, dump_owner));
  if (pass_id.pr == -2)
    ScriptedShaderElement::prepareShaderProgram(pass_id, variant, variant_code, dump_owner);
}

__forceinline void ScriptedShaderElement::preparePassId(ID_T &pass_id, int variant, unsigned int variant_code,
  ScriptedShadersBinDumpOwner const &dump_owner) const
{
  if (int(pass_id.v.load(dag::memory_order_relaxed)) < 0)
    preparePassIdOOL(pass_id, variant, variant_code, dump_owner);
}

__forceinline const ID_T &ScriptedShaderElement::lockReadPassId(int variant, unsigned int variant_code,
  ScriptedShadersBinDumpOwner const &dump_owner) const
{
  ID_T &pass_id = passes[variant].id;
  stateBlocksSpinLock.lockRead();
  // Loop, as resetStateBlocks() may run between the write and read locks
  while (int(pass_id.v.load(dag::memory_order_relaxed)) < 0) [[unlikely]]
  {
    stateBlocksSpinLock.unlockRead();
    {
      ScopedLockWriteTemplate<SpinLockReadWriteLock> lock(stateBlocksSpinLock);
      preparePassId(pass_id, variant, variant_code, dump_owner);
    }
    stateBlocksSpinLock.lockRead();
  }
  return pass_id; //-V1020 caller unlocks
}

/*********************************
 *
 * class ScriptedShaderElement
 *
 *********************************/

ScriptedShaderElement::ScriptedShaderElement(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m, const char *info) :
  shClass(*m.props.sclass), code(matcode), usedVdecl(-1)
{
  stageDest = STAGE_PS;
  dumpHandle = m.props.dumpHandle;

#if DAGOR_DBGLEVEL > 0
  selectedStaticVariantCode = interlocked_relaxed_load(m.selectedStaticVariantCode);
#endif

  // @NOTE: intervals & var names are always in the main dump -- additional dumps can only be linked to it.
  auto const &mainDump = ::shBinDump();

  S_DEBUG("make shelem for '%s'", (const char *)shClass.name);
  uint8_t *vars = getVars();
  memset(vars, 0, sizeof(uint8_t) * code.varSize);

  // execute init code
  int i;
  Tab<int> tmp_texVarOffsets(framemem_ptr());
  tmp_texVarOffsets.reserve(32);

  {
    dag::ConstSpan<int> cod = code.initCode;
    // debug("count=%d", cod.size());
    for (i = 0; i < cod.size(); i += 2)
    {
      int varId = cod[i];
      uint8_t *ptr = &vars[varId];

      switch (shaderopcode::getOp(cod[i + 1]))
      {
        case SHCOD_DIFFUSE: *(Color4 *)ptr = m.props.d3dm.diff; break;
        case SHCOD_EMISSIVE: *(Color4 *)ptr = m.props.d3dm.emis; break;
        case SHCOD_SPECULAR: *(Color4 *)ptr = m.props.d3dm.spec; break;
        case SHCOD_AMBIENT: *(Color4 *)ptr = m.props.d3dm.amb; break;
        case SHCOD_TEXTURE:
        {
          // debug("ScriptedShaderElement::ctor(), SHCOD_TEXTURE");

          int ind = shaderopcode::getOp2p1(cod[i + 1]);

          TEXTUREID texId = m.props.textureId[ind];

          if (texId == BAD_TEXTUREID)
          {
            texId = FileTextureFactory::self.getReplaceBadTexId();
            char buf[4 << 10];
            logerr("texture[%d] not set for shader '%s' (%s) [mat: %s] %s %s", ind, m.props.sclass->name.data(), info, m.getInfo(),
              texId != BAD_TEXTUREID ? " was replaced to missing" : "", dgs_get_fatal_context(buf, sizeof(buf)));
          }
          else if (texId == D3DRESID(D3DRESID::INVALID_ID2))
            texId = BAD_TEXTUREID;

          Tex &t = *(Tex *)ptr;
          t.init(texId);

          // debug("t.tex = %p, t.textureId = %i, name = %s", t.tex, t.textureId,
          //   ::get_managed_texture_name(t.texId));
          tmp_texVarOffsets.push_back(varId);
        }
        break;
        default: G_ASSERT(0);
      }
    }
  }

  // init vars from stvars
  // debug("class='%s'", shClass.name.data());
  for (int j = 0, e = code.stVarMap.size(); j < e; ++j)
  {
    int v = mapbinarysearch::unpackData(code.stVarMap[j]);
    int sv = mapbinarysearch::unpackKey(code.stVarMap[j]);

    if (m.props.sclass->localVars.v[sv].isPublic)
      continue;

    switch (m.props.sclass->localVars.v[sv].type)
    {
      case SHVT_COLOR4: *(Color4 *)&vars[v] = m.get_color4_stvar(sv); break;
      case SHVT_REAL: *(real *)&vars[v] = m.get_real_stvar(sv); break;
      case SHVT_INT:
        *(int *)&vars[v] = m.get_int_stvar(sv);
        // debug("%s: int var'%s'=%d", shClass.name.data(), VariableMap::getVariableName(shClass.localVars.v[sv].nameId),
        // m.get_int_stvar(sv));
        break;
      case SHVT_TEXTURE:
      {
        Tex &t = *(Tex *)&vars[v];
        // debug("ScriptedShaderElement::ctor(), stVarMap = {%d, %d}", v, sv);
        if (t.texId != BAD_TEXTUREID && t.tex)
        {
          for (unsigned tmpi = 0; tmpi < tmp_texVarOffsets.size(); tmpi++)
            if (tmp_texVarOffsets[tmpi] == v)
            {
              // DEBUG_CTX("shader %s, var %s", shClass.name.data(), VariableMap::getVariableName(shClass.localVars.v[sv].nameId));
              erase_items(tmp_texVarOffsets, tmpi, 1);
              break;
            }
          t.term();
        }

        t.init(m.get_tex_stvar(sv));
        tmp_texVarOffsets.push_back(v);
      }
      break;
      case SHVT_SAMPLER: *(d3d::SamplerHandle *)&vars[v] = m.get_sampler_stvar(sv); break;
      default: G_ASSERT(0);
    }
  }
  texVarOfs = tmp_texVarOffsets;

  // init passes
  S_DEBUG("code.passes.size() = %d", code.passes.size());
  clear_and_resize(passes, code.passes.size());
  mem_set_ff(passes);

  for (i = 0; i < code.passes.size(); i++)
  {
    const shaderbindump::ShaderCode::Pass &otherPass = code.passes[i];

    if (otherPass.rpass)
    {
      passes[i].id.v.store(PackedPassId::NOT_INITED_STATE_BLOCK_ID);
      passes[i].id.pr = -2;
    }
  }

  // debug("%d dynamic variants", code.dynVariants.variantCount());

  if (!code.dynVariants.codePieces.size())
  {
    // no dynamic variants - select single static variant once
  }
  else
  {
    // build dynamic variant search record
    dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> pcs = code.dynVariants.codePieces;
    bool local_ival = false;
    varMapTable.clear();
    for (i = 0; i < pcs.size(); i++)
    {
      const shaderbindump::Interval &interval = mainDump.intervals[pcs[i].intervalId];
      int var_ofs = -1;
      int type = 0;

      switch (interval.type)
      {
        case shaderbindump::Interval::TYPE_INTERVAL:
        {
          // offset -> offs in local value table
          // type -> variable type
          int varIndex = m.props.sclass->localVars.findVar(interval.nameId);
          if (varIndex < 0)
          {
            DAG_FATAL("[ERROR] variable '%s' not found in shader element", (const char *)mainDump.varMap[interval.nameId]);
            continue;
          }

          int offset = code.findVar(varIndex);
          if (offset < 0)
          {
            DAG_FATAL("[ERROR] value of variable '%s' not found in shader element", (const char *)mainDump.varMap[interval.nameId]);
            continue;
          }
          type = m.props.sclass->localVars.v[varIndex].type;
          var_ofs = offset;
          local_ival = true;
        }
        break;
        case shaderbindump::Interval::TYPE_GLOBAL_INTERVAL:
        {
          // offset -> internal global variable index
          // type -> global variable type
          int offset = mainDump.globVars.findVar(interval.nameId);
          if (offset < 0)
          {
            DAG_FATAL("[ERROR] global variable '%s' not found in shader element", (const char *)mainDump.varMap[interval.nameId]);
            continue;
          }
          type = mainDump.globVars.v[offset].type;
        }
        break;

        default: DAG_FATAL("invalid dynamic type=%d nameId/extType=%d", interval.type, interval.nameId);
      }
      if (varMapTable.empty())
        varMapTable.resize(pcs.size());
      varMapTable[i].varOfs = var_ofs;
      varMapTable[i].type = type;
      varMapTable[i].intervalId = pcs[i].intervalId;
      varMapTable[i].totalMul = pcs[i].totalMul;
    }
    if (!local_ival)
      clear_and_shrink(varMapTable);
  }
  shaders_internal::register_mat_elem(this);
  if (!code.dynVariants.codePieces.size())
    dynVariantOption = NO_DYNVARIANT;
  else if (!varMapTable.size())
    dynVariantOption = NO_VARMAP_TABLE;
  else
  {
    G_ASSERT(code.dynVariants.codePieces.size() == varMapTable.size());
    dynVariantOption = COMPLEX_VARMAP;
  }
  dynVariantCollectionId = shaderbindump::get_dynvariant_collection_id(code);
}

ScriptedShaderElement *ScriptedShaderElement::create(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m,
  const char *info)
{
  uint8_t *data = new uint8_t[sizeof(ScriptedShaderElement) + matcode.varSize];
  return new (data, _NEW_INPLACE) ScriptedShaderElement(matcode, m, info);
}

ScriptedShaderElement::~ScriptedShaderElement()
{
  resetStateBlocks();
  resetShaderPrograms();
  clear_and_shrink(passes);

  uint8_t *vars = getVars();
  for (int i = 0; i < texVarOfs.size(); i++)
  {
    int ofs = texVarOfs[i];
    ((Tex *)&vars[ofs])->term();
  }

  shaders_internal::unregister_mat_elem(this);
}

void ScriptedShaderElement::acquireTexRefs()
{
  dag::ConstSpan<int> cod = code.initCode;
  uint8_t *vars = getVars();
  for (int i = 0; i < cod.size(); i += 2)
    if (shaderopcode::getOp(cod[i + 1]) == SHCOD_TEXTURE)
      ((Tex *)&vars[cod[i]])->get();

  for (int i = 0; i < code.stVarMap.size(); i++)
  {
    int v = mapbinarysearch::unpackData(code.stVarMap[i]);
    int sv = mapbinarysearch::unpackKey(code.stVarMap[i]);

    if (!shClass.localVars.v[sv].isPublic && shClass.localVars.v[sv].type == SHVT_TEXTURE)
      ((Tex *)&vars[v])->get();
  }
}
void ScriptedShaderElement::releaseTexRefs()
{
  dag::ConstSpan<int> cod = code.initCode;
  bool released = false;
  uint8_t *vars = getVars();
  for (int i = 0; i < cod.size(); i += 2)
    if (shaderopcode::getOp(cod[i + 1]) == SHCOD_TEXTURE)
      if (((Tex *)&vars[cod[i]])->release())
        released = true;

  for (int i = 0; i < code.stVarMap.size(); i++)
  {
    int v = mapbinarysearch::unpackData(code.stVarMap[i]);
    int sv = mapbinarysearch::unpackKey(code.stVarMap[i]);

    if (!shClass.localVars.v[sv].isPublic && shClass.localVars.v[sv].type == SHVT_TEXTURE)
      if (((Tex *)&vars[v])->release())
        released = true;
  }
  if (released)
    resetStateBlocks();
}

void ScriptedShaderElement::recreateElem(const shaderbindump::ShaderCode &matcode, ScriptedShaderMaterial &m)
{
  G_ASSERTF(defaultmem->getSize(this) >= sizeof(ScriptedShaderElement) + matcode.varSize,
    "ScriptedShaderElement(%p).memsz=%d code.varSize=%d matcode.varSize=%d", this, defaultmem->getSize(this), code.varSize,
    matcode.varSize);

  VDECL oldVd = usedVdecl;
  int rc = ref_count;
  ref_count = 0;
  int oldStageDest = stageDest;
  resetStateBlocks();
  clear_and_shrink(passes);
  this->~ScriptedShaderElement();
  new (this, _NEW_INPLACE) ScriptedShaderElement(matcode, m, "n/a");
  stageDest = oldStageDest;
  ref_count = rc;
  usedVdecl = oldVd;
}

void convert_channel_code(dag::ConstSpan<ShaderChannelId> inchannel, CompiledShaderChannelId *outchannel)
{
  for (int i = 0; i < inchannel.size(); i++)
    outchannel[i].convertFrom(inchannel[i]);
}

static constexpr int MAX_COMPILED_CHANNELS = 64;

void ScriptedShaderElement::replaceVdecl(VDECL vDecl)
{
  if (usedVdecl == vDecl)
    return;
  // Programs are recreated only with the state block, so drop both
  resetStateBlocks([&] {
    usedVdecl = vDecl;
    resetShaderPrograms();
  });
}
VDECL ScriptedShaderElement::getEffectiveVDecl() const
{
  if (usedVdecl >= 0)
    return usedVdecl;
  return usedVdecl = initVdecl();
}
VDECL ScriptedShaderElement::initVdecl() const
{
  G_ASSERT(code.channel.size() < MAX_COMPILED_CHANNELS);
  CompiledShaderChannelId csc[MAX_COMPILED_CHANNELS];
  convert_channel_code(code.channel, csc);
  return dynrender::addShaderVdecl(csc, code.channel.size(), -1);
}

void ScriptedShaderElement::detachElem()
{
  resetShaderPrograms();
  resetStateBlocks();
  dynVariantOption = NO_DYNVARIANT;

  //== offsetof() doesn't work with references, so we rely on fixed layout: code; shClass; dumpHandle;
  size_t ofs = offsetof(ScriptedShaderElement, dumpHandle);
  *(const shaderbindump::ShaderCode **)(ofs - sizeof(void *) * 2 + (char *)this) = &shaderbindump::null_shader_code();
  *(const shaderbindump::ShaderClass **)(ofs - sizeof(void *) * 1 + (char *)this) = &shaderbindump::null_shader_class();
}
void ScriptedShaderElement::resetShaderPrograms(bool delete_programs)
{
  for (int i = 0; i < passes.size(); i++)
    if (passes[i].id.pr >= 0)
    {
      if (delete_programs)
        d3d::delete_program(passes[i].id.pr);
      passes[i].id.pr = -2;
    }
}
void ScriptedShaderElement::preCreateStateBlocks()
{
  auto const &dumpOwner = get_shaders_dump_owner(dumpHandle);
#if DAGOR_DBGLEVEL > 0
  // If we need debug names, we need to grab a correct variant code
  int codeIndex = int(&code - shClass.code.begin());
  selectedStaticVariantCode = uint32_t(-1);
  shClass.stVariants.enumerateCodesForVariant(codeIndex, [this](unsigned variant_code) { selectedStaticVariantCode = variant_code; });
  for (int i = 0; i < passes.size(); i++)
  {
    auto variantCode = unsigned(-1);
    code.dynVariants.enumerateCodesForVariant(i, [&variantCode](unsigned variant_code) { variantCode = variant_code; });
    preparePassId(passes[i].id, i, variantCode, dumpOwner);
  }
#else
  // If we do not need debug names, don't spend time walking tables
  for (int i = 0; i < passes.size(); i++)
    preparePassId(passes[i].id, i, invalid_variant, dumpOwner);
#endif
}
template <typename Cb>
void ScriptedShaderElement::resetStateBlocks(Cb &&cb)
{
  dag::RelocatableFixedVector<ShaderStateBlockId, 32, true, framemem_allocator> toDelete;
  // Render threads may create a block from old vars (e.g. textures before replaceTexture) right now,
  // so wait for it under the write lock, otherwise that stale block outlives this reset
  {
    ScopedLockWriteTemplate<SpinLockReadWriteLock> lock(stateBlocksSpinLock);
    cb();
    for (int i = 0; i < passes.size(); i++)
    {
      ShaderStateBlockId v = passes[i].id.v.load();
      if (v != ShaderStateBlockId::Invalid && v != PackedPassId::NOT_INITED_STATE_BLOCK_ID)
      {
        toDelete.push_back(v);
        passes[i].id.v.store(PackedPassId::NOT_INITED_STATE_BLOCK_ID);
      }
    }
  }

  for (auto &b : toDelete)
    ShaderStateBlock::delBlock(b);
}
void ScriptedShaderElement::resetStateBlocks()
{
  resetStateBlocks([] {});
}

static void report_interval_not_found_error(const ScriptedShaderElement &self, unsigned variant_code)
{
#if DAGOR_DBGLEVEL > 0
  auto const &dumpOwner = get_shaders_dump_owner(self.dumpHandle);
  auto const &dump = *dumpOwner.getDump();
  dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> pcs = self.code.dynVariants.codePieces;
  bool has_dump = shaderbindump::hasShaderInvalidVariants(self.dumpHandle, self.shClass.nameId);
  if (!shaderbindump::markInvalidVariant(self.dumpHandle, self.shClass.nameId, &self.code - self.shClass.code.begin(), variant_code))
    return;
  String tmp(framemem_ptr());
  const char *tmp_st = shaderbindump::decodeStaticVariants(dump, self.shClass, &self.code - self.shClass.code.begin());
  const char *varstr = shaderbindump::decodeVariantStr(dump, pcs, variant_code, tmp);
  logerr("%p.dynamic variant %u not found (shader %s/%d)\n\n"
         "normalized dynvariant:\n  %s\n\n"
         "in one of normalized statvariants:\n%s\n\n"
         "Check for assumes/validVariants or render code conditions",
    &self, variant_code, (const char *)self.shClass.name, &self.code - self.shClass.code.begin(), varstr, tmp_st);
  if (!has_dump)
    shaderbindump::dumpShaderInfo(dump, self.shClass);
  tmp.clear();
  DAG_FATAL("%p.dynamic variant %u not found (shader %s/%d)\n\n"
            "normalized dynvariant:\n  %s\n\n"
            "in one of normalized statvariants:\n%s\n\n"
            "Check for assumes/validVariants or render code conditions",
    &self, variant_code, (const char *)self.shClass.name, &self.code - self.shClass.code.begin(), varstr, tmp_st);
#else
  logerr("%p.dynamic variant %u not found (shader %s/%d)", &self, variant_code, (const char *)self.shClass.name,
    &self.code - self.shClass.code.begin());
#endif
}

int ScriptedShaderElement::chooseCachedDynamicVariant(unsigned int variant_code) const
{
  if (dynVariantOption == NO_DYNVARIANT)
    return code.passes.size() ? 0 : -1;
  int id = code.dynVariants.findVariant(variant_code);
  if (id == code.dynVariants.FIND_NULL)
    return -1;
  else if (DAGOR_UNLIKELY(id == code.dynVariants.FIND_NOTFOUND))
  {
    report_interval_not_found_error(*this, variant_code);
    return -1;
  }
  return id;
}

// select current passes by dynamic variants
__forceinline int ScriptedShaderElement::chooseDynamicVariant(dag::ConstSpan<uint8_t> norm_values,
  unsigned int &out_variant_code) const
{
  unsigned variant_code = 0;
  if (dynVariantOption == NO_VARMAP_TABLE) // the most common option is first
  {
    G_ASSERT(code.dynVariants.codePieces.size() != 0 && varMapTable.size() == 0);
    const auto *__restrict pci = code.dynVariants.codePieces.begin(), *pce = code.dynVariants.codePieces.end();
    do
    {
      variant_code += norm_values[pci->intervalId] * pci->totalMul;
    } while (++pci != pce);
  }
  else if (dynVariantOption == NO_DYNVARIANT)
  {
    G_ASSERT(code.dynVariants.codePieces.size() == 0);
    out_variant_code = 0;
    if (&code == &shaderbindump::null_shader_code())
      logerr("detached shaderElement=%p is used for render", this);
    return code.passes.size() ? 0 : -1;
  }
  else
  {
    G_ASSERT(code.dynVariants.codePieces.size() == varMapTable.size() && dynVariantOption == COMPLEX_VARMAP);
    // @NOTE: intervals & var names are always in the main dump -- additional dumps can only be linked to it.
    auto const &mainDump = ::shBinDump();
    for (const VarMap &vmt : varMapTable)
    {
      if (vmt.varOfs < 0)
        variant_code += norm_values[vmt.intervalId] * vmt.totalMul;
      else
      {
        const void *var_ptr = getVars() + vmt.varOfs;
        switch (vmt.type)
        {
          case SHVT_INT:
            variant_code += mainDump.intervals[vmt.intervalId].getNormalizedValue((*(int *)var_ptr)) * vmt.totalMul;
            break;
          case SHVT_REAL:
            variant_code += mainDump.intervals[vmt.intervalId].getNormalizedValue((*(real *)var_ptr)) * vmt.totalMul;
            break;
          case SHVT_COLOR4:
            variant_code += mainDump.intervals[vmt.intervalId].getNormalizedValue(
                              (((real *)var_ptr)[0] + ((real *)var_ptr)[1] + ((real *)var_ptr)[2]) / 3.) *
                            vmt.totalMul;
            break;
          case SHVT_TEXTURE:
          {
            Tex &tex = *(Tex *)var_ptr;
            if (tex.texId != BAD_TEXTUREID)
              variant_code += vmt.totalMul;
            break;
          }
        }
      }
    }
  }

  out_variant_code = variant_code;
  int id = code.dynVariants.findVariant(variant_code);

  if (DAGOR_UNLIKELY(id == code.dynVariants.FIND_NOTFOUND))
  {
    report_interval_not_found_error(*this, variant_code);
    return -1;
  }

  if (id == code.dynVariants.FIND_NULL)
    return -1;

  G_ASSERT(id >= 0 && id < code.passes.size());

  // debug("variant=%d", curVariant);
  return id;
}

int ScriptedShaderElement::chooseDynamicVariant(unsigned int &out_variant_code) const
{
  return chooseDynamicVariant(shGlobalData().globIntervalNormValues, out_variant_code);
}

void ScriptedShaderElement::update_stvar(ScriptedShaderMaterial &m, int stvarid)
{
  int v = code.findVar(stvarid);
  if (v < 0)
    return;

  uint8_t *vars = getVars();
  switch (m.props.sclass->localVars.v[stvarid].type)
  {
    case SHVT_COLOR4: *(Color4 *)&vars[v] = m.get_color4_stvar(stvarid); break;
    case SHVT_REAL: *(real *)&vars[v] = m.get_real_stvar(stvarid); break;
    case SHVT_INT: *(int *)&vars[v] = m.get_int_stvar(stvarid); break;
    case SHVT_TEXTURE:
    {
      Tex &t = *(Tex *)&vars[v];
      TEXTUREID new_texId = m.get_tex_stvar(stvarid);

      if (new_texId != t.texId)
        t.replace(new_texId);
    }
    break;
    case SHVT_SAMPLER: *(d3d::SamplerHandle *)&vars[v] = m.get_sampler_stvar(stvarid); break;
    default: G_ASSERT(0);
  }
}

#if DAGOR_DBGLEVEL > 0
static void check_state_blocks_conformity(const shaderbindump::ShaderCode &code, int dyn_var_n,
  const shaderbindump::ShaderClass &shClass)
{
  if (!code.isBlockSupported(dyn_var_n, shaderbindump::blockStateWord))
  {
    if (shaderbindump::autoBlockStateWordChange)
    {
      // debug("shader <%s>[S:%d/D:%d] doesn't support stateWord=%04X", shClass.name.data(),
      //   &code-shClass.code.data(), dyn_var_n, shaderbindump::blockStateWord);

      auto &sb = code.suppBlockUid[dyn_var_n];
      if (*sb == shader_layout::BLK_WORD_FULLMASK)
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME); // set nil blocks
      else
        ShaderGlobal::changeStateWord(*sb); // set first applicable block
    }
    else
    {
      int b_frame = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_FRAME);
      int b_scene = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_SCENE);
      int b_obj = decodeBlock(shaderbindump::blockStateWord, ShaderGlobal::LAYER_OBJECT);

      const shader_layout::blk_word_t *sb = code.suppBlockUid[dyn_var_n].data();
      int shframe = decodeBlock(*sb, ShaderGlobal::LAYER_FRAME);
      int shscene = decodeBlock(*sb, ShaderGlobal::LAYER_SCENE);
      int shobj = decodeBlock(*sb, ShaderGlobal::LAYER_OBJECT);

      auto const &globdataDump = *shGlobalData().backing->getDump();

      String errorMessage(0,
        "shader <%s>[S:%d/D:%d] doesn't support stateWord=%04X\n\n"
        "current blocks=(%s:%s:%s), supported=(%s:%s:%s)\n",
        (const char *)shClass.name, &code - shClass.code.begin(), dyn_var_n, shaderbindump::blockStateWord,
        b_frame >= 0 ? (const char *)globdataDump.blockNameMap[b_frame] : "NULL",
        b_scene >= 0 ? (const char *)globdataDump.blockNameMap[b_scene] : "NULL",
        b_obj >= 0 ? (const char *)globdataDump.blockNameMap[b_obj] : "NULL",
        shframe >= 0 ? (const char *)globdataDump.blockNameMap[shframe] : "NULL",
        shscene >= 0 ? (const char *)globdataDump.blockNameMap[shscene] : "NULL",
        shobj >= 0 ? (const char *)globdataDump.blockNameMap[shobj] : "NULL");

      if (strncmp((const char *)shClass.name, "rendinst_", 9) == 0)
        errorMessage.aprintf(0, "\nTry reproducing with `render.check_dynrend_shaders` enabled."
                                "\nThe asset name will appear in '[E] Rendinst shader used in dynrend: <assetName>'.\n");

      logerr("%s", errorMessage);

      G_ASSERTF_ONCE(0, "%s", errorMessage);
    }
  }
}
#endif

void ScriptedShaderElement::getDynamicVariantStates(int variant_code, int cur_variant, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state) const
{
  auto const &dumpOwner = get_shaders_dump_owner(dumpHandle);
  const ID_T &pass_id = lockReadPassId(cur_variant, variant_code, dumpOwner);
  state_index = pass_id.v.load(dag::memory_order_relaxed);
  program = pass_id.pr;
  render_state = ShaderStateBlock::blocks[state_index].stateIdx;
  const_state = ShaderStateBlock::blocks[state_index].constIdx;
  tex_state = ShaderStateBlock::blocks[state_index].samplerIdx;
  stateBlocksSpinLock.unlockRead();
}

bool ScriptedShaderElement::setStates() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return false;
  auto const &dumpOwner = get_shaders_dump_owner(dumpHandle);

#if DAGOR_DBGLEVEL > 0
  if (!code.suppBlockUid.empty())
    check_state_blocks_conformity(code, curVariant, shClass);
#endif
  const ID_T &pass_id = lockReadPassId(curVariant, variant_code, dumpOwner);
  setStatesForVariant(curVariant, variant_code, pass_id.pr, pass_id.v.load(dag::memory_order_relaxed));
  stateBlocksSpinLock.unlockRead();
  return true;
}

bool ScriptedShaderElement::isAnyPassWritesDepth() const
{
  auto const &dump = *get_shaders_dump_owner(dumpHandle).getDump();
  for (int i = 0; i < code.passes.size(); ++i)
  {
    const auto &rpass = code.passes[i].rpass;
    if (!rpass)
      continue;
    const uint16_t renderStateNo = rpass->renderStateNo;
    if (renderStateNo < dump.renderStates.size() && dump.renderStates[renderStateNo].zwrite)
      return true;
  }

  return false;
}

#if DAGOR_DBGLEVEL > 0
static void verify_variant_render_pass(const shaderbindump::ShaderCode::Pass &codeCp, const ScriptedShadersBinDumpOwner &dumpOwner,
  const char *shader_name)
{
  if (!codeCp.rpass)
    return;
  const auto rp = codeCp.rpass->renderPassRef();
  if (!rp)
    return;
  const auto *dumpV6 = dumpOwner.getDumpV6();
  G_ASSERT(!dumpV6 || dumpV6->externalStcodeMode != shader_layout::ExternalStcodeMode::BRANCHED_CPP);
  if (rp.group)
    ShaderRenderPass::verify_active_pass_group(dumpOwner.selfHandle, rp.index, shader_name);
  else
    ShaderRenderPass::verify_active_pass(dumpOwner.selfHandle, rp.index, rp.subpassIdx, shader_name);
}
#endif

void ScriptedShaderElement::setProgram(uint32_t variant)
{
  auto const &dumpOwner = get_shaders_dump_owner(dumpHandle);
  const ID_T &pass_id = lockReadPassId(variant, 0, dumpOwner);
  ShaderStateBlock::blocks[pass_id.v.load(dag::memory_order_relaxed)].apply(tex_level);
#if DAGOR_DBGLEVEL > 0
  verify_variant_render_pass(code.passes[variant], dumpOwner, (const char *)shClass.name);
#endif
  d3d::set_program(pass_id.pr);
  stateBlocksSpinLock.unlockRead();
}

PROGRAM ScriptedShaderElement::getComputeProgram(const shaderbindump::ShaderCode::ShRef *p, int variant_code) const
{
  G_ASSERTF(p->fshId != shaderbindump::ShaderCode::INVALID_FSH_VPR_ID, "Compute shader is null");
  G_ASSERTF(p->vprId == shaderbindump::ShaderCode::INVALID_FSH_VPR_ID, "Unexpected vpr=%d in compute shader", p->vprId);
  auto const &dumpOwner = get_shaders_dump_owner(dumpHandle);

#if DAGOR_DBGLEVEL > 0
  auto get_debug_info = [&, builder = FramememDebugInfoBuilder()](ShaderCodeType) mutable {
    build_variant_name_for_element(*this, builder.debugInfoStr, variant_code, STAGE_CS, dumpOwner);
    return eastl::string_view{builder.debugInfoStr};
  };
#else
  auto get_debug_info = [](ShaderCodeType) { return eastl::string_view{}; };
  G_UNUSED(variant_code);
#endif

  return get_compute_prg(dumpOwner, p->fshId, get_debug_info);
}

PROGRAM ScriptedShaderElement::getComputeProgram() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return BAD_PROGRAM;
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
  if (!codeCp->rpass)
    return BAD_PROGRAM;

  const shaderbindump::ShaderCode::ShRef *p = &codeCp->rpass.get();

  return getComputeProgram(p, variant_code);
}

PROGRAM ScriptedShaderElement::preCreateComputeProgram(int pass_id) const
{
  unsigned variantCode = unsigned(-1);
#if DAGOR_DBGLEVEL > 0
  int codeIndex = int(&code - shClass.code.begin());
  selectedStaticVariantCode = uint32_t(-1);
  shClass.stVariants.enumerateCodesForVariant(codeIndex, [this](unsigned variant_code) { selectedStaticVariantCode = variant_code; });
  code.dynVariants.enumerateCodesForVariant(pass_id, [&variantCode](unsigned variant_code) { variantCode = variant_code; });
#endif
  return getComputeProgram(&code.passes[pass_id].rpass.get(), variantCode);
}

void copy_current_global_variables_states(GlobalVariableStates &gv)
{
  if (DAGOR_UNLIKELY(!gv.globIntervalNormValues.size() || gv.generation != shaderbindump::get_generation()))
  {
    gv.globIntervalNormValues.resize(shGlobalData().globIntervalNormValues.size());
    gv.generation = shaderbindump::get_generation();
  }
  memcpy(gv.globIntervalNormValues.data(), shGlobalData().globIntervalNormValues.data(), shGlobalData().globIntervalNormValues.size());
}

static int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &s,
  uint32_t &program, ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state, uint32_t &variant_code)
{
  int curVariant = s.chooseDynamicVariant(global_variants_state.globIntervalNormValues, variant_code);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variant_code, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

static int get_dynamic_variant_states(const ScriptedShaderElement &s, uint32_t &program, ShaderStateBlockId &state_index,
  shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state, shaders::TexStateIdx &tex_state, uint32_t &variant_code)
{
  int curVariant = s.chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variant_code, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

static int get_cached_dynamic_variant_states(const ScriptedShaderElement &s, dag::ConstSpan<int> cache, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state, uint32_t &variant_code)
{
  variant_code = cache[s.dynVariantCollectionId];
  int curVariant = s.chooseCachedDynamicVariant(variant_code);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variant_code, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

shaders::CombinedDynVariantState get_dynamic_variant_states(const GlobalVariableStates &global_variants_state,
  const ScriptedShaderElement &s)
{
  shaders::CombinedDynVariantState state = {};
  uint32_t variantCode;
  state.variant = get_dynamic_variant_states(global_variants_state, s, state.program, state.state_index, state.render_state,
    state.const_state, state.tex_state, variantCode);
#if DAGOR_DBGLEVEL > 0
  state.variantCode = variantCode;
#endif
  return state;
}

shaders::CombinedDynVariantState get_dynamic_variant_states(const ScriptedShaderElement &s)
{
  shaders::CombinedDynVariantState state = {};
  uint32_t variantCode;
  state.variant = get_dynamic_variant_states(s, state.program, state.state_index, state.render_state, state.const_state,
    state.tex_state, variantCode);
#if DAGOR_DBGLEVEL > 0
  state.variantCode = variantCode;
#endif
  return state;
}

shaders::CombinedDynVariantState get_cached_dynamic_variant_states(const ScriptedShaderElement &s, dag::ConstSpan<int> cache)
{
  shaders::CombinedDynVariantState state = {};
  uint32_t variantCode;
  state.variant = get_cached_dynamic_variant_states(s, cache, state.program, state.state_index, state.render_state, state.const_state,
    state.tex_state, variantCode);
#if DAGOR_DBGLEVEL > 0
  state.variantCode = variantCode;
#endif
  return state;
}

bool dynamic_variant_depends_on_global_var(const ScriptedShaderElement &s, int var_id)
{
  const ScriptedShadersGlobalData &gd = shGlobalData();
  if (unsigned(var_id) >= gd.maxShadervarCnt())
    return false;
  const uint32_t varNameId = gd.varIndexMap[var_id];
  if (varNameId >= SHADERVAR_IDX_ABSENT)
    return false;
  for (const shaderbindump::VariantTable::IntervalBind &piece : s.code.dynVariants.codePieces)
  {
    const shaderbindump::Interval &interval = shBinDump().intervals[piece.intervalId];
    if (interval.type == shaderbindump::Interval::TYPE_GLOBAL_INTERVAL && interval.nameId == varNameId)
      return true;
  }
  return false;
}

uintptr_t get_static_variant(const ScriptedShaderElement &s) { return uintptr_t(&s.code); }

void ScriptedShaderElement::execute_chosen_stcode(uint16_t stcodeId, uint16_t extStcodeId,
  const shaderbindump::ShaderCode::Pass *cPass, ScriptedShadersBinDumpOwner const &dump_owner, const uint8_t *vars) const
{
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV("execute_chosen_stcode__shader");

  auto const &dump = *dump_owner.getDump();

  G_ASSERT(stcodeId < dump.stcode.size());

#if CPP_STCODE

#if VALIDATE_CPP_STCODE
  stcode::dbg::reset();
#endif

  if (DAGOR_UNLIKELY(dump_owner.stcodeCtx.dynstcodeExMode == stcode::ExecutionMode::BYTECODE))
  {
    exec_stcode(stcodeId, cPass, dump_owner, *this);
  }
  else
  {
    G_ASSERTF(&dump == &shBinDump(), "CppStcode is not supported for additional dumps yet.");
    const uint32_t dynOffset = stcode::USE_BRANCHED_DYNAMIC_ROUTINES ? cPass->rpass->branchedCppStcodeRegisterTableOffset : -1;
    stcode::run_dyn_routine(dump_owner.stcodeCtx, extStcodeId, vars, dynOffset, tex_level, shClass.name.data());
  }

#if VALIDATE_CPP_STCODE
  // Collect records
  if (dump_owner.stcodeCtx.dynstcodeExMode == stcode::ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    exec_stcode(stcodeId, cPass, dump, *this);

  stcode::dbg::validate_accumulated_records(extStcodeId, stcodeId, shClass.name.data(), true);
#endif

#else

  exec_stcode(stcodeId, cPass, dump_owner, *this);

#endif
}

static void bind_assertion_context(const ScriptedShaderElement &self, ScriptedShadersBinDumpOwner &dump_owner, unsigned variant_code)
{
  int shaderClassId = (int)(&self.shClass - dump_owner.getDump()->classes.data());
#if DAGOR_DBGLEVEL > 0
  auto get_assertion_info = [&, builder = FramememDebugInfoBuilder()]() mutable {
    build_variant_name_for_shader_assert(self, builder.debugInfoStr, variant_code, dump_owner);
    return eastl::string_view{builder.debugInfoStr};
  };
  dump_owner.assertionCtx.bind(shaderClassId, dump_owner, get_assertion_info);
#else
  dump_owner.assertionCtx.bind(shaderClassId, dump_owner);
#endif
}

void ScriptedShaderElement::setStatesForVariant(int curVariant, unsigned variant_code, uint32_t program,
  ShaderStateBlockId state_index) const
{
  const PackedPassId *curPass = &passes[curVariant];
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
  auto &dumpOwner = get_shaders_dump_owner_mut(dumpHandle);
  auto const &dump = *dumpOwner.getDump();
#if DAGOR_DBGLEVEL > 0
  if (!code.suppBlockUid.empty())
    check_state_blocks_conformity(code, curVariant, shClass);

  verify_variant_render_pass(*codeCp, dumpOwner, (const char *)shClass.name);
#endif

  d3d::set_program(program);
  G_ASSERT(curPass->id.pr == program && curPass->id.v.load(dag::memory_order_relaxed) == state_index);
  auto &ourBlock = ShaderStateBlock::blocks[state_index];
  if (shaders_internal::cached_state_block != eastl::to_underlying(state_index))
  {
    ourBlock.apply(tex_level);
    shaders_internal::cached_state_block = eastl::to_underlying(state_index);
  }
  else if (tex_level > ourBlock.texLevel)
  {
    ourBlock.reqTexLevel(tex_level);
  }

  bind_assertion_context(*this, dumpOwner, variant_code);
  if (codeCp->rpass->stcodeId != 0xFFFF)
  {
    const int stcodeId = codeCp->rpass->stcodeId;
    const int extStcodeId = stcode::USE_BRANCHED_DYNAMIC_ROUTINES ? code.branchedCppStcodeId : codeCp->rpass->branchlessCppStcodeId;
    const uint8_t *vars = getVars();

    execute_chosen_stcode(stcodeId, extStcodeId, codeCp, dumpOwner, vars);
  }
}

void set_states_for_variant(const ScriptedShaderElement &s, const shaders::CombinedDynVariantState &state)
{
#if DAGOR_DBGLEVEL > 0
  s.setStatesForVariant(state.variant, state.variantCode, state.program, state.state_index);
#else
  s.setStatesForVariant(state.variant, unsigned(-1), state.program, state.state_index);
#endif
}

void ScriptedShaderElement::render(int minv, int numv, int sind, int numf, int base_vertex, int prim) const
{
  G_UNREFERENCED(numv);
  if (!setStates())
    return;
  if (sind == RELEM_NO_INDEX_BUFFER)
  {
    d3d_err(d3d::draw(prim, minv, numf));
  }
  else
  {
    d3d_err(d3d::drawind(prim, sind, numf, base_vertex));
  }
}

bool ScriptedShaderElement::setStatesDispatch() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return false;
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
  if (!codeCp->rpass)
    return false;

  const shaderbindump::ShaderCode::ShRef *p = &codeCp->rpass.get();
  auto &dumpOwner = get_shaders_dump_owner_mut(dumpHandle);
  auto const &dump = *dumpOwner.getDump();

  d3d::set_program(getComputeProgram(p, variant_code));
  bind_assertion_context(*this, dumpOwner, variant_code);
  if (p->stcodeId != shaderbindump::ShaderCode::INVALID_FSH_VPR_ID)
  {
    const int extStcodeId = stcode::USE_BRANCHED_DYNAMIC_ROUTINES ? code.branchedCppStcodeId : p->branchlessCppStcodeId;
    execute_chosen_stcode(p->stcodeId, extStcodeId, codeCp, dumpOwner, getVars());
  }

  return true;
}

bool ScriptedShaderElement::dispatchComputeIndirect(Sbuffer *args, int ofs, GpuPipeline gpu_pipeline, bool set_states) const
{
  if (set_states && !setStatesDispatch())
    return false;
  d3d::dispatch_indirect(args, ofs, gpu_pipeline);
  return true;
}
bool ScriptedShaderElement::dispatchCompute(int tgx, int tgy, int tgz, GpuPipeline gpu_pipeline, bool set_states) const
{
  if (set_states && !setStatesDispatch())
    return false;
  d3d::dispatch(tgx, tgy, tgz, gpu_pipeline);
  return true;
}

bool ScriptedShaderElement::dispatchMesh(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z, bool set_states) const
{
  if (set_states && !setStates())
    return false;
  d3d::dispatch_mesh(groups_x, groups_y, groups_z);
  return true;
}

bool ScriptedShaderElement::dispatchMeshThreads(uint32_t threads_x, uint32_t threads_y, uint32_t threads_z, bool set_states) const
{
  auto tgs = getThreadGroupSizes();
  if (tgs == eastl::array<uint16_t, 3>{0, 0, 0})
    return false;
  G_ASSERTF_RETURN(tgs[0] > 0 && tgs[1] > 0 && tgs[2] > 0, false, "getThreadGroupSizes() returned {%u, %u, %u}", tgs[0], tgs[1],
    tgs[2]);

  const auto getGroups = [](uint32_t a, uint16_t tg) { return (a + tg - 1) / tg; };

  return dispatchMesh(getGroups(threads_x, tgs[0]), getGroups(threads_y, tgs[1]), getGroups(threads_z, tgs[2]), set_states);
}

bool ScriptedShaderElement::dispatchMeshIndirect(Sbuffer *args, uint32_t count, uint32_t stride, uint32_t offset,
  bool set_states) const
{
  if (set_states && !setStates())
    return false;
  d3d::dispatch_mesh_indirect(args, count, stride, offset);
  return true;
}

bool ScriptedShaderElement::dispatchMeshIndirectCount(Sbuffer *args, uint32_t args_offset, uint32_t args_stride, Sbuffer *count,
  uint32_t count_offset, uint32_t max_count, bool set_states) const
{
  if (set_states && !setStates())
    return false;
  d3d::dispatch_mesh_indirect_count(args, args_stride, args_offset, count, count_offset, max_count);
  return true;
}

eastl::array<uint16_t, 3> ScriptedShaderElement::getThreadGroupSizes() const
{
  auto pPassCode = getPassCode();
  if (!pPassCode)
    return {0, 0, 0};
  return {pPassCode->threadGroupSizeX, pPassCode->threadGroupSizeY, pPassCode->threadGroupSizeZ};
}
uint32_t ScriptedShaderElement::getWaveSize() const
{
#if _TARGET_SCARLETT
  if (const auto pPassCode = getPassCode())
    return (pPassCode->scarlettWave32 ? 32 : 64);
#endif
  return d3d::get_driver_desc().minWarpSize;
}
bool ScriptedShaderElement::dispatchComputeThreads(int threads_x, int threads_y, int threads_z, GpuPipeline gpu_pipeline,
  bool set_states) const
{
  auto tgs = getThreadGroupSizes();
  if (tgs == eastl::array<uint16_t, 3>{0, 0, 0})
    return false;
  G_ASSERTF_RETURN(tgs[0] > 0 && tgs[1] > 0 && tgs[2] > 0, false, "getThreadGroupSizes() returned {%u, %u, %u}", tgs[0], tgs[1],
    tgs[2]);
#define THREAD_GROUPS(a, i) (((a) + tgs[i] - 1) / tgs[i])
  return dispatchCompute(THREAD_GROUPS(threads_x, 0), THREAD_GROUPS(threads_y, 1), THREAD_GROUPS(threads_z, 2), gpu_pipeline,
    set_states);
}

int ScriptedShaderElement::getTextureCount() const { return texVarOfs.size(); }

TEXTUREID ScriptedShaderElement::getTexture(int index) const
{
  G_ASSERT_RETURN(uint32_t(index) < texVarOfs.size(), BAD_TEXTUREID);

  const uint8_t *vars = getVars();
  return ((const Tex *)&vars[texVarOfs[index]])->texId;
}

bool ScriptedShaderElement::checkAndPrefetchMissingTextures() const
{
  // Might be accessed from more than 1 thread simultaneously
  if (interlocked_relaxed_load(texturesLoaded))
    return true;
  const uint8_t *vars = getVars();
  for (int i = 0; i < texVarOfs.size(); i++)
  {
    const Tex &t = *(const Tex *)&vars[texVarOfs[i]];
    if (t.texId != BAD_TEXTUREID && !check_managed_texture_loaded(t.texId))
    {
      prefetch_managed_texture(t.texId);
      return false;
    }
  }
  interlocked_relaxed_store(texturesLoaded, true);
  return true;
}

void ScriptedShaderElement::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  const uint8_t *vars = getVars();
  for (int i = 0; i < texVarOfs.size(); i++)
  {
    const Tex &t = *(const Tex *)&vars[texVarOfs[i]];

    if (t.texId != BAD_TEXTUREID)
      tex_id_list.add(t.texId);
  }
}

bool ScriptedShaderElement::replaceTexture(TEXTUREID tex_id_prev, TEXTUREID tex_id_new)
{
  bool replaced = false;
  uint8_t *vars = getVars();
  for (auto tvo : texVarOfs)
  {
    Tex &t = *(Tex *)&vars[tvo];
    if (t.texId == tex_id_prev)
    {
      if (tex_id_prev != tex_id_new)
        t.replace(tex_id_new);
      replaced = true;
    }
  }
  if (replaced)
    resetStateBlocks();
  return replaced;
}


bool ScriptedShaderElement::hasTexture(TEXTUREID tex_id) const
{
  const uint8_t *vars = getVars();
  for (auto tvo : texVarOfs)
    if (((const Tex *)&vars[tvo])->texId == tex_id)
      return true;

  return false;
}


decltype(ShaderStateBlock::blocks) ShaderStateBlock::blocks;
int ShaderStateBlock::deleted_blocks = 0;

const shaderbindump::ShaderCode::ShRef *ScriptedShaderElement::getPassCode() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return nullptr;
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
  return &*codeCp->rpass;
}

static __forceinline ShaderStateBlockId record_st_block_via_bytecode(const char *context, const uint8_t *vars, int code_id,
  auto render_state_no, ScriptedShadersBinDump const &dump)
{
  G_ASSERT(code_id < dump.stcode.size());
  dag::ConstSpan<int> cod = dump.stcode[code_id];
  const int *__restrict codp = cod.data();
  const int *__restrict codp_end = codp + dump.stcode[code_id].size();

  auto maybeShStateBlock = execute_st_block_code(codp, codp_end, context, vars, dump.maxRegSize, code_id);
  if (!maybeShStateBlock.has_value())
    return DEFAULT_SHADER_STATE_BLOCK_ID;

  return ShaderStateBlock::addBlock(eastl::move(*maybeShStateBlock), dump.renderStates[render_state_no]);
}

static __forceinline ShaderStateBlockId record_st_block_via_cpp_stcode(const char *context, const uint8_t *vars, int code_id,
  auto render_state_no)
{
  auto const &dumpOwner = ::shBinDumpOwner(); // CppStblkcode does not support additional bindumps yet
  auto const &dump = *dumpOwner.getDump();

  ShaderStateBlock result;
  BindlessStblkcodeContext bindlessCtx{};
  stcode::cpp::StblkcodeContext ctx{&bindlessCtx, &result, vars, code_id};

  stcode::run_st_routine(dumpOwner.stcodeCtx, size_t(code_id), &ctx);
  return ShaderStateBlock::addBlock(eastl::move(result), dump.renderStates[render_state_no]);
}

ShaderStateBlockId ScriptedShaderElement::recordStateBlock(const shaderbindump::ShaderCode::ShRef &p,
  const shaderbindump::ShaderCode &sc, ScriptedShadersBinDumpOwner const &dump_owner) const
{
  using namespace shaderopcode;

  auto const &dump = *dump_owner.getDump();
  ShaderStateBlockId res = DEFAULT_SHADER_STATE_BLOCK_ID;

#if CPP_STCODE

#if VALIDATE_CPP_STCODE
  stcode::dbg::reset();
#endif

  const size_t extStcodeId = p.branchlessCppStblkcodeId;

  if (DAGOR_UNLIKELY(dump_owner.stcodeCtx.stblkcodeExMode == stcode::ExecutionMode::BYTECODE))
  {
    res = record_st_block_via_bytecode((const char *)shClass.name, getVars(), p.stblkcodeId, p.renderStateNo, dump);
  }
  else
  {
    G_ASSERTF(&dump == &shBinDump(), "CppStblkcode is not supported for additional dumps yet.");
    res = record_st_block_via_cpp_stcode((const char *)shClass.name, getVars(), extStcodeId, p.renderStateNo);
  }

#if VALIDATE_CPP_STCODE
  // Collect records
  if (dump_owner.stcodeCtx.stblkcodeExMode == stcode::ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    res = record_st_block_via_bytecode((const char *)shClass.name, getVars(), p.stblkcodeId, p.renderStateNo, dump);

  stcode::dbg::validate_accumulated_records(extStcodeId, p.stblkcodeId, shClass.name.data(), false);
#endif

#else

  res = record_st_block_via_bytecode((const char *)shClass.name, getVars(), p.stblkcodeId, p.renderStateNo, dump);

#endif

  return res;
}

void ShaderElement::invalidate_cached_state_block()
{
  if (shaders_internal::cached_state_block != eastl::to_underlying(ShaderStateBlockId::Invalid))
    shaders_internal::cached_state_block = eastl::to_underlying(ShaderStateBlockId::Invalid);
}
const char *ScriptedShaderElement::getShaderClassName() const { return (const char *)shClass.name; }

void rebuild_shaders_stateblocks()
{
  using namespace shaders_internal;
  d3d::GpuAutoLock gpuLock; // this is to avoid rendering from other thread
  shaders_internal::BlockAutoLock autoLock;

  iterate_all_shader_dumps(
    [](ScriptedShadersBinDumpOwner &owner) {
      for (auto m : owner.shaderMats)
        if (m)
        {
          ScriptedShaderElement *elem = m->getElem();
          if (elem)
            elem->resetStateBlocks();
        }
    },
    false);
  ShaderStateBlock::clear();
}

void defrag_shaders_stateblocks(bool force)
{
  d3d::GpuAutoLock gpuLock; // this is to avoid rendering from other thread
  shaders_internal::BlockAutoLock autoLock;
  if ((force && ShaderStateBlock::deleted_blocks > 0) ||
      (ShaderStateBlock::deleted_blocks > ShaderStateBlock::blocks.totalElements() / 2))
    rebuild_shaders_stateblocks();
}

void reset_bindless_samplers() { ShaderStateBlock::reset_samplers(); }

int ScriptedShaderElement::getSupportedBlock(int variant, int layer) const
{
  shader_layout::blk_word_t sb = *code.suppBlockUid[variant];
  return ShaderGlobal::decodeBlock(sb, layer);
}
