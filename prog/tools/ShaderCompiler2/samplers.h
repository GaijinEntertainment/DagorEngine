#pragma once

#include "shsyn.h"
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
  int mId = 0;
  d3d::SamplerInfo mSamplerInfo;
  ShaderTerminal::arithmetic_expr *mBorderColor = nullptr;
  ShaderTerminal::arithmetic_expr *mAnisotropicMax = nullptr;
  ShaderTerminal::arithmetic_expr *mMipmapBias = nullptr;
  bool mIsStaticSampler = false;

  Sampler &operator=(const Sampler &) = default;
  Sampler(const Sampler &) = default;

  static void add(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser);
  static const Sampler *get(const char *name);
};
