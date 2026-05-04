// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderPropSeparators.h"
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point4.h>

ShaderVariableLimits *ShaderDescriptor::findVariableDescriptor(const char *variable_name)
{
  for (auto groupIt = shaderSeparatorToProps.begin(); groupIt != shaderSeparatorToProps.end(); ++groupIt)
    for (auto varIt = groupIt->second.shaderVariables.begin(); varIt != groupIt->second.shaderVariables.end(); ++varIt)
    {
      const char *currentVariableName = varIt->first.c_str();
      if (strcmp(currentVariableName, variable_name) == 0)
        return &varIt->second;
    }

  return nullptr;
}

static void parse_dagor_shader_blk_parameter(const DataBlock &parameter_blk, ShaderVariableLimits &limits)
{
  const char *type = parameter_blk.getStr("type", "");
  if ((type[0] == 'r' && type[1] == 0) || (type[0] == 'p' && type[1] == '4' && type[2] == 0))
  {
    limits.minValue = parameter_blk.getReal("soft_min", limits.minValue);
    limits.maxValue = parameter_blk.getReal("soft_max", limits.maxValue);
  }
  else if (type[0] == 'i' && type[1] == 0)
  {
    limits.minValue = parameter_blk.getInt("soft_min", limits.minValue);
    limits.maxValue = parameter_blk.getInt("soft_max", limits.maxValue);
  }

  const char *description = parameter_blk.getStr("description", nullptr);
  if (description)
    limits.description = description;

  const char *customUi = parameter_blk.getStr("custom_ui", nullptr);
  limits.editAsColor = customUi && strcmp(customUi, "color") == 0;

  const int defaultParamIndex = parameter_blk.findParam("default");
  if (defaultParamIndex >= 0)
  {
    const int defaultParamType = parameter_blk.getParamType(defaultParamIndex);

    if (type[0] == 'b' && type[1] == 0 && defaultParamType == DataBlock::TYPE_BOOL)
    {
      limits.defaultValueType = MAT_VAR_TYPE_BOOL;
      limits.defaultValue.i = parameter_blk.getBool(defaultParamIndex) ? 1 : 0;
    }
    else if (type[0] == 'i' && type[1] == 0 && defaultParamType == DataBlock::TYPE_INT)
    {
      limits.defaultValueType = MAT_VAR_TYPE_INT;
      limits.defaultValue.i = parameter_blk.getInt(defaultParamIndex);
    }
    else if (type[0] == 'r' && type[1] == 0 && defaultParamType == DataBlock::TYPE_REAL)
    {
      limits.defaultValueType = MAT_VAR_TYPE_REAL;
      limits.defaultValue.r = parameter_blk.getReal(defaultParamIndex);
    }
    else if (type[0] == 'p' && type[1] == '4' && type[2] == 0 && defaultParamType == DataBlock::TYPE_POINT4)
    {
      limits.defaultValueType = MAT_VAR_TYPE_COLOR4;
      const Point4 val = parameter_blk.getPoint4(defaultParamIndex);
      limits.defaultValue.c4() = Color4(val.x, val.y, val.z, val.w);
    }
  }
}

static void parse_dagor_shader_blk_parameter_group(const DataBlock &group_blk, ShaderSeparatorToPropsType &separators)
{
  const char *name = group_blk.getStr("name");

  for (int i = 0; i < group_blk.blockCount(); ++i)
  {
    const DataBlock *parameterBlk = group_blk.getBlock(i);
    if (strcmp(parameterBlk->getBlockName(), "parameter") != 0)
      continue;

    const char *parameterName = parameterBlk->getStr("name");
    ShaderVariableLimits limits;
    parse_dagor_shader_blk_parameter(*parameterBlk, limits);

    auto result = separators.emplace(name, ShaderDescriptorVariableGroup());
    ShaderDescriptorVariableGroup &group = result.first->second;
    group.shaderVariables.insert({parameterName, limits});

    if (result.second) // newly inserted
      if (const char *description = group_blk.getStr("description", nullptr))
        group.description = description;
  }
}

static void parse_dagor_shader_blk_shader(const DataBlock &shader_blk,
  ska::flat_hash_map<eastl::string, ShaderDescriptor> &parameter_seperators)
{
  ShaderDescriptor descriptor;

  for (int i = 0; i < shader_blk.blockCount(); ++i)
  {
    const DataBlock *parameterBlk = shader_blk.getBlock(i);
    if (strcmp(parameterBlk->getBlockName(), "parameter") == 0)
    {
      const char *parameterName = parameterBlk->getStr("name");
      ShaderVariableLimits limits;
      parse_dagor_shader_blk_parameter(*parameterBlk, limits);

      ShaderDescriptorVariableGroup &group = descriptor.shaderSeparatorToProps[""];
      group.shaderVariables.insert({parameterName, limits});
    }
    else if (strcmp(parameterBlk->getBlockName(), "parameters_group") == 0)
    {
      parse_dagor_shader_blk_parameter_group(*parameterBlk, descriptor.shaderSeparatorToProps);
    }
  }

  if (!descriptor.shaderSeparatorToProps.empty())
  {
    if (const char *description = shader_blk.getStr("description", nullptr))
      descriptor.description = description;

    const char *name = shader_blk.getStr("name");
    parameter_seperators.insert({name, descriptor});
  }
}

