// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "commonUtils.h"
#include "cppStcode.h"
#include "blkHash.h"
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <shaders/dag_shaders.h>

struct ShHardwareOptions
{
  // default options. initialized with ShHardwareOptions() consturctor

  d3d::shadermodel::Version fshVersion; // max supported fsh version (hardware.fsh_*_*)
  bool enableHalfProfile = false;

  // set options to their default values
  constexpr ShHardwareOptions(d3d::shadermodel::Version _fsh) : fshVersion(_fsh) {}

  //////// this functions implemented in Shaders.cpp

  // generate filename for cache variant
  void appendOptsTo(String &fname) const;

  // dump info about options to debug & shaderlog
  void dumpInfo() const;
};

inline constexpr ShHardwareOptions DEFAULT_HW_OPTS{4.0_sm};

class ShCompilationInfo
{
public:
  ShCompilationInfo(const char *dest_base_filename, const char *intermediate_dir, Tab<String> &&source_list,
    const BlkHashBytes &blk_hash, StcodeCompilationDirs &&stcode_dirs, const ShHardwareOptions &a_opt = DEFAULT_HW_OPTS) :
    sourceFilesList(eastl::move(source_list)),
    intermediateDir(intermediate_dir),
    blkHash(blk_hash),
    stcodeInfo(eastl::move(stcode_dirs)),
    opt(a_opt),
    destFname(dest_base_filename)
  {
    opt.appendOptsTo(destFname);
    destFname = intermediateDir + "/" + destFname + ".lib.bin";
  }

  PINNED_TYPE(ShCompilationInfo)

  const Tab<String> &sources() const { return sourceFilesList; }
  const String &intermDir() const { return intermediateDir; }
  const BlkHashBytes &targetBlkHash() const { return blkHash; }
  ShHardwareOptions hwopts() const { return opt; }
  const String &dest() const { return destFname; }
  const StcodeCompilationDirs &stcodeDirs() const { return stcodeInfo; }

private:
  Tab<String> sourceFilesList;
  String intermediateDir;
  BlkHashBytes blkHash;
  StcodeCompilationDirs stcodeInfo;
  ShHardwareOptions opt;
  String destFname;
};
