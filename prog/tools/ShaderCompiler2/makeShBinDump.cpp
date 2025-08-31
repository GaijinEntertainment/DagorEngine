// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define _DEBUG_TAB_ 1
#include "makeShBinDump.h"
#include "linkShaders.h"
#include "loadShaders.h"

#include "shTargetContext.h"
#include "globVar.h"
#include "varMap.h"
#include "shcode.h"
#include "namedConst.h"
#include "samplers.h"
#include "shCompiler.h"
#include "globalConfig.h"

#include "cppStcode.h"
#include <shaders/cppStcodeVer.h>

#include "shLog.h"
#include <osApiWrappers/dag_files.h>
#include <generic/dag_sort.h>
#include <libTools/util/makeBindump.h>
#include <shaders/shader_ver.h>
#include <shaders/shader_layout.h>
#include <shaders/shLimits.h>
#include "transcodeShader.h"
#include "binDumpUtils.h"
#include <ioSys/dag_zstdIo.h>
#include <util/dag_hash.h>
#if _CROSS_TARGET_DX12
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#endif
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_sampler.h>
#include <EASTL/unordered_map.h>
#include <dag/dag_vector.h>
#include <generic/dag_enumerate.h>

#include <hash/sha256.h>

#include <util/dag_threadPool.h>
#include <perfMon/dag_cpuFreq.h>

using namespace mkbindump;
using namespace shader_layout;

namespace semicooked
{
// alphabetically sorted list of hardcoded names (describes nil block for each level)
static const char *hardcodedBlkName[SHADER_BLOCK_HIER_LEVELS] = {"!frame", "!obj", "!scene"};
static const ShaderBlockLevel hardcodedBlkLayer[SHADER_BLOCK_HIER_LEVELS] = {
  ShaderBlockLevel::FRAME, ShaderBlockLevel::OBJECT, ShaderBlockLevel::SCENE};
static ShaderStateBlock::BuildTimeData hardcodedBlk[SHADER_BLOCK_HIER_LEVELS];
static unsigned fullBlkLayerMask = 0;

static void addSuppCodes(Tab<const ShaderStateBlock *> &ssbHier, Tab<blk_word_t> &codes)
{
  const ShaderStateBlock &lb = *ssbHier.back();
  if (!lb.shConst.suppBlk.size())
  {
    blk_word_t c = BLK_WORD_FULLMASK;
    for (int i = 1; i < ssbHier.size(); i++)
      c = (c & ~ssbHier[i]->btd.uidMask) | ssbHier[i]->btd.uidVal;
    codes.push_back(c);
  }
  else
    for (int i = 0; i < lb.shConst.suppBlk.size(); i++)
    {
      ssbHier.push_back(lb.shConst.suppBlk[i]);
      addSuppCodes(ssbHier, codes);
      ssbHier.pop_back();
    }
}

static void processShaderBlocks(dag::Span<ShaderStateBlock *> blocks, SharedStorage<blk_word_t> &blkPartSign)
{
  int blk_count[SHADER_BLOCK_HIER_LEVELS];
  memset(blk_count, 0, sizeof(blk_count));

  for (int i = 0; i < blocks.size(); i++)
  {
    G_ASSERT(blocks[i]);
    if (blocks[i]->layerLevel == ShaderBlockLevel::GLOBAL_CONST)
      continue;
    G_ASSERT(blocks[i]->layerLevel != ShaderBlockLevel::UNDEFINED && blocks[i]->layerLevel < ShaderBlockLevel::SHADER);
    blk_count[size_t(blocks[i]->layerLevel)]++;
    fast_sort(blocks[i]->shConst.suppBlk,
      [](const bindump::Address<ShaderStateBlock> &a, const bindump::Address<ShaderStateBlock> &b) { return a.get() < b.get(); });
  }
  fast_sort(blocks,
    [](const ShaderStateBlock *b1, const ShaderStateBlock *b2) { return strcmp(b1->name.c_str(), b2->name.c_str()) < 0; });

  int bit_per_layer[SHADER_BLOCK_HIER_LEVELS];
  int sum_shift[SHADER_BLOCK_HIER_LEVELS];
  int total_bits = 0;
  for (int i = 0; i < SHADER_BLOCK_HIER_LEVELS; i++)
  {
    blk_count[i]++;
    bit_per_layer[i] = 1;
    while ((1 << bit_per_layer[i]) < blk_count[i])
      bit_per_layer[i]++;

    sum_shift[i] = total_bits;
    total_bits += bit_per_layer[i];

    hardcodedBlk[i].uidMask = ((1 << bit_per_layer[i]) - 1) << sum_shift[i];
    hardcodedBlk[i].uidVal = 0xFFFFFFFF & hardcodedBlk[i].uidMask;
  }
  // check total bit count is LESS than word bit count, since we need 1 more bit to signify invalid word
  G_ASSERT(total_bits < BLK_WORD_BITS);

  fullBlkLayerMask = (1 << total_bits) - 1;

  memset(blk_count, 0, sizeof(blk_count));
  for (int i = 0; i < blocks.size(); i++)
  {
    if (blocks[i]->layerLevel == ShaderBlockLevel::GLOBAL_CONST)
    {
      blocks[i]->btd.uidMask = blocks[i]->btd.uidVal = 0;
      continue;
    }
    auto layer = size_t(blocks[i]->layerLevel);
    blocks[i]->btd.uidMask = ((1 << bit_per_layer[layer]) - 1) << sum_shift[layer];
    blocks[i]->btd.uidVal = blk_count[layer] << sum_shift[layer];
    blk_count[layer]++;
  }

  Tab<blk_word_t> codes(tmpmem);
  Tab<const ShaderStateBlock *> ssbHier(tmpmem);
  for (int i = 0; i < blocks.size(); i++)
  {
    auto layer = blocks[i]->layerLevel;
    if (layer == ShaderBlockLevel::FRAME || layer == ShaderBlockLevel::GLOBAL_CONST)
    {
      blocks[i]->btd.suppMask = 0;
      blocks[i]->btd.suppListOfs = -1;
      continue;
    }

    codes.clear();
    ssbHier.clear();

    ssbHier.push_back(blocks[i]);
    addSuppCodes(ssbHier, codes);

    unsigned mask = (1 << sum_shift[size_t(layer)]) - 1;
    for (int j = 0; j < codes.size(); j++)
      codes[j] &= mask;
    codes.push_back(BLK_WORD_FULLMASK);
    if (codes.size() > 2)
      fast_sort(codes, [](blk_word_t a, blk_word_t b) { return a < b; });

    blkPartSign.getRef(blocks[i]->btd.suppListOfs, codes.data(), codes.size(), 64);
    blocks[i]->btd.suppMask = mask;
  }
}

//
// binary dump builder
//
bool areIntervalsEqual(const shader_layout::Interval<> &a, const shader_layout::Interval<> &b)
{
  return a.nameId == b.nameId && a.type == b.type && a.maxVal == b.maxVal;
}

struct Variables
{
  Variables(const GatherNameMap &vmap, const shc::TargetContext &a_ctx) : ctx{a_ctx}, varLists(tmpmem), varMap(vmap) {}

  int addGlobVars(Tab<int> &remappingTable)
  {
    const Tab<ShaderGlobal::Var> &globvars = ctx.globVars().getVariableList();

    int count = 0;
    for (int i = 0; i < remappingTable.size(); i++)
      if (remappingTable[i] != -1)
        count++;

    int id = append_items(varLists, 1);
    if (!count)
      return id;
    auto &vl = varLists[id];
    vl.v = vars.getElementAddress(vars.size());
    vl.v.setCount(count);
    vars.resize(vars.size() + count);

    int gv_ind = 0;
    for (int i = 0; i < remappingTable.size(); i++)
    {
      if (remappingTable[i] != -1)
      {
        const ShaderGlobal::Var &v = globvars[i];
        G_ASSERT(v.nameId >= 0 && v.nameId < varMap.xmap.size());
        vl.v[gv_ind].nameId = varMap.xmap[v.nameId];
        vl.v[gv_ind].type = v.type;
        vl.v[gv_ind].isPublic = 1;
        gv_ind++;
      }
    }

    debug("Global vars count after remapping = %d", vl.v.size());
    fillStorage(vl, 4, remappingTable);

    // Checking that the array elements are in the correct order
    for (int i = 0; i < remappingTable.size(); i++)
    {
      if (remappingTable[i] == -1)
        continue;

      const ShaderGlobal::Var &v = globvars[i];
      if (v.array_size == 1 || v.index != 0)
        continue;

      auto find_var = [&, this](const ShaderGlobal::Var &v) -> shader_layout::Var<> * {
        for (auto &var : vl.v)
          if (var.nameId == varMap.xmap[v.nameId])
            return &var;
        G_ASSERT(false);
        return nullptr;
      };

      auto first_var = find_var(v);

      for (int j = i + 1; j < i + v.array_size; j++)
      {
        const ShaderGlobal::Var &elem = globvars[j];
        auto next_var = find_var(elem);
        if ((const char *)&next_var->valPtr.get() - (const char *)&first_var->valPtr.get() != (j - i) * 4 * sizeof(float))
          sh_debug(SHLOG_ERROR, "Wrong element array order for `%s`", varMap.getOrdinalName(first_var->nameId));
      }
    }

    return id;
  }

