// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadersBinaryData.h"
#include "shStateBlock.h"
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderDbg.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_tabHlp.h>

#if DAGOR_DBGLEVEL > 0

struct ExecStcodeTime
{
  unsigned int count;
  __int64 time;
};

bool enable_measure_stcode_perf = false;
static Tab<ExecStcodeTime> current_exec_stcode_time(midmem);
static Tab<ExecStcodeTime> last_exec_stcode_time(midmem);


class ILocalDumpVariantData
{
public:
  virtual void dump(int idx) const = 0;
};

static void decodeVariant(const shaderbindump::VariantTable &vt, int code)
{
  using namespace shaderbindump;
  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = ::shBinDump().intervals[vt.codePieces[i].intervalId];
    int mul = vt.codePieces[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    debug_("%d ", subcode);
  }
}

static void dumpVariantTable(const shaderbindump::VariantTable &vt, int indent, int code_num, const ILocalDumpVariantData &vdump)
{
  using namespace shaderbindump;
  int total_variants = code_num ? 1 : 0;
  if (vt.codePieces.size() > 0)
  {
    const VariantTable::IntervalBind &ib = vt.codePieces.back();
    total_variants = ::shBinDump().intervals[ib.intervalId].getValCount() * ib.totalMul;
  }

  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL: debug_("[EQUAL, %d->%d]:\n", total_variants, code_num); break;
    case VariantTable::MAPTYPE_LOOKUP: debug_("[LOOKUP, %d->%d]:\n", total_variants, code_num); break;
    case VariantTable::MAPTYPE_QDIRECT: debug_("[QDIRECT, %d->%d->%d]:\n", total_variants, vt.mapData.size(), code_num); break;
    case VariantTable::MAPTYPE_QINTERVAL: debug_("[QLOOKUP, %d->%d->%d]:\n", total_variants, vt.mapData.size(), code_num); break;
  }

  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = ::shBinDump().intervals[vt.codePieces[i].intervalId];
    int mul = vt.codePieces[i].totalMul;

    debug_("%*scp[%d]: MUL=%d  ", indent, "", i, mul);
    switch (ival.type)
    {
      case Interval::TYPE_MODE:
        switch (ival.nameId)
        {
          case Interval::MODE_2SIDED: debug_("mode 2SIDED: 0..1\n"); break;
          case Interval::MODE_REAL2SIDED: debug_("mode REAL2SIDED: 0..1\n"); break;
        }
        break;

      case Interval::TYPE_INTERVAL:
      case Interval::TYPE_GLOBAL_INTERVAL:
        debug_("%s[%d] %s, 0..%u\n", (const char *)shBinDump().varMap[ival.nameId], ival.nameId,
          ival.type == Interval::TYPE_INTERVAL ? "local" : "global", ival.getValCount() - 1);
        break;
    }

    if (ival.type != Interval::TYPE_MODE)
    {
      debug_("%*sintervals: ", indent + 2, "");
      for (int j = 0; j < ival.maxVal.size(); j++)
        debug_("[%d] < %.3f, ", j, ival.maxVal[j]);
      debug_("[%d]other\n", ival.maxVal.size());
    }
  }

  debug_("\n%*sTable:\n", indent, "");
  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL:
      for (int i = 0; i < total_variants; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", i, i);
        decodeVariant(vt, i);
        vdump.dump(i);
      }
      break;
    case VariantTable::MAPTYPE_LOOKUP:
      for (int i = 0; i < total_variants; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", i, vt.mapData[i]);
        decodeVariant(vt, i);
        vdump.dump(vt.mapData[i]);
      }
      break;
    case VariantTable::MAPTYPE_QDIRECT:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
        decodeVariant(vt, vt.mapData[i]);
        vdump.dump(vt.mapData[i + vt.mapData.size() / 2]);
      }
      break;
    case VariantTable::MAPTYPE_QINTERVAL:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
      {
        debug_("%*s%4d..%-4d -> %5d:   ", indent + 2, "", vt.mapData[i],
          i < vt.mapData.size() / 2 ? vt.mapData[i + 1] - 1 : vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
        decodeVariant(vt, vt.mapData[i]);
        vdump.dump(vt.mapData[i + vt.mapData.size() / 2]);
      }
      break;
  }
}

