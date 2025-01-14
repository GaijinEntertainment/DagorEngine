// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "samplers.h"
#include "globVarSem.h"
#include "globVar.h"
#include "varMap.h"
#include "shLog.h"
#include "cppStcode.h"
#include <shaders/dag_shaderVarType.h>
#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

SamplerTable g_sampler_table{};

Sampler::Sampler(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser) : mSamplerDecl(&smp_decl)
{
  if (mSamplerDecl->state_name.empty())
    return;

  mIsStaticSampler = true;

  ska::flat_hash_map<eastl::string, ska::flat_hash_set<eastl::string>> state_name_to_values = {
    {"mipmap_mode",
      {
        "point",
        "linear",
      }},
    {"filter",
      {
        "point",
        "linear",
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
    eastl::string state_name = mSamplerDecl->state_name[i]->text;
    auto found_state = state_name_to_values.find(state_name);
    if (found_state == state_name_to_values.end())
    {
      eastl::string message(eastl::string::CtorSprintf{},
        "Incorrect state name '%s' for sampler '%s'\nPossible state names:", state_name.c_str(), mSamplerDecl->name->text);
      for (auto &[sn, _] : state_name_to_values)
        message.append_sprintf(" %s, ", sn.c_str());
      ShaderParser::error(message.c_str(), mSamplerDecl->state_name[i], parser);
      return;
    }
    if (found_state->second.begin()->empty() != (mSamplerDecl->expr[i] != nullptr))
    {
      ShaderParser::error(String(0, "Incorrect state value '%s' for sampler '%s'", state_name.c_str(), mSamplerDecl->name->text),
        mSamplerDecl->state_name[i], parser);
      return;
    }

    eastl::string state_value = mSamplerDecl->state[i] ? mSamplerDecl->state[i]->text : "";
    if (mSamplerDecl->state[i])
    {
      auto found_value = found_state->second.find(state_value);
      if (found_value == found_state->second.end())
      {
        eastl::string message(eastl::string::CtorSprintf{},
          "Incorrect state value '%s' for sampler state '%s::%s'\nPossible state values:", state_value.c_str(),
          mSamplerDecl->name->text, state_name.c_str());
        for (auto &sn : found_state->second)
          message.append_sprintf(" %s, ", sn.c_str());
        ShaderParser::error(message.c_str(), mSamplerDecl->state[i], parser);
        return;
      }
    }

    if (state_name == "comparison_func")
      mSamplerInfo.filter_mode = d3d::FilterMode::Compare;
    else if (state_name == "mipmap_mode")
      mSamplerInfo.mip_map_mode = state_value == "point" ? d3d::MipMapMode::Point : d3d::MipMapMode::Linear;
    else if (state_name == "filter")
      mSamplerInfo.filter_mode = state_value == "point" ? d3d::FilterMode::Point : d3d::FilterMode::Linear;
    else if (state_name == "address_u" || state_name == "address_v" || state_name == "address_w")
    {
      ska::flat_hash_map<eastl::string, d3d::AddressMode> value_to_mode = {
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
}

void SamplerTable::add(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser)
{
  int sampler_id = mSamplers.size();
  if (mSamplerIds.count(smp_decl.name->text) > 1)
  {
    if (mSamplers[mSamplerIds[smp_decl.name->text]].mIsStaticSampler || mSamplers[sampler_id].mIsStaticSampler)
    {
      eastl::string message(eastl::string::CtorSprintf{}, "Redefinition of static sampler variable '%s'. It's not supported yet.",
        smp_decl.name->text);
      ShaderParser::error(message.c_str(), smp_decl.name, parser);
    }
    return;
  }

  mSamplerIds.emplace(smp_decl.name->text, sampler_id);
  mSamplers.emplace_back(Sampler(smp_decl, parser));

  int id = VarMap::addVarId(smp_decl.name->text);
  int v = ShaderGlobal::get_var_internal_index(id);
  Tab<ShaderGlobal::Var> &variable_list = ShaderGlobal::getMutableVariableList();
  if (v >= 0)
  {
    if (variable_list[v].value.samplerInfo != mSamplers.back().mSamplerInfo)
    {
      eastl::string message(eastl::string::CtorSprintf{}, "Shader variable '%s' already declared with different properties in ",
        smp_decl.name->text);
      message += parser.get_lex_parser().get_symbol_location(id, SymbolType::GLOBAL_VARIABLE);
      ShaderParser::error(message.c_str(), smp_decl.name, parser);
    }
    return;
  }
  mSamplers.back().mId = sampler_id;
  mSamplers.back().mNameId = id;

  // @TODO: when the sampler decl is fake, this is a useless and expired pointer registration
  parser.get_lex_parser().register_symbol(id, SymbolType::GLOBAL_VARIABLE, smp_decl.name);

  v = append_items(variable_list, 1);
  variable_list[v].type = SHVT_SAMPLER;
  variable_list[v].nameId = id;
  variable_list[v].value.samplerInfo = mSamplers.back().mSamplerInfo;
  variable_list[v].isAlwaysReferenced = smp_decl.is_always_referenced ? true : false;

  g_cppstcode.globVars.setVar(SHVT_SAMPLER, smp_decl.name->text);
}

void SamplerTable::link(const Tab<Sampler> &new_samplers, Tab<int> &smp_link_table)
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

        if (memcmp(&smp.mSamplerInfo, &existing_smp.mSamplerInfo, sizeof(d3d::SamplerInfo)) != 0)
          sh_debug(SHLOG_FATAL, "Different sampler values: '%i'", smp.mNameId);
      }
    }

    if (!exists)
    {
      smp_link_table[i] = mSamplers.size();
      mSamplers.push_back(smp);
    }
  }
}

Sampler *SamplerTable::get(const char *name)
{
  auto found = mSamplerIds.find(name);
  return found == mSamplerIds.end() ? nullptr : &mSamplers[found->second];
}

Tab<Sampler> SamplerTable::releaseSamplers()
{
  auto released = eastl::move(mSamplers);
  return released;
}

void add_dynamic_sampler_for_stcode(ShaderSemCode &sh_sem_code, ShaderClass &sclass, ShaderTerminal::sampler_decl &smp_decl,
  ShaderTerminal::ShaderSyntaxParser &parser)
{
  int id = VarMap::addVarId(smp_decl.name->text);
  int v = sh_sem_code.find_var(id);
  if (v >= 0)
  {
    ShaderParser::error(
      eastl::string{eastl::string::CtorSprintf{}, "Shader variable '%s' already declared in ", smp_decl.name->text}.c_str(),
      smp_decl.name, parser);
    return;
  }

  Sampler sampler{smp_decl, parser};

  // @TODO: when the sampler decl is fake, this is a useless and expired pointer registration
  parser.get_lex_parser().register_symbol(id, SymbolType::STATIC_VARIABLE, smp_decl.name);

  v = append_items(sh_sem_code.vars, 1);
  sh_sem_code.vars[v].type = SHVT_SAMPLER;
  sh_sem_code.vars[v].nameId = id;
  sh_sem_code.vars[v].terminal = smp_decl.name;
  sh_sem_code.vars[v].dynamic = true;
  sh_sem_code.vars[v].noWarnings = false;
  sh_sem_code.vars[v].used = true;

  sh_sem_code.staticVarRegs.add(smp_decl.name->text, v);

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
      ShaderParser::error(String("static var '") + smp_decl.name->text + "' defined with different type/value", smp_decl.name, parser);
      return;
    }
  }

  {
    int i = append_items(sh_sem_code.stvarmap, 1);
    sh_sem_code.stvarmap[i].v = v;
    sh_sem_code.stvarmap[i].sv = sv;
  }
}