  int addLocalVars(dag::ConstSpan<::ShaderClass::Var> locvar)
  {
    int id = append_items(varLists, 1);
    if (locvar.empty())
      return id;
    auto &vl = varLists[id];
    vl.v = vars.getElementAddress(vars.size());
    vl.v.setCount(locvar.size());
    vars.resize(vars.size() + locvar.size());

    for (int i = 0; i < locvar.size(); i++)
    {
      const ::ShaderClass::Var &v = locvar[i];

      G_ASSERT(v.nameId >= 0 && v.nameId < varMap.xmap.size());
      vl.v[i].nameId = varMap.xmap[v.nameId];
      vl.v[i].type = v.type;
      vl.v[i].isPublic = 0;
    }

    allocStorage(vl, 1);

    for (int i = 0; i < locvar.size(); i++)
    {
      const ::ShaderClass::Var &v = locvar[i];
      int sz = 0;
      switch (v.type)
      {
        case SHVT_INT:
        case SHVT_REAL: *vl.v[i].valPtr = v.defval.i; break;
        case SHVT_INT4:
        case SHVT_COLOR4: memcpy(&vl.v[i].valPtr.get(), &v.defval, 16); break;
        case SHVT_FLOAT4X4: memset(&vl.v[i].valPtr.get(), 0, 16 * sizeof(float)); break;
        case SHVT_TEXTURE: *(TEXTUREID *)&vl.v[i].valPtr.get() = BAD_TEXTUREID; break;
        case SHVT_BUFFER: *(D3DRESID *)&vl.v[i].valPtr.get() = BAD_D3DRESID; break;
        case SHVT_SAMPLER: *(d3d::SamplerHandle *)&vl.v[i].valPtr.get() = d3d::INVALID_SAMPLER_HANDLE; break;
        case SHVT_TLAS: *(RaytraceTopAccelerationStructure **)&vl.v[i].valPtr.get() = nullptr; break;
      }
    }

    return id;
  }

  void fillStorage(shader_layout::VarList<> &vl, int tex_size, Tab<int> &remapTable)
  {
    const Tab<ShaderGlobal::Var> &globvars = ctx.globVars().getVariableList();

    allocStorage(vl, tex_size);

    for (int i = 0; i < globvars.size(); i++)
    {
      if (remapTable[i] == -1)
        continue;
      const ShaderGlobal::Var &v = globvars[i];
      for (auto &var : vl.v)
      {
        if (var.nameId == varMap.xmap[v.nameId])
        {
          G_ASSERT(v.type == var.type);
          switch (v.type)
          {
            case SHVT_INT:
            case SHVT_REAL: *var.valPtr = v.value.i; break;
            case SHVT_INT4:
            case SHVT_COLOR4: memcpy(&var.valPtr.get(), &v.value, 16); break;
            case SHVT_FLOAT4X4: memset(&var.valPtr.get(), 0, 16 * sizeof(float)); break;
            case SHVT_TEXTURE: *(TEXTUREID *)&var.valPtr.get() = BAD_TEXTUREID; break;
            case SHVT_BUFFER: *(D3DRESID *)&var.valPtr.get() = BAD_D3DRESID; break;
            case SHVT_SAMPLER: *(d3d::SamplerHandle *)&var.valPtr.get() = d3d::INVALID_SAMPLER_HANDLE; break;
            case SHVT_TLAS: *(RaytraceTopAccelerationStructure **)&var.valPtr.get() = nullptr; break;
          }
        }
      }
    }
  }

  void allocStorage(shader_layout::VarList<> &vl, int tex_size = 1)
  {
    int stor_start = storage.size();
    int stor_p = stor_start;

    for (auto &var : vl.v)
      if (var.type == SHVT_COLOR4 || var.type == SHVT_INT4 || var.type == SHVT_FLOAT4X4)
      {
        if (stor_p & 3)
          stor_p = (stor_p + 4) & ~3;
        var.valPtr = storage.getElementAddress(stor_p);
        stor_p += var.type == SHVT_FLOAT4X4 ? 4 * 4 : 4;
      }

    for (auto &var : vl.v)
      if (var.type == SHVT_TEXTURE || var.type == SHVT_BUFFER)
      {
        if (tex_size == 4 && (stor_p & 3))
          stor_p = (stor_p + 4) & ~3;
        var.valPtr = storage.getElementAddress(stor_p);
        stor_p += tex_size;
      }

    for (auto &var : vl.v)
      if (var.type == SHVT_TLAS)
      {
        if (stor_p & 3)
          stor_p = (stor_p + 4) & ~3;
        var.valPtr = storage.getElementAddress(stor_p);
        stor_p += sizeof(RaytraceTopAccelerationStructure *) / sizeof(int);
      }

    for (auto &var : vl.v)
      if (var.type == SHVT_SAMPLER)
      {
        if (stor_p & 3)
          stor_p = (stor_p + 4) & ~3;
        var.valPtr = storage.getElementAddress(stor_p);
        stor_p += sizeof(d3d::SamplerHandle) / sizeof(int);
      }

    for (auto &var : vl.v)
      if (var.type == SHVT_REAL || var.type == SHVT_INT)
        var.valPtr = storage.getElementAddress(stor_p++);

    for (auto &var : vl.v)
      if (var.type != SHVT_INT && var.type != SHVT_INT4 && var.type != SHVT_REAL && var.type != SHVT_COLOR4 &&
          var.type != SHVT_TEXTURE && var.type != SHVT_BUFFER && var.type != SHVT_TLAS && var.type != SHVT_FLOAT4X4 &&
          var.type != SHVT_SAMPLER)
        debug("[vErr] unknown type %d for var = %s", var.type, varMap.getOrdinalName(var.nameId));

    storage.resize(storage.size() + stor_p - stor_start);
  }

  const shc::TargetContext &ctx;
  bindump::VecHolder<int, 4 * sizeof(int)> storage;
  Tab<shader_layout::VarList<>> varLists;
  bindump::VecHolder<shader_layout::Var<>> vars;
  const GatherNameMap &varMap;
};

struct Variants
{
  struct VariantPair
  {
    int code;
    int codeId;

    static int cmpCode(const VariantPair *a, const VariantPair *b) { return a->code - b->code; }
  };

  Variants(const GatherNameMap &vmap, const shc::TargetContext &a_ctx) :
    ctx{a_ctx}, intervals(tmpmem), subintervals_by_interval(tmpmem), varMap(vmap)
  {}

  static void clearTemp()
  {
    clear_and_shrink(tmpVpDir);
    clear_and_shrink(tmpVpInt);
    clear_and_shrink(tmpMapData);
  }

