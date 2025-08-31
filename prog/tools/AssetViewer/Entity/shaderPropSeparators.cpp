// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderPropSeparators.h"
#include <ioSys/dag_dataBlock.h>
#include <obsolete/dag_cfg.h>

#define SEPARATOR_PREFIX "//!"
constexpr int SEPARATOR_LENGTH = sizeof SEPARATOR_PREFIX - 1;

static const char *get_separator_for_line(const CfgDiv &section, int line)
{
  for (int i = section.comm.size() - 1; i >= 0; --i)
  {
    const CfgComm &comment = section.comm[i];
    if (line >= comment.ln && strncmp(comment.text, SEPARATOR_PREFIX, SEPARATOR_LENGTH) == 0)
      return comment.text + SEPARATOR_LENGTH;
  }

  return "";
}

static ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> parse_dagors_shader_cfg(const char *path)
{
  CfgReader cfgReader;
  if (cfgReader.readfile(path, /*clear = */ true, /*read_comments = */ true) == 0)
    return {};

  ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> parameterSeperators;
  ShaderSeparatorToPropsType separators;

  for (const CfgDiv &section : cfgReader.div)
  {
    separators.clear();
    for (const CfgVar &variable : section.var)
    {
      const char *separator = get_separator_for_line(section, variable.ln);
      separators[separator].insert(variable.id);
    }

    if (!separators.empty())
      parameterSeperators.insert({section.id, separators});
  }

  return parameterSeperators;
}

ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> gatherParameterSeparators(const DataBlock &appBlk, const char *appDir)
{
  const DataBlock *avBlock = appBlk.getBlockByName("asset_viewer");
  if (!avBlock)
    return {};

  DataBlock::string_t relPath = avBlock->getStr("dagorShaderCfgLocation");
  if (!relPath)
    return {};

  eastl::string path(appDir);
  path += relPath;
  return parse_dagors_shader_cfg(path.c_str());
}
