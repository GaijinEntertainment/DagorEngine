// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shSemCode.h"
#include <generic/dag_tab.h>
#include <drv/3d/dag_sampler.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace ShaderTerminal
{
struct arithmetic_expr;
} // namespace ShaderTerminal

class Sampler
{
  ShaderTerminal::sampler_decl *mSamplerDecl = nullptr;

  Sampler(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser);
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

  friend void add_dynamic_sampler_for_stcode(ShaderSemCode &, ShaderClass &, ShaderTerminal::sampler_decl &,
    ShaderTerminal::ShaderSyntaxParser &);
};

class SamplerTable
{
  Tab<Sampler> mSamplers;
  ska::flat_hash_map<eastl::string, int> mSamplerIds;

public:
  SamplerTable() = default;
  SamplerTable(Tab<Sampler> &&a_samplers) : mSamplers(eastl::move(a_samplers)) {}

  void add(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser);
  void link(const Tab<Sampler> &new_samplers, Tab<int> &smp_link_table);
  Sampler *get(const char *name);
  Tab<Sampler> releaseSamplers();
};

extern SamplerTable g_sampler_table;

void add_dynamic_sampler_for_stcode(ShaderSemCode &sh_sem_code, ShaderClass &sclass, ShaderTerminal::sampler_decl &smp_decl,
  ShaderTerminal::ShaderSyntaxParser &parser);