  shader_layout::VariantTable<> addVariantTable(ShaderVariant::VariantTable &vt, dag::ConstSpan<int> codeRemap, int codenum,
    const char *shader_nm)
  {
    const ShaderVariant::TypeTable &typeList = vt.getTypes();
    float maxVal[256];
    shader_layout::VariantTable<>::IntervalBind ib[64];
    int vremap[64];
    int mul = 1, pcs = 0;

    // fill interval values
    for (int ti = 0; ti < typeList.getCount(); ti++)
    {
      const ShaderVariant::VariantType &ct = typeList.getType(ti);
      shader_layout::Interval<> ival;
      bindump::VecHolder<bindump::StrHolder<>> subintervals;
      int var_num = 0;

      if (ct.type == ShaderVariant::VARTYPE_MODE)
      {
        ival.type = ival.TYPE_MODE;
        switch (ct.extType)
        {
          case ShaderVariant::TWO_SIDED:
            ival.nameId = ival.MODE_2SIDED;
            var_num = 2;
            break;
          case ShaderVariant::REAL_TWO_SIDED:
            ival.nameId = ival.MODE_REAL2SIDED;
            var_num = 2;
            break;
          default:
            debug("[iOpt] skip unknown mode: %d", ct.extType);
            ival.nameId = 255;
            break;
        }

        if (ival.nameId == 255)
          continue;

        G_ASSERT(!ct.interval);
        maxVal[0] = maxVal[1] = 0;
        iValStorage.getRef(ival.maxVal, maxVal, var_num - 1);
      }
      else
      {
        const char *ivalname = get_interval_name(*ct.interval, ctx);

        if (ct.type == ShaderVariant::VARTYPE_INTERVAL)
          ival.type = ival.TYPE_INTERVAL;
        else if (ct.type == ShaderVariant::VARTYPE_GLOBAL_INTERVAL || ct.type == ShaderVariant::VARTYPE_GL_OVERRIDE_INTERVAL)
          ival.type = ival.TYPE_GLOBAL_INTERVAL;
        else
        {
          debug("[iOpt] skip unknown interval type: %d", ct.type);
          continue;
        }

        if (ct.interval->getValueCount() < 2)
        {
          debug("[iOpt] skip redundunt interval[1]");
          continue;
        }

        G_ASSERT(ct.interval);
        int id = varMap.getNameId(ivalname);
        G_ASSERT(id >= 0);
        ival.nameId = varMap.xmap[id];
        G_ASSERTF(strcmp(varMap.getOrdinalName(ival.nameId), ivalname) == 0,
          "varMap.nameCount()=%d varMap.getOrdinalName(%d->%d)=%s ivalname=%s", varMap.nameCount(), id, ival.nameId,
          varMap.getOrdinalName(ival.nameId), ivalname);

        var_num = ct.interval->getValueCount();
        G_ASSERT(var_num < 256);

        subintervals.resize(ct.interval->getValueCount());
        for (int i = 0; i < ct.interval->getValueCount(); i++)
        {
          subintervals[i] = get_interval_value_name(*ct.interval, i, ctx);
          RealValueRange r = ct.interval->getValueRange(i);

          if (i == 0)
            G_ASSERT(r.getMin() < -1e+6);
          else if (i == ct.interval->getValueCount() - 1)
            G_ASSERT(r.getMax() > 1e+6);
          else
            G_ASSERT(r.getMax() == ct.interval->getValueRange(i + 1).getMin());

          if (i < ct.interval->getValueCount() - 1)
          {
            maxVal[i] = r.getMax();
          }
        }
        iValStorage.getRef(ival.maxVal, maxVal, var_num - 1);
      }

      G_ASSERT(pcs < 64);
      G_ASSERT(mul < 65535);

      ib[pcs].intervalId = getIntervalId(ival);
      ib[pcs].totalMul = mul;
      vremap[pcs] = ti;
      mul *= var_num;

      if (ib[pcs].intervalId >= subintervals_by_interval.size())
        subintervals_by_interval.resize(ib[pcs].intervalId + 1);
      subintervals_by_interval[ib[pcs].intervalId] = eastl::move(subintervals);

      pcs++;
    }

    // prepare variant table
    shader_layout::VariantTable<> vtr;
    if (pcs)
    {
      vtPcsStorage.getRef(vtr.codePieces, ib, pcs, 0);

      // convert variant codes
      dag::ConstSpan<ShaderVariant::Variant> sv = vt.getVariants();
      tmpVpDir.resize(sv.size());
      for (int vi = 0; vi < tmpVpDir.size(); vi++)
      {
        const ShaderVariant::Variant &cvar = sv[vi];
        int code = 0;
        for (int j = 0; j < pcs; j++)
        {
          unsigned val = cvar.getValue(vremap[j]);
          G_ASSERT(val < intervals[ib[j].intervalId].maxVal.size() + 1);
          code += val * ib[j].totalMul;
        }
        if (code >= 65535)
        {
          for (int j = 0; j < pcs; j++)
          {
            const shader_layout::Interval<> &ir = intervals[ib[j].intervalId];
            unsigned val = cvar.getValue(vremap[j]);
            G_ASSERT(val < ir.maxVal.size() + 1);
            debug_("piece[%2d] mul=%-5d range=0..%-2d  val=%2d (+%5d total); intervalNameId=", j, ib[j].totalMul, ir.maxVal.size(),
              val, val * ib[j].totalMul);
            if (ir.type == ir.TYPE_MODE)
              debug("MODE/%d", ir.nameId);
            else
              debug((ir.type == ir.TYPE_INTERVAL) ? "LOCAL/%s" : "GLOBAL/%s", varMap.getOrdinalName(ir.nameId));
          }

          sh_debug(SHLOG_FATAL,
            "Variant space in shader %s is too large: encountered variant code=%d >= 65535, with last interval multiplier = "
            "%d\nConsider "
            "reducing the number of intervals/interval values. See more details in ShaderLog*\n",
            shader_nm, code, ib[pcs - 1].totalMul);
          return {};
        }
        G_ASSERT(cvar.codeId < (int)codeRemap.size());

        tmpVpDir[vi].code = code;
        if (cvar.codeId == -2)
          tmpVpDir[vi].codeId = vtr.FIND_NOTFOUND;
        else
          tmpVpDir[vi].codeId = (cvar.codeId < 0) ? vtr.FIND_NULL : codeRemap[cvar.codeId];

        if (tmpVpDir[vi].codeId == -2)
          tmpVpDir[vi].codeId = vtr.FIND_NOTFOUND;
        else if (tmpVpDir[vi].codeId < 0)
          tmpVpDir[vi].codeId = vtr.FIND_NULL;

        G_ASSERT((tmpVpDir[vi].codeId >= 0 && tmpVpDir[vi].codeId < codenum) || tmpVpDir[vi].codeId == vtr.FIND_NULL ||
                 tmpVpDir[vi].codeId == vtr.FIND_NOTFOUND);
      }

      if (mul <= 8) // don't bother to even try using qmap
        goto choose_lookup;
      if (mul > 0x10000) // can't use qmap due to 16-bit overflow
      {
        debug("switch to LOOKUP: sv.count=%d mul=%d", sv.size(), mul);
        goto choose_lookup;
      }

      // sort and get direct map
      sort(tmpVpDir, &VariantPair::cmpCode);

      // prepare interval map and reduce duplicated variants
      if (mul == tmpVpDir.size())
        tmpVpInt = tmpVpDir;
      else
      {
        // fill missing values
        VariantPair v_pair;
        Ref v_range;

        tmpVpInt.clear();
        tmpVpInt.reserve(mul);

        v_pair.codeId = vtr.FIND_NOTFOUND;
        v_range.set(0, 1);
        for (int i = 0; i < (int)tmpVpDir.size() - 1; i++)
          if (tmpVpDir[i].code + 1 == tmpVpDir[i + 1].code)
            v_range.count++;
          else
          {
            append_items(tmpVpInt, v_range.count, &tmpVpDir[v_range.start]);
            v_pair.code = tmpVpDir[i].code + 1;
            if (tmpVpDir[i].code + 1 < tmpVpDir[i + 1].code - 1)
            {
              tmpVpInt.push_back(v_pair);
              v_pair.code = tmpVpDir[i + 1].code - 1;
            }
            tmpVpInt.push_back(v_pair);
            v_range.set(i + 1, 1);
          }
        append_items(tmpVpInt, v_range.count, &tmpVpDir[v_range.start]);
      }

      for (int i = tmpVpInt.size() - 2; i > 0; i--)
        if (tmpVpInt[i].codeId == tmpVpInt[i - 1].codeId)
          erase_items(tmpVpInt, i, 1);
      if (tmpVpInt.size() && tmpVpInt[0].codeId == vtr.FIND_NOTFOUND)
        erase_items(tmpVpInt, 0, 1);

      // compare different packs
      if (tmpVpInt.size() * 2 > mul * 2 / 3 && tmpVpDir.size() * 2 > mul * 2 / 3)
      {
        // select lookup
      choose_lookup:
        vtr.mapType = vtr.MAPTYPE_LOOKUP;
        tmpMapData.resize(mul);
        if (tmpVpDir.size() < mul)
          mem_set_ff(tmpMapData);

        for (int i = 0; i < tmpVpDir.size(); i++)
          tmpMapData[tmpVpDir[i].code] = tmpVpDir[i].codeId;

        if (tmpMapData.size() < 1)
          vtr.mapType = vtr.MAPTYPE_QDIRECT; // effectively MAPTYPE_VOID
        else if (tmpMapData.size() == codenum)
        {
          vtr.mapType = vtr.MAPTYPE_EQUAL;
          for (int i = 0; i < tmpMapData.size(); i++)
            if (tmpMapData[i] != i)
            {
              vtr.mapType = vtr.MAPTYPE_LOOKUP;
              break;
            }
        }
      }
      else if (tmpVpInt.size() + 1 < tmpVpDir.size())
      {
        // select qinterval
        vtr.mapType = vtr.MAPTYPE_QINTERVAL;
        tmpMapData.resize(tmpVpInt.size() * 2);

        for (int i = 0; i < tmpVpInt.size(); i++)
        {
          tmpMapData[i] = tmpVpInt[i].code;
          tmpMapData[i + tmpVpInt.size()] = tmpVpInt[i].codeId;
          G_ASSERTF(tmpVpInt[i].code < 0x10000 && tmpVpInt[i].codeId < 0x10000, "code=%d codeId=%d", tmpVpInt[i].code,
            tmpVpInt[i].codeId);
        }
      }
      else
      {
        // select qdirect
        vtr.mapType = vtr.MAPTYPE_QDIRECT;
        tmpMapData.resize(tmpVpDir.size() * 2);

        for (int i = 0; i < tmpVpDir.size(); i++)
        {
          tmpMapData[i] = tmpVpDir[i].code;
          tmpMapData[i + tmpVpDir.size()] = tmpVpDir[i].codeId;
          G_ASSERTF(tmpVpDir[i].code < 0x10000 && tmpVpDir[i].codeId < 0x10000, "code=%d codeId=%d", tmpVpDir[i].code,
            tmpVpDir[i].codeId);
        }
      }

      // finish VarantTable construction
      if (vtr.mapType == vtr.MAPTYPE_EQUAL || tmpMapData.size() < 1)
      {
        vtr.mapData = nullptr;
      }
      else
      {
        G_ASSERT(tmpMapData.size());
        bool ret = (vtr.mapType == vtr.MAPTYPE_LOOKUP) ? vtLmapStorage.getRef(vtr.mapData, tmpMapData.data(), tmpMapData.size(), 2, 2)
                                                       : vtQmapStorage.getRef(vtr.mapData, tmpMapData.data(), tmpMapData.size(), 2, 2);
      }
    }
    return vtr;
  }

  int getIntervalId(const shader_layout::Interval<> &ival)
  {
    for (int i = intervals.size() - 1; i >= 0; i--)
      if (areIntervalsEqual(ival, intervals[i]))
        return i;
    G_ASSERT(intervals.size() < 32768);
    intervals.push_back(ival);
    return intervals.size() - 1;
  }

  const shc::TargetContext &ctx;
  Tab<shader_layout::Interval<>> intervals;
  Tab<bindump::VecHolder<bindump::StrHolder<>>> subintervals_by_interval;
  SharedStorage<real> iValStorage;
  SharedStorage<shader_layout::VariantTable<>::IntervalBind> vtPcsStorage;
  SharedStorage<uint16_t> vtLmapStorage;
  SharedStorage<uint16_t> vtQmapStorage;
  const GatherNameMap &varMap;

  static Tab<VariantPair> tmpVpDir, tmpVpInt;
  static Tab<uint16_t> tmpMapData;
};
Tab<Variants::VariantPair> Variants::tmpVpDir(tmpmem_ptr());
Tab<Variants::VariantPair> Variants::tmpVpInt(tmpmem_ptr());
Tab<uint16_t> Variants::tmpMapData(tmpmem_ptr());

static int cmpPacked2uint16Map(const uint32_t *a, const uint32_t *b) { return (a[0] & 0xFFFF) - (b[0] & 0xFFFF); }

static SharedStorage<blk_word_t> globalSuppBlkSign;

struct ShaderCodes
{
  ShaderCodes() : codes(tmpmem) {}

