// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "shCompiler.h"
#include "codeBlocks.h"
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include "shMacro.h"
#include "shLog.h"
#include <shaders/shUtils.h>
#include <shaders/shader_ver.h>
#include <libTools/util/makeBindump.h>
#include <ioSys/dag_io.h>
#include <util/dag_fastIntList.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include "shsem.h"
#include "linkShaders.h"
#include "loadShaders.h"
#include "nameMap.h"

extern void parse_shader_script(const char *fn, const ShHardwareOptions &opt, Tab<SimpleString> *out_filenames);
extern void limitMaxFSHVersion(d3d::shadermodel::Version f);

#ifdef PROFILE_OPCODE_USAGE
int opcode_usage[2][256];
#endif

namespace shc
{
static DataBlock *assumedVarsBlock = NULL;
static DataBlock *reqShadersBlock = NULL;
static bool defShaderReq = false;
static bool defTreatInvalidAsNull = false;
static String updbPath;
unsigned compileJobsCount = 0;
unsigned compileJobsMgrBase = 0;
bool relinkOnly = false;
static SCFastNameMap explicitGlobVarRef;

void startup()
{
  resetCompiler();
  enable_sh_debug_con(true);
  ShaderMacroManager::init();
}

// close compiler
void shutdown()
{
  enable_sh_debug_con(false);
  ShaderMacroManager::realize();
}

// reset shader compiler internal structures (before next compilation)
void resetCompiler()
{
  reset_source_file();

  assumedVarsBlock = NULL;
  reqShadersBlock = NULL;
}

void reset_source_file()
{
  reportUnusedAssumes();
  close_shader_class();

  ShaderMacroManager::clearMacros();

  ErrorCounter::curShader().reset();
  ErrorCounter::allShaders().reset();

  ShaderParser::reset();

  init_shader_class();
}

String get_obj_file_name_from_source(const String &source_file_name, const ShVariantName &variant_name)
{
  char intermediateDir[260];
  strcpy(intermediateDir, (const char *)variant_name.intermediateDir);
  dd_append_slash_c(intermediateDir);

  String objFileName = String(intermediateDir) + dd_get_fname(source_file_name);
  variant_name.opt.appendOpts(objFileName);
  objFileName += ".obj";

  return objFileName;
}

// check shader file cache & return true, if cache needs recompilation
bool should_recompile_sh(const ShVariantName &variant_name, const String &sourceFileName)
{
  String objFileName = get_obj_file_name_from_source(sourceFileName, variant_name);
  return !check_scripted_shader(objFileName, true);
}
bool should_recompile(const ShVariantName &variant_name)
{
  if (!check_scripted_shader(variant_name.dest, true))
    return true;

  int64_t destFileTime;
  if (!get_file_time64(variant_name.dest, destFileTime))
    return true;

  for (unsigned int sourceFileNo = 0; sourceFileNo < variant_name.sourceFilesList.size(); sourceFileNo++)
  {
    const String &sourceFileName = variant_name.sourceFilesList[sourceFileNo];
    String objFileName = get_obj_file_name_from_source(sourceFileName, variant_name);

    int64_t objFileTime;
    if (!get_file_time64(objFileName, objFileTime))
      return true;

    if (objFileTime > destFileTime)
      return true;

    if (should_recompile_sh(variant_name, sourceFileName))
      return true;
  }
  return false;
}

// compile shader files & generate variants to disk. return false, if error occurs
void compileShader(const ShVariantName &variant_name, bool no_save, bool should_rebuild, bool compile_only)
{
  sh_debug(SHLOG_INFO, "Compile shaders to '%s'", variant_name.dest.str());

  variant_name.opt.dumpInfo();

  limitMaxFSHVersion(variant_name.opt.fshVersion);

  // Compile.
#ifdef PROFILE_OPCODE_USAGE
  memset(opcode_usage, 0, sizeof(opcode_usage));
#endif

  dd_mkpath(variant_name.intermediateDir);

  for (unsigned int sourceFileNo = 0; sourceFileNo < variant_name.sourceFilesList.size(); sourceFileNo++)
  {
    const String &sourceFileName = variant_name.sourceFilesList[sourceFileNo];
    String objFileName = get_obj_file_name_from_source(sourceFileName, variant_name);

    bool need_recompile = should_rebuild || should_recompile_sh(variant_name, sourceFileName);
    if (!need_recompile)
    {
      sh_debug(SHLOG_INFO, "No changes in '%s', skipping", sourceFileName.str());
    }
    else
    {
      if (relinkOnly)
      {
        sh_debug(SHLOG_FATAL, "need to recompile %s but compilation is denied by -relinkOnly", sourceFileName.str());
        return;
      }
      sh_debug(SHLOG_NORMAL, "[INFO] compiling '%s'...", sourceFileName.str());
      fflush(stdout);
      Tab<SimpleString> dependenciesList(tmpmem_ptr());

      reset_source_file();

      CodeSourceBlocks::incFiles.reset();
      parse_shader_script(sourceFileName, variant_name.opt, &dependenciesList);

#if _CROSS_TARGET_DX12
      recompile_shaders();
#endif

      dependenciesList.reserve(dependenciesList.size() + CodeSourceBlocks::incFiles.nameCount());
      for (int i = 0, e = CodeSourceBlocks::incFiles.nameCount(); i < e; i++)
        dependenciesList.push_back() = CodeSourceBlocks::incFiles.getName(i);

      if (!no_save)
        save_scripted_shaders(objFileName, dependenciesList);
    }
  }
  if (compile_only)
    return;

#ifdef PROFILE_OPCODE_USAGE
  debug("opcode usage (stateblocks):");
  for (int i = 0; i < 256; i++)
    if (opcode_usage[0][i])
      debug("  %05d: %s[%d]", opcode_usage[0][i], ShUtils::shcod_tokname(i), i);
  debug("opcode usage (dynamic):");
  for (int i = 0; i < 256; i++)
    if (opcode_usage[1][i])
      debug("  %05d: %s[%d]", opcode_usage[1][i], ShUtils::shcod_tokname(i), i);
  debug("-----------------------");
#endif

  // Link.

  sh_debug(SHLOG_NORMAL, "[INFO] Linking...");

  resetCompiler();
  String stats;
  stats.resize(2048);
  get_memory_stats(stats, 2048);
  debug("%s", stats.str());

  if (!no_save)
  {
    int64_t reft = ref_time_ticks();
    for (unsigned int sourceFileNo = 0; sourceFileNo < variant_name.sourceFilesList.size(); sourceFileNo++)
    {
      const String &sourceFileName = variant_name.sourceFilesList[sourceFileNo];
      String objFileName = get_obj_file_name_from_source(sourceFileName, variant_name);

      file_ptr_t objFile = df_open(objFileName, DF_READ);
      G_ASSERT(objFile);

      int mmaped_len = 0;
      const void *mmaped = df_mmap(objFile, &mmaped_len);
      bool res = link_scripted_shaders((const uint8_t *)mmaped, mmaped_len, objFileName);
      df_unmap(mmaped, mmaped_len);
      G_ASSERT(res);
#if SHOW_MEM_STAT
      get_memory_stats(stats, 2048);
      debug("%s", stats.str());
#endif
      df_close(objFile);
    }
    sh_debug(SHLOG_NORMAL, "[INFO] linked in %gms", get_time_usec(reft) / 1000.);

    reft = ref_time_ticks();
    sh_debug(SHLOG_NORMAL, "[INFO] Saving...");
    save_scripted_shaders(variant_name.dest, {});
    sh_debug(SHLOG_NORMAL, "[INFO] saved in %gms", get_time_usec(reft) / 1000.);
  }
}

bool buildShaderBinDump(const char *bindump_fn, const char *sh_fn, bool forceRebuild, bool minidump, bool pack)
{
  if (!forceRebuild)
  {
    int64_t sh_time, dump_time;
    if (!get_file_time64(sh_fn, sh_time))
    {
      dd_erase(bindump_fn);
      return false;
    }

    if (get_file_time64(bindump_fn, dump_time))
      if (dump_time >= sh_time)
      {
        FullFileLoadCB crd(bindump_fn);
        int hdr[4];
        if (crd.tryRead(hdr, 16) == 16)
        {
          if (hdr[0] == _MAKE4C('VSPS') && hdr[1] == _MAKE4C('dump') && hdr[2] == SHADER_BINDUMP_VER)
          {
            sh_debug(SHLOG_INFO, "Skipping up-to-date binary %sdump '%s'\n", minidump ? "mini" : "", bindump_fn);
            return true;
          }
        }
      }
  }
  String tempbindump_fn(bindump_fn);
  tempbindump_fn += ".tmp.bin";
  if (!make_scripted_shaders_dump(tempbindump_fn, sh_fn, minidump, pack))
  {
    sh_debug(SHLOG_ERROR, "Can't build binary %sdump from: '%s'\n", minidump ? "mini" : "", sh_fn);
    dd_erase(bindump_fn);
    return false;
  }
  else
  {
    if (!dd_rename(tempbindump_fn.c_str(), bindump_fn))
      sh_debug(SHLOG_ERROR, "Can't rename binary '%s'->'%s'\n", tempbindump_fn.c_str(), bindump_fn);
  }

  sh_debug(SHLOG_INFO, "+++ Built shaders binary %sdump: '%s'\n", minidump ? "mini" : "", bindump_fn);
  return true;
}

void setAssumedVarsBlock(DataBlock *block) { assumedVarsBlock = block; }
void setRequiredShadersBlock(DataBlock *block) { reqShadersBlock = block; }
void setRequiredShadersDef(bool on) { defShaderReq = on; }

DataBlock *getAssumedVarsBlock() { return assumedVarsBlock; }
bool isShaderRequired(const char *shader_name)
{
  return reqShadersBlock ? reqShadersBlock->getBool(shader_name, defShaderReq) : defShaderReq;
}

void clearFlobVarRefList() { explicitGlobVarRef.reset(); }
void addExplicitGlobVarRef(const char *vname) { explicitGlobVarRef.addNameId(vname); }
bool isGlobVarRequired(const char *vname) { return explicitGlobVarRef.getNameId(vname) != -1; }

struct ShaderValidVariantList
{
  struct VarVal
  {
    int nameId;
    float val;
  };
  struct Ref
  {
    short startIdx, count;
  };
  SCFastNameMap vars;
  Tab<VarVal> vvData;
  Tab<Ref> vv;
  bool invalidAsNull;

