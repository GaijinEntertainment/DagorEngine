// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "samplers.h"
#include "shSemCode.h"
#include "globVarSem.h"
#include "globVar.h"
#include "varMap.h"
#include "shLog.h"
#include "shErrorReporting.h"
#include <shaders/dag_shaderVarType.h>
#include <EASTL/string.h>
#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>

// @TODO: find a more appropriate place for this function
// @TODO: reuse it for ODR validation of static constants in the static cbuf
static void hash_arithmetic_expr_ast_node(auto &hasher, ShaderTerminal::arithmetic_expr const *expr)
{
  if (!expr)
  {
    hasher.update(-1);
    return;
  }

  auto hashTerm = [&](Terminal const *t) {
    if (!t)
    {
      hasher.update(-1);
      return;
    }
    hasher.update(t->num);
    hasher.update(t->text);
  };
  auto hashOperand = [&](ShaderTerminal::arithmetic_operand const *operand) {
    if (!operand)
    {
      hasher.update(-1);
      return;
    }
    hashTerm(operand->unary_op ? operand->unary_op->op : nullptr);
    hashTerm(operand->var_name);
    if (operand->real_value)
    {
      hashTerm(operand->real_value->sign);
      hashTerm(operand->real_value->value);
    }
    else
    {
      hashTerm(nullptr);
    }
    if (operand->color_value)
    {
      hashTerm(operand->color_value->decl);
      hash_arithmetic_expr_ast_node(hasher, operand->color_value->expr0);
      hash_arithmetic_expr_ast_node(hasher, operand->color_value->expr1);
      hash_arithmetic_expr_ast_node(hasher, operand->color_value->expr2);
      hash_arithmetic_expr_ast_node(hasher, operand->color_value->expr3);
    }
    else
    {
      hashTerm(nullptr);
    }
    if (operand->func)
    {
      hashTerm(operand->func->func_name);
      for (auto const *e : operand->func->param)
        hash_arithmetic_expr_ast_node(hasher, e);
    }
    else
    {
      hashTerm(nullptr);
    }
    hash_arithmetic_expr_ast_node(hasher, operand->expr);
    if (operand->cmask)
      hashTerm(operand->cmask->channel);
    else
      hashTerm(nullptr);
  };
  auto hashExprMd = [&](ShaderTerminal::arithmetic_expr_md const *md) {
    if (!md)
    {
      hasher.update(-1);
      return;
    }
    hashOperand(md->lhs);
    for (auto const *op : md->op)
      hashTerm(op);
    for (auto const *r : md->rhs)
      hashOperand(r);
  };

  hashExprMd(expr->lhs);
  for (auto const *op : expr->op)
    hashTerm(op);
  for (auto const *r : expr->rhs)
    hashExprMd(r);
}

