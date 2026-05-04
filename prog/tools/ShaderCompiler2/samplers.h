// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shlexterm.h"
#include "globVar.h"
#include "shaderSave.h"
#include "hash.h"
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
  NameId<VarMapAdapter> mNameId;
  d3d::SamplerInfo mSamplerInfo{};
  ShaderTerminal::arithmetic_expr *mBorderColor = nullptr;
  ShaderTerminal::arithmetic_expr *mAnisotropicMax = nullptr;
  ShaderTerminal::arithmetic_expr *mMipmapBias = nullptr;
  bindump::Ptr<CryptoHash> mSourceHash{};
  bool mIsStaticSampler = false;

  Sampler &operator=(const Sampler &) = default;
  Sampler(const Sampler &) = default;

  friend void add_dynamic_sampler_for_stcode(ShaderSemCode &, ShaderClass &, ShaderTerminal::sampler_decl &, Parser &, VarNameMap &);
};

struct ImmutableSamplerRef
{
  int samplerId = -1;
  int globalVarId = -1;

  friend inline bool operator==(ImmutableSamplerRef r1, ImmutableSamplerRef r2) = default;
  friend inline bool operator!=(ImmutableSamplerRef r1, ImmutableSamplerRef r2) = default;
};

class SamplerTable
{
  Tab<Sampler> mSamplers;
  Tab<ImmutableSamplerRef> mImmutableSamplers;
  ska::flat_hash_map<eastl::string, int> mSamplerIds;
  VarNameMap &varNameMap;
  ShaderGlobal::VarTable &globvars;
  StcodeShader &cppstcode;

public:
  SamplerTable(VarNameMap &var_map, ShaderGlobal::VarTable &gvars, StcodeShader &cppcode) :
    varNameMap{var_map}, globvars{gvars}, cppstcode{cppcode}
  {}

  void add(ShaderTerminal::sampler_decl &smp_decl, Parser &parser);
  void link(dag::ConstSpan<Sampler> new_samplers, dag::ConstSpan<ImmutableSamplerRef> new_immutable_samplers,
    dag::ConstSpan<int> gvar_link_table, Tab<int> &smp_link_table);
  Sampler *get(const char *name);

  // @TODO: support static (and immutable) samplers for materials
  void reportImmutableSampler(int id, int gvar_id);

  void emplaceSamplers(Tab<Sampler> &&samplers, Tab<ImmutableSamplerRef> &&immutable_samplers)
  {
    G_ASSERT(mSamplers.empty() && mImmutableSamplers.empty() && mSamplerIds.empty());
    mSamplers = eastl::move(samplers);
    mImmutableSamplers = eastl::move(immutable_samplers);
  }
  eastl::pair<Tab<Sampler>, Tab<ImmutableSamplerRef>> releaseSamplers();

  dag::ConstSpan<Sampler> getSamplers() const { return mSamplers; }
  dag::ConstSpan<ImmutableSamplerRef> getImmutableSamplers() const { return mImmutableSamplers; }

  void clear();
};

void add_dynamic_sampler_for_stcode(ShaderSemCode &sh_sem_code, ShaderClass &sclass, ShaderTerminal::sampler_decl &smp_decl,
  Parser &parser, VarNameMap &var_name_map);
