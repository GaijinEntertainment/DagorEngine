// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC
#undef _DEBUG_TAB_ // exec_stcode if called too frequently to allow any perfomance penalty there
#endif

#include "scriptSElem.h"
#include "scriptSMat.h"
#include "shAssert.h"
#include "mapBinarySearch.h"
#include <shaders/shFunc.h>
#include <shaders/shUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shLimits.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderDbg.h>
#include "shStateBlk.h"
#include "constRemap.h"
#include "shStateBlock.h"
#include "shRegs.h"
#include <3d/dag_render.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <3d/fileTexFactory.h>
#include <supp/dag_prefetch.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <generic/dag_tab.h>
#include <math/dag_intrin.h>
#include <math/dag_TMatrix4more.h>
#include <3d/dag_texIdSet.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <supp/dag_cpuControl.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/fixed_vector.h>
#include <drv/3d/dag_renderStates.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <supp/dag_alloca.h>

#include <shaders/dag_stcode.h>
#include "profileStcode.h"
#include "stcode/compareStcode.h"

#define VEC_ALIGN(v, a)
// #define VEC_ALIGN(v, a)  G_ASSERT((v & (a-1)) == 0)

// #define DEBUG_RENDER
#if defined(DEBUG_RENDER)
#define S_DEBUG debug
#else
__forceinline bool DEBUG_F(...) { return false; };
#define S_DEBUG 0 && DEBUG_F
#endif

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
static inline VPROG get_vprog(int i, F get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (uint32_t(i) >= shBinDumpOwner().vprId.size())
    return BAD_VPROG;

  VPROG vpr = interlocked_acquire_load(shBinDumpOwner().vprId[i]);
  if (vpr != BAD_VPROG)
    return vpr;

  temporary_disable_fp_exceptions();
  ShaderBytecode tmpbuf(framemem_ptr());
  d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::VERTEX), &vpr);
  if (vpr == BAD_VPROG)
    vpr = d3d::create_vertex_shader(shBinDumpOwner().getCode(i, ShaderCodeType::VERTEX, tmpbuf).data());
  restore_fp_exceptions_state();

  if (vpr != BAD_VPROG)
  {
    VPROG pvpr = interlocked_compare_exchange(shBinDumpOwner().vprId[i], vpr, BAD_VPROG);
    if (DAGOR_LIKELY(pvpr == BAD_VPROG))
    {
#if DAGOR_DBGLEVEL > 0
      d3d::driver_command(Drv3dCommand::SET_VS_DEBUG_INFO, (void *)&vpr, (void *)get_debug_info());
#endif
    }
    else // unlikely case when other thread created and set this vprog slot first
    {
      d3d::delete_vertex_shader(vpr);
      vpr = pvpr;
    }
  }
  else
    DAG_FATAL("Cant create VS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(), d3d::get_last_error());
  return vpr;
}

template <typename F>
static inline FSHADER get_fshader(int i, F get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (i < 0 || i >= shBinDumpOwner().fshId.size())
    return BAD_FSHADER;

  FSHADER fsh = interlocked_acquire_load(shBinDumpOwner().fshId[i]);
  if (fsh != BAD_FSHADER)
    return fsh;

  temporary_disable_fp_exceptions();
  ShaderBytecode tmpbuf(framemem_ptr());
  d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::PIXEL), &fsh);
  if (fsh == BAD_FSHADER)
    fsh = d3d::create_pixel_shader(shBinDumpOwner().getCode(i, ShaderCodeType::PIXEL, tmpbuf).data());
  restore_fp_exceptions_state();

  if (fsh != BAD_FSHADER)
  {
    FSHADER pfsh = interlocked_compare_exchange(shBinDumpOwner().fshId[i], fsh, BAD_FSHADER);
    if (DAGOR_LIKELY(pfsh == BAD_FSHADER))
    {
#if DAGOR_DBGLEVEL > 0
      d3d::driver_command(Drv3dCommand::SET_PS_DEBUG_INFO, (void *)&fsh, (void *)get_debug_info());
#endif
    }
    else // unlikely case when other thread created and set this fsh slot first
    {
      d3d::delete_pixel_shader(fsh);
      fsh = pfsh;
    }
  }
  else
    DAG_FATAL("Cant create PS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(), d3d::get_last_error());

  return fsh;
}