void shaderbindump::dumpVar(const shaderbindump::VarList &vars, int i)
{
  int nameId = vars.getNameId(i);
  debug_("  %d: %s%s[%d]= ", i, vars.isPublic(i) ? "public " : "", (const char *)shBinDump().varMap[nameId], nameId);
  switch (vars.getType(i))
  {
    case SHVT_INT: debug_("int(%d)\n", vars.get<int>(i)); break;
    case SHVT_REAL: debug_("real(%.3f)\n", vars.get<real>(i)); break;
    case SHVT_COLOR4:
    {
      Color4 c = vars.get<Color4>(i);
      debug_("color4(" FMT_P4 ")\n", c.r, c.g, c.b, c.a);
      break;
    }
    case SHVT_FLOAT4X4:
    {
      debug_("float4x4()\n");
      break;
    }
    case SHVT_TEXTURE: debug_("tex(%d)\n", vars.getTex(i).texId); break;
    case SHVT_BUFFER: debug_("buf(%d)\n", vars.getBuf(i).bufId); break;
    case SHVT_TLAS: debug_("tlas(%d)\n", vars.getBuf(i).bufId); break;
    case SHVT_INT4:
    {
      const IPoint4 &i4 = vars.get<IPoint4>(i);
      debug_("int4(%d,%d,%d,%d)\n", i4.x, i4.y, i4.z, i4.w);
      break;
    }
    case SHVT_SAMPLER: debug_("sampler\n"); break;
    default: debug_("unknown type: %d\n", vars.getType(i));
  }
}

void shaderbindump::dumpVars(const shaderbindump::VarList &vars)
{
  for (int i = 0; i < vars.size(); i++)
    dumpVar(vars, i);
}

