// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "entityMatProperties.h"

#include <shaders/dag_shaderMatData.h>

#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class DataBlock;

struct ShaderVariableLimits
{
  bool hasMinMaxValue() const { return maxValue != minValue; }

  float minValue = 0.0f; // inclusive
  float maxValue = 0.0f; // inclusive
  eastl::string description;
  bool editAsColor = false;
  ShaderMatData::VarValue defaultValue;
  MatVarType defaultValueType = MAT_VAR_TYPE_NONE;
};

struct ShaderDescriptorVariableGroup
{
  eastl::string description;
  ska::flat_hash_map<eastl::string, ShaderVariableLimits> shaderVariables;
};

using ShaderSeparatorToPropsType = ska::flat_hash_map<eastl::string /*group name*/, ShaderDescriptorVariableGroup>;

struct ShaderDescriptor
{
  ShaderVariableLimits *findVariableDescriptor(const char *variable_name);

  eastl::string description;
  ShaderSeparatorToPropsType shaderSeparatorToProps;
};

ska::flat_hash_map<eastl::string /*shader name*/, ShaderDescriptor> gatherParameterSeparators(const DataBlock &appBlk,
  const char *appDir);

void set_defaults_from_shader_bin_dump(const char *shader_name, ShaderDescriptor &descriptor);