template <typename F>
static PROGRAM get_compute_prg(int i, F get_debug_info)
{
  G_ASSERT(d3d::is_inited());
  if (i < 0 || i >= shBinDumpOwner().fshId.size())
    return BAD_PROGRAM;

  static constexpr PROGRAM CS_FLAG_BIT = 0x10000000;

  PROGRAM csh = interlocked_acquire_load(shBinDumpOwner().fshId[i]);
  if (csh != BAD_PROGRAM)
  {
    G_ASSERT(csh & CS_FLAG_BIT);
    return csh & ~CS_FLAG_BIT;
  }

  temporary_disable_fp_exceptions();
  ShaderBytecode tmpbuf(framemem_ptr());
  d3d::driver_command(Drv3dCommand::GET_SHADER, void_ptr_cast(i), void_ptr_cast(ShaderCodeType::COMPUTE), &csh);
  if (csh == BAD_PROGRAM)
    csh = d3d::create_program_cs(shBinDumpOwner().getCode(i, ShaderCodeType::COMPUTE, tmpbuf).data(), CSPreloaded::Yes);
  restore_fp_exceptions_state();

  if (csh != BAD_PROGRAM)
  {
    FSHADER pcsh = interlocked_compare_exchange(shBinDumpOwner().fshId[i], csh | CS_FLAG_BIT, BAD_PROGRAM);
    if (DAGOR_LIKELY(pcsh == BAD_FSHADER))
    {
      // TODO: set debug info for the driver object
    }
    else // unlikely case when other thread created and set this fsh slot first
    {
      G_ASSERT(pcsh & CS_FLAG_BIT);
      d3d::delete_program(csh);
      csh = pcsh & ~CS_FLAG_BIT;
    }
  }
  else
    DAG_FATAL("Cant create CS #%d: %s\nDriver3d error:\n%s", i, get_debug_info(), d3d::get_last_error());

  return csh;
}

__forceinline void push_int(Tab<uint8_t> &values, const int *v) { append_items(values, sizeof(int), (const uint8_t *)v); }

template <typename T>
__forceinline void push_ptr(Tab<uint8_t> &values, T *v)
{
  S_DEBUG("push ptr: %X (size=%d)", v, sizeof(*v));
  uint64_t ptr64 = (uint64_t)(uintptr_t)v;
  append_items(values, sizeof(ptr64), (const uint8_t *)ptr64);
  S_DEBUG("%d %d %d %d - %X", values[0], values[1], values[2], values[3], *(T *)&values[0]);
}

__forceinline void push_real(Tab<uint8_t> &values, const real *v) { append_items(values, sizeof(real), (const uint8_t *)v); }

__forceinline void push_vec(Tab<uint8_t> &values, const real *v) { append_items(values, sizeof(real) * 4, (const uint8_t *)v); }


// push 'count' vectors
__forceinline void push_vec(Tab<uint8_t> &values, const real *v, int count)
{
  append_items(values, sizeof(real) * 4 * count, (const uint8_t *)v);
}