  static void clearTemp() { clear_and_shrink(tmpPasses); }

  int addCode(::ShaderCode &scode, Variants &vt, const char *shader_name, int code_idx)
  {
    tmpPasses.clear();
    Tab<int> tmpPassesRemap(tmpmem_ptr());
    tmpPassesRemap.resize(scode.passes.size());
    for (int k = 0; k < scode.passes.size(); k++)
      if (scode.passes[k])
        tmpPassesRemap[k] = addCodePasses(*scode.passes[k]);
      else
        tmpPassesRemap[k] = -1;

    // contruct shader code
    shader_layout::ShaderCode<> scr;

    scr.dynVariants = vt.addVariantTable(scode.dynVariants, tmpPassesRemap, tmpPasses.size(), shader_name);
    scr.passes = make_span(tmpPasses);

    chanStorage.getRef(scr.channel, scode.channel.data(), scode.channel.size(), 8);
    icStorage.getRef(scr.initCode, scode.initcode.data(), scode.initcode.size(), 8);

    // sort stVarMap indices
    Tab<uint32_t> tmpPair(tmpmem_ptr());
    tmpPair.resize(scode.stvarmap.size());
    for (int i = 0; i < tmpPair.size(); i++)
    {
      G_ASSERT(scode.stvarmap[i].v >= 0 && scode.stvarmap[i].v < 65535);
      G_ASSERT(scode.stvarmap[i].sv >= 0 && scode.stvarmap[i].sv < 65535);
      tmpPair[i] = (scode.stvarmap[i].v << 16) | (scode.stvarmap[i].sv & 0xFFFF);
    }
    sort(tmpPair, &cmpPacked2uint16Map);

    svStorage.getRef(scr.stVarMap, tmpPair.data(), tmpPair.size(), 8);

    G_ASSERT(scode.varsize >= 0 && scode.varsize < 32768);
    G_ASSERT(scode.flags >= 0 && scode.flags < 32768);
    G_ASSERT(scode.getVertexStride() < 32768);

    scr.varSize = scode.varsize;
    scr.codeFlags = scode.flags;
    if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
      scr.branchedCppStcodeId = scode.branchedCppStcodeId;
    scr.vertexStride = scode.getVertexStride();

    if (tmpPasses.size())
    {
      scr.suppBlockUid.resize(tmpPasses.size());
      for (int k = 0; k < scode.passes.size(); k++)
        if (scode.passes[k])
          scr.suppBlockUid[tmpPassesRemap[k]] = addSuppBlkCodes(scode.passes[k]->suppBlk);
    }

    staticTextureTypesByCode.emplace_back(scode.staticTextureTypes);

    codes.push_back(scr);
    return codes.size() - 1;
  }

  int addCodePasses(::ShaderCode::PassTab &pt)
  {
    shader_layout::Pass<> p;
    shader_layout::ShaderCode<>::ShRef sr{};
    if (!pt.rpass)
      p.rpass = nullptr;
    else
    {
      G_ASSERT(pt.rpass->vprog >= -1);
      G_ASSERT(pt.rpass->fsh >= -1);
      G_ASSERT(pt.rpass->stcodeNo >= -1 && pt.rpass->stcodeNo < 65535);
      G_ASSERT(pt.rpass->stblkcodeNo >= -1 && pt.rpass->stblkcodeNo < 65535);
      G_ASSERT(pt.rpass->renderStateNo >= -1 && pt.rpass->renderStateNo < 65535);
      sr.vprId = pt.rpass->vprog;
      sr.fshId = pt.rpass->fsh;
      sr.stcodeId = pt.rpass->stcodeNo;
      sr.stblkcodeId = pt.rpass->stblkcodeNo;
      sr.renderStateNo = pt.rpass->renderStateNo;
      sr.threadGroupSizeX = pt.rpass->threadGroupSizes[0];
      sr.threadGroupSizeY = pt.rpass->threadGroupSizes[1];
      sr.threadGroupSizeZ = pt.rpass->threadGroupSizes[2];
      sr.branchlessCppStblkcodeId = pt.rpass->branchlessCppStblkcodeId;
      if (shc::config().cppStcodeMode == shader_layout::ExternalStcodeMode::BRANCHED_CPP)
        sr.branchedCppStcodeRegisterTableOffset = pt.rpass->branchedCppStcodeTableOffset;
      else
        sr.branchlessCppStcodeId = pt.rpass->branchlessCppStcodeId;
      G_ASSERT(pt.rpass->threadGroupSizes[2] < (1 << 15));
      sr.scarlettWave32 = pt.rpass->scarlettWave32;
      bindump::Span<shader_layout::ShaderCode<>::ShRef> rpass;
      shrefStorage.getRef(rpass, &sr, 1);
      p.rpass = rpass;
    }

    tmpPasses.push_back(p);
    return tmpPasses.size() - 1;
  }

  static bindump::Span<blk_word_t> addSuppBlkCodes(dag::ConstSpan<bindump::Address<ShaderStateBlock>> blocks)
  {
    Tab<blk_word_t> codes(tmpmem);
    Tab<const ShaderStateBlock *> ssbHier(tmpmem);
    for (int i = 0; i < blocks.size(); i++)
    {
      const ShaderStateBlock &sb = *blocks[i];
      ssbHier.resize(1);
      ssbHier[0] = blocks[i];
      int st_code_idx = codes.size();
      addSuppCodes(ssbHier, codes);

      for (int j = st_code_idx; j < codes.size(); j++)
        codes[j] = ((codes[j] & ~sb.btd.uidMask) | sb.btd.uidVal) & fullBlkLayerMask;
    }

    if (!codes.size())
      codes.push_back(BLK_WORD_FULLMASK & fullBlkLayerMask);

    codes.push_back(BLK_WORD_FULLMASK);
    if (codes.size() > 2)
      fast_sort(codes, [](blk_word_t a, blk_word_t b) { return a < b; });

    bindump::Span<blk_word_t> idx;
    globalSuppBlkSign.getRef(idx, codes.data(), codes.size(), 64);
    return idx;
  }

  Tab<shader_layout::ShaderCode<>> codes;
  SharedStorage<shader_layout::ShaderCode<>::ShRef> shrefStorage;
  SharedStorage<ShaderChannelId> chanStorage;
  SharedStorage<int> icStorage;
  SharedStorage<uint32_t> svStorage;
  Tab<Tab<ShaderVarTextureType>> staticTextureTypesByCode;

  static Tab<shader_layout::Pass<>> tmpPasses;
};
Tab<shader_layout::Pass<>> ShaderCodes::tmpPasses(tmpmem_ptr());

struct ShaderClassRecEx
{
  void initShaderClass(::ShaderClass &sc, GatherNameMap &shNameMap, Variants &vt, Variables &vars)
  {
    // add shader codes
    tmpCodeRemap.resize(sc.code.size());
    for (int i = 0; i < sc.code.size(); i++)
      if (sc.code[i])
        tmpCodeRemap[i] = codes.addCode(*sc.code[i], vt, sc.name.c_str(), i);
      else
        tmpCodeRemap[i] = -1;

    // setup other data
    shClass.nameId = shNameMap.getNameId(sc.name.c_str());
    G_ASSERT(shClass.nameId >= 0);
    shClass.nameId = shNameMap.xmap[shClass.nameId];
    shClass.timestamp = {sc.timestamp};

    shClass.stVariants = vt.addVariantTable(sc.staticVariants, tmpCodeRemap, codes.codes.size(), sc.name.c_str());
    vars.addLocalVars(sc.stvar);
  }

  static void clearTemp()
  {
    ShaderCodes::clearTemp();
    clear_and_shrink(tmpCodeRemap);
  }

  shader_layout::ShaderClass<> shClass;
  ShaderCodes codes;

  static Tab<int> tmpCodeRemap;
};
Tab<int> ShaderClassRecEx::tmpCodeRemap(tmpmem_ptr());

void addRemappedStrData(GatherNameMap &varMap, Variables &vars, Variants &vt, bindump::VecHolder<bindump::StrHolder<>> &var_names)
{
  var_names.resize(varMap.xmap.size());
  int remapNameId = 0;
  Tab<int> remapNameIdTable(tmpmem);
  remapNameIdTable.resize(varMap.xmap.size());
  for (int i = 0; i < varMap.xmap.size(); i++)
  {
    bool not_found = true;
    for (int j = 0; j < vars.vars.size(); j++)
      if (i == vars.vars[j].nameId)
      {
        var_names[remapNameId] = varMap.getOrdinalName(i);
        remapNameIdTable[i] = remapNameId;
        remapNameId++;
        not_found = false;
        break;
      }
    if (not_found)
    {
      remapNameIdTable[i] = -1;
      debug("Name %s skipped", varMap.getOrdinalName(i));
    }
  }
  var_names.resize(remapNameId);

  // remap nameId to synchronize after stripped
  for (int j = 0; j < vars.vars.size(); j++)
  {
    G_ASSERT(vars.vars[j].nameId >= 0 && vars.vars[j].nameId < remapNameIdTable.size());
    vars.vars[j].nameId = remapNameIdTable[vars.vars[j].nameId];
  }
  for (int j = 0; j < vt.intervals.size(); j++)
  {
    if (vt.intervals[j].type == shader_layout::Interval<>::TYPE_MODE)
      continue;
    G_ASSERT(vt.intervals[j].nameId >= 0 && vt.intervals[j].nameId < remapNameIdTable.size());
    vt.intervals[j].nameId = remapNameIdTable[vt.intervals[j].nameId];
  }
}
} // namespace semicooked

using namespace semicooked;