Sampler::Sampler(ShaderTerminal::sampler_decl &smp_decl, Parser &parser) : mSamplerDecl(&smp_decl)
{
  if (mSamplerDecl->state_name.empty())
    return;

  mIsStaticSampler = true;

  const dag::VectorMap<eastl::string_view, dag::VectorSet<eastl::string_view>> state_name_to_values = {
    {"mipmap_mode",
      {
        "point",
        "linear",
        "disabled",
      }},
    {"filter",
      {
        "point",
        "linear",
        "disabled",
      }},
    {"address_u", {"wrap", "mirror", "clamp", "border", "mirror_once"}},
    {"address_v", {"wrap", "mirror", "clamp", "border", "mirror_once"}},
    {"address_w", {"wrap", "mirror", "clamp", "border", "mirror_once"}},
    {"border_color", {{}}},
    {"anisotropic_max", {{}}},
    {"mipmap_bias", {{}}},
    {"comparison_func", {"less"}},
  };

  for (int i = 0; i < mSamplerDecl->state_name.size(); i++)
  {
    eastl::string_view state_name = mSamplerDecl->state_name[i]->text;
    auto found_state = state_name_to_values.find(state_name);
    if (found_state == state_name_to_values.end())
    {
      eastl::string message(eastl::string::CtorSprintf{},
        "Incorrect state name '%.*s' for sampler '%s'\nPossible state names:", state_name.length(), state_name.data(),
        mSamplerDecl->name->text);
      for (auto &[sn, _] : state_name_to_values)
        message.append_sprintf(" %.*s, ", sn.length(), sn.data());
      report_error(parser, mSamplerDecl->state_name[i], message.c_str());
      return;
    }
    if (found_state->second.begin()->empty() != (mSamplerDecl->expr[i] != nullptr))
    {
      report_error(parser, mSamplerDecl->state_name[i], "Incorrect state value '%.*s' for sampler '%s'", state_name.length(),
        state_name.data(), mSamplerDecl->name->text);
      return;
    }

    eastl::string_view state_value = mSamplerDecl->state[i] ? mSamplerDecl->state[i]->text : "";
    if (mSamplerDecl->state[i])
    {
      auto found_value = found_state->second.find(state_value);
      if (found_value == found_state->second.end())
      {
        eastl::string message(eastl::string::CtorSprintf{},
          "Incorrect state value '%.*s' for sampler state '%s::%.*s'\nPossible state values:", state_value.length(),
          state_value.data(), mSamplerDecl->name->text, state_name.length(), state_name.data());
        for (auto &sn : found_state->second)
          message.append_sprintf(" %.*s, ", sn.length(), sn.data());
        report_error(parser, mSamplerDecl->state[i], message.c_str());
        return;
      }
    }

    if (state_name == "comparison_func")
      mSamplerInfo.filter_mode = d3d::FilterMode::Compare;
    else if (state_name == "mipmap_mode")
    {
      const dag::VectorMap<eastl::string_view, d3d::MipMapMode> value_to_mode = {
        {"point", d3d::MipMapMode::Point},
        {"linear", d3d::MipMapMode::Linear},
        {"disabled", d3d::MipMapMode::Disabled},
      };
      mSamplerInfo.mip_map_mode = value_to_mode.find(state_value)->second;
    }
    else if (state_name == "filter")
    {
      const dag::VectorMap<eastl::string_view, d3d::FilterMode> value_to_mode = {
        {"point", d3d::FilterMode::Point},
        {"linear", d3d::FilterMode::Linear},
        {"disabled", d3d::FilterMode::Disabled},
      };
      mSamplerInfo.filter_mode = value_to_mode.find(state_value)->second;
    }
    else if (state_name == "address_u" || state_name == "address_v" || state_name == "address_w")
    {
      const dag::VectorMap<eastl::string_view, d3d::AddressMode> value_to_mode = {
        {"wrap", d3d::AddressMode::Wrap},
        {"mirror", d3d::AddressMode::Mirror},
        {"clamp", d3d::AddressMode::Clamp},
        {"border", d3d::AddressMode::Border},
        {"mirror_once", d3d::AddressMode::MirrorOnce},
      };
      d3d::AddressMode mode = value_to_mode.find(state_value)->second;

      if (state_name == "address_u")
        mSamplerInfo.address_mode_u = mode;
      else if (state_name == "address_v")
        mSamplerInfo.address_mode_v = mode;
      else
        mSamplerInfo.address_mode_w = mode;
    }
    else if (state_name == "border_color")
      mBorderColor = mSamplerDecl->expr[i];
    else if (state_name == "anisotropic_max")
      mAnisotropicMax = mSamplerDecl->expr[i];
    else if (state_name == "mipmap_bias")
      mMipmapBias = mSamplerDecl->expr[i];
    else
      G_ASSERT(false);
  }

  if (mIsStaticSampler)
  {
    auto hasher = CryptoHasher{};
    hash_arithmetic_expr_ast_node(hasher, mBorderColor);
    hash_arithmetic_expr_ast_node(hasher, mAnisotropicMax);
    hash_arithmetic_expr_ast_node(hasher, mMipmapBias);
    mSourceHash.create(hasher.hash());
  }
}