  ShaderValidVariantList() : vvData(midmem), vv(midmem), invalidAsNull(defTreatInvalidAsNull) {}
  void fill(const DataBlock &blk)
  {
    int nid = blk.getNameId("invalid");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        const DataBlock &b = *blk.getBlock(i);
        Ref &r = vv.push_back();
        r.startIdx = vvData.size();
        for (int j = 0; j < b.paramCount(); j++)
          if (b.getParamType(j) == DataBlock::TYPE_INT)
          {
            VarVal &v = vvData.push_back();
            v.nameId = vars.addNameId(b.getParamName(j));
            v.val = (real)b.getInt(j);
          }
          else if (b.getParamType(j) == DataBlock::TYPE_REAL)
          {
            VarVal &v = vvData.push_back();
            v.nameId = vars.addNameId(b.getParamName(j));
            v.val = b.getReal(j);
          }
        r.count = vvData.size() - r.startIdx;
        if (!r.count)
          vv.pop_back();
      }
    vvData.shrink_to_fit();
    vv.shrink_to_fit();
    invalidAsNull = blk.getBool("treatInvalidAsNull", invalidAsNull);
  }
  void makeTypeMap(Tab<int> &map, const ShaderVariant::TypeTable &v)
  {
    map.resize(vars.nameCount());
    mem_set_ff(map);
    for (int i = 0; i < v.getCount(); i++)
    {
      int id = vars.getNameId(v.getIntervalName(i));
      if (id != -1)
        map[id] = i;
    }
  }
  void gatherRelevantIdx(FastIntList &idx, dag::ConstSpan<int> map)
  {
    idx.reset();
    for (int i = 0; i < vv.size(); i++)
    {
      bool all_found = true;
      for (int j = vv[i].startIdx, end = j + vv[i].count; j < end; j++)
        if (map[vvData[j].nameId] == -1)
        {
          all_found = false;
          break;
        }
      if (all_found)
        idx.addInt(i);
    }
  }
  void gatherRelevantIdx2(FastIntList &idx, dag::ConstSpan<int> map, const FastIntList &idx_base, dag::ConstSpan<int> base_map)
  {
    idx.reset();
    for (int i = 0; i < vv.size(); i++)
      if (!idx_base.hasInt(i))
      {
        bool all_found = true;
        for (int j = vv[i].startIdx, end = j + vv[i].count; j < end; j++)
          if (map[vvData[j].nameId] == -1 && base_map[vvData[j].nameId] == -1)
          {
            all_found = false;
            break;
          }
        if (all_found)
          idx.addInt(i);
      }
  }

