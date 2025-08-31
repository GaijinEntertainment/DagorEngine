// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "loadShaders.h"
#include "shCompiler.h"
#include "linkShaders.h"
#include "shTargetContext.h"
#include "shCompiler.h"
#include "shLog.h"
#include "cppStcode.h"
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <util/dag_simpleString.h>
#include "shCacheVer.h"
#include "shaderTab.h"
#include "shcode.h"
#include "shaderSave.h"
#include "globVar.h"
#include "varMap.h"
#include "namedConst.h"
#include "samplers.h"
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_renderStates.h>


bool get_file_time64(const char *fn, int64_t &ft)
{
  DagorStat st;
  if (df_stat(fn, &st) == 0)
  {
    ft = st.mtime;
    return true;
  }
  return false;
}

CompilerAction check_scripted_shader(const char *filename, dag::ConstSpan<String> current_deps, const ShCompilationInfo &comp,
  bool check_cppstcode)
{
  bindump::FileReader reader(filename);
  if (!reader)
    return CompilerAction::COMPILE_AND_LINK;

  ShadersBindumpHeader header;
  try
  {
    bindump::streamRead(header, reader);
    if (header.cache_sign != SHADER_CACHE_SIGN)
    {
      sh_debug(SHLOG_NORMAL, "Invalid file format");
      return CompilerAction::COMPILE_AND_LINK;
    }

    if (header.cache_version != SHADER_CACHE_VER)
    {
      sh_debug(SHLOG_NORMAL, "[INFO] Outdated shader cache version (loaded %c%c%c%c, but required %c%c%c%c), rebuilding",
        _DUMP4C(header.cache_version), _DUMP4C(SHADER_CACHE_VER));
      return CompilerAction::COMPILE_AND_LINK;
    }

    if (!header.hash.isLayoutValid())
    {
      sh_debug(SHLOG_NORMAL, "Invalid shader layout hash");
      return CompilerAction::COMPILE_AND_LINK;
    }
  }
  catch (IGenLoad::LoadException &e)
  {
    // The file could not be read, or the file is missing or the file is too large, etc.
    // It's not a fatal error to interrupt the entire compilation, just let this shader recompile completely
    sh_debug(SHLOG_NORMAL, "Invalid shader file %s: %s, code %i", filename, e.excDesc, e.excCode);
    bindump::close();
    return CompilerAction::COMPILE_AND_LINK;
  }

  int64_t srcFt;
  if (!get_file_time64(filename, srcFt))
  {
    sh_debug(SHLOG_WARNING, "Cannot detect cache file time");
    return CompilerAction::COMPILE_AND_LINK;
  }

  if (header.last_blk_hash != comp.targetBlkHash())
  {
    sh_debug(SHLOG_NORMAL, "[INFO] Outdated blk hash '%s' in header of obj file '%s'", blk_hash_string(header.last_blk_hash).c_str(),
      dd_get_fname(filename));
    return CompilerAction::COMPILE_AND_LINK;
  }

  if (check_cppstcode)
  {
    auto cppstcodeStatMaybe = get_gencpp_files_stat(filename, comp);
    if (!cppstcodeStatMaybe || cppstcodeStatMaybe->savedBlkHash != comp.targetBlkHash())
    {
      sh_debug(SHLOG_NORMAL, "[INFO] Cppstcode files for '%s' are out of sync", dd_get_fname(filename));
      return CompilerAction::COMPILE_AND_LINK;
    }

    srcFt = min(srcFt, cppstcodeStatMaybe->mtime);
  }

  if (!current_deps.empty())
  {
    NameMap currentDepSet;
    for (int i = 0; i < current_deps.size(); i++)
      currentDepSet.addNameId(current_deps[i].c_str());
    NameMap previousDepSet;
    for (int i = 0; i < header.dependency_files.size(); i++)
    {
      if (currentDepSet.getNameId(header.dependency_files[i].c_str()) == -1)
      {
        sh_debug(SHLOG_NORMAL, "Source shader file '%s' is not in the list of current dependencies",
          header.dependency_files[i].c_str());
        return CompilerAction::LINK_ONLY;
      }
    }
    for (int i = 0; i < header.dependency_files.size(); i++)
      previousDepSet.addNameId(header.dependency_files[i].c_str());
    for (int i = 0; i < current_deps.size(); i++)
    {
      if (previousDepSet.getNameId(current_deps[i].c_str()) == -1)
      {
        sh_debug(SHLOG_NORMAL, "Source shader file '%s' is not in the list of previous dependencies", current_deps[i].c_str());
        return CompilerAction::LINK_ONLY;
      }
    }
  }
  else
  {
    int existsCount = 0;
    for (int i = 0; i < header.dependency_files.size(); i++)
    {
      int64_t ft;
      if (get_file_time64(header.dependency_files[i].c_str(), ft))
      {
        existsCount++;
        if (srcFt <= ft)
        {
          debug("Source shader file '%s' has been changed since last cache build", header.dependency_files[i].c_str());
          return CompilerAction::COMPILE_AND_LINK;
        }
      }
      else
      {
        sh_debug(SHLOG_NORMAL, "Source shader file '%s' is missing", header.dependency_files[i].c_str());
      }
    }

    if (existsCount > 0 && existsCount != header.dependency_files.size())
    {
      sh_debug(SHLOG_NORMAL, "Some source shader files are missing");
      return CompilerAction::COMPILE_AND_LINK;
    }
  }
  return CompilerAction::NOTHING;
}

