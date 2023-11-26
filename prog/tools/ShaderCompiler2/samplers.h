#pragma once

#include "shsyn.h"
#include <generic/dag_tab.h>
#include <3d/dag_sampler.h>

namespace ShaderTerminal
{
struct arithmetic_expr;
} // namespace ShaderTerminal

class Sampler
{
  ShaderTerminal::sampler_decl *mSamplerDecl = nullptr;

  Sampler(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser);

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

  static void add(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser);
  static void link(const Tab<Sampler> &samplers, Tab<int> &smp_link_table);
  static const Sampler *get(const char *name);
};

extern Tab<Sampler> g_samplers;