__forceinline void ScriptedShaderElement::prepareShaderProgram(ID_T &pass_id, int variant, unsigned int variant_code) const
{
#if DAGOR_DBGLEVEL > 0
  String debugInfoStr(framemem_ptr());
  auto get_debug_info = [&]() {
    if (debugInfoStr.empty())
    {
      bool hvc = variant_code != ~0u;
#if _TARGET_C1 || _TARGET_C2

#else
      const char *separator = "\n";
#endif
      debugInfoStr.printf(0, "%s%s", (const char *)shClass.name, hvc ? separator : "");
      if (hvc)
        shaderbindump::decodeVariantStr(code.dynVariants.codePieces, variant_code, debugInfoStr);
    }
    return debugInfoStr.c_str();
  };
#else
  auto get_debug_info = [] { return (const char *)nullptr; };
  G_UNUSED(variant_code);
#endif

  VPROG vpr = get_vprog(code.passes[variant].rpass->vprId, get_debug_info);
  FSHADER fsh = get_fshader(code.passes[variant].rpass->fshId, get_debug_info);

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
void ScriptedShaderElement::preparePassIdOOL(ID_T &pass_id, int variant, unsigned int variant_code) const
{
  pass_id.v.store(recordStateBlock(*code.passes[variant].rpass));
  if (pass_id.pr == -2)
    ScriptedShaderElement::prepareShaderProgram(pass_id, variant, variant_code);
}

__forceinline void ScriptedShaderElement::preparePassId(ID_T &pass_id, int variant, unsigned int variant_code) const
{
  if (pass_id.v.load(std::memory_order_relaxed) == PackedPassId::DELETED_STATE_BLOCK_ID)
    preparePassIdOOL(pass_id, variant, variant_code);
}

static __forceinline void applyPassIdShaders(int prog, const char *name, VDECL usedVdecl)
{
  G_UNUSED(name);
  G_UNUSED(usedVdecl);
#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
  d3d::driver_command(Drv3dCommand::AFTERMATH_MARKER, (void *)name, /*copyname*/ (void *)(uintptr_t)1);
#endif

  d3d::set_program(prog);
#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
  G_ASSERT(usedVdecl == d3d::get_program_vdecl(prog));
#endif
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

  S_DEBUG("make shelem for '%s'", (const char *)shClass.name);
  uint8_t *vars = getVars();
  memset(vars, 0, sizeof(uint8_t) * code.varSize);

  // execute init code
  int i;
  Tab<int> tmp_texVarOffsets(tmpmem);
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
      passes[i].id.v.store(PackedPassId::DELETED_STATE_BLOCK_ID);
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

    clear_and_resize(varMapTable, pcs.size());
    for (i = 0; i < pcs.size(); i++)
    {
      const shaderbindump::Interval &interval = shBinDump().intervals[pcs[i].intervalId];
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
            DAG_FATAL("[ERROR] variable '%s' not found in shader element", (const char *)shBinDump().varMap[interval.nameId]);
            continue;
          }

          int offset = code.findVar(varIndex);
          if (offset < 0)
          {
            DAG_FATAL("[ERROR] value of variable '%s' not found in shader element", (const char *)shBinDump().varMap[interval.nameId]);
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
          int offset = shBinDump().globVars.findVar(interval.nameId);
          if (offset < 0)
          {
            DAG_FATAL("[ERROR] global variable '%s' not found in shader element", (const char *)shBinDump().varMap[interval.nameId]);
            continue;
          }
          type = shBinDump().globVars.v[offset].type;
        }
        break;

        default: DAG_FATAL("invalid dynamic type=%d nameId/extType=%d", interval.type, interval.nameId);
      }
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
  usedVdecl = vDecl;
  resetShaderPrograms();
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

void ScriptedShaderElement::preCreateShaderPrograms()
{
  for (int i = 0; i < passes.size(); i++)
    if (passes[i].id.pr == -2)
      prepareShaderProgram(passes[i].id, i, invalid_variant);
}

void ScriptedShaderElement::detachElem()
{
  resetShaderPrograms();
  resetStateBlocks();
  dynVariantOption = NO_DYNVARIANT;

  //== offsetof() doesn't work with references, so we rely on fixed layout: code; shClass; stateBlocksSpinLock;
  size_t ofs = offsetof(ScriptedShaderElement, stateBlocksSpinLock);
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
  for (int i = 0; i < passes.size(); i++)
    preparePassId(passes[i].id, i, invalid_variant);
}
void ScriptedShaderElement::resetStateBlocks()
{
  eastl::fixed_vector<ShaderStateBlockId, 32, true> toDelete;
  // we need this spinlock, because we here invalidate pass_id.v and we can be creating in render thread.
  // so, when we were creating we could potentially create it with old textures (BaseTexture*)
  // we can't render with old texture*, because it is released in replaceTexture
  //   (it can't be released in just destructor, because we can't render destructing shader anyway)
  //   so, in order to remove this spin lock we have to stop delete textures immediately on replace.
  //  We also have to guarantee that textures stateblocks were written with new textures
  //    which we can do by just capture shaders_internal::BlockAutoLock autoLock in replaceTexture
  while (!stateBlocksSpinLock.tryLock())
    sleep_msec(1);
  for (int i = 0; i < passes.size(); i++)
  {
    ShaderStateBlockId v = passes[i].id.v.load();
    if (v != ShaderStateBlockId::Invalid && v != PackedPassId::DELETED_STATE_BLOCK_ID)
    {
      toDelete.push_back(v);
      passes[i].id.v.store(PackedPassId::DELETED_STATE_BLOCK_ID);
    }
  }
  stateBlocksSpinLock.unlock();

  for (auto &b : toDelete)
    ShaderStateBlock::delBlock(b);
}

int ScriptedShaderElement::chooseCachedDynamicVariant(unsigned int variant_code) const
{
  if (dynVariantOption == NO_DYNVARIANT)
    return code.passes.size() ? 0 : -1;
  int id = code.dynVariants.findVariant(variant_code);
  if (id == code.dynVariants.FIND_NULL)
    return -1;
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
            variant_code += shBinDump().intervals[vmt.intervalId].getNormalizedValue((*(int *)var_ptr)) * vmt.totalMul;
            break;
          case SHVT_REAL:
            variant_code += shBinDump().intervals[vmt.intervalId].getNormalizedValue((*(real *)var_ptr)) * vmt.totalMul;
            break;
          case SHVT_COLOR4:
            variant_code += shBinDump().intervals[vmt.intervalId].getNormalizedValue(
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
#if DAGOR_DBGLEVEL > 0
    dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> pcs = code.dynVariants.codePieces;
    bool has_dump = shaderbindump::hasShaderInvalidVariants(shClass.nameId);
    if (!shaderbindump::markInvalidVariant(shClass.nameId, &code - shClass.code.begin(), variant_code))
      return -1;
    String tmp(framemem_ptr());
    const char *tmp_st = shaderbindump::decodeStaticVariants(shClass, &code - shClass.code.begin());
    const char *varstr = shaderbindump::decodeVariantStr(pcs, variant_code, tmp);
    logerr("%p.dynamic variant %u not found (shader %s/%d)\n\n"
           "normalized dynvariant:\n  %s\n\n"
           "in one of normalized statvariants:\n%s\n\n"
           "Check for assumes/validVariants or render code conditions",
      this, variant_code, (const char *)shClass.name, &code - shClass.code.begin(), varstr, tmp_st);
    if (!has_dump)
      shaderbindump::dumpShaderInfo(shClass);
    tmp.clear();
    DAG_FATAL("%p.dynamic variant %u not found (shader %s/%d)\n\n"
              "normalized dynvariant:\n  %s\n\n"
              "in one of normalized statvariants:\n%s\n\n"
              "Check for assumes/validVariants or render code conditions",
      this, variant_code, (const char *)shClass.name, &code - shClass.code.begin(), varstr, tmp_st);
#else
    logerr("%p.dynamic variant %u not found (shader %s/%d)", this, variant_code, (const char *)shClass.name,
      &code - shClass.code.begin());
#endif
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
  return chooseDynamicVariant(shBinDumpOwner().globIntervalNormValues, out_variant_code);
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
static void scripted_shader_element_default_before_resource_used_callback(const ShaderElement *selem, const D3dResource *,
  const char *){};
void (*scripted_shader_element_on_before_resource_used)(const ShaderElement *selem, const D3dResource *,
  const char *) = &scripted_shader_element_default_before_resource_used_callback;
#else
static void scripted_shader_element_on_before_resource_used(const ShaderElement *, const D3dResource *, const char *){};
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

  STCODE_PROFILE_BEGIN();

  MEASURE_STCODE_PERF_START;
  const vec4f *tm_world_c = NULL, *tm_lview_c = NULL;

  // ShUtils::shcod_dump(cod);

  shader_assert::ScopedShaderAssert scoped_shader_assert(this_elem.shClass);

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
        color4_reg(regs, ro) = shBinDump().globVars.get<Color4>(index);
      }
      break;
      case SHCOD_GET_GMAT44:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        float4x4_reg(regs, ro) = shBinDump().globVars.get<TMatrix4>(index);
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
        real_reg(regs, ro) = shBinDump().globVars.get<real>(index);
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
        d3d::SamplerHandle smp = shBinDump().globVars.get<d3d::SamplerHandle>(id);
        if (smp != d3d::INVALID_SAMPLER_HANDLE)
          d3d::set_sampler(stage, slot, smp);

        stcode::dbg::record_set_sampler(stcode::dbg::RecordType::REFERENCE, stage, slot, smp);
      }
      break;
      case SHCOD_SAMPLER:
      {
        const uint32_t slot = shaderopcode::getOpStageSlot_Slot(opc);
        const uint32_t ofs = shaderopcode::getOpStageSlot_Reg(opc);
        d3d::SamplerHandle smp = *(d3d::SamplerHandle *)&vars[ofs];
        if (smp != d3d::INVALID_SAMPLER_HANDLE)
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
        tex_reg(regs, reg) = shBinDump().globVars.getTex(index).texId;
      }
      break;
      case SHCOD_GET_GBUF:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        Sbuffer *&tex = buf_reg(regs, reg);
        tex = shBinDump().globVars.getBuf(index).buf;
      }
      break;
      case SHCOD_GET_GTLAS:
      {
        const uint32_t reg = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        RaytraceTopAccelerationStructure *&tlas = tlas_reg(regs, reg);
        tlas = shBinDump().globVars.get<RaytraceTopAccelerationStructure *>(index);
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
          ShUtils::shcod_dump(cod, &shBinDump().globVars, &this_elem.shClass.localVars, this_elem.code.stVarMap);
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
        int *reg = get_reg_ptr<int>(regs, ro);
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
            ShUtils::shcod_dump(cod, &shBinDump().globVars, &this_elem.shClass.localVars, this_elem.code.stVarMap);
            DAG_FATAL("divide by zero [color4[%d]] while exec shader code. stopped at operand #%d", j, codp - cod.data());
          }
