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

static void decodeVariant(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt, int code)
{
  using namespace shaderbindump;
  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = dump.intervals[vt.codePieces[i].intervalId];
    int mul = vt.codePieces[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    debug_("%d ", subcode);
  }
}

static int find_tex_slot_for_var(dag::ConstSpan<int> init_code, const shaderbindump::VarList &local_vars, int var_name_id)
{
  for (int i = 0; i < init_code.size(); i += 2)
  {
    int stVarId = init_code[i];
    if (local_vars.v[stVarId].nameId == var_name_id && shaderopcode::getOp(init_code[i + 1]) == SHCOD_TEXTURE)
      return shaderopcode::getOp2p1(init_code[i + 1]);
  }
  return -1;
}

static void dump_variant_intervals(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt, int indent,
  const shaderbindump::VarList *local_vars = nullptr, dag::ConstSpan<int> init_code = {})
{
  using namespace shaderbindump;

  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = dump.intervals[vt.codePieces[i].intervalId];
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
      {
        const char *varName = (const char *)dump.varMap[ival.nameId];
        int texSlot = (ival.type == Interval::TYPE_INTERVAL && local_vars && init_code.size())
                        ? find_tex_slot_for_var(init_code, *local_vars, ival.nameId)
                        : -1;
        if (texSlot >= 0)
          debug_("%s(tex%d)[%d] %s, 0..%u\n", varName, texSlot, ival.nameId, ival.type == Interval::TYPE_INTERVAL ? "local" : "global",
            ival.getValCount() - 1);
        else
          debug_("%s[%d] %s, 0..%u\n", varName, ival.nameId, ival.type == Interval::TYPE_INTERVAL ? "local" : "global",
            ival.getValCount() - 1);
        break;
      }
    }

    if (ival.type != Interval::TYPE_MODE)
    {
      debug_("%*sintervals: ", indent + 2, "");
      for (int j = 0; j < ival.maxVal.size(); j++)
        debug_("[%d] < %.3f, ", j, ival.maxVal[j]);
      debug_("[%d]other", ival.maxVal.size());

      const VarList *vars = nullptr;
      if (ival.type == Interval::TYPE_GLOBAL_INTERVAL)
        vars = &shBinDump().globVars;
      else if (ival.type == Interval::TYPE_INTERVAL)
        vars = local_vars;

      if (vars)
      {
        int vi = vars->findVar(ival.nameId);
        if (vi >= 0)
        {
          float val = 0;
          switch (vars->getType(vi))
          {
            case SHVT_INT: val = (float)vars->get<int>(vi); break;
            case SHVT_REAL: val = vars->get<float>(vi); break;
            case SHVT_COLOR4:
            {
              Color4 c = vars->get<Color4>(vi);
              val = (c.r + c.g + c.b) / 3.f;
              break;
            }
            case SHVT_TEXTURE: val = 0; break;
            default: break;
          }
          debug_("  (default=%d)", ival.getNormalizedValue(val));
        }
      }

      debug_("\n");
    }
  }
}

static void dumpVariantTable(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt, int indent, int code_num,
  const ILocalDumpVariantData &vdump, const shaderbindump::VarList *local_vars = nullptr, dag::ConstSpan<int> init_code = {})
{
  using namespace shaderbindump;
  int total_variants = code_num ? 1 : 0;
  if (vt.codePieces.size() > 0)
  {
    const VariantTable::IntervalBind &ib = vt.codePieces.back();
    total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
  }

  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL: debug_("[EQUAL, %d->%d]:\n", total_variants, code_num); break;
    case VariantTable::MAPTYPE_LOOKUP: debug_("[LOOKUP, %d->%d]:\n", total_variants, code_num); break;
    case VariantTable::MAPTYPE_QDIRECT: debug_("[QDIRECT, %d->%d->%d]:\n", total_variants, vt.mapData.size(), code_num); break;
    case VariantTable::MAPTYPE_QINTERVAL: debug_("[QLOOKUP, %d->%d->%d]:\n", total_variants, vt.mapData.size(), code_num); break;
  }

  dump_variant_intervals(dump, vt, indent, local_vars, init_code);

  debug_("%*sTable:\n", indent, "");
  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL:
      for (int i = 0; i < total_variants; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", i, i);
        decodeVariant(dump, vt, i);
        vdump.dump(i);
      }
      break;
    case VariantTable::MAPTYPE_LOOKUP:
      for (int i = 0; i < total_variants; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", i, vt.mapData[i]);
        decodeVariant(dump, vt, i);
        vdump.dump(vt.mapData[i]);
      }
      break;
    case VariantTable::MAPTYPE_QDIRECT:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
      {
        debug_("%*s%4d -> %5d:   ", indent + 2, "", vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
        decodeVariant(dump, vt, vt.mapData[i]);
        vdump.dump(vt.mapData[i + vt.mapData.size() / 2]);
      }
      break;
    case VariantTable::MAPTYPE_QINTERVAL:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
      {
        debug_("%*s%4d..%-4d -> %5d:   ", indent + 2, "", vt.mapData[i],
          i < vt.mapData.size() / 2 ? vt.mapData[i + 1] - 1 : vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
        decodeVariant(dump, vt, vt.mapData[i]);
        vdump.dump(vt.mapData[i + vt.mapData.size() / 2]);
      }
      break;
  }
}

