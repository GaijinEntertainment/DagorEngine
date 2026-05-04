// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globVar.h"
#include "shTargetContext.h"
#include "intervals.h"
#include "varMap.h"
#include <shaders/dag_shaderCommon.h>

#include <generic/dag_tabUtils.h>
#include <generic/dag_initOnDemand.h>
#include "shLog.h"

namespace ShaderGlobal
{

const char *get_var_name(const Var &variable, const shc::TargetContext &ctx) { return ctx.varNameMap().getName(variable.nameId); }

Tab<Var> &VarTable::getMutableVariableList() { return variableList; }
IntervalList &VarTable::getMutableIntervalList() { return intervals; }

const Tab<Var> &VarTable::getVariableList() const { return variableList; }

// get interval list
const IntervalList &VarTable::getIntervalList() const { return intervals; }

// return variable by ID
int VarTable::getVarCount() const { return variableList.size(); }

Var &VarTable::getVar(int internal_index) { return variableList[internal_index]; }

// delete all variables
void VarTable::clear()
{
  clear_and_shrink(variableList);
  intervals.clear();
  clear_and_shrink(variableByNameId);
}

// find id by id
int VarTable::getVarInternalIndex(int variable_id) const
{
  if (variable_id == VarNameMap::BAD_ID)
    return -1;
  if (variable_id < variableByNameId.size())
    return variableByNameId[variable_id];
  else
    return -1;
}

void VarTable::updateVarNameIdMapping(int var_id)
{
  const auto &var = variableList[var_id];
  if (variableByNameId.size() <= var.nameId)
    variableByNameId.resize(var.nameId + 1, -1);
  G_ASSERT(variableByNameId[var.nameId] == -1);
  variableByNameId[var.nameId] = var_id;
}

bool Var::operator==(const Var &right) const
{
  G_ASSERT(type == right.type);
  if (!definesValue || !right.definesValue)
    return true;
  switch (type)
  {
    case SHVT_INT: return value.i == right.value.i;
    case SHVT_INT4: return memcmp(&value.i4, &right.value.i4, sizeof(value.i4)) == 0;
    case SHVT_REAL: return value.r == right.value.r;
    case SHVT_COLOR4: return memcmp(&value.c4, &right.value.c4, sizeof(value.c4)) == 0;
    case SHVT_FLOAT4X4: return true;
    case SHVT_BUFFER: return value.bufId == right.value.bufId;
    case SHVT_TLAS: return value.tlas == right.value.tlas;
    case SHVT_TEXTURE: return value.texId == right.value.texId;
    case SHVT_SAMPLER: return memcmp(&value.samplerInfo, &right.value.samplerInfo, sizeof(value.samplerInfo)) == 0;
    default: G_ASSERT(0);
  }
  return false;
}

void VarTable::link(const Tab<ShaderGlobal::Var> &variables, const IntervalList &intervalsFromFile, Tab<int> &global_var_link_table,
  Tab<ShaderVariant::ExtType> &interval_link_table)
{
  // Global vars.
  int varsNum = variables.size();
  global_var_link_table.resize(varsNum);
  for (unsigned int varNo = 0; varNo < varsNum; varNo++)
  {
    const Var &var = variables[varNo];

    bool exists = false;
    for (int existingVarId = 0; existingVarId < variableList.size(); existingVarId++)
    {
      Var &existingVar = variableList[existingVarId];
      if (var.nameId == existingVar.nameId)
      {
        exists = true;
        global_var_link_table[varNo] = existingVarId;

        if (var.type != existingVar.type)
          sh_debug(SHLOG_FATAL, "Different variable types: '%s'", varNameMap.getName(var.nameId));
        if (var != existingVar)
          sh_debug(SHLOG_FATAL, "Different variable values: '%s'", varNameMap.getName(var.nameId));
        if (var.array_size != existingVar.array_size)
          sh_debug(SHLOG_FATAL, "Different variable array sizes: '%s'", varNameMap.getName(var.nameId));
        if (var.definesValue && existingVar.definesValue && var.isLiteral != existingVar.isLiteral)
          sh_debug(SHLOG_FATAL, "Different variable literal states: '%s'", varNameMap.getName(var.nameId));

        existingVar.isAlwaysReferenced |= var.isAlwaysReferenced;
        existingVar.isImplicitlyReferenced |= var.isImplicitlyReferenced;
        if (var.definesValue && !existingVar.definesValue)
        {
          existingVar.definesValue = true;
          existingVar.isLiteral = var.isLiteral;
          existingVar.value = var.value;
        }
      }
    }

    if (!exists)
    {
      global_var_link_table[varNo] = variableList.size();
      variableList.push_back(var);
    }
  }

  // Intervals.
  interval_link_table.resize(intervalsFromFile.getCount());
  for (unsigned int intervalNo = 0; intervalNo < intervalsFromFile.getCount(); intervalNo++)
  {
    if (intervalsFromFile.getInterval(intervalNo))
    {
      bool res = intervals.addInterval(*intervalsFromFile.getInterval(intervalNo));
      if (!res)
        sh_debug(SHLOG_FATAL, "Different intervals: '%s'",
          intervalNameMap.getName(intervalsFromFile.getInterval(intervalNo)->getNameId()));

      interval_link_table[intervalNo] = intervals.getIntervalIndex(intervalsFromFile.getInterval(intervalNo)->getNameId());
    }
    else
    {
      interval_link_table[intervalNo] = INTERVAL_NOT_INIT;
    }
  }
}

void validate_linked_gvar_collection(const shc::TargetContext &ctx)
{
  for (const Var &var : ctx.globVars().variableList)
  {
    if (var.isLiteral)
      G_ASSERT(var.definesValue);
    else if (!var.definesValue)
      G_ASSERT(!var.isLiteral);

    // @NOTE: non primary dumps only allow references and constants
    if (ctx.compCtx().compInfo().compilingAdditionalDump())
    {
      if (var.definesValue && !var.isLiteral)
      {
        sh_debug(SHLOG_FATAL,
          "Global mutable variable '%s' is defined in a non-primary dump, while only references and consts are allowed.",
          ctx.globVars().varNameMap.getName(var.nameId));
      }
    }
  }
}

} // namespace ShaderGlobal