  bool checkValid(dag::ConstSpan<int> idx, dag::ConstSpan<int> map, const ShaderVariant::VariantSrc &v)
  {
    const ShaderVariant::TypeTable &vt = v.getTypes();
    for (int i = 0; i < idx.size(); i++)
    {
      bool all_eq = true;
      for (int j = vv[idx[i]].startIdx, end = j + vv[idx[i]].count; j < end; j++)
      {
        int mid = map[vvData[j].nameId];
        G_ASSERT(mid >= 0);
        if (vt.normalizeValue(mid, vvData[j].val) != v.getNormalizedValue(mid))
        {
          all_eq = false;
          break;
        }
      }
      if (all_eq)
        return true;
    }
    return false;
  }
  bool checkValid2(dag::ConstSpan<int> idx, dag::ConstSpan<int> map, const ShaderVariant::VariantSrc &v, dag::ConstSpan<int> map2,
    const ShaderVariant::VariantSrc &v2)
  {
    const ShaderVariant::TypeTable &vt = v.getTypes();
    const ShaderVariant::TypeTable &vt2 = v2.getTypes();
    for (int i = 0; i < idx.size(); i++)
    {
      bool all_eq = true;
      for (int j = vv[idx[i]].startIdx, end = j + vv[idx[i]].count; j < end; j++)
      {
        int mid = map[vvData[j].nameId];
        int mid2 = map2[vvData[j].nameId];
        G_ASSERT(mid >= 0 || mid2 >= 0);
        if ((mid >= 0 && vt.normalizeValue(mid, vvData[j].val) != v.getNormalizedValue(mid)) ||
            (mid2 >= 0 && vt.normalizeValue(mid2, vvData[j].val) != v2.getNormalizedValue(mid2)))
        {
          all_eq = false;
          break;
        }
      }
      if (all_eq)
        return true;
    }
    return false;
  }

