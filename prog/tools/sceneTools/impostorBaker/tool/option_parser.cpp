// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "option_parser.h"

#include <regex>
#include <string>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

static eastl::vector<eastl::string> parseVector(const eastl::string &str)
{
  eastl::vector<eastl::string> ret;
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
    ret.push_back(str.substr(from));
  return ret;
}

void print_impostor_options()
{
  printf("Usage: %s [options...] <path_to_app_blk>", dgs_argv[0]);
  printf("\nOptions:");
  printf("\n  Argument format: -<option_name>:<value>");
  printf("\n  Flag format:      -<flag_name>");
  printf("\n  -debug:    classic|default");
  printf("\n  -rootdir:  <path_to_develop_folder>");
  printf("\n  -dump_changed:  <path_to_output_file>");
  printf("\n              if specified, baker tool will create a txt/blk file that contains a list "
         "of changed/added and removed files");
  printf("\n  -assets:   <asset_name1;asset_name2...>");
  printf("\n              if specified, only these assets will be proccessed");
  printf("\n  -packs:    <pack_name1;pack_name2...>");
  printf("\n              if specified, only assets from these packs will be processed");
  printf("\n  -clean:     <yes|no> ");
  printf("\n              Default is no. If set to yes, all unused impostor files are removed");
  printf("\n  -skipGen:   <yes|no> ");
  printf("\n              Default is no. If set to yes, update *.tex.blk and skip generating impostors");
  printf("\n  -folderblk: <disabled|dont_replace|replace> ");
  printf("\n              Default is dont_replace. Specifies when the .folder.blk of the impostors "
         "should be created/replaced");
  printf("\n  -quiet      do not show message boxes on errors");
  printf("\n  -dry    Run baker but only prints list of what assets and why need baking, ");
  printf("\n  -force_rebake  Rebake assets even if they weren't change");
}

ImpostorOptions parse_impostor_options()
{
  std::regex optionReg("\\s*-(\\w+):(\\S*)\\s*");
  std::regex flagReg("\\s*-(\\w+)\\s*");
  std::smatch m;
  ImpostorOptions ret;
  unsigned int argInd = 0;
  for (int i = 1; i < dgs_argc; ++i)
  {
    std::string str{dgs_argv[i]};
    if (std::regex_match(str, m, optionReg))
    {
      const eastl::string key = m[1].str().c_str();
      const eastl::string value = m[2].str().c_str();
      if (key == "debug")
      {
        if (value == "classic")
          ret.classic_dbg = true;
        else if (value != "default")
        {
          DEBUG_CTX("No such debug option: %s", value.c_str());
          ret.valid = false;
        }
      }
      else if (key == "assets")
      {
        ret.assetsToBuild = parseVector(value);
        if (ret.assetsToBuild.size() == 0)
        {
          DEBUG_CTX("No assets given in %s", key.c_str());
          ret.valid = false;
        }
      }
      else if (key == "packs")
      {
        ret.packsToBuild = parseVector(value);
        if (ret.packsToBuild.size() == 0)
        {
          DEBUG_CTX("No packs given in %s", key.c_str());
          ret.valid = false;
        }
      }
      else if (key == "rootdir")
      {
        // nothing to do
      }
      else if (key == "clean")
      {
        if (value == "yes")
          ret.clean = true;
        else if (value == "no")
          ret.clean = false;
        else
        {
          DEBUG_CTX("Invalid option for '': '%s'", key.c_str(), value.c_str());
          ret.valid = false;
        }
      }
      else if (key == "skipGen")
      {
        if (value == "yes")
          ret.skipGen = true;
        else if (value == "no")
          ret.skipGen = false;
        else
        {
          DEBUG_CTX("Invalid option for '': '%s'", key.c_str(), value.c_str());
          ret.valid = false;
        }
      }
      else if (key == "dump_changed")
      {
        ret.changedFileOutput = value;
      }
      else if (key == "folderblk")
      {
        if (value == "disabled")
          ret.folderBlkGenMode = ImpostorOptions::FolderBlkGenMode::DISABLED;
        else if (value == "dont_replace")
          ret.folderBlkGenMode = ImpostorOptions::FolderBlkGenMode::DONT_REPLACE;
        else if (value == "replace")
          ret.folderBlkGenMode = ImpostorOptions::FolderBlkGenMode::REPLACE;
        else
        {
          DEBUG_CTX("Invalid option for '': '%s'", key.c_str(), value.c_str());
          ret.valid = false;
        }
      }
      else
      {
        DEBUG_CTX("Unhandled option: %s", key.c_str());
      }
    }
    else if (std::regex_match(str, m, flagReg))
    {
      const eastl::string flag = m[1].str().c_str();
      if (flag == "quiet")
        dgs_execute_quiet = true;
      else if (flag == "dry")
        ret.dryMode = true;
      else if (flag == "force_rebake")
        ret.forceRebake = true;
      else
      {
        DEBUG_CTX("Unknown flag: -%s", flag.c_str());
      }
    }
    else
    {
      switch (argInd++)
      {
        case 0: ret.appBlk = str.c_str(); break;
        default: DEBUG_CTX("Too many non-option arguments: %s", str.c_str()); ret.valid = false;
      }
    }
  }
  if (argInd != 1)
    ret.valid = false;
  return ret;
}
