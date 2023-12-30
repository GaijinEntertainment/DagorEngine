#include "samplers.h"
#include "globVarSem.h"
#include "globVar.h"
#include "varMap.h"
#include <shaders/dag_shaderVarType.h>
#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

Tab<Sampler> g_samplers;
ska::flat_hash_map<eastl::string, int> g_samplers_id;

void Sampler::add(ShaderTerminal::sampler_decl &smp_decl, ShaderTerminal::ShaderSyntaxParser &parser)
{
  int sampler_id = g_samplers.size();
  auto [it, inserted] = g_samplers_id.emplace(smp_decl.name->text, sampler_id);
  if (!inserted)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "Redefinition of sampler variable '%s'", smp_decl.name->text);
    ShaderParser::error(message.c_str(), smp_decl.name, parser);
    return;
  }

  g_samplers.emplace_back(Sampler(smp_decl, parser));

  int id = VarMap::addVarId(smp_decl.name->text);
  int v = ShaderGlobal::get_var_internal_index(id);
  if (v >= 0)
  {
    eastl::string message(eastl::string::CtorSprintf{}, "Shader variable '%s' already declared in ", smp_decl.name->text);
    message += parser.get_lex_parser().get_symbol_location(id, SymbolType::GLOBAL_VARIABLE);
    ShaderParser::error(message.c_str(), smp_decl.name, parser);
    return;
  }
  g_samplers.back().mId = sampler_id;
  g_samplers.back().mNameId = id;
  parser.get_lex_parser().register_symbol(id, SymbolType::GLOBAL_VARIABLE, smp_decl.name);

  Tab<ShaderGlobal::Var> &variable_list = ShaderGlobal::getMutableVariableList();
  v = append_items(variable_list, 1);
  variable_list[v].type = SHVT_SAMPLER;
  variable_list[v].nameId = id;
}

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

void Sampler::link(const Tab<Sampler> &samplers, Tab<int> &smp_link_table)
{
  int smp_num = samplers.size();
  smp_link_table.resize(smp_num);
  for (unsigned int i = 0; i < smp_num; i++)
  {
    auto &smp = samplers[i];

    bool exists = false;
    for (unsigned int existing_var = 0; existing_var < g_samplers.size(); existing_var++)
    {
      if (smp.mNameId == g_samplers[existing_var].mNameId)
      {
        auto &existing_smp = g_samplers[existing_var];
        exists = true;
        smp_link_table[i] = existing_var;

        if (memcmp(&smp.mSamplerInfo, &existing_smp.mSamplerInfo, sizeof(d3d::SamplerInfo)) != 0)
          DAG_FATAL("Different sampler values: '%i'", smp.mNameId);
      }
    }

    if (!exists)
    {
      smp_link_table[i] = g_samplers.size();
      g_samplers.push_back(smp);
    }
  }
}

const Sampler *Sampler::get(const char *name)
{
  auto found = g_samplers_id.find(name);
  return found == g_samplers_id.end() ? nullptr : &g_samplers[found->second];
}
