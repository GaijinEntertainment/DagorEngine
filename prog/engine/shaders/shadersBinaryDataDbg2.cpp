// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadersBinaryData.h"
#include "shBindumpsPrivate.h"
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderDbg.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>

#if DAGOR_DBGLEVEL > 0

class ILocalDumpVariantData2
{
public:
  virtual bool pretest(int idx) = 0;
  virtual void dump(int idx, const shaderbindump::VariantTable &vt, int code) = 0;
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
static bindump::vector<shaderbindump::ShaderInterval> decodeVariantEx(ScriptedShadersBinDump const &dump,
  const shaderbindump::VariantTable &vt, int code)
{
  using namespace shaderbindump;
  bindump::vector<ShaderInterval> result;
  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = dump.intervals[vt.codePieces[i].intervalId];
    int mul = vt.codePieces[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    auto &v = result.emplace_back();
    v.name = bindump::string(dump.varMap[ival.nameId].c_str());
    v.value = subcode;
    v.valueCount = ival.getValCount();
  }
  return result;
}
static void decodeVariantVerbose(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt, int code)
{
  using namespace shaderbindump;
  for (int i = 0; i < vt.codePieces.size(); i++)
  {
    const Interval &ival = dump.intervals[vt.codePieces[i].intervalId];
    int mul = vt.codePieces[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    switch (ival.type)
    {
      case Interval::TYPE_MODE:
        switch (ival.nameId)
        {
          case Interval::MODE_2SIDED: debug_("2SIDED=%d ", subcode); break;
          case Interval::MODE_REAL2SIDED: debug_("REAL2SIDED=%d ", subcode); break;
          case Interval::MODE_LIGHTING: debug_("LIGHTING=%d ", subcode); break;
        }
        break;

      case Interval::TYPE_INTERVAL:
      case Interval::TYPE_GLOBAL_INTERVAL:
        debug_("%s:%c=%d ", (const char *)dump.varMap[ival.nameId], ival.type == Interval::TYPE_INTERVAL ? 'L' : 'G', subcode);
        break;
    }
  }
  debug_("\n");
}

static void dumpVariantTable(ScriptedShadersBinDump const &dump, const shaderbindump::VariantTable &vt, int indent,
  ILocalDumpVariantData2 &vdump)
{
  using namespace shaderbindump;
  int total_variants = 1;
  if (vt.codePieces.size() > 0)
  {
    const VariantTable::IntervalBind &ib = vt.codePieces.back();
    total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
  }
#define SET_BRAKE() has_break = true
#define APPLY_BRAKE()                    \
  if (has_break)                         \
  {                                      \
    has_break = false;                   \
    debug_("%*s    ....\n", indent, ""); \
  }
  bool has_break = false;

  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL:
      for (int i = 0; i < total_variants; i++)
        if (vdump.pretest(i))
        {
          APPLY_BRAKE();
          debug_("%*s%4d -> %5d:   ", indent, "", i, i);
          decodeVariant(dump, vt, i);
          vdump.dump(i, vt, i);
        }
        else
          SET_BRAKE();
      break;
    case VariantTable::MAPTYPE_LOOKUP:
      for (int i = 0; i < total_variants; i++)
        if (vdump.pretest(vt.mapData[i]))
        {
          APPLY_BRAKE();
          debug_("%*s%4d -> %5d:   ", indent, "", i, vt.mapData[i]);
          decodeVariant(dump, vt, i);
          vdump.dump(vt.mapData[i], vt, i);
        }
        else
          SET_BRAKE();
      break;
    case VariantTable::MAPTYPE_QDIRECT:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
        if (vdump.pretest(vt.mapData[i + vt.mapData.size() / 2]))
        {
          APPLY_BRAKE();
          debug_("%*s%4d -> %5d:   ", indent, "", vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
          decodeVariant(dump, vt, vt.mapData[i]);
          vdump.dump(vt.mapData[i + vt.mapData.size() / 2], vt, vt.mapData[i]);
        }
        else
          SET_BRAKE();
      break;
    case VariantTable::MAPTYPE_QINTERVAL:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
        if (vdump.pretest(vt.mapData[i + vt.mapData.size() / 2]))
        {
          APPLY_BRAKE();
          debug_("%*s%4d..%-4d -> %5d:   ", indent, "", vt.mapData[i],
            i < vt.mapData.size() / 2 ? vt.mapData[i + 1] - 1 : vt.mapData[i], vt.mapData[i + vt.mapData.size() / 2]);
          decodeVariant(dump, vt, vt.mapData[i]);
          vdump.dump(vt.mapData[i + vt.mapData.size() / 2], vt, vt.mapData[i]);
        }
        else
          SET_BRAKE();
      break;
  }
#undef SET_BRAKE
#undef APPLY_BRAKE
}

static inline bool shader_pass_used(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode::Pass &p)
{
  static constexpr int BAD = shaderbindump::ShaderCode::INVALID_FSH_VPR_ID;

  if (!p.rpass)
    return false;

  if (p.rpass->vprId != BAD && dump_owner.vprId[p.rpass->vprId] < 0)
    return false;

  // TODO: split off csh from fsh in bindump
  if (p.rpass->fshId != BAD && dump_owner.fshId[p.rpass->fshId] < 0 && dump_owner.cshId[p.rpass->fshId] < 0)
    return false;

  return true;
}

static uint32_t shader_pass_size(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode::Pass &p)
{
  static constexpr int BAD = shaderbindump::ShaderCode::INVALID_FSH_VPR_ID;

  uint32_t sz = 0;
  if (p.rpass->vprId != BAD && dump_owner.vprId[p.rpass->vprId] >= 0)
    sz += dump_owner.getCode(p.rpass->vprId, ShaderCodeType::VERTEX).uncompressedSize;
  if (p.rpass->fshId != BAD && dump_owner.fshId[p.rpass->fshId] >= 0)
    sz += dump_owner.getCode(p.rpass->fshId, ShaderCodeType::PIXEL).uncompressedSize;
  return sz;
}

static inline bool shader_code_used(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode &code)
{
  if (!(interlocked_relaxed_load(code.codeFlags) & shaderbindump::ShaderCode::CF_USED))
    return false;
  for (auto &cp : code.passes)
    if (shader_pass_used(dump_owner, cp))
      return true;
  return false;
}

static inline bool shader_pass_allused(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode::Pass &p)
{
  static constexpr int BAD = shaderbindump::ShaderCode::INVALID_FSH_VPR_ID;

  if (p.rpass)
  {
    if (p.rpass->vprId != BAD && dump_owner.vprId[p.rpass->vprId] < 0)
      return false;
    else if (p.rpass->fshId != BAD && dump_owner.fshId[p.rpass->fshId] < 0)
      return false;
  }
  return true;
}

static inline bool shader_code_allused(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode &code)
{
  if (!(interlocked_relaxed_load(code.codeFlags) & shaderbindump::ShaderCode::CF_USED))
    return false;
  for (int i = 0; i < code.passes.size(); i++)
    if (!shader_pass_used(dump_owner, code.passes[i]))
      return false;
  return true;
}

static void gather_shader_ids(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode::Pass &p,
  FastIntList &fsh, FastIntList &vpr, FastIntList &unused_fsh, FastIntList &unused_vpr)
{
  static constexpr int BAD = shaderbindump::ShaderCode::INVALID_FSH_VPR_ID;

  if (p.rpass)
  {
    if (p.rpass->vprId != BAD)
    {
      vpr.addInt(p.rpass->vprId);
      if (dump_owner.vprId[p.rpass->vprId] < 0)
        unused_vpr.addInt(p.rpass->vprId);
    }
    if (p.rpass->fshId != BAD)
    {
      fsh.addInt(p.rpass->fshId);
      if (dump_owner.fshId[p.rpass->fshId] < 0)
        unused_fsh.addInt(p.rpass->fshId);
    }
  }
}
static void gather_shader_ids(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderCode &code, FastIntList &fsh,
  FastIntList &vpr, FastIntList &unused_fsh, FastIntList &unused_vpr)
{
  for (int i = 0; i < code.passes.size(); i++)
    gather_shader_ids(dump_owner, code.passes[i], fsh, vpr, unused_fsh, unused_vpr);
}
static void gather_shader_ids(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderClass &cls, FastIntList &fsh,
  FastIntList &vpr, FastIntList &unused_fsh, FastIntList &unused_vpr)
{
  for (int i = 0; i < cls.code.size(); i++)
    gather_shader_ids(dump_owner, cls.code[i], fsh, vpr, unused_fsh, unused_vpr);
}
static int calc_shaders_size(ScriptedShadersBinDumpOwner const &dump_owner, const FastIntList &idx, bool vpr)
{
  int sz = 0;
  if (vpr)
    for (int i = 0; i < idx.getList().size(); i++)
      sz += dump_owner.getCode(idx.getList()[i], ShaderCodeType::VERTEX).uncompressedSize;
  else
    for (int i = 0; i < idx.getList().size(); i++)
      sz += dump_owner.getCode(idx.getList()[i], ShaderCodeType::PIXEL).uncompressedSize;
  return sz;
}

void shaderbindump::dumpUnusedVariants(ScriptedShadersBinDumpOwner const &dump_owner, const shaderbindump::ShaderClass &cls)
{
  using namespace shaderbindump;
  auto const &dump = *dump_owner.getDump();

  FastIntList fsh, vpr, unused_fsh, unused_vpr;
  gather_shader_ids(dump_owner, cls, fsh, vpr, unused_fsh, unused_vpr);
  int vpr_sz = calc_shaders_size(dump_owner, vpr, true);
  int fsh_sz = calc_shaders_size(dump_owner, fsh, false);
  int vpr_usz = calc_shaders_size(dump_owner, unused_vpr, true);
  int fsh_usz = calc_shaders_size(dump_owner, unused_fsh, false);

  bool shader_used = false;
  for (int i = 0; i < cls.code.size(); i++)
    if (shader_code_used(dump_owner, cls.code[i]))
    {
      shader_used = true;
      break;
    }
  if (!shader_used)
  {
    debug_("--- unused shader[%d] %s (%dK in %d VS, %dK in %d PS)\n", cls.nameId, cls.name.data(), vpr_usz >> 10,
      unused_vpr.getList().size(), fsh_usz >> 10, unused_fsh.getList().size());
    return;
  }

  for (int i = 0; i < cls.code.size(); i++)
    if (!shader_code_allused(dump_owner, cls.code[i]))
    {
      shader_used = false;
      break;
    }
  if (shader_used)
    return;

  debug_("--- shader[%d] %s (%dK/%dK in %d/%d VS, %dK/%dK in %d/%d PS)\n", cls.nameId, cls.name.data(), vpr_usz >> 10, vpr_sz >> 10,
    unused_vpr.getList().size(), vpr.getList().size(), fsh_usz >> 10, fsh_sz >> 10, unused_fsh.getList().size(), fsh.getList().size());

  class DumpStVar : public ILocalDumpVariantData2
  {
    ScriptedShadersBinDumpOwner const &own;
    ScriptedShadersBinDump const &dmp;
    ShaderClass const &cls;
    FastIntList idxShown;

  public:
    DumpStVar(ScriptedShadersBinDumpOwner const &o, ScriptedShadersBinDump const &d, shaderbindump::ShaderClass const &c) :
      own(o), dmp(d), cls(c)
    {}

    virtual bool pretest(int idx)
    {
      if (idx >= 0xFFFE)
        return false;
      if (idxShown.hasInt(idx))
        return true;
      return !shader_code_allused(own, cls.code[idx]);
    }
    virtual void dump(int idx, const shaderbindump::VariantTable &vt, int vt_code)
    {
      if (!idxShown.addInt(idx))
      {
        debug_("-\n");
        return;
      }
      const ShaderCode &code = cls.code[idx];

      FastIntList fsh, vpr, unused_fsh, unused_vpr;
      gather_shader_ids(own, code, fsh, vpr, unused_fsh, unused_vpr);
      int vpr_sz = calc_shaders_size(own, vpr, true);
      int fsh_sz = calc_shaders_size(own, fsh, false);
      int vpr_usz = calc_shaders_size(own, unused_vpr, true);
      int fsh_usz = calc_shaders_size(own, unused_fsh, false);
      if (!shader_code_used(own, code))
      {
        debug_("unused statvariant (%dK/%dK in %d/%d VS, %dK/%dK in %d/%d PS) ", vpr_usz >> 10, vpr_sz >> 10,
          unused_vpr.getList().size(), vpr.getList().size(), fsh_usz >> 10, fsh_sz >> 10, unused_fsh.getList().size(),
          fsh.getList().size());
        decodeVariantVerbose(dmp, vt, vt_code);
        return;
      }

      class DumpDynVar : public ILocalDumpVariantData2
      {
        ScriptedShadersBinDumpOwner const &own;
        ScriptedShadersBinDump const &dmp;
        ShaderCode const &code;
        FastIntList idxShown;

      public:
        DumpDynVar(ScriptedShadersBinDumpOwner const &o, ScriptedShadersBinDump const &d, ShaderCode const &c) :
          own(o), dmp(d), code(c)
        {}

        virtual bool pretest(int idx)
        {
          if (idx >= 0xFFFE)
            return false;
          if (idxShown.hasInt(idx))
            return true;
          return !shader_pass_allused(own, code.passes[idx]);
        }
        virtual void dump(int idx, const shaderbindump::VariantTable &vt, int vt_code)
        {
          if (!idxShown.addInt(idx))
          {
            debug_("-\n");
            return;
          }
          debug_("unused dynvariant: ");
          decodeVariantVerbose(dmp, vt, vt_code);
        }
      };
      debug_("(unused %dK/%dK in %d/%d VS, %dK/%dK in %d/%d PS) ", vpr_usz >> 10, vpr_sz >> 10, unused_vpr.getList().size(),
        vpr.getList().size(), fsh_usz >> 10, fsh_sz >> 10, unused_fsh.getList().size(), fsh.getList().size());
      decodeVariantVerbose(dmp, vt, vt_code);
      DumpDynVar dynvardump(own, dmp, code);
      dumpVariantTable(dmp, code.dynVariants, 4, dynvardump);
    }
  };

  DumpStVar statvardump(dump_owner, dump, cls);
  dumpVariantTable(dump, cls.stVariants, 0, statvardump);
}


const char *shaderbindump::decodeVariantStr(ScriptedShadersBinDump const &dump,
  dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> p, unsigned code, String &tmp)
{
  for (int i = 0; i < p.size(); i++)
  {
    const Interval &ival = dump.intervals[p[i].intervalId];
    int mul = p[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    tmp.aprintf(32, "%s=%d ", (const char *)dump.varMap[ival.nameId], subcode);
  }
  return tmp;
}

dag::ConstSpan<unsigned> shaderbindump::getVariantCodesForIdx(ScriptedShadersBinDump const &dump,
  const shaderbindump::VariantTable &vt, int code_idx)
{
  static Tab<unsigned> tmp(inimem);
  tmp.clear();

  int half_cnt = vt.mapData.size() / 2;
  int total_variants = 1;
  if (vt.codePieces.size() > 0)
  {
    const VariantTable::IntervalBind &ib = vt.codePieces.back();
    total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
  }

  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL:
      if (code_idx < total_variants)
        tmp.push_back(code_idx);
      break;
    case VariantTable::MAPTYPE_LOOKUP:
      for (int i = 0; i < total_variants; i++)
        if (vt.mapData[i] == code_idx)
          tmp.push_back(i);
      break;
    case VariantTable::MAPTYPE_QDIRECT:
      for (int i = 0; i < half_cnt; i++)
        if (vt.mapData[i + half_cnt] == code_idx)
          tmp.push_back(vt.mapData[i]);
      break;
    case VariantTable::MAPTYPE_QINTERVAL:
      for (int i = 0; i < half_cnt; i++)
        if (vt.mapData[i + half_cnt] == code_idx)
          for (int j = vt.mapData[i]; j <= (i < half_cnt ? vt.mapData[i + 1] - 1 : vt.mapData[i]); j++)
            tmp.push_back(j);
      break;
  }
  return tmp;
}

const char *shaderbindump::decodeStaticVariants(ScriptedShadersBinDump const &dump, const shaderbindump::ShaderClass &shClass,
  int code_idx)
{
  static String tmp;
  tmp.clear();
  String tmp2(framemem_ptr());
  dag::ConstSpan<unsigned> codes = getVariantCodesForIdx(dump, shClass.stVariants, code_idx);
  for (int i = 0; i < codes.size(); i++)
  {
    tmp2.clear();
    tmp.aprintf(128, "  %s\n", decodeVariantStr(dump, shClass.stVariants.codePieces, codes[i], tmp2));
  }
  return tmp;
}

void dump_unused_shader_variants()
{
  int vpr_sz = 0;
  int fsh_sz = 0;
  int vpr_usz = 0;
  int fsh_usz = 0;
  int vpr_cnt = 0;
  int fsh_cnt = 0;
  int vpr_ucnt = 0;
  int fsh_ucnt = 0;

  debug("dumping unused shader variants");
  debug_("\n");

  iterate_all_shader_dumps(
    [&](ScriptedShadersBinDumpOwner &owner) {
      auto const &dump = *owner.getDump();

      FastIntList fsh, vpr, unused_fsh, unused_vpr;
      for (int i = 0; i < dump.classes.size(); i++)
      {
        gather_shader_ids(owner, dump.classes[i], fsh, vpr, unused_fsh, unused_vpr);
        shaderbindump::dumpUnusedVariants(owner, dump.classes[i]);
      }

      vpr_sz += calc_shaders_size(owner, vpr, true);
      fsh_sz += calc_shaders_size(owner, fsh, false);
      vpr_usz += calc_shaders_size(owner, unused_vpr, true);
      fsh_usz += calc_shaders_size(owner, unused_fsh, false);
      vpr_cnt += vpr.getList().size();
      fsh_cnt += fsh.getList().size();
      vpr_ucnt += unused_vpr.getList().size();
      fsh_ucnt += unused_fsh.getList().size();
    },
    false);

  debug("total unused %dK/%dK in %d/%d VS, %dK/%dK in %d/%d PS", vpr_usz >> 10, vpr_sz >> 10, vpr_ucnt, vpr_cnt, fsh_usz >> 10,
    fsh_sz >> 10, fsh_ucnt, fsh_cnt);
}

static void dump_variant_table(ScriptedShadersBinDump const &dump, shaderbindump::VariantTable const &vt,
  ILocalDumpVariantData2 &vdump)
{
  using namespace shaderbindump;
  int total_variants = 1;
  if (vt.codePieces.size() > 0)
  {
    const VariantTable::IntervalBind &ib = vt.codePieces.back();
    total_variants = dump.intervals[ib.intervalId].getValCount() * ib.totalMul;
  }

  switch (vt.mapType)
  {
    case VariantTable::MAPTYPE_EQUAL:
      for (int i = 0; i < total_variants; i++)
        if (vdump.pretest(i))
          vdump.dump(i, vt, i);
      break;
    case VariantTable::MAPTYPE_LOOKUP:
      for (int i = 0; i < total_variants; i++)
        if (vdump.pretest(vt.mapData[i]))
          vdump.dump(vt.mapData[i], vt, i);
      break;
    case VariantTable::MAPTYPE_QDIRECT:
    case VariantTable::MAPTYPE_QINTERVAL:
      for (int i = 0; i < vt.mapData.size() / 2; i++)
        if (vdump.pretest(vt.mapData[i + vt.mapData.size() / 2]))
          vdump.dump(vt.mapData[i + vt.mapData.size() / 2], vt, vt.mapData[i]);
      break;
  }
}

static bindump::vector<shaderbindump::ShaderStaticVariant> dump_shader_stat(ScriptedShadersBinDumpOwner const &dump_owner,
  shaderbindump::ShaderClass const &cls)
{
  using namespace shaderbindump;
  bindump::vector<ShaderStaticVariant> result;
  if (cls.code.empty())
    return result;

  class DumpStVar : public ILocalDumpVariantData2
  {
    ScriptedShadersBinDumpOwner const &own;
    ScriptedShadersBinDump const &dmp;
    ShaderClass const &cls;
    FastIntList idxShown;
    bindump::vector<ShaderStaticVariant> &result;

  public:
    DumpStVar(ScriptedShadersBinDumpOwner const &o, ScriptedShadersBinDump const &d, shaderbindump::ShaderClass const &c,
      bindump::vector<ShaderStaticVariant> &r) :
      own(o), dmp(d), cls(c), result(r)
    {}

    virtual bool pretest(int idx)
    {
      if (idx >= 0xFFFE)
        return false;
      if (idxShown.hasInt(idx))
        return true;
      return shader_code_used(own, cls.code[idx]);
    }
    virtual void dump(int idx, const shaderbindump::VariantTable &vt, int vt_code)
    {
      if (!idxShown.addInt(idx))
        return;
      const ShaderCode &code = cls.code[idx];

      class DumpDynVar : public ILocalDumpVariantData2
      {
        ScriptedShadersBinDumpOwner const &own;
        ScriptedShadersBinDump const &dmp;
        ShaderCode const &code;
        FastIntList idxShown;
        bindump::vector<ShaderVariant> &result;

      public:
        DumpDynVar(ScriptedShadersBinDumpOwner const &o, ScriptedShadersBinDump const &d, const ShaderCode &c,
          bindump::vector<ShaderVariant> &r) :
          own(o), dmp(d), code(c), result(r)
        {}

        virtual bool pretest(int idx)
        {
          if (idx >= 0xFFFE)
            return false;
          if (idxShown.hasInt(idx))
            return true;
          return shader_pass_used(own, code.passes[idx]);
        }
        virtual void dump(int idx, const shaderbindump::VariantTable &vt, int vt_code)
        {
          result.emplace_back();
          if (code.passes[idx].rpass)
          {
            result.back().vprId = code.passes[idx].rpass->vprId;
            result.back().fshId = code.passes[idx].rpass->fshId;
          }
          if (!idxShown.addInt(idx))
            return;
          result.back().intervals = decodeVariantEx(dmp, vt, vt_code);
          result.back().size = shader_pass_size(own, code.passes[idx]);
        }
      };
      result.emplace_back();
      result.back().staticIntervals = decodeVariantEx(dmp, vt, vt_code);
      DumpDynVar dynvardump(own, dmp, code, result.back().dynamicVariants);
      dump_variant_table(dmp, code.dynVariants, dynvardump);
    }
  };

  auto const &dump = *dump_owner.getDump();
  DumpStVar statvardump(dump_owner, dump, cls, result);
  dump_variant_table(dump, cls.stVariants, statvardump);
  return result;
}

void dump_shader_statistics(const char *filename)
{
  bindump::vector<shaderbindump::ShaderStatistics> stat;

  iterate_all_shader_dumps(
    [&](ScriptedShadersBinDumpOwner &owner) {
      auto const &dump = *owner.getDump();
      for (int i = 0; i < dump.classes.size(); i++)
      {
        stat.emplace_back();
        stat.back().shaderName = bindump::string(dump.classes[i].name.data());
        stat.back().dumpName.assign(owner.name.str(), owner.name.length());
        stat.back().staticVariants = dump_shader_stat(owner, dump.classes[i]);
      }
    },
    false);

  bindump::writeToFileFast(stat, filename);
}

#else
void dump_unused_shader_variants() {}
#endif