bool load_scripted_shaders(const char *filename, bool check_dep, shc::TargetContext &out_ctx)
{
  ShadersBindump shaders;
  bindump::FileReader reader(filename);
  if (!load_shaders_bindump(shaders, reader, out_ctx))
    sh_debug(SHLOG_FATAL, "corrupted OBJ file: %s", filename);

  ShaderTargetStorage &stor = out_ctx.storage();

  stor.renderStates = eastl::move(shaders.renderStates);
  stor.ldShFsh = eastl::move(shaders.shadersFsh);
  stor.ldShVpr = eastl::move(shaders.shadersVpr);
  stor.shadersStcode = eastl::move(shaders.shadersStcode);
  stor.stcodeConstValidationMasks = eastl::move(shaders.stcodeConstMasks);
  stor.shaderClass = eastl::move(shaders.shaderClasses);

  out_ctx.globVars().getMutableVariableList() = eastl::move(shaders.variable_list);
  out_ctx.globVars().getMutableIntervalList() = eastl::move(shaders.intervals);

  out_ctx.samplers().emplaceSamplers(eastl::move(shaders.static_samplers));

  out_ctx.blocks().link(shaders.state_blocks, {}, {});

  out_ctx.cppStcode().dynamicRoutineHashes = eastl::move(shaders.dynamicCppcodeHashes);
  out_ctx.cppStcode().staticRoutineHashes = eastl::move(shaders.staticCppcodeHashes);
  out_ctx.cppStcode().dynamicRoutineNames = eastl::move(shaders.dynamicCppcodeRoutineNames);
  out_ctx.cppStcode().staticRoutineNames = eastl::move(shaders.staticCppcodeRoutineNames);

  out_ctx.cppStcode().regTable.offsets = eastl::move(shaders.cppcodeRegisterTableOffsets);

  out_ctx.cppStcode().regTable.combinedTable.reserve(shaders.cppcodeRegisterTables.size());
  for (auto &&table : shaders.cppcodeRegisterTables)
    out_ctx.cppStcode().regTable.combinedTable.emplace_back(eastl::move(table));

  for (auto cl : stor.shaderClass)
  {
    cl->staticVariants.setContextRef(out_ctx);
    cl->staticVariants.linkIntervalList();
    for (auto code : cl->code)
    {
      code->link(out_ctx);
      code->dynVariants.setContextRef(out_ctx);
      code->dynVariants.linkIntervalList();
    }
  }

  return true;
}