//
// shaders binary dump builder
//
bool make_scripted_shaders_dump(const char *dump_name, const char *cache_filename, bool strip_shaders_and_stcode,
  BindumpPackingFlags packing_flags, const shc::CompilationContext &ctx)
{
  int64_t reft;
  String finalReport{};

  //
  // Load shaders and construct bindump from them
  //
  shc::TargetContext targetCtx = ctx.makeTargetContext(dump_name);

  Tab<uint32_t> gvmap(tmpmem);

  GatherNameMap varMap, shNameMap;
  Variants vt(varMap, targetCtx);
  Variables vars(varMap, targetCtx);
  Tab<ShaderClassRecEx> dumpClasses(tmpmem);

  sh_debug(SHLOG_NORMAL, "[INFO] Building bindump at '%s'...", dump_name);
  reft = ref_time_ticks();

  FullFileSaveCB fileWriter{dump_name};
  if (!fileWriter.fileHandle)
    return false;

  //
  // read data from intermediate shader file (.lib.bin)
  //
  if (!load_scripted_shaders(cache_filename, false, targetCtx))
    return false;

  sh_debug(SHLOG_NORMAL, "[INFO] Loaded linker output in %gms", get_time_usec(reft) / 1000.);
  reft = ref_time_ticks();

  ShaderTargetStorage &stor = targetCtx.storage();

  if (strip_shaders_and_stcode)
  {
    for (int i = 0; i < stor.shadersStcode.size(); i++)
      clear_and_shrink(stor.shadersStcode[i]);
    for (int i = 0; i < stor.ldShVpr.size(); i++)
      clear_and_shrink(stor.ldShVpr[i]);
    for (int i = 0; i < stor.ldShFsh.size(); i++)
      clear_and_shrink(stor.ldShFsh[i]);
    clear_and_shrink(stor.shadersStcode);
    clear_and_shrink(stor.ldShVpr);
    clear_and_shrink(stor.ldShFsh);
    clear_and_shrink(stor.renderStates);
    for (auto *&ptr : stor.shadersCompProg)
      delete eastl::exchange(ptr, nullptr);
    clear_and_shrink(stor.shadersCompProg);
    targetCtx.globVars().clear();
    targetCtx.samplers().clear();
    targetCtx.blocks() = {};
    for (::ShaderClass *sc : stor.shaderClass)
    {
      SerializableTab<::ShaderCode *> uniqueCodes{};
      Tab<int> codeIdRemapping(sc->code.size(), -1);
      for (auto [ind, cd] : enumerate(sc->code))
      {
        if (cd->passes.size() > 1)
          cd->passes.resize(1);
        if (cd->allPasses.size() > 1)
          cd->allPasses.resize(1);
        if (cd->passes.size())
        {
          if (auto *pt = cd->passes[0])
          {
            clear_and_shrink(pt->suppBlk);
            if (cd->allPasses.size())
              pt->rpass = &cd->allPasses[0];
          }
        }
        if (cd->allPasses.size())
        {
          auto &p = cd->allPasses[0];
          p.stblkcodeNo = p.stcodeNo = p.branchlessCppStblkcodeId = p.branchlessCppStcodeId = p.renderStateNo = p.fsh = p.vprog = -1;
          p.branchedCppStblkcodeTableOffset = p.branchedCppStcodeTableOffset = -1;
          p.threadGroupSizes = {};
          p.scarlettWave32 = false;
        }

        cd->dynVariants.reset();
        cd->branchedCppStblkcodeId = cd->branchedCppStcodeId = -1;
        clear_and_shrink(cd->stvarmap);
        cd->varsize = cd->regsize = 0;

        const auto strippedCodesIsEq = [cd](const ::ShaderCode *other) {
          const auto dataArraysAreEq = [](const auto &arr1, const auto &arr2) {
            G_ASSERT(elem_size(arr1) == elem_size(arr2));
            return arr1.size() == arr2.size() && memcmp(arr1.data(), arr2.data(), arr1.size() * elem_size(arr1)) == 0;
          };
          return dataArraysAreEq(other->channel, cd->channel) && other->initcode == cd->initcode &&
                 other->staticTextureTypes == cd->staticTextureTypes && other->flags == cd->flags;
        };
        if (auto it = eastl::find_if(uniqueCodes.begin(), uniqueCodes.end(), strippedCodesIsEq); it != uniqueCodes.end())
        {
          codeIdRemapping[ind] = eastl::distance(uniqueCodes.begin(), it);
          delete cd;
        }
        else
        {
          codeIdRemapping[ind] = uniqueCodes.size();
          uniqueCodes.push_back(cd);
        }
      }
      sc->code = eastl::move(uniqueCodes);
      for (int i = 0; i < sc->staticVariants.getVarCount(); ++i)
      {
        auto &var = *sc->staticVariants.getVariant(i);
        if (var.codeId >= 0)
          var.codeId = codeIdRemapping[var.codeId];
      }
    }
  }

  // process shader blocks
  Tab<ShaderStateBlock *> blocks = strip_shaders_and_stcode ? Tab<ShaderStateBlock *>{} : targetCtx.blocks().release();
  SharedStorage<blk_word_t> blkPartSign;
  processShaderBlocks(make_span(blocks), blkPartSign);
  globalSuppBlkSign.clear();

  bindumphlp::sortShaders(blocks, targetCtx);

  dag::Vector<int> stcode_type(stor.shadersStcode.size()); // 1 - blk, 2 - shclass blk, 3 -- shclass
  for (int i = 0; i < blocks.size(); i++)
  {
    int id = blocks[i]->stcodeId;
    if (id < 0)
      continue;
    G_ASSERT(id < stor.shadersStcode.size());
    stcode_type[id] = 1;
  }
  for (int i = 0; i < stor.shaderClass.size(); i++)
  {
    ::ShaderClass &sc = *stor.shaderClass[i];
    for (int j = 0; j < sc.code.size(); j++)
      if (sc.code[j])
        for (int k = 0; k < sc.code[j]->allPasses.size(); k++)
        {
          int id = sc.code[j]->allPasses[k].stblkcodeNo;
          if (id < 0)
            continue;
          G_ASSERT(id < stor.shadersStcode.size());
          if (stcode_type[id] == 0)
            stcode_type[id] = 2;
        }
  }
  for (int i = 0; i < stor.shaderClass.size(); i++)
  {
    ::ShaderClass &sc = *stor.shaderClass[i];
    for (int j = 0; j < sc.code.size(); j++)
      if (sc.code[j])
        for (int k = 0; k < sc.code[j]->allPasses.size(); k++)
        {
          int id = sc.code[j]->allPasses[k].stcodeNo;
          if (id < 0)
            continue;
          G_ASSERT(id < stor.shadersStcode.size());
          if (stcode_type[id] == 0)
            stcode_type[id] = 3;
        }
  }

  // prepare new sorted varMap
  for (int i = 0; i < targetCtx.varNameMap().getIdCount(); i++)
    varMap.addNameId(targetCtx.varNameMap().getName(i));
  varMap.prepareXmap();

  // prepare new sorted shaderNameMap
  for (int i = 0; i < stor.shaderClass.size(); i++)
    shNameMap.addNameId(stor.shaderClass[i]->name.c_str());
  shNameMap.prepareXmap();

  if (stor.shaderClass.size() != shNameMap.xmap.size())
  {
    sh_debug(SHLOG_ERROR, "%d shaders, only %d unique, duplicates are:", stor.shaderClass.size(), shNameMap.xmap.size());

    SmallTab<int, TmpmemAlloc> cnt;
    clear_and_resize(cnt, shNameMap.nameCount());
    mem_set_0(cnt);
    for (int i = 0; i < stor.shaderClass.size(); i++)
      cnt[shNameMap.getNameId(stor.shaderClass[i]->name.c_str())]++;

    for (int i = 0; i < shNameMap.xmap.size(); i++)
      if (cnt[i] > 1)
        sh_debug(SHLOG_INFO, "   %s - %d times", shNameMap.getOrdinalName(i), cnt[i]);
    return false;
  }

  // remap shader classes to be in sync with shaderNameMap
  {
    SmallTab<::ShaderClass *, TmpmemAlloc> n_shClass;
    clear_and_resize(n_shClass, stor.shaderClass.size());
    for (int i = 0; i < stor.shaderClass.size(); i++)
      n_shClass[shNameMap.xmap[i]] = stor.shaderClass[i];
    mem_copy_from(stor.shaderClass, n_shClass.data());
  }

  Tab<int> remapTable(tmpmem);
  bindumphlp::countRefAndRemapGlobalVars(remapTable, stor.shaderClass, varMap, targetCtx);

  bindump::VecHolder<bindump::StrHolder<>> shader_names;
  bindump::VecHolder<bindump::StrHolder<>> blk_names;
  bindump::VecHolder<bindump::StrHolder<>> var_names;

  shader_names.resize(shNameMap.xmap.size());
  for (int i = 0; i < shNameMap.xmap.size(); i++)
    shader_names[i] = shNameMap.getOrdinalName(i);

  blk_names.resize(blocks.size() + SHADER_BLOCK_HIER_LEVELS);
  for (int i = 0; i < SHADER_BLOCK_HIER_LEVELS; i++)
    blk_names[i] = hardcodedBlkName[i];
  for (int i = 0; i < blocks.size(); i++)
    blk_names[i + SHADER_BLOCK_HIER_LEVELS] = blocks[i]->name;

  // prepare global vars
  vars.addGlobVars(remapTable);

  // Generate the cpp stcode file for them
  if (shc::config().compileCppStcode() && !strip_shaders_and_stcode)
  {
    StcodeGlobalVars stcodeGlobvars(StcodeGlobalVars::Type::MAIN_COLLECTION);
    for (int i = 0; i < vars.varLists.back().v.size(); ++i)
    {
      const auto &var = vars.varLists.back().v[i];
      const char *name = vars.varMap.getOrdinalName(var.nameId);

      // Skip fake vars: array size getters and elements
      if (strchr(name, '(') || strchr(name, '['))
        continue;

      stcodeGlobvars.setVar((ShaderVarType)var.type, name, i);
    }

    save_stcode_global_vars(eastl::move(stcodeGlobvars), ctx.compInfo());
  }

  // convert data into binary dump prefabs
  int classnum = 0, codenum = 0, codepasses = 0;
  int new_codenum = 0;
  int max_reg_size = 0;
  int vpr_bytes = 0, fsh_bytes = 0, stcode_bytes0 = 0, stcode_bytes1 = 0, stcode_bytes2 = 0;
  int hs_ds_gs_bytes = 0, cs_bytes = 0;

  dumpClasses.resize(stor.shaderClass.size());
  eastl::unordered_map<eastl::string, uint8_t> assumedIntervals;
  for (int i = 0; i < stor.shaderClass.size(); i++)
  {
    ::ShaderClass &sc = *stor.shaderClass[i];

    for (auto &inter : sc.assumedIntervals)
      assumedIntervals[inter.name] = inter.value;

    dumpClasses[i].initShaderClass(sc, shNameMap, vt, vars);
    new_codenum += dumpClasses[i].codes.codes.size();

    for (int j = 0; j < sc.code.size(); j++)
      if (sc.code[j])
      {
        if (max_reg_size < sc.code[j]->regsize)
          max_reg_size = sc.code[j]->regsize;
        if (sc.code[j]->regsize > MAX_TEMP_REGS)
          sh_debug(SHLOG_FATAL, "  ShaderClass=%s has too many registers %d, limit is %d", sc.name, sc.code[j]->regsize,
            MAX_TEMP_REGS);

        for (int k = 0; k < sc.code[j]->passes.size(); k++)
          if (sc.code[j]->passes[k])
            codepasses++;
        codenum++;
      }
    classnum++;
  }

  for (auto &interval : vt.intervals)
    assumedIntervals.erase(varMap.getOrdinalName(interval.nameId));

  for (auto &[name, value] : assumedIntervals)
  {
    shader_layout::Interval<> ival;
    ival.type = ival.TYPE_ASSUMED_INTERVAL;
    int id = varMap.getNameId(name.c_str());
    if (id < 0)
      // Don't have correspond variable for interval
      continue;
    ival.nameId = varMap.xmap[id];
    ival.maxVal.setCount(value);
    vt.getIntervalId(ival);
  }

  addRemappedStrData(varMap, vars, vt, var_names);

  gvmap.resize(vars.varLists[0].v.size());
  for (int i = 0; i < gvmap.size(); i++)
    gvmap[i] = (i << 16) | (vars.varLists[0].v[i].nameId & 0xFFFF);
  sort(gvmap, &cmpPacked2uint16Map);

  for (int i = 0; i < blocks.size(); i++)
    if (max_reg_size < blocks[i]->regSize)
      max_reg_size = blocks[i]->regSize;
  ShaderClassRecEx::clearTemp();
  Variants::clearTemp();

  sh_debug(SHLOG_INFO, "  ShaderClasses=%d CodeNum=%d->%d CodePasses=%d", classnum, codenum, new_codenum, codepasses);
  sh_debug(SHLOG_INFO, "  VPR.num=%d FSH.num=%d stcode.num=%d", stor.ldShVpr.size(), stor.ldShFsh.size(), stor.shadersStcode.size());

  sh_debug(SHLOG_INFO, "  intervals=%d  iValStor=%d pcsStor=%d  lmap/qmap=%d/%d", vt.intervals.size(), vt.iValStorage.data.size(),
    vt.vtPcsStorage.data.size(), vt.vtLmapStorage.data.size(), vt.vtQmapStorage.data.size());

  //
  // write shaders binary dump
  //
  bindump::Master<shader_layout::ScriptedShadersBinDumpCompressed> shaders_dump_compressed;
  bindump::Master<shader_layout::ScriptedShadersBinDumpV3> shaders_dump;

  // write dump header
  shaders_dump_compressed.header.magicPart1 = _MAKE4C('VSPS');
  shaders_dump_compressed.header.magicPart2 = _MAKE4C('dump');
  shaders_dump_compressed.header.version = SHADER_BINDUMP_VER;

  shaders_dump.maxRegSize = max_reg_size;

  // write render states
  shaders_dump.renderStates = make_span(stor.renderStates);

  // write stcode data
  shaders_dump.stcode.resize(stor.shadersStcode.size());
  for (int i = 0; i < stor.shadersStcode.size(); i++)
  {
    if (stcode_type[i] == 0)
      continue;
    dag::ConstSpan<int> st = ::transcode_stcode(make_span_const(stor.shadersStcode[i]));

    shaders_dump.stcode[i] = st;
    if (stcode_type[i] == 1)
      stcode_bytes0 += data_size(st);
    else if (stcode_type[i] == 2)
      stcode_bytes1 += data_size(st);
    else
      stcode_bytes2 += data_size(st);
  }

  shaders_dump.externalStcodeMode = shc::config().cppStcodeMode;
  shaders_dump.externalStcodeVersion = uint32_t(CPP_STCODE_COMMON_VER);
  CryptoHash externalStcodeHash{};
  memset(externalStcodeHash.data, 0, sizeof(externalStcodeHash.data));

  if (shc::config().compileCppStcode() && !strip_shaders_and_stcode)
  {
    // Calculate overall cpp stcode hash for matching.
    CryptoHasher hasher{};
    for (const auto &hash : targetCtx.cppStcode().dynamicRoutineHashes)
      hasher.update(hash);
    for (const auto &hash : targetCtx.cppStcode().staticRoutineHashes)
      hasher.update(hash);
    externalStcodeHash = hasher.hash();

    eastl::string logHash{};
    for (uint8_t byte : externalStcodeHash.data)
      logHash.append_sprintf("%02x", byte);
    finalReport.aprintf(0, "\nGenerated stcode with\n  | version : %d\n  | hash    : %s\n", CPP_STCODE_COMMON_VER, logHash.c_str());

    // If validation masks are present, save them to the bindump
    if (shc::config().generateCppStcodeValidationData)
    {
      G_ASSERT(stor.stcodeConstValidationMasks.size() == shaders_dump.stcode.size());
      shaders_dump.stcodeConstValidationMasks.resize(stor.stcodeConstValidationMasks.size());
      for (const auto &[id, mask] : enumerate(stor.stcodeConstValidationMasks))
      {
        G_ASSERT(mask);
        shaders_dump.stcodeConstValidationMasks[id] = *mask;
        delete mask;
      }
    }

    // Generate stcode main file w/ the final stcode hash
    save_stcode_dll_main(eastl::move(targetCtx.cppStcode()), externalStcodeHash, ctx.compInfo());
  }

  static_assert(sizeof(shaders_dump.externalStcodeHash) == sizeof(externalStcodeHash.data));
  memcpy(shaders_dump.externalStcodeHash, externalStcodeHash.data, sizeof(shaders_dump.externalStcodeHash));

  auto &iValStorage = vt.iValStorage.getVecHolder();

  auto createHashTable = [](const dag::Vector<uint32_t> &hashes, uint32_t table_size, uint32_t shift, auto create_hash_table) {
    if (shift == 32)
      return eastl::pair{dag::Vector<uint32_t>{}, shift};
    dag::Vector<uint32_t> table(table_size);
    for (uint32_t i = 0; i < hashes.size(); i++)
    {
      uint32_t index = (hashes[i] >> shift) % table_size;
      if (table[index] != 0)
        return create_hash_table(hashes, table_size, shift + 1, create_hash_table);
      table[index] = i + 1;
    }
    return eastl::pair{table, shift};
  };

  auto nextPowerOfTwo = [](uint32_t u) {
    --u;
    u |= u >> 1;
    u |= u >> 2;
    u |= u >> 4;
    u |= u >> 8;
    u |= u >> 16;
    return ++u;
  };

  uint32_t bucketsCount = nextPowerOfTwo(vt.subintervals_by_interval.size());
  shaders_dump.intervalInfosBuckets.resize(bucketsCount);

  dag::Vector<uint32_t> intervalNameHashes(vt.subintervals_by_interval.size());
  eastl::unordered_map<uint32_t, bindump::StrHolder<> &> intervalNameUniqueHashes;
  for (size_t i = 0; i < vt.subintervals_by_interval.size(); i++)
  {
    if (vt.intervals[i].type != shader_layout::Interval<>::TYPE_GLOBAL_INTERVAL)
      continue;
    auto &name = var_names[vt.intervals[i].nameId];
    intervalNameHashes[i] = mem_hash_fnv1<32>(name.c_str(), name.length());
    if (intervalNameHashes[i] == 0)
      sh_debug(SHLOG_ERROR, "The hash is zero for the interval '%s', try renaming it", name.c_str());
    auto [it, inserted] = intervalNameUniqueHashes.emplace(intervalNameHashes[i], name);
    if (!inserted)
      sh_debug(SHLOG_ERROR, "Hash collision detected for the intervals '%s' and '%s', try renaming the intervals", name.c_str(),
        it->second.c_str());
  }

  for (size_t bucket = 0; bucket < bucketsCount; bucket++)
  {
    dag::Vector<uint32_t> hashes;
    for (uint32_t hash : intervalNameHashes)
    {
      if (hash != 0 && hash % bucketsCount == bucket)
        hashes.emplace_back(hash);
    }
    if (hashes.empty())
      continue;

    uint32_t tableSize = nextPowerOfTwo(hashes.size());
    constexpr uint32_t MAX_TABLE_SIZE = 16;
    for (; tableSize <= MAX_TABLE_SIZE; tableSize *= 2)
    {
      auto [table, shift] = createHashTable(hashes, tableSize, 0, createHashTable);
      if (!table.empty())
      {
        shaders_dump.intervalInfosBuckets[bucket].hashShift = shift;
        shaders_dump.intervalInfosBuckets[bucket].intervalInfoByHash.resize(table.size());
        break;
      }
    }
    if (tableSize > MAX_TABLE_SIZE)
    {
      sh_debug(SHLOG_ERROR, "The hash table for intervals is too large (bucket size %i for %i hashes), try renaming the intervals",
        tableSize, hashes.size());
    }
  }

  for (size_t i = 0; i < vt.subintervals_by_interval.size(); i++)
  {
    if (intervalNameHashes[i] == 0)
      continue;

    auto &info = shaders_dump.getIntervalInfoByHash(intervalNameHashes[i]);
    G_ASSERT(info.intervalId == 0);

    dag::Vector<uint32_t> hashes;
    eastl::unordered_map<uint32_t, bindump::StrHolder<> &> uniqueHashes;
    hashes.reserve(vt.subintervals_by_interval[i].size());
    for (auto &subinterval : vt.subintervals_by_interval[i])
    {
      hashes.emplace_back(mem_hash_fnv1<32>(subinterval.c_str(), subinterval.length()));
      auto [it, inserted] = uniqueHashes.emplace(hashes.back(), subinterval);
      if (!inserted)
      {
        sh_debug(SHLOG_ERROR,
          "Hash collision detected for the interval '%s', subintervals '%s' and '%s', try renaming the subintervals",
          var_names[vt.intervals[i].nameId].c_str(), subinterval.c_str(), it->second.c_str());
      }
    }

    info.intervalId = i;
    info.intervalNameHash = intervalNameHashes[i];
    info.subintervals = vt.subintervals_by_interval[i];
    info.subintervalHashes = hashes;

    uint32_t tableSize = nextPowerOfTwo(hashes.size());
    constexpr uint32_t MAX_TABLE_SIZE = 256;
    for (; tableSize <= MAX_TABLE_SIZE; tableSize *= 2)
    {
      auto [table, shift] = createHashTable(hashes, tableSize, 0, createHashTable);
      if (!table.empty())
      {
        dag::Vector<uint8_t> values;
        for (uint32_t id : table)
        {
          uint8_t index = id == 0 ? 0 : id - 1;
          values.emplace_back(index);
        }
        info.subintervalIndexByHash = values;
        info.hashShift = shift;
        break;
      }
    }
    if (tableSize > MAX_TABLE_SIZE)
    {
      sh_debug(SHLOG_ERROR,
        "The hash table for interval '%s' is too large (size %i for %i subintervals), try renaming the subintervals",
        var_names[vt.intervals[i].nameId].c_str(), tableSize, hashes.size());
    }
  }

  shaders_dump.iValStorage = eastl::move(iValStorage);
  shaders_dump.intervals = make_span(vt.intervals);

  // write data accessed by variant table
  shaders_dump.vtPcsStorage = eastl::move(vt.vtPcsStorage.getVecHolder());
  shaders_dump.vtQmapStorage = eastl::move(vt.vtQmapStorage.getVecHolder());
  shaders_dump.vtLmapStorage = eastl::move(vt.vtLmapStorage.getVecHolder());

  // write storage sizes for vprId/fshId
  shaders_dump.vprCount = stor.ldShVpr.size();
  shaders_dump.fshCount = stor.ldShFsh.size();

  // write global vars data
  shaders_dump.gvMap = make_span(gvmap);

  // write vars storage
  shaders_dump.varStorage = eastl::move(vars.storage);
  shaders_dump.variables = vars.vars;
  shaders_dump.globVars.v = vars.varLists[0].v;

  // write strings
  shaders_dump.varMap = var_names;
  shaders_dump.shaderNameMap = shader_names;
  shaders_dump.blockNameMap = blk_names;

  // write shader blocks test codes
  shaders_dump.blkPartSign = eastl::move(blkPartSign.getVecHolder());
  shaders_dump.globalSuppBlkSign = eastl::move(globalSuppBlkSign.getVecHolder());

  // write shader blocks
  shaders_dump.blocks.resize(SHADER_BLOCK_HIER_LEVELS + blocks.size());
  for (int i = 0; i < SHADER_BLOCK_HIER_LEVELS; i++)
  {
    shaders_dump.blocks[i].uidMask = hardcodedBlk[size_t(hardcodedBlkLayer[i])].uidMask;
    shaders_dump.blocks[i].uidVal = hardcodedBlk[size_t(hardcodedBlkLayer[i])].uidVal;
    shaders_dump.blocks[i].stcodeId = -1;
    shaders_dump.blocks[i].cppStcodeId = -1;
    shaders_dump.blocks[i].nameId = i;
  }

  for (int i = 0; i < blocks.size(); i++)
  {
    ShaderStateBlock &b = *blocks[i];

    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].uidMask = b.btd.uidMask;
    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].uidVal = b.btd.uidVal;
    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].suppBlkMask = b.btd.suppMask;
    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].stcodeId = b.stcodeId;
    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].cppStcodeId = b.cppStcodeId;
    shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].nameId = i + SHADER_BLOCK_HIER_LEVELS;
    if (b.btd.suppListOfs >= 0)
      shaders_dump.blocks[i + SHADER_BLOCK_HIER_LEVELS].suppBlockUid = shaders_dump.blkPartSign.getElementAddress(b.btd.suppListOfs);
  }

  {
    auto samplers = targetCtx.samplers().releaseSamplers();
    shaders_dump.samplers.resize(samplers.size());
    int smp_id = 0;
    for (const auto &s : samplers)
      shaders_dump.samplers[smp_id++] = s.mSamplerInfo;
  }

  // write shader classes
  SharedStorage<int> shInitCodeStorage;
  shaders_dump.classes.resize(dumpClasses.size());
  shaders_dump.messagesByShclass.resize(dumpClasses.size());
  for (int i = 0; i < dumpClasses.size(); i++)
  {
    ShaderCodes &code = dumpClasses[i].codes;
    auto &out_c = shaders_dump.classes[i];
    out_c = dumpClasses[i].shClass;

    out_c.shrefStorage = eastl::move(code.shrefStorage.getVecHolder());

    out_c.chanStorage = eastl::move(code.chanStorage.getVecHolder());
    out_c.icStorage = eastl::move(code.icStorage.getVecHolder());
    out_c.svStorage = eastl::move(code.svStorage.getVecHolder());
    out_c.localVars.v = vars.varLists[i + 1].v;
    out_c.code = make_span(code.codes);
    out_c.name = shaders_dump.shaderNameMap[out_c.nameId].getElementAddress(0);
    out_c.name.setCount(shaders_dump.shaderNameMap[out_c.nameId].size());

    if (shc::config().addTextureType && !code.staticTextureTypesByCode.empty())
      out_c.staticTextureTypeBySlot = code.staticTextureTypesByCode.front();

    shInitCodeStorage.getRef(out_c.initCode, stor.shaderClass[i]->shInitCode.data(), stor.shaderClass[i]->shInitCode.size(), 8);

    shaders_dump.messagesByShclass[i].resize(stor.shaderClass[i]->messages.size());
    for (int j = 0; j < stor.shaderClass[i]->messages.size(); j++)
      shaders_dump.messagesByShclass[i][j] = stor.shaderClass[i]->messages[j];
  }
  shaders_dump.shInitCodeStorage = eastl::move(shInitCodeStorage.getVecHolder());

  sh_debug(SHLOG_NORMAL, "[INFO] Built bindump data in %gms", get_time_usec(reft) / 1000.);

  // write vpr/fsh data
  // 1) collecting all transcoded shader codes into one container

  reft = ref_time_ticks();

  dag::Vector<dag::ConstSpan<uint32_t>> shaders(stor.ldShVpr.size() + stor.ldShFsh.size());
  for (int i = 0; i < stor.ldShVpr.size(); i++)
  {
    bool combi_type = stor.ldShVpr[i].size() > 4 && stor.ldShVpr[i][0] == _MAKE4C('DX11') && stor.ldShVpr[i][4] == _MAKE4C('DX11');
    dag::ConstSpan<uint32_t> sh = ::transcode_vertex_shader(stor.ldShVpr[i]);
    shaders[i] = sh;
    if (!combi_type)
      vpr_bytes += data_size(sh);
    else
    {
      vpr_bytes += sh[4] * 4;
      hs_ds_gs_bytes += (sh[5] + sh[6] + sh[7]) * 4;
    }
  }
  for (int i = 0; i < stor.ldShFsh.size(); i++)
  {
    int type = stor.ldShFsh[i].size() ? stor.ldShFsh[i][0] : 0;
    dag::ConstSpan<uint32_t> sh = ::transcode_pixel_shader(stor.ldShFsh[i]);
    shaders[i + stor.ldShVpr.size()] = sh;
    if (type != _MAKE4C('D11c'))
      fsh_bytes += data_size(sh);
    else
      cs_bytes += data_size(sh);
  }

