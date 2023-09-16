#include "loadShaders.h"
#include "linkShaders.h"
#include "shLog.h"
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
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <debug/dag_debug.h>
#include <3d/dag_renderStates.h>

namespace loadedshaders
{
Tab<TabFsh> fsh(midmem_ptr());
Tab<TabVpr> vpr(midmem_ptr());
Tab<TabStcode> stCode(midmem_ptr());
Tab<shaders::RenderState> render_state(midmem_ptr());
Tab<ShaderClass *> shClass(midmem_ptr());
} // namespace loadedshaders

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

bool check_scripted_shader(const char *filename, bool check_dep)
{
  bindump::FileReader reader(filename);
  if (!reader)
    return false;

  ShadersBindumpHeader header;
  try
  {
    bindump::streamRead(header, reader);
    if (header.cache_sign != SHADER_CACHE_SIGN)
    {
      sh_debug(SHLOG_NORMAL, "Invalid file format");
      return false;
    }

    if (header.cache_version != SHADER_CACHE_VER)
    {
      sh_debug(SHLOG_NORMAL, "Invalid shader cache version (loaded %c%c%c%c, but required %c%c%c%c)", _DUMP4C(header.cache_version),
        _DUMP4C(SHADER_CACHE_VER));
      return false;
    }

    if (!header.hash.isLayoutValid())
    {
      sh_debug(SHLOG_NORMAL, "Invalid shader layout hash");
      return false;
    }

    if (!check_dep)
      return true;
  }
  catch (IGenLoad::LoadException &e)
  {
    // The file could not be read, or the file is missing or the file is too large, etc.
    // It's not a fatal error to interrupt the entire compilation, just let this shader recompile completely
    sh_debug(SHLOG_NORMAL, "Invalid shader file %s: %s, code %i", filename, e.excDesc, e.excCode);
    bindump::close();
    return false;
  }

  int64_t srcFt;
  if (!get_file_time64(filename, srcFt))
  {
    sh_debug(SHLOG_WARNING, "Cannot detect cache file time");
    return false;
  }

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
        return false;
      }
    }
  }

  if (existsCount > 0 && existsCount != header.dependency_files.size())
  {
    sh_debug(SHLOG_NORMAL, "Some source shader files are missing");
    return false;
  }
  return true;
}

bool load_scripted_shaders(const char *filename, bool check_dep)
{
  VarMap::clear();

  ShadersBindump shaders;
  bindump::FileReader reader(filename);
  if (!load_shaders_bindump(shaders, reader))
    fatal("corrupted OBJ file: %s", filename);

  loadedshaders::render_state = eastl::move(shaders.render_states);
  loadedshaders::fsh = eastl::move(shaders.shaders_fsh);
  loadedshaders::vpr = eastl::move(shaders.shaders_vpr);
  loadedshaders::stCode = eastl::move(shaders.shaders_stcode);
  loadedshaders::shClass = eastl::move(shaders.shader_classes);

  ShaderGlobal::getMutableVariableList() = eastl::move(shaders.variable_list);
  ShaderGlobal::getMutableIntervalList() = eastl::move(shaders.intervals);

  ShaderStateBlock::link(shaders.state_blocks, {});

  for (auto cl : loadedshaders::shClass)
  {
    cl->staticVariants.linkIntervalList();
    for (auto code : cl->code)
    {
      code->link();
      code->dynVariants.linkIntervalList();
    }
  }

  return true;
}

void unload_scripted_shaders()
{
  using loadedshaders::shClass;

  ShaderGlobal::clear();
  VarMap::clear();

  clear_and_shrink(loadedshaders::fsh);
  clear_and_shrink(loadedshaders::vpr);
  clear_and_shrink(loadedshaders::stCode);
  clear_and_shrink(loadedshaders::render_state);

  for (int i = 0; i < shClass.size(); i++)
    del_it(shClass[i]);
  clear_and_shrink(shClass);
}
