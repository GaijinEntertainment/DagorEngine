#pragma once

#include <generic/dag_tab.h>

struct ShHardwareOptions
{
  // default options. initialized with ShHardwareOptions() consturctor
  static ShHardwareOptions Defaults;

  d3d::shadermodel::Version fshVersion; // max supported fsh version (hardware.fsh_*_*)
  bool enableHalfProfile = true;

  // set options to their default values
  inline ShHardwareOptions(d3d::shadermodel::Version _fsh) : fshVersion(_fsh) {}

  //////// this functions implemented in Shaders.cpp

  // generate filename for cache variant
  void appendOpts(String &fname) const;

  // dump info about options to debug & shaderlog
  void dumpInfo() const;
};


struct ShVariantName
{
  Tab<String> sourceFilesList;
  String intermediateDir;
  ShHardwareOptions opt;
  String dest;


  // init filename from source shader file name & options
  inline ShVariantName(const char *dest_base_filename, const ShHardwareOptions &_opt = ShHardwareOptions::Defaults) :
    sourceFilesList(midmem), opt(4.0_sm)
  {
    init(dest_base_filename, _opt);
  }

  void init(const char *dest_base_filename, const ShHardwareOptions &opt = ShHardwareOptions::Defaults);
};