  void dump()
  {
    for (int i = 0; i < vv.size(); i++)
    {
      debug("  invalid %d:", vv[i].count);
      for (int j = 0; j < vv[i].count; j++)
        debug("    %s[%d]=%f:", vars.getName(vvData[vv[i].startIdx + j].nameId), vvData[vv[i].startIdx + j].nameId,
          vvData[vv[i].startIdx + j].val);
    }
  }
};
static eastl::vector<ShaderValidVariantList> svvl;
static SCFastNameMap svvlNames;
static ShaderValidVariantList *curSvvl = NULL;
static const ShaderVariant::TypeTable *curStatVariantTypes = NULL, *curDynVariantTypes = NULL;
static FastIntList curVvStIdx, curVvDynIdx;
static Tab<int> stTypeMap(midmem), dynTypeMap(midmem);

void setInvalidAsNullDef(bool on) { defTreatInvalidAsNull = on; }
void setValidVariantsBlock(DataBlock *block)
{
  svvl.clear();
  svvlNames.reset();
  if (!block)
    return;

  svvl.emplace_back();
  svvl.back().fill(*block);
  int nid = block->getNameId("invalid");
  for (int i = 0; i < block->blockCount(); i++)
  {
    if (block->getBlock(i)->getBlockNameId() == nid)
      continue;
    if (svvlNames.getNameId(block->getBlock(i)->getBlockName()) != -1)
    {
      sh_debug(SHLOG_ERROR, "duplicate block <%s> in valid_variants block", block->getBlock(i)->getBlockName());
      continue;
    }

    svvl.emplace_back();
    ShaderValidVariantList &l = svvl.back();
    l.fill(*block);
    l.fill(*block->getBlock(i));
    if (l.vv.size())
      svvlNames.addNameId(block->getBlock(i)->getBlockName());
    else
      svvl.pop_back();
  }
  if (svvl.size() == 1 && !svvl[0].vv.size())
  {
    svvl.clear();
    svvlNames.reset();
  }

  for (int i = 0; i < svvl.size(); i++)
  {
    debug("shader <%s>, %d", i == 0 ? "ALL" : svvlNames.getName(i - 1), svvl[i].vv.size());
    svvl[i].dump();
  }
}