void shaderbindump::dumpShaderInfo(const shaderbindump::ShaderClass &cls, bool dump_variants)
{
  uint32_t total_passes = 0;
  uint32_t total_unique_shaders = 0;
  {
    ska::flat_hash_set<uint32_t> unique_shader_ids = {};
    for (const ShaderCode &code : cls.code)
    {
      total_passes += code.passes.size();
      for (const ShaderCode::Pass &pass : code.passes)
        unique_shader_ids.insert((pass.rpass->vprId << 16) | pass.rpass->fshId);
    }
    total_unique_shaders = unique_shader_ids.size();
  }

  debug("--- shader[%d] %s dump (%d passes, %d unique passes):\n"
        " timestamp(%lld)\n"
        " local vars(%d):",
    cls.nameId, (const char *)cls.name, total_passes, total_unique_shaders, cls.getTimestamp(), cls.localVars.v.size());
  shaderbindump::dumpVars(cls.localVars);

  debug_("\n static init(%d):\n", cls.initCode.size() / 2);
  for (int i = 0; i < cls.initCode.size(); i += 2)
  {
    int stVarId = cls.initCode[i];
    const char *varname = (const char *)shBinDump().varMap[cls.localVars.v[stVarId].nameId];

    switch (shaderopcode::getOp(cls.initCode[i + 1]))
    {
      case SHCOD_TEXTURE: debug_("  %s <- texture[%u]\n", varname, shaderopcode::getOp2p1(cls.initCode[i + 1])); break;
      case SHCOD_DIFFUSE: debug_("  %s <- mat.diff.color\n", varname); break;
      case SHCOD_EMISSIVE: debug_("  %s <- mat.emis.color\n", varname); break;
      case SHCOD_SPECULAR: debug_("  %s <- mat.spec.color\n", varname); break;
      case SHCOD_AMBIENT: debug_("  %s <- mat. amb.color\n", varname); break;

      default:
        debug_("  unsupported opcode=%08x (%s), var=%s\n", cls.initCode[i + 1],
          ShUtils::shcod_tokname(shaderopcode::getOp(cls.initCode[i + 1])), varname);
    }
  }

  class DumpStVar : public ILocalDumpVariantData
  {
  public:
    virtual void dump(int) const { debug_("\n"); }
  };

  if (!dump_variants)
  {
    const auto &vt = cls.stVariants;
    if (cls.code.size() && vt.codePieces.size() > 0)
    {
      const VariantTable::IntervalBind &ib = vt.codePieces.back();
      int total_variants = ::shBinDump().intervals[ib.intervalId].getValCount() * ib.totalMul;
      debug("\n static variant table: %d variants (codepieces%s for %d variants)", cls.code.size(),
        total_variants >= (64 << 10) ? "!" : "", total_variants);
    }
    else
      debug("\n static variant table: %d variants", cls.code.size());
  }
  else
  {
    debug_("\n static variant table");
    dumpVariantTable(cls.stVariants, 2, cls.code.size(), DumpStVar());
  }

  for (int i = 0; i < cls.code.size(); i++)
  {
    const ShaderCode &code = cls.code[i];

    debug_("  code[%d]: passes=%d initCode=%d, channels=%d,"
           " varSize=%d, codeFlags=0x%08x vertexStride=%d\n",
      i, code.passes.size(), code.initCode.size(), code.channel.size(), code.varSize, code.codeFlags, code.vertexStride);

    debug_("   stvarmap(%d):\n", code.stVarMap.size());
    for (int j = 0; j < code.stVarMap.size(); j++)
      debug_("    [%u] %s  -> %u\n", code.stVarMap[j] & 0xFFFF,
        (const char *)shBinDump().varMap[cls.localVars.v[code.stVarMap[j] & 0xFFFF].nameId], code.stVarMap[j] >> 16);

    debug_("\n  vertex channels(%d):\n", code.channel.size());
    for (int j = 0; j < code.channel.size(); j++)
      debug_("   %d: %s: %s%d [%d] %d/%d\n", j, ShUtils::channel_type_name(code.channel[j].t),
        ShUtils::channel_usage_name(code.channel[j].u), code.channel[j].ui, code.channel[j].mod, code.channel[j].vbu,
        code.channel[j].vbui);

    class DumpDynVar : public ILocalDumpVariantData
    {
      const ShaderCode &code;

    public:
      DumpDynVar(const ShaderCode &c) : code(c) {}
      virtual void dump(int idx) const
      {
        if (idx == 0xFFFE)
        {
          debug_("NULL\n");
          return;
        }
        if (idx == 0xFFFF)
        {
          debug_("not found\n");
          return;
        }
        const ShaderCode::Pass &p = code.passes[idx];
        debug_("shref[0]=(v%d,p%d,s%d,s%d)", p.rpass ? p.rpass->vprId : -1, p.rpass ? p.rpass->fshId : -1,
          p.rpass ? p.rpass->stcodeId : -1, p.rpass ? p.rpass->stblkcodeId : -1);

        if (!code.suppBlockUid.empty())
        {
          const shader_layout::blk_word_t *sb = code.suppBlockUid[idx].begin();
          debug_(" supports:");
          if (!sb || *sb == shader_layout::BLK_WORD_FULLMASK)
            debug_(" no block");
          else
            while (*sb != shader_layout::BLK_WORD_FULLMASK)
            {
              int b_frame = decodeBlock(*sb, ShaderGlobal::LAYER_FRAME);
              int b_scene = decodeBlock(*sb, ShaderGlobal::LAYER_SCENE);
              int b_obj = decodeBlock(*sb, ShaderGlobal::LAYER_OBJECT);

              debug_(" {%04X}(%s:%s:%s)", *sb, b_frame >= 0 ? (const char *)::shBinDump().blockNameMap[b_frame] : "NULL",
                b_scene >= 0 ? (const char *)::shBinDump().blockNameMap[b_scene] : "NULL",
                b_obj >= 0 ? (const char *)::shBinDump().blockNameMap[b_obj] : "NULL");
              sb++;
            }
        }

        debug_("\n");
      }
    };
    if (!dump_variants)
    {
      const auto &vt = code.dynVariants;
      if (code.passes.size() && vt.codePieces.size() > 0)
      {
        const VariantTable::IntervalBind &ib = vt.codePieces.back();
        int total_variants = ::shBinDump().intervals[ib.intervalId].getValCount() * ib.totalMul;
        debug("\n   dynamic variant table: %d variants (codepieces%s for %d variants)", code.passes.size(),
          total_variants >= (64 << 10) ? "!" : "", total_variants);
      }
      else
        debug("\n   dynamic variant table: %d variants", code.passes.size());
      for (int idx = 0; idx < code.passes.size(); idx++)
      {
        const shader_layout::blk_word_t *sb = code.suppBlockUid[idx].begin();
        if (sb && *sb != shader_layout::BLK_WORD_FULLMASK)
        {
          debug_("     pass[%d] supports blocks:", idx);
          while (*sb != shader_layout::BLK_WORD_FULLMASK)
          {
            int b_frame = decodeBlock(*sb, ShaderGlobal::LAYER_FRAME);
            int b_scene = decodeBlock(*sb, ShaderGlobal::LAYER_SCENE);
            int b_obj = decodeBlock(*sb, ShaderGlobal::LAYER_OBJECT);

            debug_(" {%04X}(%s:%s:%s)", *sb, b_frame >= 0 ? (const char *)::shBinDump().blockNameMap[b_frame] : "NULL",
              b_scene >= 0 ? (const char *)::shBinDump().blockNameMap[b_scene] : "NULL",
              b_obj >= 0 ? (const char *)::shBinDump().blockNameMap[b_obj] : "NULL");
            sb++;
          }
          debug("");
        }
      }
    }
    else
    {
      debug_("\n   dynamic variant table");
      dumpVariantTable(code.dynVariants, 4, code.passes.size(), DumpDynVar(code));
    }
  }
  debug("--- end of dump ---\n");
}