template <class T>
static void dumpVarImpl(const shaderbindump::VarList &vars, const T &states, int i)
{
  switch (vars.getType(i))
  {
    case SHVT_INT: debug_("int(%d)\n", states.template get<int>(i)); break;
    case SHVT_REAL: debug_("real(%.3f)\n", states.template get<real>(i)); break;
    case SHVT_COLOR4:
    {
      Color4 c = states.template get<Color4>(i);
      debug_("color4(" FMT_P4 ")\n", c.r, c.g, c.b, c.a);
      break;
    }
    case SHVT_FLOAT4X4:
    {
      debug_("float4x4()\n");
      break;
    }
    case SHVT_TEXTURE: debug_("tex(%d)\n", states.template get<shaders_internal::Tex>(i).texId); break;
    case SHVT_BUFFER: debug_("buf(%d)\n", states.template get<shaders_internal::Buf>(i).bufId); break;
    case SHVT_TLAS: debug_("tlas(%d)\n", states.template get<RaytraceTopAccelerationStructure *>(i)); break;
    case SHVT_INT4:
    {
      const IPoint4 &i4 = states.template get<IPoint4>(i);
      debug_("int4(%d,%d,%d,%d)\n", i4.x, i4.y, i4.z, i4.w);
      break;
    }
    case SHVT_SAMPLER: debug_("sampler\n"); break;
    default: debug_("unknown type: %d\n", vars.getType(i));
  }
}

void shaderbindump::dumpVar(ScriptedShadersBinDump const &dump, const shaderbindump::VarList &vars, const ShaderVarsState *state,
  int i)
{
  int nameId = vars.getNameId(i);
  debug_("  %d: %s%s[%d]= ", i, vars.isPublic(i) ? "public " : "", (const char *)dump.varMap[nameId], nameId);
  if (state)
    dumpVarImpl(vars, *state, i);
  else
    dumpVarImpl(vars, vars, i);
}

void shaderbindump::dumpVars(ScriptedShadersBinDump const &dump, const shaderbindump::VarList &vars, const ShaderVarsState *state)
{
  for (int i = 0; i < vars.size(); i++)
    dumpVar(dump, vars, state, i);
}

void shaderbindump::getTotalPasses(const shaderbindump::ShaderClass &cls, uint32_t &total_passes, uint32_t &total_unique_shaders)
{
  total_passes = 0;
  total_unique_shaders = 0;
  ska::flat_hash_set<uint32_t> unique_shader_ids = {};
  for (const ShaderCode &code : cls.code)
  {
    total_passes += code.passes.size();
    for (const ShaderCode::Pass &pass : code.passes)
      unique_shader_ids.insert((pass.rpass->vprId << 16) | pass.rpass->fshId);
  }
  total_unique_shaders = unique_shader_ids.size();
}

void shaderbindump::dumpShaderInfo(ScriptedShadersBinDump const &dump, const shaderbindump::ShaderClass &cls, bool dump_variants)
{
  shaderbindump::ShaderDumpDetails det = shaderbindump::DumpDetails(shaderbindump::DumpDetails::Preset::DEFAULT).shaderDetails;
  det.dumpVariants = dump_variants;
  dumpShaderInfo(dump, cls, det);
}