void prepareTestVariantShader(const char *name)
{
  curStatVariantTypes = NULL;
  curDynVariantTypes = NULL;
  curSvvl = svvl.size() && name ? &svvl[svvlNames.getNameId(name) + 1] : NULL;
  curVvStIdx.reset();
  curVvDynIdx.reset();
}
void prepareTestVariant(const ShaderVariant::TypeTable *sv, const ShaderVariant::TypeTable *dv)
{
  if (!curSvvl)
    return;
  if (sv)
  {
    if (curStatVariantTypes != sv)
    {
      curSvvl->makeTypeMap(stTypeMap, *sv);
      curSvvl->gatherRelevantIdx(curVvStIdx, stTypeMap);
    }
    curStatVariantTypes = sv;
  }

  if (dv)
  {
    if (curDynVariantTypes != dv)
    {
      curSvvl->makeTypeMap(dynTypeMap, *dv);
      curSvvl->gatherRelevantIdx2(curVvDynIdx, dynTypeMap, curVvStIdx, stTypeMap);
    }
    curDynVariantTypes = dv;
  }
  else
  {
    // reset dynamic variant
    curDynVariantTypes = NULL;
  }
}
bool isValidVariant(const ShaderVariant::VariantSrc *sv, const ShaderVariant::VariantSrc *dv)
{
  if (!curSvvl)
    return true;

  if (!dv)
    return !curSvvl->checkValid(curVvStIdx.getList(), stTypeMap, *sv);
  return !curSvvl->checkValid2(curVvDynIdx.getList(), dynTypeMap, *dv, stTypeMap, *sv);
}
bool shouldMarkInvalidAsNull() { return curSvvl ? curSvvl->invalidAsNull : true; }
void setOutputUpdbPath(const char *path) { updbPath = path; }
const char *getOutputUpdbPath() { return updbPath; }

static SCFastNameMap allAssumedVars, knownAssumedVars;
void registerAssumedVar(const char *name, bool known)
{
  allAssumedVars.addNameId(name);
  if (known)
    knownAssumedVars.addNameId(name);
}
void reportUnusedAssumes()
{
  for (int i = 0, e = allAssumedVars.nameCount(); i < e; i++)
    if (knownAssumedVars.getNameId(allAssumedVars.getName(i)) == -1)
      sh_debug(SHLOG_SILENT_WARNING, "Assume variables: variable \"%s\" not declared", allAssumedVars.getName(i));
  allAssumedVars.reset();
  knownAssumedVars.reset();
}
} // namespace shc