#endif
      }
      break;

      case SHCOD_GET_GINT:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        int_reg(regs, ro) = shBinDump().globVars.get<int>(index);
      }
      break;
      case SHCOD_GET_GINT_TOREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        real_reg(regs, ro) = shBinDump().globVars.get<int>(index);
      }
      break;
      case SHCOD_GET_GIVEC_TOREAL:
      {
        const uint32_t ro = shaderopcode::getOp2p1(opc);
        const uint32_t index = shaderopcode::getOp2p2(opc);
        const IPoint4 &ivec = shBinDump().globVars.get<IPoint4>(index);
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

  STCODE_PROFILE_END();
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

      String errorMessage(0,
        "shader <%s>[S:%d/D:%d] doesn't support stateWord=%04X\n\n"
        "current blocks=(%s:%s:%s), supported=(%s:%s:%s)\n",
        (const char *)shClass.name, &code - shClass.code.begin(), dyn_var_n, shaderbindump::blockStateWord,
        b_frame >= 0 ? (const char *)shBinDump().blockNameMap[b_frame] : "NULL",
        b_scene >= 0 ? (const char *)shBinDump().blockNameMap[b_scene] : "NULL",
        b_obj >= 0 ? (const char *)shBinDump().blockNameMap[b_obj] : "NULL",
        shframe >= 0 ? (const char *)shBinDump().blockNameMap[shframe] : "NULL",
        shscene >= 0 ? (const char *)shBinDump().blockNameMap[shscene] : "NULL",
        shobj >= 0 ? (const char *)shBinDump().blockNameMap[shobj] : "NULL");

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
  const PackedPassId *__restrict curPasses = &passes[cur_variant];
  OSSpinlockScopedLock lock(stateBlocksSpinLock);
  ID_T &pass_id = curPasses->id;
  ShaderStateBlockId v = pass_id.v.load(std::memory_order_relaxed);
  if (v == PackedPassId::DELETED_STATE_BLOCK_ID)
  {
    preparePassIdOOL(pass_id, curPasses - passes.data(), variant_code);
    v = pass_id.v.load(std::memory_order_relaxed);
  }
  state_index = v;
  program = pass_id.pr;
  render_state = ShaderStateBlock::blocks[state_index].stateIdx;
  const_state = ShaderStateBlock::blocks[state_index].constIdx;
  tex_state = ShaderStateBlock::blocks[state_index].samplerIdx;
}

