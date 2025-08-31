// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shlexterm.h"
#include "globVar.h"
#include <generic/dag_tab.h>
#include <drv/3d/dag_sampler.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class ShaderSemCode;
class ShaderClass;
class StcodeShader;

namespace ShaderTerminal
{
struct arithmetic_expr;
} // namespace ShaderTerminal

class Sampler
{
  ShaderTerminal::sampler_decl *mSamplerDecl = nullptr;

  Sampler(ShaderTerminal::sampler_decl &smp_decl, Parser &parser);
  friend class SamplerTable;

public:
  Sampler() = default;
  int mId = 0;
  int mNameId = 0;
  d3d::SamplerInfo mSamplerInfo;
  ShaderTerminal::arithmetic_expr *mBorderColor = nullptr;
  ShaderTerminal::arithmetic_expr *mAnisotropicMax = nullptr;
  ShaderTerminal::arithmetic_expr *mMipmapBias = nullptr;
  bool mIsStaticSampler = false;

  Sampler &operator=(const Sampler &) = default;
  Sampler(const Sampler &) = default;

  friend void add_dynamic_sampler_for_stcode(ShaderSemCode &, ShaderClass &, ShaderTerminal::sampler_decl &, Parser &, VarNameMap &);
};

class SamplerTable
{
  Tab<Sampler> mSamplers;
  ska::flat_hash_map<eastl::string, int> mSamplerIds;
  VarNameMap &varNameMap;
  ShaderGlobal::VarTable &globvars;
  StcodeShader &cppstcode;

public:
  SamplerTable(VarNameMap &var_map, ShaderGlobal::VarTable &gvars, StcodeShader &cppcode) :
    varNameMap{var_map}, globvars{gvars}, cppstcode{cppcode}
  {}

  void add(ShaderTerminal::sampler_decl &smp_decl, Parser &parser);
  void link(const Tab<Sampler> &new_samplers, Tab<int> &smp_link_table);
  Sampler *get(const char *name);

  void emplaceSamplers(Tab<Sampler> &&samplers)
  {
    G_ASSERT(mSamplers.empty() && mSamplerIds.empty());
    mSamplers = eastl::move(samplers);
  }
  Tab<Sampler> releaseSamplers();

  void clear();
};

void add_dynamic_sampler_for_stcode(ShaderSemCode &sh_sem_code, ShaderClass &sclass, ShaderTerminal::sampler_decl &smp_decl,
  Parser &parser, VarNameMap &var_name_map);