void shaderbindump::dumpShaderInfo(ScriptedShadersBinDump const &dump, const shaderbindump::ShaderClass &cls,
  shaderbindump::ShaderDumpDetails details)
{
  uint32_t total_passes = 0;
  uint32_t total_unique_shaders = 0;
  getTotalPasses(cls, total_passes, total_unique_shaders);

  if (details.headerOnly)
  {
    debug("\n  shader[%d] - %s\n"
          "    %d passes, %d unique passes",
      cls.nameId, (const char *)cls.name, total_passes, total_unique_shaders);
    return;
  }

  debug("\n--- shader[%d] - %s ---\n"
        "    %d passes, %d unique passes\n"
        "    timestamp (%lld)\n",
    cls.nameId, (const char *)cls.name, total_passes, total_unique_shaders, cls.getTimestamp());


  if (details.localVars)
  {
    debug(" local vars (%d):", cls.localVars.v.size());
    shaderbindump::dumpVars(dump, cls.localVars, nullptr);
    debug("");
  }


  if (details.staticInit)
  {
    debug(" static init (%d):", cls.initCode.size() / 2);
    for (int i = 0; i < cls.initCode.size(); i += 2)
    {
      if (shaderopcode::getOp(cls.initCode[i + 1]) == SHCOD_TEXTURE_STUBCOL)
      {
        int tt = shaderopcode::getOp2p1(cls.initCode[i + 1]);
        int stVarId = shaderopcode::getOp2p2(cls.initCode[i + 1]);
        uint32_t col = uint32_t(cls.initCode[i]);
        const char *varname = (const char *)dump.varMap[cls.localVars.v[stVarId].nameId];
        debug_("  %s <- stub col=%x tt=%d\n", varname, col, tt);
        continue;
      }

      int stVarId = cls.initCode[i];
      const char *varname = (const char *)dump.varMap[cls.localVars.v[stVarId].nameId];

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
    debug("");
  }


  if (details.varSummary)
  {
    uint32_t totalVariants = 0;
    uint32_t staticVariants = 0;
    uint32_t dynamicVariants = 0;
    dag::Vector<uint32_t> dynamicVariantsPerCode(cls.code.size());

    {
      const auto &svt = cls.stVariants;
      if (cls.code.size() > 0 && svt.codePieces.size() > 0)
      {
        const VariantTable::IntervalBind &ib = svt.codePieces.back();
        staticVariants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
      }

      for (int i = 0; i < cls.code.size(); i++)
      {
        const ShaderCode &code = cls.code[i];
        const auto &dvt = code.dynVariants;
        if (dvt.codePieces.size() > 0)
        {
          const VariantTable::IntervalBind &ib = dvt.codePieces.back();
          dynamicVariantsPerCode[i] = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
          dynamicVariants += dynamicVariantsPerCode[i];
        }
      }

      totalVariants = staticVariants + dynamicVariants;
    }

    debug(" variant summmary:\n"
          "  %d total",
      totalVariants);
    if (totalVariants > 0)
    {
      debug("   static - %d", staticVariants);
      if (staticVariants > 0)
        dump_variant_intervals(dump, cls.stVariants, 4, &cls.localVars, cls.initCode);
      debug("");

      debug("   dynamic - %d\n", dynamicVariants);
      if (dynamicVariants > 0)
      {
        for (int i = 0; i < cls.code.size(); i++)
        {
          debug("    code[%d] - %d", i, dynamicVariantsPerCode[i]);
          dump_variant_intervals(dump, cls.code[i].dynVariants, 5, &cls.localVars, cls.initCode);
          debug("");
        }
      }
    }
    else
    {
      debug("");
    }
  }


  if (details.staticVariantTable)
  {
    if (!details.dumpVariants)
    {
      debug_(" static variant table: %d variants", cls.code.size());
      const auto &vt = cls.stVariants;
      if (cls.code.size() && vt.codePieces.size() > 0)
      {
        const VariantTable::IntervalBind &ib = vt.codePieces.back();
        int total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
        debug(" (codepieces%s for %d variants)", total_variants >= (64 << 10) ? "!" : "", total_variants);
      }
      else
      {
        debug("");
      }
    }
    else
    {
      class DumpStVar : public ILocalDumpVariantData
      {
      public:
        virtual void dump(int) const override { debug(""); }
      };

      debug_(" static variant table ");
      dumpVariantTable(dump, cls.stVariants, 2, cls.code.size(), DumpStVar(), &cls.localVars, cls.initCode);
      debug("");
    }
  }


  if (details.codeBlocks)
    for (int i = 0; i < cls.code.size(); i++)
    {
      const ShaderCode &code = cls.code[i];

      debug("  code[%d]: passes=%d initCode=%d, channels=%d,"
            " varSize=%d, codeFlags=0x%08x vertexStride=%d",
        i, code.passes.size(), code.initCode.size(), code.channel.size(), code.varSize, code.codeFlags, code.vertexStride);

      if (details.stvarmap)
      {
        debug("   stvarmap (%d):", code.stVarMap.size());
        for (int j = 0; j < code.stVarMap.size(); j++)
          debug("    [%u] %s -> %u", code.stVarMap[j] & 0xFFFF,
            (const char *)dump.varMap[cls.localVars.v[code.stVarMap[j] & 0xFFFF].nameId], code.stVarMap[j] >> 16);
      }

      if (details.vertexChannels)
      {
        debug("   vertex channels (%d):", code.channel.size());
        for (int j = 0; j < code.channel.size(); j++)
          debug("    %d: %s: %s%d [%d] %d/%d", j, ShUtils::channel_type_name(code.channel[j].t),
            ShUtils::channel_usage_name(code.channel[j].u), code.channel[j].ui, code.channel[j].mod, code.channel[j].vbu,
            code.channel[j].vbui);
      }

      if (details.dynamicVariantTable)
      {
        if (!details.dumpVariants)
        {
          debug_("   dynamic variant table: %d variants", code.passes.size());
          const auto &vt = code.dynVariants;
          if (code.passes.size() && vt.codePieces.size() > 0)
          {
            const VariantTable::IntervalBind &ib = vt.codePieces.back();
            int total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
            debug(" (codepieces%s for %d variants)", total_variants >= (64 << 10) ? "!" : "", total_variants);
          }
          else
          {
            debug("");
          }

          for (int idx = 0; idx < code.passes.size(); idx++)
          {
            const shader_layout::blk_word_t *sb = code.suppBlockUid[idx].begin();
            if (sb && *sb != shader_layout::BLK_WORD_FULLMASK)
            {
              debug_("    pass[%d] supports blocks:", idx);
              while (*sb != shader_layout::BLK_WORD_FULLMASK)
              {
                int b_frame = decodeBlock(*sb, ShaderGlobal::LAYER_FRAME);
                int b_scene = decodeBlock(*sb, ShaderGlobal::LAYER_SCENE);
                int b_obj = decodeBlock(*sb, ShaderGlobal::LAYER_OBJECT);

                debug_(" {%04X}(%s:%s:%s)", *sb, b_frame >= 0 ? (const char *)dump.blockNameMap[b_frame] : "NULL",
                  b_scene >= 0 ? (const char *)dump.blockNameMap[b_scene] : "NULL",
                  b_obj >= 0 ? (const char *)dump.blockNameMap[b_obj] : "NULL");
                sb++;
              }
              debug("");
            }
          }
        }
        else
        {
          class DumpDynVar : public ILocalDumpVariantData
          {
            ScriptedShadersBinDump const &dmp;
            ShaderCode const &code;

          public:
            DumpDynVar(ScriptedShadersBinDump const &d, const ShaderCode &c) : dmp(d), code(c) {}
            virtual void dump(int idx) const override
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

                    debug_(" {%04X}(%s:%s:%s)", *sb, b_frame >= 0 ? (const char *)dmp.blockNameMap[b_frame] : "NULL",
                      b_scene >= 0 ? (const char *)dmp.blockNameMap[b_scene] : "NULL",
                      b_obj >= 0 ? (const char *)dmp.blockNameMap[b_obj] : "NULL");
                    sb++;
                  }
              }

              debug_("\n");
            }
          };

          debug_("   dynamic variant table ");
          dumpVariantTable(dump, code.dynVariants, 4, code.passes.size(), DumpDynVar(dump, code), &cls.localVars, cls.initCode);
          debug("");
        }
      }

      debug("");
    }


  debug("--- end of dump ---\n\n");
}