bool ScriptedShaderElement::setStates() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return false;
  const PackedPassId *curPasses = &passes[curVariant];

  OSSpinlockScopedLock lock(stateBlocksSpinLock);

#if DAGOR_DBGLEVEL > 0
  if (!code.suppBlockUid.empty())
    check_state_blocks_conformity(code, curVariant, shClass);
#endif
  ID_T &pass_id = curPasses->id;
  ShaderStateBlockId v = pass_id.v.load(std::memory_order_relaxed);
  if (v == PackedPassId::DELETED_STATE_BLOCK_ID)
  {
    preparePassIdOOL(pass_id, curPasses - passes.data(), variant_code);
    v = pass_id.v.load(std::memory_order_relaxed);
  }
  setStatesForVariant(curVariant, pass_id.pr, v);
  return true;
}

void ScriptedShaderElement::setProgram(uint32_t variant)
{
  OSSpinlockScopedLock lock(stateBlocksSpinLock);
  auto &pass = passes[variant];
  ShaderStateBlockId v = pass.id.v.load(std::memory_order_relaxed);
  if (v == PackedPassId::DELETED_STATE_BLOCK_ID)
  {
    preparePassIdOOL(pass.id, &pass - passes.data(), 0);
    v = pass.id.v.load(std::memory_order_relaxed);
  }
  ShaderStateBlock::blocks[v].apply(tex_level);
  d3d::set_program(pass.id.pr);
}