void SamplerTable::add(ShaderTerminal::sampler_decl &smp_decl, Parser &parser)
{
  Sampler smp{smp_decl, parser};

  if (mSamplerIds.count(smp_decl.name->text) >= 1)
  {
    auto &existingSampler = mSamplers[mSamplerIds[smp_decl.name->text]];
    if (existingSampler.mIsStaticSampler && smp.mIsStaticSampler)
    {
      G_ASSERT(smp.mSourceHash && existingSampler.mSourceHash);
      bool infoMismatch = memcmp(&smp.mSamplerInfo, &existingSampler.mSamplerInfo, sizeof(d3d::SamplerInfo)) != 0;
      bool declMismatch = memcmp(smp.mSourceHash.get(), existingSampler.mSourceHash.get(), sizeof(CryptoHash)) != 0;
      if (infoMismatch || declMismatch)
      {
        eastl::string message(eastl::string::CtorSprintf{}, "Redefinition of static sampler variable '%s' with different declaration.",
          smp_decl.name->text);
        report_error(parser, smp_decl.name, message.c_str());
      }
      return;
    }

    int id = varNameMap.addVarId(smp_decl.name->text);
    int v = globvars.getVarInternalIndex(id);
    G_ASSERT(v >= 0);
    auto &var = globvars.getMutableVariableList()[v];

    if (!existingSampler.mIsStaticSampler && smp.mIsStaticSampler)
    {
      existingSampler.mIsStaticSampler = true;
      existingSampler.mSamplerInfo = smp.mSamplerInfo;
      existingSampler.mSourceHash = smp.mSourceHash;
      existingSampler.mBorderColor = smp.mBorderColor;
      existingSampler.mAnisotropicMax = smp.mAnisotropicMax;
      existingSampler.mMipmapBias = smp.mMipmapBias;
      var.definesValue = true;
      var.isLiteral = true;
      var.value.samplerInfo = smp.mSamplerInfo;
    }

    var.isAlwaysReferenced |= smp_decl.is_always_referenced ? true : false;
    return;
  }

  int sampler_id = mSamplers.size();

  mSamplerIds.emplace(smp_decl.name->text, sampler_id);
  mSamplers.emplace_back(smp);

  int id = varNameMap.addVarId(smp_decl.name->text);
  G_ASSERT(globvars.getVarInternalIndex(id) < 0);

  mSamplers.back().mId = sampler_id;
  mSamplers.back().mNameId = id;

  // @TODO: when the sampler decl is fake, this is a useless and expired pointer registration
  parser.get_lexer().register_symbol(id, SymbolType::GLOBAL_VARIABLE, smp_decl.name);

  Tab<ShaderGlobal::Var> &variable_list = globvars.getMutableVariableList();
  int v = append_items(variable_list, 1);
  auto &var = variable_list[v];
  var.type = SHVT_SAMPLER;
  var.nameId = id;
  var.value.samplerInfo = mSamplers.back().mSamplerInfo;
  var.isAlwaysReferenced = smp_decl.is_always_referenced ? true : false;
  var.definesValue = smp.mIsStaticSampler;
  var.isLiteral = smp.mIsStaticSampler;

  globvars.updateVarNameIdMapping(v);
  cppstcode.globVars.setVar(SHVT_SAMPLER, smp_decl.name->text, true);
}

