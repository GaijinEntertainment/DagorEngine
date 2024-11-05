// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderPropSeparators.h"
#include <Windows.h>
#include <ioSys/dag_DataBlock.h>
#include <dag/dag_vector.h>

#define SEPARATOR_PREFIX "//!"
constexpr int SEPARATOR_LENGTH = sizeof SEPARATOR_PREFIX - 1;

static dag::Vector<eastl::string> getSectionNames(const eastl::string &path)
{
  dag::Vector<eastl::string> result;

  constexpr int MAX_BUF_FOR_SEC_NAMES = 8192;
  TCHAR buff[MAX_BUF_FOR_SEC_NAMES];
  DWORD size = ::GetPrivateProfileSectionNames(buff, MAX_BUF_FOR_SEC_NAMES, path.c_str()); // size not including the terminating null
                                                                                           // character
  eastl::string section;
  for (int i = 0, start = 0; i <= size; i++)
    if (buff[i] == '\0' && i > start + 1)
    {
      result.emplace_back(buff + start, buff + i);
      start = i + 1;
    }
  return result;
}


static ShaderSeparatorToPropsType getSeparators(const eastl::string &path, const eastl::string &section)
{
  ShaderSeparatorToPropsType result;

  constexpr int MAX_CFG_DATA = 2048;
  TCHAR buff[MAX_CFG_DATA];
  DWORD size = ::GetPrivateProfileSection(section.c_str(), buff, MAX_CFG_DATA, path.c_str()); // size not including the terminating
                                                                                              // null character

  eastl::string separator;

  for (int i = 0, start = 0; i <= size; i++)
    if (buff[i] == '\0' && i > start + 1)
    {
      eastl::string data(buff + start, buff + i);
      eastl_size_t pos = data.find(SEPARATOR_PREFIX);
      if (pos != eastl::string::npos)
      {
        separator = eastl::string(eastl::begin(data) + pos + SEPARATOR_LENGTH, eastl::end(data));
        start = i + 1;
        continue;
      }
      pos = data.find('=');
      if (pos != eastl::string::npos)
        result[separator].insert(data.substr(0, pos));
      start = i + 1;
    }

  return result;
}

ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> gatherParameterSeparators(const DataBlock &appBlk, const char *appDir)
{
  ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> result;

  const DataBlock *avBlock = appBlk.getBlockByName("asset_viewer");
  if (!avBlock)
    return result;

  DataBlock::string_t relPath = avBlock->getStr("dagorShaderCfgLocation");
  if (!relPath)
    return result;

  eastl::string path(appDir);
  path += relPath;

  dag::Vector<eastl::string> sections = getSectionNames(path);
  for (const eastl::string &section : sections)
  {
    ShaderSeparatorToPropsType separators = getSeparators(path, section);
    if (!separators.empty())
      result.emplace(section, eastl::move(separators));
  }
  return result;
}