PROGRAM ScriptedShaderElement::getComputeProgram(const shaderbindump::ShaderCode::ShRef *p) const
{
  G_ASSERTF(p->fshId != shaderbindump::ShaderCode::INVALID_FSH_VPR_ID, "Compute shader is null");
  G_ASSERTF(p->vprId == shaderbindump::ShaderCode::INVALID_FSH_VPR_ID, "Unexpected vpr=%d in compute shader", p->vprId);

  // TODO: debug info for CS is not implemented yet
  return get_compute_prg(p->fshId, [] { return (const char *)nullptr; });
}

void copy_current_global_variables_states(GlobalVariableStates &gv)
{
  if (DAGOR_UNLIKELY(!gv.globIntervalNormValues.size() || gv.generation != shaderbindump::get_generation()))
  {
    gv.globIntervalNormValues.resize(shBinDumpOwner().globIntervalNormValues.size());
    gv.generation = shaderbindump::get_generation();
  }
  memcpy(gv.globIntervalNormValues.data(), shBinDumpOwner().globIntervalNormValues.data(),
    shBinDumpOwner().globIntervalNormValues.size());
}

int get_dynamic_variant_states(const GlobalVariableStates &global_variants_state, const ScriptedShaderElement &s, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state)
{
  unsigned int variant_code;
  int curVariant = s.chooseDynamicVariant(global_variants_state.globIntervalNormValues, variant_code);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variant_code, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

int get_dynamic_variant_states(const ScriptedShaderElement &s, uint32_t &program, ShaderStateBlockId &state_index,
  shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state, shaders::TexStateIdx &tex_state)
{
  unsigned int variant_code;
  int curVariant = s.chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variant_code, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

int get_cached_dynamic_variant_states(const ScriptedShaderElement &s, dag::ConstSpan<int> cache, uint32_t &program,
  ShaderStateBlockId &state_index, shaders::RenderStateId &render_state, shaders::ConstStateIdx &const_state,
  shaders::TexStateIdx &tex_state)
{
  const int variantCode = cache[s.dynVariantCollectionId];
  int curVariant = s.chooseCachedDynamicVariant(variantCode);
  if (curVariant < 0)
    return -1;
  s.getDynamicVariantStates(variantCode, curVariant, program, state_index, render_state, const_state, tex_state);
  return curVariant;
}

uintptr_t get_static_variant(const ScriptedShaderElement &s) { return uintptr_t(&s.code); }

void ScriptedShaderElement::execute_chosen_stcode(uint16_t stcodeId, const shaderbindump::ShaderCode::Pass *cPass, const uint8_t *vars,
  bool is_compute) const
{
  TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV("execute_chosen_stcode__shader");

  G_ASSERT(stcodeId < shBinDump().stcode.size());

#if CPP_STCODE

#if VALIDATE_CPP_STCODE
  stcode::dbg::reset();
#endif

#if STCODE_RUNTIME_CHOICE
  if (stcode::execution_mode() == stcode::ExecutionMode::BYTECODE)
    exec_stcode(stcodeId, cPass, *this);
  else
#endif
    stcode::run_routine(stcodeId, vars, is_compute, tex_level, shClass.name.data());

#if VALIDATE_CPP_STCODE
  // Collect records
  if (stcode::execution_mode() == stcode::ExecutionMode::TEST_CPP_AGAINST_BYTECODE)
    exec_stcode(stcodeId, cPass, *this);

  stcode::dbg::validate_accumulated_records(stcodeId, shClass.name.data());
#endif

#else

  exec_stcode(stcodeId, cPass, *this);

#endif
}

void ScriptedShaderElement::setStatesForVariant(int curVariant, uint32_t program, ShaderStateBlockId state_index) const
{
  const PackedPassId *curPass = &passes[curVariant];
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
#if DAGOR_DBGLEVEL > 0
  if (!code.suppBlockUid.empty())
    check_state_blocks_conformity(code, curVariant, shClass);
#endif

  d3d::set_program(program);
  G_ASSERT(curPass->id.pr == program && curPass->id.v.load(std::memory_order_relaxed) == state_index);
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

  if (codeCp->rpass->stcodeId != 0xFFFF)
  {
    const int stcodeId = codeCp->rpass->stcodeId;
    const uint8_t *vars = getVars();

    execute_chosen_stcode(stcodeId, codeCp, vars, false);
  }
}

void set_states_for_variant(const ScriptedShaderElement &s, int curVariant, uint32_t program, ShaderStateBlockId state_index)
{
  s.setStatesForVariant(curVariant, program, state_index);
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

  d3d::set_program(getComputeProgram(p));
  if (p->stcodeId != shaderbindump::ShaderCode::INVALID_FSH_VPR_ID)
    execute_chosen_stcode(p->stcodeId, codeCp, getVars(), true);
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

#include "stateBlockStCode.h"

const shaderbindump::ShaderCode::ShRef *ScriptedShaderElement::getPassCode() const
{
  unsigned int variant_code;
  int curVariant = chooseDynamicVariant(variant_code);
  if (curVariant < 0)
    return nullptr;
  const shaderbindump::ShaderCode::Pass *codeCp = &code.passes[curVariant];
  return &*codeCp->rpass;
}

ShaderStateBlockId ScriptedShaderElement::recordStateBlock(const shaderbindump::ShaderCode::ShRef &p) const
{
  using namespace shaderopcode;
  G_ASSERT(p.stblkcodeId < shBinDump().stcode.size());
  dag::ConstSpan<int> cod = shBinDump().stcode[p.stblkcodeId];
  const int *__restrict codp = cod.data();
  const int *__restrict codp_end = codp + shBinDump().stcode[p.stblkcodeId].size();

  const uint8_t *vars = getVars();
  auto maybeShStateBlock =
    execute_st_block_code(codp, codp_end, (const char *)shClass.name, p.stblkcodeId, [&](const int &codAt, char *regs, Point4 *, int) {
      uint32_t opc = codAt;
      switch (getOp(opc))
      {
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
          Tex &t = *(Tex *)&vars[ofs];
          tex_reg(regs, reg) = t.texId == D3DRESID(D3DRESID::INVALID_ID2) ? BAD_TEXTUREID : t.texId;
          t.get();
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
          int *reg = get_reg_ptr<int>(regs, ro);
          reg[0] = *(int *)&vars[ofs];
          reg[1] = *(int *)&vars[ofs + 1];
          reg[2] = *(int *)&vars[ofs + 2];
          reg[3] = *(int *)&vars[ofs + 3];
        }
        break;


        default:
          logerr("recordEmulatedStateBlock: illegal instruction %u %s (index=%d)", getOp(opc), ShUtils::shcod_tokname(getOp(opc)),
            &codAt - cod.data());
          return false;
      }
      return true;
    });

  if (!maybeShStateBlock.has_value())
    return DEFAULT_SHADER_STATE_BLOCK_ID;

  return ShaderStateBlock::addBlock(eastl::move(*maybeShStateBlock), shBinDump().renderStates[p.renderStateNo]);
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

  for (auto m : shader_mats)
    if (m)
    {
      ScriptedShaderElement *elem = m->getElem();
      if (elem)
        elem->resetStateBlocks();
    }
  close_shader_block_stateblocks(false);
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