static void parse_dagor_shader_blk_category(const DataBlock &category_blk,
  ska::flat_hash_map<eastl::string, ShaderDescriptor> &parameter_seperators)
{
  for (int i = 0; i < category_blk.blockCount(); ++i)
  {
    const DataBlock *shaderBlk = category_blk.getBlock(i);
    if (strcmp(shaderBlk->getBlockName(), "shader") == 0)
      parse_dagor_shader_blk_shader(*shaderBlk, parameter_seperators);
  }
}

static ska::flat_hash_map<eastl::string, ShaderDescriptor> parse_dagor_shader_blk(const char *path)
{
  ska::flat_hash_map<eastl::string, ShaderDescriptor> parameterSeperators;
  DataBlock blk(path);

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *shaderCategoryBlk = blk.getBlock(i);
    if (strcmp(shaderCategoryBlk->getBlockName(), "shader_category") == 0)
      parse_dagor_shader_blk_category(*shaderCategoryBlk, parameterSeperators);
  }

  return parameterSeperators;
}

static void log_default_value_difference(const char *shader_name, const ShaderVariableLimits &limits, const MatVarDesc &var)
{
  if (limits.defaultValueType == MAT_VAR_TYPE_NONE)
    return;

  if (limits.defaultValueType == var.type)
  {
    if (var.value != limits.defaultValue)
    {
      switch (var.type)
      {
        case MAT_VAR_TYPE_BOOL:
        case MAT_VAR_TYPE_INT:
          logwarn("Material Editor: shader variable %s in shader %s has different default value in the shader (%d) than in "
                  "dagorShaders.blk (%d)",
            var.name, shader_name, var.value.i, limits.defaultValue.i);
          break;

        case MAT_VAR_TYPE_REAL:
          logwarn("Material Editor: shader variable %s in shader %s has different default value in the shader (%g) than in "
                  "dagorShaders.blk (%g)",
            var.name, shader_name, var.value.r, limits.defaultValue.r);
          break;

        case MAT_VAR_TYPE_COLOR4:
          logwarn("Material Editor: shader variable %s in shader %s has different default value in the shader (%g, %g, %g, %g) than "
                  "in dagorShaders.blk (%g, "
                  "%g, %g, %g)",
            var.name, shader_name, var.value.c[0], var.value.c[1], var.value.c[2], var.value.c[3], limits.defaultValue.c[0],
            limits.defaultValue.c[1], limits.defaultValue.c[2], limits.defaultValue.c[3]);
          break;
      }
    }
  }
  else
  {
    G_STATIC_ASSERT(MAT_VAR_TYPE_COUNT == 5);
    const char *typeNames[MAT_VAR_TYPE_COUNT] = {"int", "real", "color4", "bool", "lighting"};
    logwarn("Material Editor: shader variable %s in shader %s has different default value type in the shader (%s) than in "
            "dagorShaders.blk (%s)",
      var.name, shader_name, typeNames[var.type], typeNames[limits.defaultValueType]);
  }
}

void set_defaults_from_shader_bin_dump(const char *shader_name, ShaderDescriptor &descriptor)
{
  const dag::Vector<MatVarDesc> varDefaults = get_shclass_vars_default(shader_name);
  for (const MatVarDesc &varDefault : varDefaults)
  {
    ShaderVariableLimits *limits = descriptor.findVariableDescriptor(varDefault.name);
    if (limits)
      log_default_value_difference(shader_name, *limits, varDefault);
    else
      limits = &descriptor.shaderSeparatorToProps[""].shaderVariables[eastl::string(varDefault.name)];

    switch (varDefault.type)
    {
      case MAT_VAR_TYPE_BOOL:
      case MAT_VAR_TYPE_ENUM_LIGHTING:
      case MAT_VAR_TYPE_INT:
      case MAT_VAR_TYPE_REAL:
      case MAT_VAR_TYPE_COLOR4:
        limits->defaultValueType = varDefault.type;
        limits->defaultValue = ShaderMatData::VarValue();
        limits->defaultValue = varDefault.value;
        break;
    }
  }
}

ska::flat_hash_map<eastl::string, ShaderDescriptor> gatherParameterSeparators(const DataBlock &appBlk, const char *appDir)
{
  const DataBlock *avBlock = appBlk.getBlockByName("asset_viewer");
  if (!avBlock)
    return {};

  DataBlock::string_t relPath = avBlock->getStr("dagorShaderBlkLocation", nullptr);
  if (!relPath)
    return {};

  eastl::string path(appDir);
  path += relPath;
  return parse_dagor_shader_blk(path.c_str());
}