#if MEASURE_STCODE_PERF
static int last_frames = 0;
static int last_time = 0;
static int prev_frame_no = 0;
static int last_frame_no = 0;
static bool overdraw_to_stencil = false;

void shaderbindump::add_exec_stcode_time(const shaderbindump::ShaderClass &cls, const __int64 &time)
{
  if (current_exec_stcode_time.size() != shBinDump().classes.size())
  {
    current_exec_stcode_time.resize(shBinDump().classes.size());
    last_exec_stcode_time.resize(shBinDump().classes.size());
    mem_set_0(current_exec_stcode_time);
    mem_set_0(last_exec_stcode_time);
  }

  if (dagor_frame_no() != prev_frame_no && ::get_time_msec() > last_time + 5000)
  {
    last_exec_stcode_time = current_exec_stcode_time;
    last_frames = dagor_frame_no() - last_frame_no;

    last_time = ::get_time_msec();
    last_frame_no = dagor_frame_no();
    mem_set_0(current_exec_stcode_time);
  }

  prev_frame_no = dagor_frame_no();

  unsigned int shaderNo = &cls - shBinDump().classes.begin();
  G_ASSERT(shaderNo < shBinDump().classes.size());

  current_exec_stcode_time[shaderNo].count++;
  current_exec_stcode_time[shaderNo].time += time;
}

static int sort_by_time(int *a, int *b) { return (int)(last_exec_stcode_time[*b].time - last_exec_stcode_time[*a].time); }

void dump_exec_stcode_time()
{
  if (!enable_measure_stcode_perf)
  {
    enable_measure_stcode_perf = true;
    debug("STCODE: stcode profiling enabled");
    return;
  }

  using namespace shaderbindump;
  if (current_exec_stcode_time.size() != ::shBinDump().classes.size() || last_frames == 0)
    return;

  debug("STCODE: exec_stcode time by shader (count per frame, 1 call average time, 1 frame average time):");
  int total_time = 0;
  Tab<int> idxs(tmpmem);
  idxs.reserve(::shBinDump().classes.size());
  idxs.resize(::shBinDump().classes.size());
  for (int i = 0; i < idxs.size(); ++i)
    idxs[i] = i;
  dag_qsort(idxs.data(), idxs.size(), sizeof(int), (cmp_func_t)&sort_by_time);
  for (int i = 0; i < idxs.size(); ++i)
  {
    const int shaderNo = idxs[i];
    ExecStcodeTime &time = last_exec_stcode_time[shaderNo];

    if (time.count == 0)
      continue;

    debug("STCODE: %4u, %4.2f us, %5.2f us - '%s'", time.count / last_frames, (float)time.time / time.count,
      (float)time.time / last_frames, ::shBinDump().classes[shaderNo].name.data());
    total_time += time.time;
  }
  debug("STCODE: Total: %.3f ms per frame", 0.001f * total_time / last_frames);

  int num_active_blocks = 0;
  ShaderStateBlock::blocks.iterate([&](auto &b) { num_active_blocks += b.refCount > 0; });
  debug("STCODE: ShaderStateBlock %d blocks(%d total), %d render states", num_active_blocks, ShaderStateBlock::blocks.getTotalCount(),
    shaders::render_states::get_count());
  dump_bindless_states_stat();
  dump_slot_textures_states_stat();
}


void enable_overdraw_to_stencil(bool enable) { overdraw_to_stencil = enable; }

bool is_overdraw_to_stencil_enabled() { return overdraw_to_stencil; }
#endif

#include <generic/dag_tab.h>
#include <util/dag_fastIntList.h>
static Tab<FastIntList> perShaderMarks(inimem);
bool shaderbindump::markInvalidVariant(int shader_nid, unsigned short stat_varcode, unsigned short dyn_varcode)
{
  if (shader_nid >= perShaderMarks.size())
    perShaderMarks.resize(shader_nid + 1);
  return perShaderMarks[shader_nid].addInt((unsigned(stat_varcode) << 16) | dyn_varcode);
}
bool shaderbindump::hasShaderInvalidVariants(int shader_nid)
{
  if (shader_nid >= perShaderMarks.size())
    return false;
  return perShaderMarks[shader_nid].getList().size() > 0;
}
void shaderbindump::resetInvalidVariantMarks() { clear_and_shrink(perShaderMarks); }

#endif

#if !(MEASURE_STCODE_PERF && DAGOR_DBGLEVEL > 0)
void dump_exec_stcode_time() {}
void enable_overdraw_to_stencil(bool) {}
bool is_overdraw_to_stencil_enabled() { return false; }
#endif