void SamplerTable::link(dag::ConstSpan<Sampler> new_samplers, dag::ConstSpan<ImmutableSamplerRef> new_immutable_samplers,
  dag::ConstSpan<int> gvar_link_table, Tab<int> &smp_link_table)
{
  int smp_num = new_samplers.size();
  smp_link_table.resize(smp_num);
  for (unsigned int i = 0; i < smp_num; i++)
  {
    auto &smp = new_samplers[i];

    bool exists = false;
    for (unsigned int existing_var = 0; existing_var < mSamplers.size(); existing_var++)
    {
      if (smp.mNameId == mSamplers[existing_var].mNameId)
      {
        auto &existing_smp = mSamplers[existing_var];
        exists = true;
        smp_link_table[i] = existing_var;

        bool infoMismatch = memcmp(&smp.mSamplerInfo, &existing_smp.mSamplerInfo, sizeof(d3d::SamplerInfo)) != 0;
        bool declMismatch = smp.mSourceHash && existing_smp.mSourceHash &&
                            memcmp(smp.mSourceHash.get(), existing_smp.mSourceHash.get(), sizeof(CryptoHash)) != 0;
        if (infoMismatch || declMismatch)
        {
          if (smp.mIsStaticSampler && existing_smp.mIsStaticSampler)
          {
            G_ASSERT(smp.mSourceHash && existing_smp.mSourceHash);
            sh_debug(SHLOG_FATAL, "Different static sampler %s%s%s: '%s'", infoMismatch ? "values" : "",
              infoMismatch && declMismatch ? " and " : "", declMismatch ? "declaration expressions" : "",
              varNameMap.getName(smp.mNameId));
          }
          else if (smp.mIsStaticSampler && !existing_smp.mIsStaticSampler)
          {
            G_ASSERT(smp.mSourceHash);
            G_ASSERT(!existing_smp.mSourceHash);
            existing_smp.mIsStaticSampler = true;
            existing_smp.mSamplerInfo = smp.mSamplerInfo;
            existing_smp.mSourceHash = smp.mSourceHash;
            // Not copying expressions, as they are already compiled down
          }
        }
      }
    }

    if (!exists)
    {
      smp_link_table[i] = mSamplers.size();
      mSamplers.push_back(smp);
    }
  }

  for (auto [smpId, gvarId] : new_immutable_samplers)
  {
    const ImmutableSamplerRef linkedRef{smp_link_table[smpId], gvar_link_table[gvarId]};
    if (eastl::find(eastl::begin(mImmutableSamplers), eastl::end(mImmutableSamplers), linkedRef) == eastl::end(mImmutableSamplers))
    {
      mImmutableSamplers.push_back(linkedRef);
    }
  }
}

Sampler *SamplerTable::get(const char *name)
{
  auto found = mSamplerIds.find(name);
  return found == mSamplerIds.end() ? nullptr : &mSamplers[found->second];
}

void SamplerTable::reportImmutableSampler(int id, int gvar_id) { mImmutableSamplers.emplace_back(id, gvar_id); }

eastl::pair<Tab<Sampler>, Tab<ImmutableSamplerRef>> SamplerTable::releaseSamplers()
{
  return {eastl::move(mSamplers), eastl::move(mImmutableSamplers)};
}

void SamplerTable::clear()
{
  clear_and_shrink(mSamplers);
  clear_and_shrink(mSamplerIds);
}

void add_dynamic_sampler_for_stcode(ShaderSemCode &sh_sem_code, ShaderClass &sclass, ShaderTerminal::sampler_decl &smp_decl,
  Parser &parser, VarNameMap &var_name_map)
{
  int id = var_name_map.addVarId(smp_decl.name->text);
  int v = sh_sem_code.find_var(id);
  if (v >= 0)
  {
    report_error(parser, smp_decl.name, "Shader variable '%s' already declared in ", smp_decl.name->text);
    return;
  }

  Sampler sampler{smp_decl, parser};

  // @TODO: when the sampler decl is fake, this is a useless and expired pointer registration
  parser.get_lexer().register_symbol(id, SymbolType::STATIC_VARIABLE, smp_decl.name);

  v = append_items(sh_sem_code.vars, 1);
  sh_sem_code.vars[v].type = SHVT_SAMPLER;
  sh_sem_code.vars[v].nameId = id;
  sh_sem_code.vars[v].terminal = smp_decl.name;
  sh_sem_code.vars[v].dynamic = true;
  sh_sem_code.vars[v].noWarnings = false;
  sh_sem_code.vars[v].used = true;

  sh_sem_code.staticStcodeVars.add(smp_decl.name->text, v);

  int sv = sclass.find_static_var(id);
  if (sv < 0)
  {
    sv = append_items(sclass.stvar, 1);
    sclass.stvar[sv].type = SHVT_SAMPLER;
    sclass.stvar[sv].nameId = id;
    sclass.stvar[sv].defval.samplerInfo = sampler.mSamplerInfo;
  }
  else
  {
    if (sclass.stvar[sv].type != SHVT_SAMPLER || sclass.stvar[sv].defval.samplerInfo != sampler.mSamplerInfo)
    {
      report_error(parser, smp_decl.name, "static var '%s' defined with different type/value", smp_decl.name->text);
      return;
    }
  }

  {
    int i = append_items(sh_sem_code.stvarmap, 1);
    sh_sem_code.stvarmap[i].v = v;
    sh_sem_code.stvarmap[i].sv = sv;
  }
}