#if _CROSS_TARGET_DX12
  shaders_dump.shaderHashes.resize(shaders.size());
  for (int i = 0; i < shaders.size(); i++)
  {
    auto *mapped = bindump::map<::dxil::ShaderContainer>((const uint8_t *)shaders[i].data());
    shaders_dump.shaderHashes[i] = mapped->dataHash;
  }
#endif

  // 2) rearranging shaders into groups and creating a mapping to get the original shader by index

  struct GroupInfo
  {
    int size = 0;
    dag::Vector<int> sh_ids;
  };

  const bool packGroups = (packing_flags & BindumpPackingFlagsBits::SHADER_GROUPS);

  const size_t dictSizeBytes = packGroups ? shc::config().dictionarySizeInKb << 10 : 0;
  const bool needToTrainDict = dictSizeBytes > 0;
  const size_t groupThresholdBytes = packGroups ? shc::config().shGroupSizeInKb << 10 : 0;
  const bool packEachShaderIntoSeparateGroup = groupThresholdBytes == 0;
  const int groupCompressionLevel = shc::config().shGroupCompressionLevel;
  const int dumpCompressionLevel = shc::config().shDumpCompressionLevel;

  dag::Vector<GroupInfo> groups;
  groups.reserve(shaders.size());
  shaders_dump.shGroupsMapping.resize(shaders.size());

  for (int sh = 0; sh < shaders.size(); sh++)
  {
    int index = -1;
    if (!packEachShaderIntoSeparateGroup)
    {
      for (int i = 0; i < groups.size(); i++)
        if (groups[i].size + data_size(shaders[sh]) < groupThresholdBytes)
        {
          index = i;
          break;
        }
    }

    auto &group_elem = index == -1 ? groups.emplace_back() : groups[index];
    shaders_dump.shGroupsMapping[sh].groupId = eastl::distance(groups.begin(), &group_elem);
    shaders_dump.shGroupsMapping[sh].indexInGroup = group_elem.sh_ids.size();
    group_elem.size += data_size(shaders[sh]);
    group_elem.sh_ids.push_back(sh);
  }

  auto make_group = [&shaders](const GroupInfo &gi) {
    bindump::Master<shader_layout::ShGroup> group;
    group.shaders.resize(gi.sh_ids.size());
    for (int sh = 0; sh < gi.sh_ids.size(); sh++)
      group.shaders[sh] = make_span_const((const uint8_t *)shaders[gi.sh_ids[sh]].data(), data_size(shaders[gi.sh_ids[sh]]));
    return group;
  };

  sh_debug(SHLOG_NORMAL, "[INFO] Collected shader groups in %gms", get_time_usec(reft) / 1000.);

  // 3) forming a solid array of all groups to create a common dictionary

  reft = ref_time_ticks();

  ZSTD_CDict_s *dict = nullptr;
  shaders_dump.dictionary = dag::Vector<char>{};

  if (needToTrainDict)
  {
    dag::Vector<char> samples_buffer;
    dag::Vector<size_t> samples_sizes;
    unsigned samples_total_size = 0;
    for (const auto &g : groups)
    {
      bindump::MemoryWriter writer;
      bindump::streamWrite(make_group(g), writer);
      samples_sizes.push_back(writer.mData.size());
      samples_buffer.insert(samples_buffer.end(), writer.mData.begin(), writer.mData.end());
      samples_total_size += writer.mData.size();
    }

    dag::Vector<char> dict_buffer;
    dict_buffer.resize(dictSizeBytes);
    size_t dict_size = zstd_train_dict_buffer(make_span(dict_buffer), groupCompressionLevel, samples_buffer, samples_sizes);
    debug("using dictionary %dK: trained from %d samples (%dK total size) to trained size=%dK (%d)", shc::config().dictionarySizeInKb,
      samples_sizes.size(), samples_total_size >> 10, dict_size >> 10, dict_size);
    dict_buffer.resize(dict_size);
    shaders_dump.dictionary = dict_buffer;

    dict = zstd_create_cdict(dag::Span<char>(dict_buffer.data(), dict_buffer.size()), groupCompressionLevel);

    sh_debug(SHLOG_NORMAL, "[INFO] Trained group compression dict in %gms", get_time_usec(reft) / 1000.);
  }

  // 4) compression of all groups

  reft = ref_time_ticks();

  shaders_dump.shGroups.resize(groups.size());

  // If just writing to memory or not using threads, just for-loop
  if (!packGroups || !shc::is_multithreaded())
  {
    ZSTD_CCtx_s *cctx = packGroups ? zstd_create_cctx() : nullptr;
    // @NOTE: calling compress with level = -1 means "write as is without compression"
    int packLevel = packGroups ? groupCompressionLevel : -1;
    for (size_t i = 0; i < shaders_dump.shGroups.size(); ++i)
      shaders_dump.shGroups[i].compress(make_group(groups[i]), packLevel, cctx, dict);

    if (cctx)
      zstd_destroy_cctx(cctx);
  }
  // Otherwise, compress in threads
  else if (!shaders_dump.shGroups.empty())
  {
    struct CompressionJob : shc::Job
    {
      using CompressedGroupRef = decltype(shaders_dump.shGroups[0]);
      using Group = decltype(make_group(groups[0]));

      CompressedGroupRef dest;
      Group src;
      const ZSTD_CDict_s *cdict;
      const int groupCompressionLevel;

      CompressionJob(CompressedGroupRef shGroup, const Group &&group, const ZSTD_CDict_s *dict, int groupCompressionLevel) :
        dest(shGroup), src(group), cdict(dict), groupCompressionLevel(groupCompressionLevel)
      {}

      void doJobBody() final
      {
        // Using TLS did not give any performance benefit while testing, but creates a slight mem leak
        ZSTD_CCtx_s *cctx = zstd_create_cctx();
        dest.compress(src, groupCompressionLevel, cctx, cdict);
        zstd_destroy_cctx(cctx);
      }

      void releaseJobBody() final {}
    };

    const int jobsCnt = shaders_dump.shGroups.size();
    dag::Vector<CompressionJob> jobs{};
    jobs.reserve(jobsCnt);
    uint32_t queuePos;
    for (int i = 0; i < jobsCnt; ++i)
    {
      jobs.emplace_back(shaders_dump.shGroups[i], make_group(groups[i]), dict, groupCompressionLevel);
      shc::add_job(&jobs.back(), shc::JobMgrChoiceStrategy::ROUND_ROBIN);
    }

    shc::await_all_jobs();
  }

  if (dict)
    zstd_destroy_cdict(dict);

  sh_debug(SHLOG_NORMAL, "[INFO] Compressed groups in %gms", get_time_usec(reft) / 1000.);
  reft = ref_time_ticks();

  const bool packBin = (packing_flags & BindumpPackingFlagsBits::WHOLE_BINARY);
  const int compressedSize =
    shaders_dump_compressed.scriptedShadersBindumpCompressed.compress(shaders_dump, packBin ? dumpCompressionLevel : -1);

  sh_debug(SHLOG_NORMAL, "[INFO] Compressed dump in %gms", get_time_usec(reft) / 1000.);
  reft = ref_time_ticks();

  // Splat data and calculate sha256 hash (of everything except the header)

  bindump::VectorWriter contentWriter{};
  bindump::streamWrite(shaders_dump_compressed, contentWriter);

  auto mapped = bindump::map<ScriptedShadersBinDumpCompressed>(contentWriter.mData.data());
  G_ASSERT(mapped);

  static_assert(offsetof(eastl::remove_pointer_t<decltype(mapped)>, scriptedShadersBindumpCompressed) == sizeof(mapped->header));
  auto content = dag::ConstSpan<uint8_t>{contentWriter.mData.data(), contentWriter.mData.size()}.subspan(sizeof(mapped->header));

  uint8_t checksum[SHA256_DIGEST_LENGTH];
  sha256_csum(content.data(), content.size(), checksum);

  static_assert(sizeof(mapped->header.checksumHash) == sizeof(checksum));
  memcpy(mapped->header.checksumHash, checksum, sizeof(mapped->header.checksumHash));

  sh_debug(SHLOG_NORMAL, "[INFO] Calculated checksum in %gms", get_time_usec(reft) / 1000.);
  reft = ref_time_ticks();

  {
    eastl::string logHash{};
    for (uint8_t byte : mapped->header.checksumHash)
      logHash.append_sprintf("%02x", byte);
    finalReport.aprintf(0, "Scripted shaders bindump:\n  | version : %d\n  | csum    : %s\n", mapped->header.version, logHash.c_str());
  }

  sh_debug(SHLOG_INFO, "  %d vars (%d global vars), storage=%d", vars.vars.size(), vars.varLists[0].v.size(),
    vars.storage.size() * sizeof(vars.storage[0]));
  sh_debug(SHLOG_INFO, "  shaders: %d bytes (VS: %d,  HS/DS/GS: %d,  PS: %d,  CS: %d)",
    vpr_bytes + fsh_bytes + hs_ds_gs_bytes + cs_bytes, vpr_bytes, hs_ds_gs_bytes, fsh_bytes, cs_bytes);
  sh_debug(SHLOG_INFO, "  stcode: %d/%d/%d bytes", stcode_bytes0, stcode_bytes1, stcode_bytes2);

  sh_debug(SHLOG_NORMAL, "  scripted shaders size: %d bytes (%d K)", compressedSize, compressedSize >> 10);
  if (shaders_dump_compressed.scriptedShadersBindumpCompressed.getDecompressedSize())
  {
    sh_debug(SHLOG_NORMAL, "  scripted shaders size uncompressed: %d bytes (%d K)",
      shaders_dump_compressed.scriptedShadersBindumpCompressed.getDecompressedSize(),
      shaders_dump_compressed.scriptedShadersBindumpCompressed.getDecompressedSize() >> 10);
  }

  //
  // write dump to output stream
  //
  if (ErrorCounter::allShaders().err != 0)
    return false;

  fileWriter.seekto(0);
  fileWriter.write(contentWriter.mData.data(), contentWriter.mData.size());

  sh_debug(SHLOG_NORMAL, "[INFO] Saved bindump in %gms", get_time_usec(reft) / 1000.);
  sh_debug(SHLOG_NORMAL, "%s", finalReport.c_str());

  return true;
}