#include <generic/dag_tab.h>
#include <util/dag_fastIntList.h>
static ska::flat_hash_map<uint32_t, Tab<FastIntList>> perShaderMarks{};
bool shaderbindump::markInvalidVariant(ShaderBindumpHandle dump_hnd, int shader_nid, unsigned short stat_varcode,
  unsigned short dyn_varcode)
{
  auto listIt = perShaderMarks.find(uint32_t(dump_hnd));
  if (DAGOR_UNLIKELY(listIt == perShaderMarks.end()))
  {
    auto [newListIt, _] = perShaderMarks.emplace(uint32_t(dump_hnd), Tab<FastIntList>{});
    listIt = newListIt;
  }
  auto &list = listIt->second;
  if (shader_nid >= list.size())
    list.resize(shader_nid + 1);
  return list[shader_nid].addInt((unsigned(stat_varcode) << 16) | dyn_varcode);
}
bool shaderbindump::hasShaderInvalidVariants(ShaderBindumpHandle dump_hnd, int shader_nid)
{
  auto listIt = perShaderMarks.find(uint32_t(dump_hnd));
  if (listIt == perShaderMarks.end())
    return false;
  auto &list = listIt->second;
  if (shader_nid >= list.size())
    return false;
  return list[shader_nid].getList().size() > 0;
}
void shaderbindump::resetInvalidVariantMarks(ShaderBindumpHandle dump_hnd)
{
  if (auto listIt = perShaderMarks.find(uint32_t(dump_hnd)); listIt != perShaderMarks.end())
    clear_and_shrink(listIt->second);
}
void shaderbindump::resetAllInvalidVariantMarks() { clear_and_shrink(perShaderMarks); }

#endif
