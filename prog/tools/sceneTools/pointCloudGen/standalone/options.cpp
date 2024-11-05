// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "options.h"
#include "generic/dag_tab.h"

#include <dag/dag_vector.h>
#include <regex>
#include <string>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

namespace plod
{


static dag::Vector<plod::String> parseVector(const plod::String &str)
{
  dag::Vector<plod::String> ret;
  unsigned int from = 0;
  for (unsigned int i = 0; i < str.size(); ++i)
  {
    if (str[i] == ';')
    {
      if (from < i)
        ret.push_back(str.substr(from, i - from));
      from = i + 1;
    }
  }
  if (from < str.size())
    ret.push_back(str.substr(0, from));
  return ret;
}

void printOptions()
{
  printf("Usage: %s [options...] <path_to_app_blk>", __argv[0]);
  printf("\nOptions:");
  printf("\n  Argument format: -<option_name>:<value>");
  printf("\n  Flag format:      -<flag_name>");
  printf("\n  -assets:   <asset_name1;asset_name2...>");
  printf("\n              if specified, only these assets will be proccessed");
  printf("\n  -packs:    <pack_name1;pack_name2...>");
  printf("\n              if specified, only assets from these packs will be processed");
  printf("\n  -quiet      do not show message boxes on errors");
  printf("\n  -dry        do not run generation, only printing processed assets");
}

AppOptions parseOptions()
{
  std::regex optionReg("\\s*-(\\w+):(\\S*)\\s*");
  std::regex flagReg("\\s*-(\\w+)\\s*");
  std::smatch m;
  AppOptions options;
  unsigned int argInd = 0;
  for (int i = 1; i < __argc; ++i)
  {
    std::string str{__argv[i]};
    if (std::regex_match(str, m, optionReg))
    {
      const plod::String key = m[1].str().c_str();
      const plod::String value = m[2].str().c_str();
      if (key == "assets")
      {
        options.assetsToBuild = parseVector(value);
        if (options.assetsToBuild.size() == 0)
        {
          DEBUG_CTX("No assets given in %s", key.c_str());
          options.valid = false;
        }
      }
      else if (key == "packs")
      {
        options.packsToBuild = parseVector(value);
        if (options.packsToBuild.size() == 0)
        {
          DEBUG_CTX("No packs given in %s", key.c_str());
          options.valid = false;
        }
      }
      else
      {
        DEBUG_CTX("Unhandled option: %s", key.c_str());
      }
    }
    else if (std::regex_match(str, m, flagReg))
    {
      const plod::String flag = m[1].str().c_str();
      if (flag == "quiet")
        dgs_execute_quiet = true;
      else if (flag == "dry")
        options.dryMode = true;
      else
      {
        DEBUG_CTX("Unknown flag: -%s", flag.c_str());
      }
    }
    else
    {
      switch (argInd++)
      {
        case 0: options.appBlk = str.c_str(); break;
        default: DEBUG_CTX("Too many non-option arguments: %s", str.c_str()); options.valid = false;
      }
    }
  }
  if (argInd != 1)
    options.valid = false;
  return options;
}


} // namespace plod
